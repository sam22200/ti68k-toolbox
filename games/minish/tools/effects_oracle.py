#!/usr/bin/env python3
"""Independent uncompressed expectations for every roll/destruction pose."""
import struct
import numpy as np
from PIL import Image
from art import grayscale
from traversal import GAME


def main():
    source=np.load(GAME/'fixtures/source_art.npz')
    actors=np.load(GAME/'fixtures/source_effects.npz')['actors']
    for zoom in (False,True):
        ix=(np.arange(504)*2+1)*10//14;iy=(np.arange(224)*2+1)*10//14
        scene=grayscale(source['scene'][iy[:,None],ix] if zoom else source['scene'])
        fg=source['foreground'][iy[:,None],ix] if zoom else source['foreground']
        with (GAME/'fixtures'/('effects_pixels_zoom.bin' if zoom else 'effects_pixels.bin')).open('wb') as f:
            f.write(struct.pack('<H',len(actors)*3))
            for group,(px,py) in enumerate(((320,184),(246,160),(552,128))):
                scale=lambda v:v*7//10 if zoom else v
                x,y=scale(px),scale(py)
                cx=max(0,min(x-80,scene.shape[1]-160));cy=max(0,min(y-58,scene.shape[0]-100))
                for pose,rgba in enumerate(actors):
                    if zoom:rgba=np.asarray(Image.fromarray(rgba).resize((45,45),Image.Resampling.NEAREST))
                    world=scene.copy();anchor=(23,34) if zoom else (32,48)
                    mask=np.pad(rgba[:,:,3]!=0,1);halo=mask.copy()
                    for dy in (-1,0,1):
                        for dx in (-1,0,1):halo[1:-1,1:-1]|=mask[1+dy:mask.shape[0]-1+dy,1+dx:mask.shape[1]-1+dx]
                    levels=np.pad(grayscale(rgba[:,:,:3],True),1);levels[~mask]=0
                    ax,ay=x-anchor[0]-1,y-anchor[1]-1;h,w=halo.shape
                    region=world[ay:ay+h,ax:ax+w];region[halo]=levels[halo]
                    cover=fg[ay:ay+h,ax:ax+w];region[cover]=scene[ay:ay+h,ax:ax+w][cover]
                    f.write(struct.pack('<H',512+group*len(actors)+pose));f.write(world[cy:cy+100,cx:cx+160].tobytes())
        print('Roll/effect oracle:', '70%' if zoom else '100%',len(actors)*3*160*100,'pixels')


if __name__=='__main__':main()
