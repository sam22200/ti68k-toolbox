#!/usr/bin/env python3
"""Observe source scene actors/cameras, including the Academy handoff.

Local snapshots only. Ordinary confirmation taps; no scene or combat RAM writes.
"""
import argparse,json,hashlib
import numpy as np
from pathlib import Path
import oracle

HERE=Path(__file__).resolve().parents[1]
OUT=HERE/'x/opening/choreography'

def actors(p):
    result=[]
    for i in range(24):
        b=0x800b7308+i*0x440
        if not p.read(b+10,2):continue
        if p.read(b+4)>=16 or p.read(b+6)>=135 or p.read(b+0x1e0,2)>=512:continue
        parts=[]
        for j in range(min(8,p.read(b+0x31f))):
            a=b+0x32a+7*j
            parts.append([p.read(a,1,True),p.read(a+1,1,True)]+[p.read(a+k) for k in range(2,7)])
        sprite=None
        if p.read(b+0x1e0,2)>=210:
            vram=np.frombuffer(bytes(p.vram),'<u2').reshape(512,1024)
            tp=p.read(b+0x320,2);clut=p.read(b+0x322,2)
            tx=(tp&15)*64;ty=(tp&16)*16;px=(clut&63)*16;py=clut>>6
            raw=vram[py,px:px+16].tobytes()+bytes(480)+vram[ty:ty+256,tx:tx+64].tobytes()
            sprite=hashlib.sha256(raw).hexdigest()+'.spr'
            (OUT/sprite).write_bytes(raw)
        result.append(dict(slot=i,id=p.read(b+4),sheet=p.read(b+6),parts=parts,sprite=sprite,
                           xyz=[p.read(b+0x40+2*k,2,True) for k in range(3)],
                           facing=p.read(b+0x70,2),animation=p.read(b+0x1dc,2),
                           frame=p.read(b+0x1e0,2),layer=p.read(b+0x14,2)))
    return result

def main():
    ap=argparse.ArgumentParser(__doc__);ap.add_argument('disc');args=ap.parse_args()
    OUT.mkdir(parents=True,exist_ok=True)
    for start,limit in ((2,18000),(6,24000)):
        p=oracle.PSX(args.disc);p.load(str(HERE/('x/opening/reference/event-%02d.state'%start)))
        log=[];active=set();last=-1
        for f in range(limit):
            p.pressed=oracle.key('CIRCLE') if f%90<4 else 0;p.run()
            event=oracle.var(p,oracle.EVENT)
            if event!=last:
                p.save(str(OUT/('event-%02d.state'%event)));last=event
                print('event',event,'frame',f,flush=True)
            now=set();messages=[]
            for w in range(6):
                b=0x8016e43c+w*0x118
                if p.read(b,4,True)==1:
                    key=(event,w,p.read(b+8,4));now.add(key)
                    if key not in active:messages.append(dict(window=w,message=key[2]))
            if messages or f%120==0:
                row=dict(frame=f,event=event,messages=messages,actors=actors(p),
                         matrix=[p.read(0x80098a24+2*i,2,True) for i in range(9)])
                log.append(row)
                if messages or f%600==0:p.image().save(OUT/('%02d-%05d.png'%(start,f)))
            active=now
            if (start==2 and event>=4) or (start==6 and event>=9):break
        (OUT/('scene-%02d.json'%start)).write_text(json.dumps(log,indent=2)+'\n')

if __name__=='__main__':main()
