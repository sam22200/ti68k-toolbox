#!/usr/bin/env python3
"""Check FWM3 against uncompressed source pixels, actual68000 cache and animation."""
from pathlib import Path
import argparse
import re
import struct
import subprocess
import sys

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
OUT=HERE/'x/world-storage'
sys.path.insert(0,str(HERE.parent/'tools'))
import extract
import interface
import units
import world_assets


def run(args,cwd=HERE):
    p=subprocess.run(list(map(str,args)),cwd=cwd,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    if p.returncode:print(p.stdout);p.check_returncode()
    return p.stdout


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('disc');parser.add_argument('--contrast',choices=('source','strong','light'),default='source')
    args=parser.parse_args();OUT.mkdir(parents=True,exist_ok=True)
    disc,files=extract.disc_files(args.disc)
    g=world_assets.scaled_texture(world_assets.shades(world_assets.texture(disc,files),args.contrast))
    import numpy as np
    raw=b''.join(np.packbits((g&bit)!=0,axis=1).tobytes() for bit in (1,2))
    shp=units.shp_frames(disc.read(*files['BATTLE/TYPE1.SHP']));spr=disc.read(*files['BATTLE/RAMUZA.SPR'])
    animation=b''.join(interface.planes(world_assets.scaled_sprite(units.sprite(spr,shp[k])),16) for k in (11,10,9,10,11,12,13,12))
    ref=b'FWM2'+struct.pack('>5HBB',16,16+len(raw),0,0,0,g.shape[1]//16,g.shape[0]//16)+raw+animation
    (OUT/'ffwref.bin').write_bytes(ref)
    run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat','ffwref.bin','ffwref'],OUT)
    for bank in ('ffwm','ffmg'):
        p=OUT/(bank+'.bin')
        if not p.exists():p.symlink_to(HERE/'data'/(bank+'.bin'))
    # Review-independent oracle fixture: all source locations/routes visible.
    # It is never installed into the campaign or used as shipping progression.
    bank=bytearray((HERE/'data/ffwm.bin').read_bytes())
    if len(bank)&1:bank.append(0)
    struct.pack_into('>H',bank,12,len(bank))
    bank+=bytes((43,))+b''.join(struct.pack('>BBHBx',i,2,0,0) for i in range(43))
    (OUT/'ffwall.bin').write_bytes(bank)
    run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth','-89','dat','ffwall.bin','ffwall'],OUT)
    common=[HERE/'check_world_storage.c',ROOT/'runtime/core/rt_core.c']
    run(['cc','-std=gnu99','-O1','-o',OUT/'storage',*common,ROOT/'runtime/platform-sw/rt_sw.c'])
    pc=run([OUT/'storage'],OUT)
    run([ROOT/'tools/bin/ti-cc','-DRT_CYCLES','-o',OUT/'wstorec',*common,ROOT/'lib/unpack68k.s',ROOT/'tools/extgraph/lib/tilemap.a'])
    ti=run([ROOT/'tools/bin/ti-cycles','--frames',0,'--max',10000,
            '--file',HERE/'data/ffwm.89y','--file',HERE/'data/ffmg.89y','--file',OUT/'ffwref.89y',
            '--file',OUT/'ffwall.89y',OUT/'wstorec.89z'],OUT)
    (OUT/'native.log').write_text(ti);(OUT/'pc.log').write_text(pc)
    match=re.search(r'storage ([0-9A-F]+) failures 0 checks (\d+)',pc)
    values=[int(v,16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)',ti)]
    assert match and values[-3:]==[int(match[1],16),0,int(match[2])],(pc,values[-5:])
    print(pc.strip()+'; source pixels, shifts, edges, eviction, eight poses and cold/warm decorated strips agree PC/68000.')


if __name__=='__main__':main()
