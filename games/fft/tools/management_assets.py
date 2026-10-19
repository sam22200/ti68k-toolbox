#!/usr/bin/env python3
"""Extract FFT font references, small pixel UI fonts and a read-only Gariland fixture.

All generated art/data stay local in x/management/data. No replacement system font.
"""
from pathlib import Path
import argparse
import hashlib
import json
import struct
import sys

import numpy as np
from PIL import Image, ImageDraw

import extract
import interface
import battle
import oracle
import units
import art
import management_research

HERE = Path(__file__).resolve().parents[1]
ROOT = HERE.parents[1]
OUT = HERE / 'x/management/data'
FONT_ADDR = 0x8013B8F8
WIDTH_ADDR = 0x801533E0
WORLD_LOAD = 0x800E0000
CHARS = ''.join(chr(i) for i in range(32, 127))
# Extended-page punctuation, identified on the full source glyph contact sheet.
SYMBOLS = {'!': 62, '?': 63, '.': 2054, ',': 2196, ':': 2061, ';': 2197,
           '(': 2062, ')': 2063, '=': 2192, "'": 2065, '"': 2199}


def glyph_id(c):
    if c.isascii() and c.isdigit(): return ord(c) - 48
    if 'A' <= c <= 'Z': return ord(c) - 65 + 10
    if 'a' <= c <= 'z': return ord(c) - 97 + 36
    return SYMBOLS.get(c)


def source_glyph(font, i):
    bits = np.unpackbits(np.frombuffer(font[35*i:35*(i+1)], np.uint8))
    b = bits.reshape(14, 10, 2)
    return b[:, :, 0]*2 + b[:, :, 1]


def adapted(src, compact):
    # 0 transparent, 1 face, 2 bevel, 3 down/right shadow. The glyph values
    # are palette indices, not a linear opacity ramp.
    level = np.array([0, 3, 2, 1], np.uint8)[src[2:14, :8]]
    if compact:
        # Area filter the ink, then tighten contrast offline. No runtime scaling.
        ink = Image.fromarray(np.uint8(level[:10]*85)).resize((8, 8), Image.Resampling.BOX)
        a = np.array(ink)
        level = np.where(a >= 160, 3, np.where(a >= 75, 2, np.where(a >= 35, 1, 0)))
    return level.astype(np.uint8)


def packed(g):
    return b''.join(np.packbits(m, axis=1).tobytes()
                    for m in ((g & 1) != 0, (g & 2) != 0, g == 0))


def pixel_fonts():
    """Five/eight-row monochrome ink and mask; no shadow pixels in either size."""
    os_data=(ROOT/'tools/rom/TI89Titanium_OS.89u').read_bytes()
    fonts=[];advances=[]
    for small in (False,True):
        images=[];widths=[]
        for c in CHARS:
            code=ord(c)
            if small:
                off=1023838+6*code;adv=os_data[off]
                rows=os_data[off+1:off+6]
            else:
                off=764986+8*code;adv=6;rows=os_data[off:off+8]
            bits=np.unpackbits(np.frombuffer(rows,np.uint8)).reshape(len(rows),8)
            # Rebase one empty left column; preserve the source pixel shapes.
            bits=np.concatenate((bits[:,1:],np.zeros((len(rows),1),np.uint8)),axis=1)
            ink=np.packbits(bits,axis=1).tobytes()
            images.append(ink+bytes(v^255 for v in ink));widths.append(adv)
        fonts.append(images);advances.append(bytes(widths))
    data=b'FFMF\x03\x08\x05\x5f'+b''.join(advances)
    data+=b''.join(g for glyphs in fonts for g in glyphs)
    assert len(data)==2668
    return data


