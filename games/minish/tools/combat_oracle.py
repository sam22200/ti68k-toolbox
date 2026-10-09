#!/usr/bin/env python3
"""Independent uncompressed enemy/hero/canopy expectations at both scales."""
import json
import struct
import numpy as np
from PIL import Image
from art import grayscale
from traversal import GAME

def halo(mask):
    padded=np.pad(mask,1);out=padded.copy()
    for dy in (-1,0,1):
        for dx in (-1,0,1):out[1:-1,1:-1]|=padded[1+dy:padded.shape[0]-1+dy,1+dx:padded.shape[1]-1+dx]
    return out

def main():
    source=np.load(GAME/'fixtures/source_art.npz')
    combat=np.load(GAME/'fixtures/source_combat.npz');enemies,hurt=combat['actors'],combat['hurt']
    # Native damage flash on four greys: one step darker or a black body.
    flashes=(np.array([0,2,3,3],np.uint8),np.array([0,3,3,3],np.uint8))
    art=json.loads((GAME/'fixtures/art.json').read_text())
    for zoom in (False,True):
        ix=(np.arange(504)*2+1)*10//14;iy=(np.arange(224)*2+1)*10//14
        scene=grayscale(source['scene'][iy[:,None],ix] if zoom else source['scene'])
        fg=source['foreground'][iy[:,None],ix] if zoom else source['foreground']
        actor=source['actors'][art['idle'][2]]
        if zoom:
            small=np.asarray(Image.fromarray(actor).resize((22,28),Image.Resampling.NEAREST))
            actor=np.zeros((32,32,4),np.uint8);actor[2:30,5:27]=small
        def draw(world,rgba,x,y,anchor,lut=None):
            mask=rgba[:,:,3]!=0;levels=grayscale(rgba[:,:,:3],True);levels[~mask]=0
            if lut is not None:levels=lut[levels]
            levels=np.pad(levels,1);mask=halo(mask)
            ax,ay=x-anchor[0]-1,y-anchor[1]-1
            h,w=mask.shape;x0,y0,x1,y1=max(0,ax),max(0,ay),min(scene.shape[1],ax+w),min(scene.shape[0],ay+h)
            if x0>=x1 or y0>=y1:return
            part=world[y0:y1,x0:x1];m=mask[y0-ay:y1-ay,x0-ax:x1-ax];l=levels[y0-ay:y1-ay,x0-ax:x1-ax]
            part[m]=l[m];cover=fg[y0:y1,x0:x1];part[cover]=scene[y0:y1,x0:x1][cover]
        with (GAME/'fixtures'/('combat_pixels_zoom.bin' if zoom else 'combat_pixels.bin')).open('wb') as f:
            f.write(struct.pack('<H',72))
            for group in range(3):
                px,py=(248,136) if group==2 else (280,184)
                ex,ey=(246,160) if group==2 else (292,184) if group==1 else (280,152)
                scale=lambda v:v*7//10 if zoom else v
                cx=max(0,min(scale(px)-80,scene.shape[1]-160));cy=max(0,min(scale(py)-58,scene.shape[0]-100))
                for pose,enemy in enumerate(enemies):
                    if zoom:enemy=np.asarray(Image.fromarray(enemy).resize((34,34),Image.Resampling.NEAREST))
                    world=scene.copy()
                    eh=(17,23) if zoom else (24,32);ph=(16,24) if zoom else (16,32)
                    if ey<py:draw(world,enemy,scale(ex),scale(ey),eh)
                    draw(world,actor,scale(px),scale(py),ph)
                    if ey>=py:draw(world,enemy,scale(ex),scale(ey),eh)
                    f.write(struct.pack('<H',300+group*20+pose));f.write(world[cy:cy+100,cx:cx+160].tobytes())
            # Doors 408..417: knockback poses; 418/419: idle Link in two flash phases.
            scale=lambda v:v*7//10 if zoom else v
            cx=max(0,min(scale(280)-80,scene.shape[1]-160));cy=max(0,min(scale(184)-58,scene.shape[0]-100))
            for pose,rgba in enumerate(hurt):
                if zoom:rgba=np.asarray(Image.fromarray(rgba).resize((45,45),Image.Resampling.NEAREST))
                world=scene.copy();draw(world,rgba,scale(280),scale(184),(23,34) if zoom else (32,48))
                f.write(struct.pack('<H',408+pose));f.write(world[cy:cy+100,cx:cx+160].tobytes())
            for door,lut in zip((419,418),flashes):
                world=scene.copy();draw(world,actor,scale(280),scale(184),(16,24) if zoom else (16,32),lut)
                f.write(struct.pack('<H',door));f.write(world[cy:cy+100,cx:cx+160].tobytes())
        print('Combat oracle:', '70%' if zoom else '100%',72*160*100,'pixels')

if __name__=='__main__':main()
