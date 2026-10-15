#!/usr/bin/env python3
"""Rebuild the original training study from cold boot, without a Sony BIOS.
No ROM-derived data is checked in. Matrix translations are observations, not
confirmed writable physics state. States and every input have provenance.
"""
import argparse,json,os,sys,csv,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'.claude/skills/ti-port-ps1/scripts'))
from psxrun import PSX,PAD
from extract import archive,mad
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--disc',default=str(ROOT/'roms/ps1/Tenchu - Stealth Assassins (USA)/Tenchu - Stealth Assassins (USA).cue'))
    a=ap.parse_args();dest=HERE/'local/oracle';dest.mkdir(parents=True,exist_ok=True)
    out=os.fdopen(os.dup(1),'w',buffering=1)
    os.dup2(os.open(os.devnull,os.O_WRONLY),1) # core logs
    p=PSX(a.disc)
    start={t:'START' for t in range(600,6000,300)}
    start.update({t+8:None for t in range(600,6000,300)})
    phases=[(6000,start),(1500,{30:'TRIANGLE',38:None}),(600,{30:'START',31:None}),
            (300,{30:'DOWN',38:None}),(3000,{30:'CROSS',38:None}),(3600,{30:'CROSS',38:None})]
    timeline=[];frame=0
    for count,events in phases:
        p.pressed=0
        for f in range(count):
            if f in events:
                key=events[f];p.pressed=(1<<PAD[key]) if key else 0;timeline.append((frame,key))
            p.run();frame+=1
    raw=bytes(p.ram);data,_,sha=archive(a.disc)
    _,verified=mad(data['MAP.MAD'],raw)
    (dest/'training.ram').write_bytes(raw);p.save(str(dest/'training.state'))
    p.image().save(dest/'training.png')
    ram_base=raw.find(data['MAP.MAD'][:128]);address=ram_base-440
    samples={}
    for name,length in [('walk',100),('jump',120)]:
        p.load(str(dest/'training.state'));rows=[]
        for f in range(length):
            p.pressed=(1<<PAD['UP']) if name=='walk' else ((1<<PAD['CROSS']) if 30<=f<38 else 0)
            p.run();rows.append([f]+[p.read(address+offset,4,True) for offset in (0,4,8)])
        with (dest/f'{name}.csv').open('w',newline='') as file:
            writer=csv.writer(file);writer.writerow(['video_frame','matrix_x','matrix_y','matrix_z']);writer.writerows(rows)
        samples[name]=rows
    delta=[b[3]-a[3] for a,b in zip(samples['walk'],samples['walk'][1:])]
    result=dict(boot_frames=frame,inputs=timeline,archive_sha256=sha,
                verified=verified,observation_address=hex(0x80000000+address),
                walking_mode_delta=max(set(delta),key=lambda d:sum(x==d for x in delta)) if any(delta) else 0,
                maximum_positive_walk_delta=max(delta),observed_jump_apex=-min(r[2] for r in samples['jump']),
                png_sha256=hashlib.sha256((dest/'training.png').read_bytes()).hexdigest(),
                limitation='Observed render translations; ignored position pokes do not establish a physics master')
    (dest/'provenance.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps(result,indent=2),file=out)
if __name__=='__main__':main()
