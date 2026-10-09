#!/usr/bin/env python3
"""Independent unpacked pixel expectations for swords and changed scenery."""
import struct
import numpy as np
from PIL import Image
from art import grayscale
from ground import ground_gray
from reference import extract
from traversal import GAME


def main():
    source=np.load(GAME/'fixtures/source_art.npz')
    actions=np.load(GAME/'fixtures/source_actions.npz')
    assets=extract();ids=np.frombuffer(assets['map_bottom'],dtype='<u2').reshape(63,63)
    types=np.frombuffer(assets['types_bottom'],dtype='<u2')
    for zoom in (False,True):
        scene=grayscale(source['scene']);fg=source['foreground']
        changed=scene.copy()
        for y in range(20):
            for x in range(45):
                t=types[ids[y,x]]
                if t in (63,78):
                    changed[y*16:y*16+16,x*16:x*16+16]=ground_gray(actions['patches'][int(t==78)])
        changed[fg]=scene[fg]
        if zoom:
            ix=(np.arange(504)*2+1)*10//14;iy=(np.arange(224)*2+1)*10//14
            scene=scene[iy[:,None],ix];changed=changed[iy[:,None],ix];fg=fg[iy[:,None],ix]
        cases=[(100+i,i,False) for i in range(40)]+[(141,-1,True),(142,14,True)]
        with (GAME/f'fixtures/action_pixels{"_zoom" if zoom else ""}.bin').open('wb') as f:
            f.write(struct.pack('<H',len(cases)))
            for scenario,pose,cut in cases:
                world=(changed if cut else scene).copy()
                if pose<0:
                    rgba=source['actors'][0];anchor=(16,32)
                    if zoom:
                        # The existing walking bank retains its separate M2
                        # canvas/anchor convention.
                        small=np.asarray(Image.fromarray(rgba).resize((22,28),Image.Resampling.NEAREST))
                        rgba=np.zeros((32,32,4),np.uint8);rgba[2:30,5:27]=small;anchor=(16,24)
                else:
                    rgba=actions['actors'][pose];anchor=(32,36)
                    if zoom:
                        rgba=np.asarray(Image.fromarray(rgba).resize((45,39),Image.Resampling.NEAREST));anchor=(23,25)
                mask=np.pad(rgba[:,:,3]!=0,1);level=np.pad(grayscale(rgba[:,:,:3],True),1)
                level[~mask]=0
                halo=mask.copy()
                for dy in (-1,0,1):
                    for dx in (-1,0,1):halo[1:-1,1:-1]|=mask[1+dy:mask.shape[0]-1+dy,1+dx:mask.shape[1]-1+dx]
                px,py=(424*7//10,160*7//10) if zoom else (424,160)
                ax,ay=px-anchor[0]-1,py-anchor[1]-1
                h,w=halo.shape;region=world[ay:ay+h,ax:ax+w]
                region[halo]=level[halo]
                region[fg[ay:ay+h,ax:ax+w]]=(changed if cut else scene)[ay:ay+h,ax:ax+w][fg[ay:ay+h,ax:ax+w]]
                cy=py-58;cx=px-80
                f.write(struct.pack('<H',scenario))
                f.write(world[cy:cy+100,cx:cx+160].tobytes())
        print('Sword/cut pixel oracle:', '70%' if zoom else '100%',len(cases)*160*100,'pixels')


if __name__=='__main__':main()
