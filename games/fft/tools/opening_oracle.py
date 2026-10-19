#!/usr/bin/env python3
"""Observe the unmodified US opening from cold boot; no event-skipping pokes.

States, original text and screenshots remain local under x/opening/reference.
"""
import argparse, hashlib, json
from pathlib import Path
import oracle

HERE=Path(__file__).resolve().parents[1]
OUT=HERE/'x/opening/reference'

def main():
    ap=argparse.ArgumentParser(__doc__);ap.add_argument('disc');ap.add_argument('--frames',type=int,default=36000)
    ap.add_argument('--story',action='store_true',help='Record dialogue and let the original AI finish the opening battle')
    args=ap.parse_args()
    OUT.mkdir(parents=True,exist_ok=True);p=oracle.PSX(args.disc)
    records=[];seen=set();last=None;event_at=0;last_turn=None;messages=[];active=set();player_saved=False
    for frame in range(args.frames):
        # The same ordinary logo/name/birthday taps as the existing boot recipe,
        # but absolutely no CURRENT_EVENT/NEXT_SCENARIO writes.
        p.pressed=(oracle.key('START') if (frame//60)%3==0 else oracle.key('CIRCLE')) if frame>300 and frame%60<4 else 0
        if args.story and last==5:
            # Explicit instrumentation: AI delegation only, never force damage/victory.
            for slot in range(11):
                a=oracle.UNITS+slot*oracle.UNIT_SIZE+5;p.write(a,p.read(a)&~8)
        p.run();event=oracle.var(p,oracle.EVENT)
        if event!=last:
            records.append(dict(frame=frame,event=event,next=oracle.var(p,oracle.NEXT)))
            print('event',records[-1],flush=True);last=event;event_at=frame
        now=set()
        if 1<=event<=6:
            for w in range(6):
                a=0x8016e43c+w*0x118
                if p.read(a,4,signed=True)!=1:continue
                mid=p.read(a+8,4);key=(event,w,mid);now.add(key)
                if key not in active and mid<200:
                    units=[]
                    for slot in range(16):
                        b=oracle.UNITS+slot*oracle.UNIT_SIZE
                        units.append(dict(slot=slot,gfx=p.read(b),job=p.read(b+3),x=p.read(b+0x47),z=p.read(b+0x48),face=p.read(b+0x49)&3,hp=p.read(b+0x28,2)))
                    messages.append(dict(frame=frame,event=event,window=w,message=mid,units=units))
                    print('message',event,mid,flush=True)
        active=now
        if 0<event<7 and frame-event_at==360 and event not in seen:
            p.save(str(OUT/('event-%02d.state'%event)));p.image().save(OUT/('event-%02d.png'%event))
            seen.add(event);(OUT/('event-%02d-units.txt'%event)).write_text('\n'.join(oracle.unit_table(p))+'\n')
        if event==2 and frame-event_at in (900,1800,3000,4200):
            stem='chapel-%04d'%(frame-event_at);p.save(str(OUT/(stem+'.state')));p.image().save(OUT/(stem+'.png'))
        if event in (3,4,5,6):
            turn=p.read(oracle.TURN_UNIT)
            if turn!=last_turn:
                print('turn',frame,turn,flush=True);last_turn=turn
            if event==5 and turn==0 and p.read(oracle.UNITS+0x186) and not player_saved:
                p.save(str(OUT/'orbonne-player.state'));p.image().save(OUT/'orbonne-player.png')
                (OUT/'orbonne-player-units.txt').write_text('\n'.join(oracle.unit_table(p))+'\n')
                records.append(dict(frame=frame,event=event,door='first Ramza turn'));player_saved=True
                if not args.story:break
        if args.story and event>=7:break
    p.save(str(OUT/'last.state'));p.image().save(OUT/'last.png')
    report=dict(scope=('Original cold boot; controller input, then player-control bit cleared for AI observation; no event/damage/victory pokes' if args.story else 'Original cold boot with controller input only; no event/battle RAM pokes'),frames=frame+1,events=records)
    (OUT/'boot.json').write_text(json.dumps(report,indent=2)+'\n')
    if args.story:(OUT/'story.json').write_text(json.dumps(messages,indent=2)+'\n')
    print(json.dumps(report,indent=2),flush=True)

if __name__=='__main__':main()
