#!/usr/bin/env python3
"""Style survey of upcoming FFT battle maps for shared projected-block banks.

Every map (Gariland and Orbonne included) goes through ONE uniform offline pipeline:
the original mesh/textures rendered as extract.py does (4x supersampled, campaign
handedness), four greys by the map's own luminance percentiles, plus the selected
geometric height edges (isometric_style.outline). All four rotations.
Outputs under x/survey/ (local; ROM-derived).
"""
from pathlib import Path
import argparse
import json
import struct
import sys
import time
import numpy as np
from PIL import Image, ImageDraw

import build
extract=build.extract
sys.path.insert(0,str(build.GAME/'tools'))
from isometric_style import ledges,outline
import vq

OUT=build.OUT/'survey'
# Story battles in order (map IDs from cavesofnarshe.com/fft/maps, PSX).
MAPS=[(56,'Orbonne Monastery'),(22,'Gariland'),(85,'Mandalia Plains'),(74,'Sweegy Woods'),
      (31,'Dorter Trade City'),(34,'Sand Rat Cellar'),(91,"Thieves' Fort"),(77,'Lenalia Plateau'),
      (72,'Fovoham Plains'),(49,'Fort Zeakden'),(80,'Araguay Woods'),(83,'Zirekile Falls'),
      (35,'Zaland Fort City'),(84,'Bariaus Hill'),(78,'Zigolis Swamp'),(38,'Goug Machine City'),
      (87,'Bariaus Valley'),(63,'Golgorand Execution Site'),(12,'Lionel Castle Gate'),
      (13,'Inside Lionel Castle'),(27,'Goland Coal City'),(28,'Colliery Underground 1')]


def render_map(disc,files,number):
    extract.MAP=number;recs=extract.gns_records(disc,files)
    mesh=disc.read(*next(r[1:] for r in recs if r[0] in (0x2e01,0x2f01)))  # 0x2f01: Sand Rat Cellar's primary mesh
    t=np.frombuffer(disc.read(*next(r[1:] for r in recs if r[0]==0x1701)),np.uint8)
    tex=np.empty((1024,256),np.uint8);tex[:,::2]=(t&15).reshape(1024,128);tex[:,1::2]=(t>>4).reshape(1024,128)
    pal=np.frombuffer(mesh,'<u2',256,struct.unpack_from('<I',mesh,0x44)[0]).reshape(16,16)
    at=struct.unpack_from('<I',mesh,0x68)[0];nx,nz=mesh[at:at+2]
    polys=extract.polygons(mesh);edges=ledges(polys)
    hmax=max(int(-v[:,1].min()/12)+1 for v,*_ in polys)
    width=(12*(nx+nz)+15)//8*8;rows=6*(nx+nz)+6*hmax+8
    extract.SC_W=width;extract.SC_OY=6*hmax+8
    views=[];q=None
    for rot in range(4):
        lum,_,_,depth=extract.render(polys,tex,pal,rot,nx,nz,rows,return_depth=True)
        small=lum.reshape(rows,4,width,4).mean((1,3));bg=(lum==extract.BG).reshape(rows,4,width,4).mean((1,3))>.5
        if q is None:q=np.percentile(small[~bg],[25,55,82])
        g=np.where(small>q[2],0,np.where(small>q[1],1,np.where(small>q[0],2,3))).astype(np.uint8);g[bg]=0
        g,_=outline(g,depth,edges,rot,nx,nz);g[bg]=0
        g=g[:,::-1]  # campaign handedness (mirrored like ffgv*)
        keep=np.flatnonzero((g!=0).any(1));g=g[max(keep[0]-1,0):keep[-1]+2]
        views.append(np.pad(g,((0,(-g.shape[0])%8),(0,0))).astype(np.int8))
    extract.SC_W,extract.SC_OY=320,64
    return views,dict(nx=int(nx),nz=int(nz),width=width)


def main():
    parser=argparse.ArgumentParser(__doc__);parser.add_argument('--render',action='store_true');parser.add_argument('--all',action='store_true')
    args=parser.parse_args();OUT.mkdir(parents=True,exist_ok=True)
    disc,files=extract.disc_files(str(build.ROOT/'roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue'))
    info={}
    parser_maps=MAPS if not args.all else [(n,'') for n in range(1,120) if 'MAP/MAP%03d.GNS'%n in files]
    for number,name in parser_maps:
        path=OUT/('map%03d.npz'%number)
        if args.render or not path.exists():
            t=time.time()
            try:views,meta=render_map(disc,files,number)
            except Exception as e:
                extract.SC_W,extract.SC_OY=320,64;print(number,name,'skipped:',type(e).__name__,e,flush=True);continue
            np.savez_compressed(path,*views);meta['seconds']=time.time()-t
            info[number]=meta;print(number,name,meta,flush=True)
            Image.fromarray((255-views[0].astype(int)*85).astype(np.uint8)).save(OUT/('map%03d.png'%number))
    if info:(OUT/'render.json').write_text(json.dumps(info,indent=2)+'\n')

if __name__=='__main__':main()