def equipment_icons(disc,files,d):
    """Original 16x16 item/palette selection, converted offline to 8x8 TI icons."""
    raw=disc.read(*files['EVENT/ITEM.BIN'])
    assert len(raw)==33280
    a=np.frombuffer(raw[:32768],np.uint8).reshape(256,128)
    sheet=np.empty((256,256),np.uint8);sheet[:,::2]=a&15;sheet[:,1::2]=a>>4
    palettes=np.frombuffer(raw[32768:],'<u2').reshape(16,16).astype(int)
    data=bytearray(b'FFMI\x01\x08\x08\x06')
    data+=bytes(16)+bytes([255]*8) # empty slot, no invented icon
    records=[];previews=[]
    for item in (1,0x13,0x9d,0xba,0xd0):
        rec=d.at(battle.ITEMS+12*item,12);pal_id,sprite=rec[0],rec[1]
        x=(sprite%15)*16;y=32+(sprite//15)*16
        px=sheet[y:y+16,x:x+16];pal=palettes[pal_id]
        lum=(pal&31)*.299+((pal>>5)&31)*.587+((pal>>10)&31)*.114
        g=np.zeros((8,8),np.uint8)
        opaque=np.zeros((8,8),bool)
        for yy in range(8):
            for xx in range(8):
                src=px[yy*2:yy*2+2,xx*2:xx*2+2];values=src[src!=0]
                if values.size:
                    opaque[yy,xx]=True;v=float(lum[values].mean())
                    g[yy,xx]=3 if v<=7 else 2 if v<=16 else 1 if v<=24 else 0
        data+=b''.join(np.packbits(mask,axis=1).tobytes() for mask in
                       ((g&1)!=0,(g&2)!=0,~opaque))
        records.append(dict(item=item,palette=pal_id,sprite=sprite,source_uv=[x,y],
                            icon_sha256=hashlib.sha256(data[-24:]).hexdigest()))
        src_g=interface.greys(px,pal)
        source=Image.fromarray(np.uint8(np.where(src_g<0,216,255-src_g.clip(0)*85)))
        dest=Image.fromarray(np.uint8(np.where(opaque,255-g*85,216)))
        cell=Image.new('L',(128,80),216)
        cell.paste(source.resize((64,64),Image.Resampling.NEAREST),(0,0))
        cell.paste(dest.resize((64,64),Image.Resampling.NEAREST),(64,0))
        ImageDraw.Draw(cell).text((2,66),battle.ITEM_NAMES[item],fill=0)
        previews.append(cell)
    contact=Image.new('L',(128*5,80),216)
    for i,cell in enumerate(previews):contact.paste(cell,(128*i,0))
    contact.save(OUT.parent/'equipment-icons.png')
    assert len(data)==152
    (OUT/'fftmicon.bin').write_bytes(data)
    return dict(source='EVENT/ITEM.BIN',source_sha256=hashlib.sha256(raw).hexdigest(),
                layout='32768 packed low-nibble-first pixel bytes, then 16 BGR555 palettes',
                bank_bytes=len(data),records=records)


def title_strip(disc,files,index):
    """Shared retail location-title conversion; no duplicate font or title artwork."""
    title_raw=disc.read(*files['EVENT/MAPTITLE.BIN'])
    attack=disc.read(*files['EVENT/ATTACK.OUT'])
    pal=np.frombuffer(attack[0x801d5ff0-0x801bf000:][:32],'<u2').astype(int)
    lum=((pal&31)*299+((pal>>5)&31)*587+((pal>>10)&31)*114)//1000
    b=np.frombuffer(title_raw[index*2560:(index+1)*2560],np.uint8).reshape(20,128)
    px=np.empty((20,256),np.uint8);px[:,::2]=b&15;px[:,1::2]=b>>4
    yy,xx=np.nonzero(px);crop=px[yy.min():yy.max()+1,xx.min():xx.max()+1]
    levels=lum[crop];im=Image.fromarray(np.uint8(levels*255//31))
    if im.width>144 or im.height>14:
        ratio=min(144/im.width,14/im.height);im=im.resize((round(im.width*ratio),round(im.height*ratio)),Image.Resampling.BOX)
    a=np.array(im);g=np.where(a>=205,0,np.where(a>=110,1,np.where(a>=42,2,-1)))
    canvas=np.full((14,144),-1);x=(144-im.width)//2;canvas[:im.height,x:x+im.width]=g
    data=bytearray()
    for col in range(18):
        part=canvas[:,col*8:col*8+8]
        data+=b''.join(np.packbits(m,axis=1).tobytes() for m in
                      ((part>=0)&((part&1)!=0),(part>=0)&((part&2)!=0),part<0))
    return bytes(data),canvas,list(crop.shape)


def visual_bank(disc,files):
    """Only idle poses at a second scale and two narrow original title strips."""
    data=bytearray(b'FFMV\x01\x10\x10\x00');definitions=[]
    definitions.append('#define MV_SMALL %d'%len(data))
    # Resample the existing outlined actors offline, never in a TI frame.
    raw=(HERE/'fftu.bin').read_bytes()
    for gfx in (0,2,3,4,5):
        for frame in range(5):
            src=raw[gfx*44*208+frame*208:][:156]
            rows=np.frombuffer(src,'>u2').reshape(3,26)
            bits=((rows[:,:,None]>>(15-np.arange(16)))&1)
            g=np.where(bits[2],-1,bits[0]+2*bits[1]).astype(np.int16)
            # Remove the ground shadow; preserve a white visibility outline after scaling.
            g[23:]=-1
            core=np.array(Image.fromarray(g.astype(np.int32)).resize((10,16),Image.Resampling.NEAREST))
            core=interface.outline(core)
            canvas=np.full((16,16),-1);canvas[:,:10]=core
            data+=interface.planes(canvas,16)
    definitions.append('#define MV_JOB %d'%len(data))
    for sheet in ('KNIGHT_M','KNIGHT_W','YUMI_M','YUMI_W'):
        spr=disc.read(*files['BATTLE/'+sheet+'.SPR'])
        shp=units.shp_frames(disc.read(*files['BATTLE/TYPE1.SHP']))
        for f in range(9,14):data+=interface.planes(units.sprite(spr,shp[f]),16)
    definitions.append('#define MV_TITLE %d'%len(data))
    title_raw=disc.read(*files['EVENT/MAPTITLE.BIN'])
    attack=disc.read(*files['EVENT/ATTACK.OUT'])
    pal=np.frombuffer(attack[0x801d5ff0-0x801bf000:][:32],'<u2').astype(int)
    lum=((pal&31)*299+((pal>>5)&31)*587+((pal>>10)&31)*114)//1000
    contacts=[];titles=[]
    for index,name in ((55,'Orbonne Monastery'),(21,'Magic City Gariland')):
        strip,canvas,shape=title_strip(disc,files,index);data+=strip
        contacts.append(Image.fromarray(np.uint8(np.where(canvas<0,0,255-canvas.clip(0)*85))))
        titles.append(dict(name=name,index=index,source_shape=shape,stored_shape=[14,144]))
    (OUT/'fftmvis.bin').write_bytes(data)
    Image.fromarray(np.concatenate([np.array(i) for i in contacts],axis=0)).resize((720,140),Image.Resampling.NEAREST).save(OUT.parent/'location-source.png')
    return definitions,dict(bank_bytes=len(data),small_idle_bytes=5*5*96,job_idle_bytes=4*5*156,
                           title_bytes=2*18*42,titles=titles,source_sha256=hashlib.sha256(title_raw).hexdigest())

def main():
    ap = argparse.ArgumentParser(__doc__)
    ap.add_argument('disc')
    ap.add_argument('--state', type=Path, default=ROOT/'sources/fft_ps1/gariland_t1.state')
    args = ap.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    disc, files = extract.disc_files(args.disc)
    font = disc.read(*files['EVENT/FONT.BIN'])
    world = disc.read(*files['WORLD/WORLD.BIN'])
    assert len(font) == 2200*35
    off = FONT_ADDR-WORLD_LOAD
    assert world[off:off+len(font)] == font, 'WORLD font symbol no longer matches disc'
    widths = world[WIDTH_ADDR-WORLD_LOAD:WIDTH_ADDR-WORLD_LOAD+2200]
    advances = []
    native, small = [], []
    source_sheet = Image.new('L', (16*24, 6*34), 255)
    label = ImageDraw.Draw(source_sheet)
    for n, c in enumerate(CHARS):
        i = glyph_id(c)
        # Unsupported ASCII has a visible original question mark. Slash is a tiny
        # offline construction from the original colon strokes, explicitly recorded.
        src = source_glyph(font, i if i is not None else 63) if c != ' ' else np.zeros((14,10),np.uint8)
        if c == '/':
            src[:] = 0
            for y in range(3, 11): src[y, 5-((y-3)>>1)] = 1
        advance = int(widths[i]) if i is not None else 6
        if c in SYMBOLS or c == '/':
            # Extended punctuation lives in a Japanese ten-pixel cell. Rebase its
            # ink and use a compact Latin advance; retain the exact source pixels.
            cols = np.flatnonzero(np.any(src, axis=0))
            if len(cols):
                src = np.roll(src, -int(cols[0]), axis=1)
                advance = min(8, int(cols[-1]-cols[0])+2)
                if np.any(src[:2]):src=np.roll(src,1,axis=0)
        if c == ' ': advance = 3
        # Include the source shadow without forcing all glyphs to monospace.
        assert advance <= 8, (c, advance)
        advances.append(advance)
        native.append(adapted(src, False)); small.append(adapted(src, True))
        x,y=n%16*24,n//16*34
        source_sheet.paste(Image.fromarray(np.uint8(255-np.array([0,3,2,1])[src]*85)).resize((20,28),Image.Resampling.NEAREST),(x,y))
        label.text((x,y+25),c,fill=0)
    # Keep the original extraction as reference; the user selected a smaller clean
    # pixel font rather than a vertically filtered version of the curved FFT font.
    data=pixel_fonts()
    (OUT/'fftmfont.bin').write_bytes(data)
    source_sheet.resize((768,408),Image.Resampling.NEAREST).save(OUT.parent/'font-extracted.png')
    # Load the reproducible original battle state, not randomly generated port stats.
    p = oracle.PSX(args.disc); p.load(str(args.state))
    fixture = [];alternatives=[]
    d=battle.Disc(args.disc)
    for slot,gfx in ((16,0),(17,2),(18,5),(19,4),(20,2)):
        a=oracle.UNITS+oracle.UNIT_SIZE*slot
        r=lambda o,n=1:p.read(a+o,n)
        raw=bytes(p.ram[p.addr(a+0x12c):p.addr(a+0x12c)+16]).split(b'\xfe')[0].split(b'\xff')[0]
        name=interface.decode(raw) if slot!=16 else 'Ramza'
        eq=[r(o) for o in (0x1d,0x20,0x1a,0x1b,0x1c)]
        # A read-only snapshot: no campaign progression or inventory writes.
        extra={0:'Empty',0x1cc:'Equip Axe',0x1df:'Defend',0x1e0:'Equip Change'}
        sets={0:'Empty',**battle.SET_NAMES}
        slots=[sets.get(r(0x12),'Unknown'),sets.get(r(0x13),'Unknown')]
        slots += [extra.get(r(o,2),'Unknown') for o in (0x14,0x16,0x18)]
        fixture.append(dict(slot=slot,gfx=gfx,name=name,job='Chemist' if gfx in (4,5) else 'Squire',
                            hp=r(0x2a,2),mp=r(0x2e,2),level=r(0x22),brave=r(0x24),faith=r(0x26),
                            unlocked_jobs=bytes(p.ram[p.addr(a+0x96):p.addr(a+0x99)]).hex(),
                            job_levels=bytes(p.ram[p.addr(a+0xd2):p.addr(a+0xdc)]).hex(),
                            pa=r(0x36),ma=r(0x37),speed=r(0x38),move=r(0x3a),jump=r(0x3b),equipment=eq,ability_slots=slots))
    requirements=d.at(0x800660c4,30)
    assert requirements==bytes(10)+bytes([0x20])+bytes(9)+bytes([0x20])+bytes(9)
    assert all(u['unlocked_jobs']=='c00000' and u['job_levels'].startswith('11') for u in fixture)
    for u in fixture:
        a=oracle.UNITS+oracle.UNIT_SIZE*u['slot'];r=lambda o,n=1:p.read(a+o,n)
        raw=[r(0x72+3*i,3) for i in range(5)]
        job=(0x4a if u['gfx']>=4 else 0x4b)
        cats=d.job(job)['cats'];eq=[r(0x1a+i) for i in range(7)]
        for i,it in enumerate(eq):
            if it!=battle.NONE:
                cat=d.item(it)['type']
                if not cats[cat>>3]&(0x80>>(cat&7)):eq[i]=battle.NONE
        stats=battle.stats(d,job,u['level'],raw,eq)
        v=dict(u);v.update(stats);v['speed']=stats['sp']
        v['gfx']=(3 if u['gfx']==5 else 2) if job==0x4a else (5 if u['gfx']==3 else 4)
        v['job']='Squire' if job==0x4a else 'Chemist'
        v['equipment']=[eq[i] for i in (3,6,0,1,2)]
        v['ability_slots']=list(u['ability_slots']);v['ability_slots'][0]='Basic Skill' if job==0x4a else 'Item'
        alternatives.append(v)
    definitions,visual_report=visual_bank(disc,files)
    audit=management_research.audit(args.disc)
    icon_report=equipment_icons(disc,files,d)
    costs={name:struct.unpack('<H',d.at(battle.ABILITY+8*i,2))[0]
           for i,name in battle.NAMES.items()}
    def quoted(s): return json.dumps(s)
    lines=['// Generated locally by tools/management_assets.py. Never committed.',
           'static const MenuUnit party[5] = {']
    for u in fixture+alternatives:
        if u is alternatives[0]:lines+=['};','static const MenuUnit alternate[5] = {']
        vals=[u[k] for k in ('gfx','hp','mp','level','brave','faith','pa','ma','speed','move','jump')]
        equip=[battle.ITEM_NAMES.get(i,'Empty') for i in u['equipment']]
        icon_ids={1:1,0x13:2,0x9d:3,0xba:4,0xd0:5}
        icons=[icon_ids.get(i,0) for i in u['equipment']]
        lines.append('    { %s, %s, { %s }, { %s }, { %s }, { %s } },' %
                     (quoted(u['name']),quoted(u['job']),', '.join(map(str,vals)),', '.join(map(quoted,equip)),
                      ', '.join(map(quoted,u['ability_slots'])),', '.join(map(str,icons))))
    lines+=['};','static const u16 learn_costs[4] = { %s };' % ', '.join(str(costs[n]) for n in ('Accumulate','Dash','Throw Stone','Heal'))]
    lines+=['static const u16 item_costs[5] = { %s };' % ', '.join(str(costs[n]) for n in ('Potion','Antidote','Eye Drop','Echo Grass','Phoenix Down'))]
    lines+=definitions
    # A compact copy of the existing hand fits beside ten-pixel actors in 32-pixel cells.
    hand=np.array([[v if v is not None else -1 for v in row] for row in art.outlined_pattern(art.HAND,18)],dtype=np.int32)
    hand=np.array(Image.fromarray(hand).resize((8,7),Image.Resampling.NEAREST))
    hand_planes=[np.packbits(mask,axis=1).tobytes() for mask in
                 ((hand>=0)&((hand&1)!=0),(hand>=0)&((hand&2)!=0),hand<0)]
    lines+=['static const u8 small_hand[3][7] = { '+', '.join('{ '+', '.join(map(str,p))+' }' for p in hand_planes)+' };']
    # Include the longest measured page intact; preserve all printed spaces.
    sample=audit['longest_portrait']
    lines+=['#define DIALOGUE_CAPACITY %d'%sample['characters'],
            'static const char dialogue_stress[] = '+quoted('\n'.join(sample['lines']))+';']
    (OUT/'fixture.h').write_text('\n'.join(lines)+'\n')
    report=dict(source_font='EVENT/FONT.BIN',font_sha256=hashlib.sha256(font).hexdigest(),
                world_sha256=hashlib.sha256(world).hexdigest(),bitmap_address=hex(FONT_ADDR),
                width_address=hex(WIDTH_ADDR),world_font_byte_equal=True,source_shape=[2200,14,10],
                source_encoding='2bpp MSB first, 35 bytes per glyph',source_palette_indices=[0,1,2,3],
                font_origin='AMS runtime F_4x6/F_6x8 bitmap shapes; FFT font retained as reference only',
                ink_heights=[8,5],stored_heights=[8,5],default='5-row menu and dialogue ink, optional 8-row menu ink, no shadows',
                prototype_levels=[0,3],selected_ascii=95,synthetic_symbols=[],equipment_icons=icon_report,visuals=visual_report,
                unsupported_ascii=[],
                bank_bytes=len(data),bank_sha256=hashlib.sha256(data).hexdigest(),
                snapshot_sha256=hashlib.sha256(args.state.read_bytes()).hexdigest(),party=fixture,job_requirement_bytes=requirements.hex(),job_alternatives=alternatives,
                ability_jp=costs,scope='Read-only UI fixtures; menu Learn balance is an explicit 200 JP demonstration.')
    (OUT.parent/'extraction.json').write_text(json.dumps(report,indent=2)+'\n')
    print('FFT font reference: %d glyphs; small pixel UI bank: %d bytes; five original unit snapshots' % (2200,len(data)))


if __name__ == '__main__': main()
