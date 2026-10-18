#!/usr/bin/env python3
"""Local disc textures -> native affine turn UVs and compact grey/alpha atlases."""
import hashlib
import json
from pathlib import Path
import struct

import numpy as np
from attack_texture import load_scene

HERE=Path(__file__).resolve().parents[1]


def prepare(destination):
    destination.mkdir(parents=True,exist_ok=True)
    faces,source,palettes,thresholds,strong,water=load_scene()
    groups=sorted({(p,page,bool(water[i])) for i,(_,uv,p,page) in enumerate(faces) if uv is not None})
    assert len(groups)==16
    luminance=((palettes&31)*.299+((palettes>>5)&31)*.587+((palettes>>10)&31)*.114)*255/31
    pages=[]
    for p,page,wet in groups:
        indices=source[page*256:(page+1)*256]
        opaque=palettes[p,indices]!=0
        lum=luminance[p,indices]
        # Filter once offline at 2x2. Runtime samples one nearest texel at 80x50.
        count=opaque.reshape(128,2,128,2).sum((1,3))
        total=np.where(opaque,lum,0).reshape(128,2,128,2).sum((1,3))
        average=total/np.maximum(count,1)
        q=thresholds if wet else strong
        grey=np.where(average>q[2],0,np.where(average>q[1],1,np.where(average>q[0],2,3))).astype(np.uint8)
        if wet: grey=np.clip(grey,1,2)
        grey[count==0]=4 # transparent nibble; no separate alpha fetch per pixel
        pages.append((grey[:,::2]<<4 | grey[:,1::2]).tobytes())
    uv=bytearray(struct.pack('>4H',0x5454,1,len(faces),len(groups)))
    for i,(_,coords,p,page) in enumerate(faces):
        if coords is None: uv+=bytes([0]*6+[255,0]); continue
        coord=(np.asarray(coords,dtype=np.uint16)//2).astype(np.uint8)
        uv+=bytes([*coord.ravel(),groups.index((p,page,bool(water[i]))),0])
    (destination/'fftuv.bin').write_bytes(uv)
    names=['fftuv']
    for bank,start in enumerate(range(0,len(pages),7)):
        name=f'fftat{bank}'; names.append(name)
        (destination/(name+'.bin')).write_bytes(b''.join(pages[start:start+7]))
    report=dict(scope='Original MAP022 UVs and palette/page textures; no camera images or projected draw commands.',
        faces=len(faces),groups=len(groups),atlas='128x128, two grey/alpha nibbles per byte; 0..3 shades, 4 transparent',
        filtering='Offline 2x2 source filtering, existing contrast-plus thresholds; water limited to light/dark grey. Runtime nearest neighbour.',
        banks={n:dict(bytes=(destination/(n+'.bin')).stat().st_size,sha256=hashlib.sha256((destination/(n+'.bin')).read_bytes()).hexdigest()) for n in names})
    (destination/'texture-assets.json').write_text(json.dumps(report,indent=2)+'\n')
    return report


if __name__=='__main__': print(json.dumps(prepare(HERE/'x/turn-texture/assets'),indent=2))
