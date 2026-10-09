#!/usr/bin/env python3
"""Prepare a separate 70-percent view from verified local source pixels."""
import json
import struct
from pathlib import Path
import numpy as np
from PIL import Image
from art import grayscale, sprite_words
from reference import sha

GAME = Path(__file__).resolve().parents[1]
WIDTH, HEIGHT, STRIDE = 504, 224, 64
PLANE = STRIDE * HEIGHT


def bitplane(bits):
    padded = np.pad(bits, ((0,0),(0,512-WIDTH)))
    return np.packbits(padded, axis=1).tobytes()


def main():
    source = np.load(GAME/'fixtures/source_art.npz')
    identity = json.loads((GAME/'fixtures/art.json').read_text())
    assert str(source['rom_sha256']) == identity['rom_sha256']
    # The whole world shares one sampling lattice. Keep crisp source pixels;
    # sample canopy coverage on the identical lattice, then restore scene bits.
    ix = ((np.arange(WIDTH)*2+1)*10//14).astype(int)
    iy = ((np.arange(HEIGHT)*2+1)*10//14).astype(int)
    scene = grayscale(source['scene'][iy[:,None],ix])
    foreground = source['foreground'][iy[:,None],ix]
    bank = bitplane(scene&1)+bitplane(scene>>1)+bitplane(foreground)
    assert len(bank) == PLANE*3
    (GAME/'mizscene.bin').write_bytes(bank)
    actors=[]; masks=[]; actor_words=[]
    for rgba in source['actors']:
        small=np.asarray(Image.fromarray(rgba).resize((22,28),Image.Resampling.NEAREST))
        level=np.zeros((32,32),np.uint8);mask=np.zeros((32,32),bool)
        level[2:30,5:27]=grayscale(small[:,:,:3],True)
        mask[2:30,5:27]=small[:,:,3]!=0
        level[~mask]=0
        expanded=mask.copy()
        for dy in (-1,0,1):
            for dx in (-1,0,1):
                expanded[1:-1,1:-1] |= mask[1+dy:31+dy,1+dx:31+dx]
        assert not expanded[[0,-1],:].any() and not expanded[:,[0,-1]].any()
        actors.append(level); masks.append(expanded)
        actor_words.extend(sprite_words(level,expanded,32))
    assert len(actors)==44
    scale=[x*7//10 for x in range(721)]
    for order,suffix in (('<',''),('>','.be')):
        payload=struct.pack(order+f'{len(actor_words)}I',*actor_words)
        payload+=struct.pack(order+'721H',*scale)
        (GAME/f'mizactor{suffix}.bin').write_bytes(payload)
    bounds=[]
    for mask in masks:
        yy,xx=np.nonzero(mask)
        bounds.append([int(xx.min()),int(yy.min()),int(xx.max()-xx.min()+1),int(yy.max()-yy.min()+1)])
    bounds_c='static const u8 zoom_actor_bounds[44][4]={'+','.join('{'+','.join(map(str,b))+'}' for b in bounds)+'};\n'
    (GAME/'zoom_generated.h').write_text(f'''/* Offline 70-percent view. */
#define ZOOM_W {WIDTH}
#define ZOOM_H {HEIGHT}
#define ZOOM_PLANE {PLANE}
#define ZOOM_SCENE_SIZE {len(bank)}
#define ZOOM_SCALE_OFFSET {len(actor_words)*4}
#define ZOOM_ACTOR_SIZE {len(actor_words)*4+len(scale)*2}
'''+bounds_c)
    # Full target oracle from source-derived arrays, independent of native
    # packed-plane shifts. Check poses, depths, camera phases and clipping.
    scenarios=[(7,246,160,2,True),(8,246,160,2,False)]
    scenarios += [(16+i,248,136,i,True) for i in range(44)]
    scenarios += [(64+i,320+i,184,2,True) for i in range(32)]
    scenarios += [(96+i,x,y,2,True) for i,(x,y) in enumerate(((8,12),(712,12),(8,316),(712,316)))]
    with (GAME/'fixtures/zoom_pixels.bin').open('wb') as f:
        f.write(struct.pack('<H',len(scenarios)))
        for n,x,y,pose,cover in scenarios:
            ax,ay=scale[x]-16,scale[y]-24
            cx=max(0,min(scale[x]-80,WIDTH-160))
            cy=max(0,min(scale[y]-58,HEIGHT-100))
            world=scene.copy()
            x0,y0,x1,y1=max(0,ax),max(0,ay),min(WIDTH,ax+32),min(HEIGHT,ay+32)
            region=world[y0:y1,x0:x1]
            mask=masks[pose][y0-ay:y1-ay,x0-ax:x1-ax]
            level=actors[pose][y0-ay:y1-ay,x0-ax:x1-ax]
            region[mask]=level[mask]
            if cover:
                fg=foreground[y0:y1,x0:x1]
                region[fg]=scene[y0:y1,x0:x1][fg]
            f.write(struct.pack('<H',n))
            f.write(world[cy:cy+100,cx:cx+160].tobytes())
    out=GAME/'captures/zoom';out.mkdir(parents=True,exist_ok=True)
    Image.fromarray(255-scene*85).save(out/'world.png')
    preview=Image.new('RGBA',(256,192),(170,170,170,255))
    for i,(level,mask) in enumerate(zip(actors,masks)):
        rgba=np.zeros((32,32,4),np.uint8)
        rgba[:,:,:3]=(255-level*85)[:,:,None];rgba[:,:,3]=mask*255
        preview.alpha_composite(Image.fromarray(rgba),(i%8*32,i//8*32))
    preview.save(out/'poses.png')
    metadata={'scale':[7,10],'world':[WIDTH,HEIGHT],'actor_canvas':[32,32],
              'actor_anchor':[16,24],'poses':len(actors),'source_sha256':sha((GAME/'fixtures/source_art.npz').read_bytes()),
              'pixel_checks':len(scenarios)*160*100,
              'banks':{n:sha((GAME/n).read_bytes()) for n in ('mizscene.bin','mizactor.bin','mizactor.be.bin')}}
    (GAME/'fixtures/zoom.json').write_text(json.dumps(metadata,indent=2)+'\n')
    print(f'70% view: {WIDTH}x{HEIGHT}, {len(actors)} poses, {metadata["pixel_checks"]} oracle pixels')


if __name__=='__main__':main()
