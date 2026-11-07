#!/usr/bin/env python3
"""Runtime-projection bank: shared 8x8 face textures + shared texel maps per piece shape.

Writes under x/proj/: spgeo.bin (common: shape tables, per-piece texel maps,
textures), spmgar.bin/spmorb.bin (per-tile heights + face pattern indices), the
oracle six-plane views <map><rot>.raw and .89y variables. Same face sampling and
Gariland-first sharing as faces.py.
"""
from pathlib import Path
import argparse
import json
import struct
import subprocess
import numpy as np
from PIL import Image

import build
import faces

OUT=build.OUT/'proj'


def row_spans(mask,tu,tw):
    rows=[]
    for r in range(mask.shape[0]):
        x=np.flatnonzero(mask[r])
        if not len(x):rows.append((0,b''));continue
        assert x[-1]-x[0]+1==len(x),'non-convex piece row'
        rows.append((int(x[0]),bytes(int(tw[r,c])*8+int(tu[r,c]) for c in x)))
    return rows


def serialize(maps,pats,views,k_top,k_wall,extras):
    chosen,assigned=faces.choose_bank(pats,k_top,k_wall,extras)
    textures=np.vstack([chosen['top'],chosen['wall']]).astype(np.uint8)
    wall_base=len(chosen['top']);assert len(textures)<=255
    # Every piece shape any rotation of either map needs.
    shapes=faces.Shapes();keys=set()
    for m in maps:
        for rot in range(4):keys|={p[1] for p in faces.geometry(m,rot)}
    tops=sorted(k for k in keys if k[0]=='top')
    shape_keys=sorted({k[1] for k in tops});shape_index={s:i for i,s in enumerate(shape_keys)}
    walls=[('unit',0),('unit',1)]+sorted(k for k in keys if k[0]=='cap')
    pieces=tops+walls;pid={p:i for i,p in enumerate(pieces)};assert len(pieces)<255
    bank=bytearray(24);shapes_off=len(bank)
    bank+=bytes(shape_index.get(i,255) for i in range(256));tops_off=len(bank)
    for s in shape_keys:bank+=bytes(pid.get(('top',s,r),255) for r in range(4))
    units_off=len(bank);bank+=bytes((pid['unit',0],pid['unit',1]))
    caps_off=len(bank)
    for side in range(2):bank+=bytes(pid.get(('cap',side,d),255) for d in range(-3,4))
    if len(bank)&1:bank.append(0)
    directory=len(bank);bank+=bytes(4*len(pieces))
    for i,p in enumerate(pieces):
        ox,oy,mask,tu,tw=shapes(p);rows=row_spans(mask,tu,tw)
        struct.pack_into('>BbbB',bank,directory+4*i,len(rows),ox,oy,0)
        # Offsets follow the directory in order: a running cursor, no table.
        for x0,texels in rows:bank+=bytes((x0,len(texels)))+texels
    geometry_bytes=len(bank)-directory
    if len(bank)&1:bank.append(0)
    # Stored bit 7 = light plane, bit 6 = dark plane (proj68k.s shifts them into X).
    textures_off=len(bank);bank+=(((textures&1)<<7)|((textures&2)<<5)).astype(np.uint8).tobytes()
    struct.pack_into('>4sHBB7H',bank,0,b'SPG1',len(pieces),len(shape_keys),len(textures),
                     shapes_off,tops_off,units_off,caps_off,directory,textures_off,len(bank))
    (OUT/'spgeo.bin').write_bytes(bank)
    report=dict(k_top=k_top,k_wall=k_wall,orbonne_extras=extras,patterns=len(textures),pieces=len(pieces),
                common_bank_bytes=len(bank),texel_map_bytes=geometry_bytes,
                texture_bytes_expanded=len(textures)*64,texture_bytes_if_2bpp=len(textures)*16,maps={})
    for m in maps:
        a=assigned[m['name']];tile_bytes=bytearray()
        for i,t in enumerate(m['tiles']):
            rec=bytearray(16);rec[:4]=bytes(t[:4])
            rec[4]=a[('top',i)][0] if ('top',i) in a else 0
            for w0 in range(4):
                for level in range(2):
                    f=('wall',i,w0,level)
                    rec[5+2*w0+level]=wall_base+a[f][0] if f in a else wall_base
            tile_bytes+=rec
        data=struct.pack('>4sBBHH6x',b'SPM1',m['nx'],m['nz'],m['rows'],len(m['tiles']))+tile_bytes
        (OUT/('spm'+m['name'][:3]+'.bin')).write_bytes(data)
        # Oracle: the same per-face texture lookup, painted by build's independent painter.
        meta=np.frombuffer(tile_bytes,np.uint8).reshape(-1,16);ratios=[]
        for rot in range(4):
            geo=faces.geometry(m,rot);bpieces=[];pictures=[];commands=[]
            for n,(face,piece,cx,cy,d) in enumerate(geo):
                ox,oy,mask,tu,tw=shapes(piece)
                if face[0]=='top':pat=meta[face[1],4]
                else:pat=meta[face[1],5+2*face[2]+face[3]]
                pic=np.full(mask.shape,-1,np.int8);pic[mask]=textures[pat][tw[mask]*8+tu[mask]]
                bpieces.append((32,mask.shape[0],ox,oy,None));pictures.append(pic);commands.append((n,cx,cy,d))
            raw,color=build.oracle(m,commands,bpieces,pictures)
            (OUT/(m['name']+str(rot)+'.raw')).write_bytes(raw)
            Image.fromarray((255-color.astype(int)*85).astype(np.uint8)).save(OUT/(m['name']+str(rot)+'.png'))
            _,orig,scenery=views[m['name']][rot];ratios.append(float((orig!=color)[scenery].mean()))
        report['maps'][m['name']]=dict(metadata_bytes=len(data),changed_scenery_fraction=ratios)
    return report


def convert():
    for path in OUT.glob('sp*.bin'):
        subprocess.run([build.ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat',path.name,path.stem],cwd=OUT,check=True,capture_output=True)
        v=path.with_suffix('.89y');raw=bytearray(v.read_bytes());raw[0x49]=3;v.write_bytes(raw)


def main():
    parser=argparse.ArgumentParser(__doc__)
    parser.add_argument('--disc',default=str(build.ROOT/'roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue'))
    parser.add_argument('--bank',default='32:64:16',help='tops:walls:orbonne extras')
    args=parser.parse_args();OUT.mkdir(parents=True,exist_ok=True)
    disc,files=build.extract.disc_files(args.disc);maps=build.maps_from_disc(disc,files)
    shapes=faces.Shapes();pats={};views={}
    for m in maps:pats[m['name']],views[m['name']]=faces.sample_patterns(m,shapes)
    report=serialize(maps,pats,views,*map(int,args.bank.split(':')))
    (OUT/'storage.json').write_text(json.dumps(report,indent=2)+'\n');convert()
    print(json.dumps(report,indent=2))


if __name__=='__main__':main()
