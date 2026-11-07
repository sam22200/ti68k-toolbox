#!/usr/bin/env python3
"""Source-order PC supervisor; private save in x/play-data, one process per chapter."""
import argparse
import subprocess
from pathlib import Path
import sys
HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE.parent/'tools'))
import save_dump
STEPS=(('fftscn_pc',0),('fftbatflow_pc',100),('fftscn_pc',1),
       ('fftdep_pc',200),('fftscn_pc',2),('fftbatflow_pc',200))

def resume(data):
    name='ffsave';selection=data/'ffslot.sav'
    if selection.exists():
        selected=selection.read_bytes()
        if len(selected)==6 and selected[:4]==b'FSL1' and 1<=selected[4]<=3 and not selected[5]:
            name='ffsave'+str(selected[4])
    p=data/(name+'.sav')
    if not p.exists():return 0
    s=save_dump.decode(p.read_bytes())
    if 1 in s['flags']:return 6
    if 0 not in s['flags']:return 0
    phase=s.get('flow',0)
    return 5 if phase>=3 else 4 if phase>=2 else 3 if phase>=1 else 2

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--new',action='store_true');args=ap.parse_args()
    data=HERE/'x/play-data';data.mkdir(parents=True,exist_ok=True)
    for folder in (HERE.parent/'opening/data',HERE/'data'):
        for source in folder.glob('*.bin'):
            p=data/source.name
            if p.is_symlink():p.unlink()
            if not p.exists():p.symlink_to(source)
    def run(program,scenario=0):
        return subprocess.run([str(HERE/program),'--scenario',str(scenario)],cwd=data).returncode
    if run('ffttitle_pc',1 if args.new else 0) not in (1,2):return
    k=resume(data)
    while k<6:
        if run(*STEPS[k])!=23:return
        k+=1
    saved=0
    while True:
        result=run('fftmap_pc',saved);saved=0
        if not result:return
        if result==1:run('fftmng_pc')
        elif result==2:saved=int(run('ffttitle_pc',3)==3)
        elif result>=100:
            if run('fftbatflow_pc',result)!=23:return
        else:raise RuntimeError('Unexpected map result '+str(result))

if __name__=='__main__':main()
