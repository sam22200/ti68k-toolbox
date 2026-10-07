#!/usr/bin/env python3
"""Cold-boot the local USA ROM and verify deterministic reference snapshots."""
import hashlib
import json
from pathlib import Path
import sys

ROOT=Path(__file__).resolve().parents[3]
GAME=ROOT/'games/megamanx'
OUT=ROOT/'sources/megamanx_snes'
ROM=ROOT/'roms/snes/Mega Man X (USA).sfc'
ROM_HASH='3e1209f473bff8cd4bcbf71d071e7f8df17a2d564e9a5c4c427ee8198cebb615'
sys.path.insert(0,str(ROOT/'tools/snes'))
from snesrun import SNES,script,PAD,DEFAULT_CORE

X,Y,VX,VY=0x7e0bad,0x7e0bb0,0x7e0bc2,0x7e0bc4
CAMX,CAMY,STAGE=0x7e1e4d,0x7e1e50,0x7e1f7a

def run(s,buttons=()):
    s.pressed=sum(1<<PAD[b] for b in buttons)
    s.lib.retro_run()

def snapshot(s):
    return tuple(hashlib.sha256(p).hexdigest() for p in (s.dump(),*s.ppu(),s.image().tobytes()))

def save_scene(s,name):
    s.image().save(OUT/(name+'.png'))
    (OUT/(name+'.wram')).write_bytes(s.dump())
    for n,p in zip(('vram','cgram','oam','regs'),s.ppu()):
        (OUT/(name+'.'+n)).write_bytes(p)

def boot(s):
    keys=script(GAME/'keys/ref_boot.txt');s.pressed=0
    for f in range(2200):
        if f in keys:s.pressed=keys[f]
        s.lib.retro_run()
    assert s.read(STAGE)==0 and (s.read(X,2),s.read(Y,2))==(128,367)
    assert s.read(0x7e0baa)==0 and s.read(VX,2)==0 and s.read(VY,2)==0

def main():
    assert hashlib.sha256(ROM.read_bytes()).hexdigest()==ROM_HASH
    OUT.mkdir(parents=True,exist_ok=True)
    s=SNES(ROM)
    try:
        boot(s);s.save(OUT/'start.state');save_scene(s,'start')
        identity={**s.identity,'rom_sha256':ROM_HASH,'copier_header':False,
            'core_sha256':hashlib.sha256(DEFAULT_CORE.read_bytes()).hexdigest(),
            'core_revision':'fae2fea08f74180759ef540ee94259213f503480',
            'options':{k.decode():v.decode() for k,v in s.options.items()},
            'boot_frames':2200,'keys_sha256':hashlib.sha256((GAME/'keys/ref_boot.txt').read_bytes()).hexdigest(),
            'injections':[],'sampling':'after retro_run; zero-based frame after state load',
            'spawn':{'x':s.read(X,2),'y':s.read(Y,2),'camera_x':s.read(CAMX,2),'camera_y':s.read(CAMY,2)}}
        (OUT/'start.json').write_text(json.dumps(identity,indent=2)+'\n')
        traces=[]
        for repeat in range(2):
            s.load(OUT/'start.state');rows=[]
            for f in range(100):
                run(s,['RIGHT','B','Y'] if f<24 else ['LEFT'] if f>=70 else [])
                rows.append(snapshot(s))
            traces.append(rows)
        assert traces[0]==traces[1]
        print('Cold Highway door:',identity['spawn'],'100 WRAM/PPU/video replay frames agree')
    finally:s.close()

if __name__=='__main__':main()
