#!/usr/bin/env python3
"""Extract local PAL PPU/ROM art offline; no original pixels enter git."""
import json
import struct
import sys
from functools import lru_cache
from pathlib import Path

import numpy as np
from PIL import Image

from reference import ROOT, OUT, ROM
from terrain import load_map, tile, TOP, WIDTH, HEIGHT

GAME = ROOT / 'games/yoshi'
ART = OUT / 'art'
PAGE_OFFSETS = 0xc32a4
MAP16_BASE = 0xc33f2
sys.path.insert(0, str(ROOT / 'tools/snes'))
from snesrun import SNES, PAD
from actors_reference import prepare


def palette(raw):
    words = np.frombuffer(raw, dtype='<u2').astype(np.uint32)
    return np.array([((words >> n) & 31) * 255 // 31 for n in (0, 5, 10)], dtype=np.uint8).T


class PPU:
    def __init__(self, vram, cgram, oam, regs):
        self.vram, self.colors, self.oam, self.regs = vram, palette(cgram), oam, regs

    @lru_cache(maxsize=None)
    def tile(self, address, bpp=4):
        pixels = np.zeros((8, 8), dtype=np.uint8)
        for y in range(8):
            for plane in range(bpp):
                bits = self.vram[(address + (plane // 2) * 16 + y * 2 + (plane & 1)) & 65535]
                for x in range(8):
                    pixels[y, x] |= ((bits >> (7 - x)) & 1) << plane
        return pixels

    def bg_tile(self, word, layer=0):
        bpp = 2 if layer == 2 else 4
        base = ((self.regs[11 + layer // 2] >> (4 * (layer & 1))) & 15) << 13
        pixels = self.tile((base + (word & 1023) * (bpp * 8)) & 65535, bpp)
        if word & 0x4000: pixels = pixels[:, ::-1]
        if word & 0x8000: pixels = pixels[::-1]
        colors = self.colors[pixels + ((word >> 10) & 7) * (1 << bpp)]
        return np.dstack((colors, (pixels != 0).astype(np.uint8) * 255))

    def background(self, layer):
        setting = self.regs[7 + layer]
        base = (setting & 252) << 9
        width, height = (64 if setting & 1 else 32), (64 if setting & 2 else 32)
        size = 16 if self.regs[5] & (16 << layer) else 8
        result = np.zeros((height * size, width * size, 4), dtype=np.uint8)
        for y in range(height):
            for x in range(width):
                block = (y // 32) * (width // 32) + x // 32
                word = struct.unpack_from('<H', self.vram, (base + block * 2048 + ((y & 31) * 32 + (x & 31)) * 2) & 65535)[0]
                if size == 8:
                    result[y * size:y * size + size, x * size:x * size + size] = self.bg_tile(word, layer)
                else:
                    # Flip the whole 16x16 cell, rather than its four quarters.
                    tile = np.zeros((16, 16, 4), dtype=np.uint8)
                    for dy in range(2):
                        for dx in range(2):
                            sub = (word & 0x3c00) | (((word & 1023) + dx + 16 * dy) & 1023)
                            tile[dy * 8:dy * 8 + 8, dx * 8:dx * 8 + 8] = self.bg_tile(sub, layer)
                    if word & 0x4000: tile = tile[:, ::-1]
                    if word & 0x8000: tile = tile[::-1]
                    result[y * size:y * size + size, x * size:x * size + size] = tile
        return result

    def objects(self):
        base = (self.regs[1] & 7) << 14
        sizes = ((8,16),(8,32),(8,64),(16,32),(16,64),(32,64),(16,32),(16,32))
        for n in range(127, -1, -1):
            x, y, tile, flags = self.oam[n * 4:n * 4 + 4]
            high = (self.oam[512 + n // 4] >> (2 * (n & 3))) & 3
            x |= (high & 1) << 8
            if x >= 256: x -= 512
            if y >= 224: continue
            size = sizes[self.regs[1] >> 5][high >> 1]
            address = base + ((self.regs[1] >> 3 & 3) + 1) * 8192 * (flags & 1)
            pixels = np.zeros((size, size), dtype=np.uint8)
            for dy in range(size // 8):
                for dx in range(size // 8):
                    number = ((tile + dx) & 15) | ((tile + dy * 16) & 240)
                    pixels[dy * 8:dy * 8 + 8, dx * 8:dx * 8 + 8] = self.tile((address + number * 32) & 65535)
            if flags & 64: pixels = pixels[:, ::-1]
            if flags & 128: pixels = pixels[::-1]
            colors = self.colors[128 + ((flags >> 1) & 7) * 16 + pixels]
            yield n, x, y, (flags >> 1) & 7, np.dstack((colors, (pixels != 0).astype(np.uint8) * 255))


def snapshot(index):
    directory = OUT / 'survey'
    return PPU(*[(directory / f'view{index}.{kind}').read_bytes() for kind in ('vram','cgram','oam','regs')])


def foreground(rom, numbers, ppu):
    assert rom[PAGE_OFFSETS:PAGE_OFFSETS + 16] == bytes.fromhex('00004807d00d700e380f580ff80fc010')
    result = np.zeros((HEIGHT * 16, WIDTH * 16, 4), dtype=np.uint8)
    for y, row in enumerate(numbers):
        for x, number in enumerate(row):
            address = MAP16_BASE + struct.unpack_from('<H', rom, PAGE_OFFSETS + (number >> 8) * 2)[0] + (number & 255) * 8
            for q, word in enumerate(struct.unpack_from('<4H', rom, address)):
                dx, dy = (q & 1) * 8, (q >> 1) * 8
                result[y * 16 + dy:y * 16 + dy + 8, x * 16 + dx:x * 16 + dx + 8] = ppu.bg_tile(word)
    return result


def composite(canvas, rgba, x, y):
    h, w = rgba.shape[:2]
    x0, y0, x1, y1 = max(0,x), max(0,y), min(canvas.shape[1],x+w), min(canvas.shape[0],y+h)
    if x1 <= x0 or y1 <= y0: return
    src = rgba[y0-y:y1-y,x0-x:x1-x]
    dest = canvas[y0:y1,x0:x1]
    opaque = src[:,:,3] != 0
    dest[opaque] = src[opaque]


def grey(rgba, background=False):
    reduced = np.asarray(Image.fromarray(rgba).resize((rgba.shape[1]//2,rgba.shape[0]//2),Image.Resampling.BOX))
    rgb = reduced[:,:,:3].astype(np.uint16)
    lum = (rgb[:,:,0]*77 + rgb[:,:,1]*150 + rgb[:,:,2]*29) >> 8
    if background:
        levels = (lum < 200).astype(np.uint8) # background only white/light grey
    else:
        levels = np.digitize(lum, [72,144,216])
        levels = (3-levels).astype(np.uint8)
    return levels, reduced[:,:,3] >= 96


def scene(fg, ppu):
    bg = ppu.background(1)
    yy, xx = np.indices(fg.shape[:2])
    # Flatten parallax at the first view's alignment, then repeat the original
    # tree/cloud pattern. No scroll/color-math/HDMA work on the calculator.
    backdrop = bg[(yy + TOP - 962) % bg.shape[0], xx % bg.shape[1]].copy()
    backdrop[backdrop[:,:,3] == 0] = [255,255,255,255]
    levels, _ = grey(backdrop, True)
    front, opaque = grey(fg)
    levels[opaque] = front[opaque]
    padded = np.zeros((256,768),dtype=np.uint8)
    padded[:,:640] = levels
    planes = b''.join(np.packbits((padded >> n) & 1,axis=1).tobytes() for n in (0,1))
    assert len(planes) == 49152
    (GAME/'generated/scene.bin').write_bytes(planes)
    Image.fromarray((255-padded[:,:640]*85).astype(np.uint8)).save(ART/'scene-grey.png')


def hero(snes, solo=False):
    pp = PPU(*snes.ppu())
    x = snes.read(0x70008c,2)-snes.read(0x7e0039,2)
    y = snes.read(0x700090,2)-snes.read(0x7e003b,2)
    canvas = np.zeros((48,48,4),dtype=np.uint8)
    for n,ox,oy,pal,img in pp.objects():
        tile = pp.oam[n*4+2]
        if pal == 5 and (tile <= 10 or not solo and tile in (0x62,0x64)) and abs(ox-x)<40 and abs(oy-y)<40:
            composite(canvas,img,ox-x+12,oy-y+12)
    assert np.count_nonzero(canvas[:,:,3]) > 100
    return canvas


def baby(snes, bubble=True):
    pp = PPU(*snes.ppu())
    x = snes.read(0x7010e2,2)-snes.read(0x7e0039,2)
    y = snes.read(0x701182,2)-snes.read(0x7e003b,2)
    canvas = np.zeros((48,48,4),dtype=np.uint8)
    for n,ox,oy,pal,img in pp.objects():
        tile = pp.oam[n*4+2]
        if ((pal == 5 and tile in (0x62,0x64)) or
            (bubble and pal == 4 and tile in (0x9c,0x7e))) and abs(ox-x)<24 and abs(oy-y)<32:
            composite(canvas,img,ox-x+12,oy-y+12)
    assert np.count_nonzero(canvas[:,:,3]) > 60
    return canvas


def shy(snes):
    pp = PPU(*snes.ppu())
    x = snes.read(0x7010e2+92,2)-snes.read(0x7e0039,2)
    y = snes.read(0x701182+92,2)-snes.read(0x7e003b,2)
    canvas = np.zeros((24,24,4),dtype=np.uint8)
    for n,ox,oy,pal,img in pp.objects():
        if (pal == 4 or pal == 0 and pp.oam[n*4+2] == 158) and abs(ox-x)<12 and abs(oy-y)<18:
            composite(canvas,img,ox-x+4,oy-y+4)
    assert np.count_nonzero(canvas[:,:,3]) > 60
    return canvas


def sprite(rgba, origin):
    levels, opaque = grey(rgba)
    levels = np.pad(levels,1)
    opaque = np.pad(opaque,1)
    outline = opaque.copy()
    for dy in (-1,0,1):
        for dx in (-1,0,1): outline |= np.roll(np.roll(opaque,dy,axis=0),dx,axis=1)
    levels[~opaque] = 0
    # Empty rows would still cost full sprite loops on the 68000. Crop after
    # dilation, keeping the geometry anchor and every visible outline pixel.
    ys,xs = np.nonzero(outline)
    x0,y0,x1,y1 = xs.min(),ys.min(),xs.max()+1,ys.max()+1
    return levels[y0:y1,x0:x1], ~outline[y0:y1,x0:x1], (origin[0]-1+int(x0),origin[1]-1+int(y0))


def sprites():
    groups = {}
    solos = {}
    sources = []
    snes = SNES(ROM)
    def add(name, rgba, origin=(-6,-6)):
        groups.setdefault(name,[]).append(sprite(rgba,origin))
        Image.fromarray(rgba).save(ART/f'{name}-{len(groups[name])-1}.png')
        sources.append({'group':name,'x':snes.read(0x70008c,2),'y':snes.read(0x700090,2),
                        'frame':frame,'mouth':snes.read(0x700150,2),'flutter':snes.read(0x7000d2,2)})
        if name not in ('shy','egg','baby','bubble','coin'):
            solos.setdefault('solo_'+name,[]).append(sprite(hero(snes,True),origin))
    try:
        for name,buttons,indices in [('idle',[],[98,138,311,350]),('walk',['RIGHT'],[4,8,12,16,20,24,28,32]),
                                    ('jump',['B'],[6,18,32,45]),('flutter',['B'],[60,64,68,72]),
                                    ('tongue',['Y'],[4]),('up',['Y','UP'],[4])]:
            prepare(snes)
            for frame in range(max(indices)+1):
                snes.pressed = sum(1<<PAD[b] for b in buttons)
                snes.lib.retro_run()
                if frame in indices: add(name,hero(snes))
        prepare(snes,True)
        for frame in range(90):
            snes.pressed = 0; snes.lib.retro_run()
            if frame in (24,32,40,48): add('shy',shy(snes),(-2,-2))
        prepare(snes,True)
        for frame in range(65):
            snes.pressed = (1<<PAD['Y']) if frame==0 else ((1<<PAD['DOWN']) if frame>=20 else 0)
            snes.lib.retro_run()
            if frame == 14: add('holding',hero(snes))
            if frame in (20,23,26,33,36,39,41,43): add('swallow',hero(snes))
        pp = PPU(*snes.ppu())
        eggs = [img for n,x,y,pal,img in pp.objects() if pal == 0 and pp.oam[n*4+2] == 128]
        assert len(eggs)==1
        add('egg',eggs[0],(0,0))
        # Holding/throwing poses from a real captured enemy and generated egg.
        prepare(snes,True)
        for frame in range(107):
            buttons=['Y'] if frame==0 else ['DOWN'] if 20<=frame<60 else ['A'] if frame in (65,95) else []
            snes.pressed=sum(1<<PAD[b] for b in buttons); snes.lib.retro_run()
            if frame in (70,80,96,100): add('throw',hero(snes))
        prepare(snes)
        rom,_=load_map()
        for frame in range(25):
            snes.pressed=0; snes.lib.retro_run()
            if frame in (0,8,16,24):
                pp=PPU(*snes.ppu())
                add('coin',foreground(rom,[[0x6000]],pp)[:16,:16],(0,0))
        snes.load(OUT/'damage/contact.state')
        for frame in range(73):
            snes.pressed=0; snes.lib.retro_run()
            if frame in (60,64,68,72):
                add('bubble',baby(snes))
                add('baby',baby(snes,False))
        groups.update(solos)
    finally: snes.close()
    (ART/'poses.json').write_text(json.dumps(sources,indent=2)+'\n')
    records, images, identifiers = [], [], []
    count = sum(len(g)*2 for g in groups.values())
    position = 16 + count*16
    for name, poses in groups.items():
        identifiers.append(f'#define YART_{name.upper()} {len(records)}\n#define YART_{name.upper()}_N {len(poses)}\n')
        for facing in (0,1):
            for levels, mask, (ox,oy) in poses:
                if facing: levels,mask,ox = levels[:,::-1],mask[:,::-1],8-ox-levels.shape[1]
                record = bytearray()
                restored = np.zeros_like(levels)
                restored_mask = np.ones_like(mask)
                for start in (0,16):
                    if name == 'bubble' and start:
                        record.extend(bytes(8)); continue
                    if start >= levels.shape[1]: record.extend(bytes(8)); continue
                    partmask = mask[:,start:start+(32 if name == 'bubble' else 16)]
                    ys,xs = np.nonzero(~partmask)
                    if not len(xs): record.extend(bytes(8)); continue
                    x0,y0,x1,y1 = xs.min(),ys.min(),xs.max()+1,ys.max()+1
                    pixels = levels[y0:y1,start+x0:start+x1]
                    partmask = partmask[y0:y1,x0:x1]
                    restored[y0:y1,start+x0:start+x1] = pixels
                    restored_mask[y0:y1,start+x0:start+x1] = partmask
                    width = 8 if x1-x0 <= 8 else 16 if x1-x0 <= 16 else 32
                    height = pixels.shape[0]
                    if position & 3:
                        pad = 4-(position&3); images.append(bytes(pad)); position+=pad
                    planes = []
                    for bits,fill in ((pixels&1,0),((pixels>>1)&1,0),(partmask,1)):
                        pad = np.pad(bits,((0,0),(0,width-bits.shape[1])),constant_values=fill)
                        planes.append([int.from_bytes(row.tobytes(),'big') for row in np.packbits(pad,axis=1)])
                    length = width//8*height
                    record.extend(struct.pack('>BBbbHH',width,height,ox+start+int(x0),oy+int(y0),position,length))
                    images.append((width,planes))
                    position += length*3
                assert np.array_equal(restored,levels) and np.array_equal(restored_mask,mask)
                records.append(record)
    for endian,suffix in (('=',''),('>','.be')):
        data = bytearray(b'YAR2'+struct.pack('>HH',len(records),position)+bytes(8)+b''.join(records))
        for piece in images:
            if isinstance(piece,bytes): data.extend(piece)
            else:
                width,planes=piece
                for rows in planes: data.extend(struct.pack(endian+str(len(rows))+{8:'B',16:'H',32:'I'}[width],*rows))
        assert len(data)==position
        while len(data)&3: data.append(0)
        fast_offset=len(data)
        # Masked 8px sprites at all 16 sub-word positions. Six padded rows
        # per phase make the phase lookup two shifts, while ten rows draw.
        bases=[]
        for name in ('egg','coin'):
            base=next(int(line.splitlines()[0].split()[2]) for line in identifiers if line.startswith(f'#define YART_{name.upper()} '))
            bases.extend(base+n for n in range(len(groups[name])))
        for base in bases:
            width,height,ox,oy,offset,length=struct.unpack_from('>BBbbHH',records[base])
            assert width==8 and height==10 and not records[base][8]
            l,d,m=(data[offset+n*length:offset+(n+1)*length] for n in range(3))
            for shift in range(16):
                rows=[]
                for row in range(16):
                    rows.extend(((l[row]<<(24-shift)),(d[row]<<(24-shift)),
                                 (0xffffffff^((255^m[row])<<(24-shift)))) if row<height else (0,0,0xffffffff))
                data.extend(struct.pack(endian+'48I',*rows))
        cursor_offset=len(data)
        for locked in (0,1):
            core=np.zeros((15,15),dtype=bool)
            for y in range(-6,7):
                for x in range(-6,7):
                    ring=13<=x*x+y*y<=22
                    arms=(abs(x)<=1 and abs(y)>=4) or (abs(y)<=1 and abs(x)>=4)
                    core[y+7,x+7]=ring or arms or (locked and abs(x)<=1 and abs(y)<=1)
            opaque=core.copy()
            for dy in (-1,0,1):
                for dx in (-1,0,1): opaque|=np.roll(np.roll(core,dy,axis=0),dx,axis=1)
            for shift in range(16):
                planes=[]
                for plane_index,pixels in enumerate((core,core,~opaque)):
                    values=[]
                    for y in range(16):
                        black=0
                        if y<15:
                            for x in range(15):
                                if (pixels[y,x] if plane_index<2 else opaque[y,x]):black|=1<<(31-x-shift)
                        values.append(black if plane_index<2 else 0xffffffff^black)
                    planes.extend(values)
                data.extend(struct.pack(endian+'48I',*planes))
        struct.pack_into('>HHH',data,6,len(data),fast_offset,cursor_offset)
        data[:4]=b'YAR4'
        assert len(data)<65518
        (GAME/f'yjart{suffix}.bin').write_bytes(data)
    (GAME/'generated/art_ids.h').write_text('/* Local PPU-derived sprite bank indices, generated and ignored. */\n'+''.join(identifiers)+f'#define YART_COUNT {len(records)}\n')
    # Review sheet: transparent padding remains white; outlines are native pixels.
    sheet=Image.new('L',(16*64,((len(records)+15)//16)*64),255)
    n=0
    for poses in groups.values():
        for facing in (0,1):
            for levels,mask,origin in poses:
                if facing: levels=levels[:,::-1]
                picture=Image.fromarray((255-levels*85).astype(np.uint8)).resize((levels.shape[1]*2,levels.shape[0]*2),Image.Resampling.NEAREST)
                sheet.paste(picture,((n%16)*64,(n//16)*64));n+=1
    sheet.save(ART/'sprites-grey.png')
    return len(records),len(data)


def main():
    ART.mkdir(parents=True, exist_ok=True)
    rom, numbers = load_map()
    ppu = snapshot(0)
    fg = foreground(rom, numbers, ppu)
    Image.fromarray(fg).save(ART / 'foreground.png')
    for n in (1, 2): Image.fromarray(ppu.background(n)).save(ART / f'bg{n+1}.png')
    samples = json.loads((OUT / 'survey/survey.json').read_text())['captures']
    # Compare the independently expanded ROM Map16 against streamed PPU words.
    count = 0
    for n, sample in enumerate(samples):
        pp = snapshot(n)
        tilemap = pp.background(0)
        directory = OUT/'survey'
        sram = (directory/f'view{n}.sram').read_bytes()
        wram = (directory/f'view{n}.wram').read_bytes()
        current_numbers = [[tile(sram,wram,x,TOP//16+y) for x in range(WIDTH)] for y in range(HEIGHT)]
        current = foreground(rom,current_numbers,pp) # flowers animate; collected coins disappear
        for ty in range((sample['camera_y'] + 15) // 16, (sample['camera_y'] + 208) // 16):
            for tx in range((sample['camera_x'] + 15) // 16, min(WIDTH, (sample['camera_x'] + 240) // 16)):
                orig = current[ty * 16 - TOP:ty * 16 - TOP + 16, tx * 16:tx * 16 + 16]
                visible = tilemap[(ty * 16) % 256:(ty * 16) % 256 + 16, (tx * 16) % 512:(tx * 16) % 512 + 16]
                assert np.array_equal(orig, visible), (n, tx, ty)
                count += 1
    print('ROM Map16 art matches', count, 'streamed original cells across five views.')
    coins=sorted((x*16,TOP+y*16) for y,row in enumerate(numbers) for x,number in enumerate(row) if number==0x6000)
    assert len(coins)==21
    # Erase only the foreground coin layer, keeping the original backdrop.
    for x,y in coins: fg[y-TOP:y-TOP+16,x:x+16]=0
    starts=[sum(x<bin*128 for x,y in coins) for bin in range(11)]
    (GAME/'generated/coins.h').write_text('/* Local PAL Map16 coin placements, grouped by 128px X bin. */\n#define YI_COINS '+str(len(coins))+'\nstatic const unsigned char coin_start[11] = {'+','.join(map(str,starts))+'};\nstatic const unsigned short coin_xy[YI_COINS][2] = {\n'+''.join(f'    {{{x},{y}}},\n' for x,y in coins)+'};\n')
    scene(fg,ppu)
    print('Sprite bank:',sprites())
    (ART/'source.json').write_text(json.dumps({'source':json.loads((OUT/'start.json').read_text()),
        'map16_offsets':PAGE_OFFSETS,'map16_base':MAP16_BASE,'matched_cells':count,
        'background':'BG2 repeat, first-view alignment, light grey; parallax flattened; BG3 foreground omitted',
        'foreground':'Full Map16 band; 21 coin cells removed before composition and drawn as animated collectables',
        'sprites':'Original OAM objects at controlled pose doors; BOX half scale, four greys, one-pixel white outline; both directions precomputed'},indent=2)+'\n')


if __name__ == '__main__': main()
