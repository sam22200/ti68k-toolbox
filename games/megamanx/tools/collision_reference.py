#!/usr/bin/env python3
"""Controlled roller/shot hitbox and braking-threshold probes on the ROM."""
from reference import *
from rom import DATA,pc

BASE=0x7e0e68
SHOT=0x7e1228
def main():
    directory=OUT/'collision';directory.mkdir(exist_ok=True)
    s=SNES(ROM);cases=[]
    try:
        for kind,name in [(0,'pellet'),(1,'medium'),(3,'large')]:
            state=OUT/'enemy'/f'collider-{name}.state';s.load(state)
            assert s.read(BASE+0x27)==2 and s.read(SHOT+10)==kind
            for leftward in (False,True):
                rows=[]
                for axis in ('x','y'):
                    for delta in range(-56 if axis=='x' else -36,57 if axis=='x' else 37):
                        s.load(state)
                        # Component isolation: current and previous positions
                        # agree, motion is zero, and the hero stays untouched.
                        for address,x,y in [(BASE,600,364),(SHOT,600+(delta if axis=='x' else 0),364+(delta if axis=='y' else 0))]:
                            for offset,value in [(5,x),(8,y),(0x22,x),(0x24,y),(0x1a,0)]:s.write(address+offset,value&65535,2)
                            s.write(address+4,0)
                        flags=s.read(SHOT+0x11)
                        s.write(SHOT+0x11,(flags&~64) if leftward else (flags|64))
                        run(s,[])
                        hp=s.read(BASE+0x27)&127;phase=s.read(BASE+1)
                        rows.append(dict(axis=axis,delta=delta,hit=hp!=2 or phase==4,
                            enemy_x=s.read(BASE+5,2),shot_x=s.read(SHOT+5,2),hp=hp,phase=phase))
                bounds={a:(min(r['delta'] for r in rows if r['axis']==a and r['hit']),max(r['delta'] for r in rows if r['axis']==a and r['hit'])) for a in ('x','y')}
                dx=[r['shot_x']-r['enemy_x'] for r in rows if r['axis']=='x' and r['hit']]
                actual_bounds=dict(x=(min(dx),max(dx)),y=bounds['y'])
                cases.append(dict(kind=kind,leftward=leftward,bounds=bounds,actual_bounds=actual_bounds,samples=rows));print(name,leftward,actual_bounds)
        thresholds=[]
        for vx in range(-180,-149):
            s.load(OUT/'enemy/brake.state');s.write(BASE+0x1a,vx&65535,2);run(s,[])
            thresholds.append(dict(vx=vx,after=s.read(BASE+0x1a,2,True),fuse=s.read(BASE+0x34),flag=s.read(BASE+0x3b)))
        print('braking fuse threshold',[(r['vx'],r['fuse'],r['after']) for r in thresholds if r['fuse']])
        duration=[]
        for vx in (-379,-180,0,180):
            s.load(OUT/'enemy/brake.state');s.write(BASE+0x1a,vx&65535,2)
            first=None
            for frame in range(48):
                run(s,[])
                if first is None and s.read(BASE+0x34):first=frame
            duration.append(dict(vx=vx,first_fuse_frame=first))
        print('braking duration after VX changes',duration)
        contact=[]
        for axis in ('x','y'):
            for delta in range(-36,37):
                s.load(OUT/'combat/natural-260.state')
                x=600+(delta if axis=='x' else 0);y=364+(delta if axis=='y' else 0)
                for offset,value in [(5,x),(8,y),(0x22,x),(0x24,y)]:s.write(0x7e0ba8+offset,value,2)
                run(s,[])
                contact.append(dict(axis=axis,delta=delta,hit=s.read(0x7e0bcf)!=16,
                    player_x=s.read(X,2),player_y=s.read(Y,2),enemy_x=s.read(BASE+5,2)))
        contact_bounds={a:(min(r['delta'] for r in contact if r['axis']==a and r['hit']),max(r['delta'] for r in contact if r['axis']==a and r['hit'])) for a in ('x','y')}
        print('contact bounds relative to placed enemy',contact_bounds)
        profiles=[]
        for kind,hold in [(0,1),(1,45),(3,110)]:
            s.load(OUT/'start.state')
            if hold>1:
                for frame in range(hold):run(s,['Y'])
            rows=[]
            for age in range(52):
                if age:
                    # Keep only this shot on-screen to measure its complete
                    # finite formation and repeated flight collision profile.
                    s.write(SHOT+5,200,2);s.write(SHOT+0x22,200,2)
                run(s,['Y'] if kind==0 and not age else [])
                pointer=s.read(SHOT+0x20,2)
                raw=DATA[pc(0x860000|pointer):pc(0x860000|pointer)+4] if pointer else bytes(4)
                rows.append(dict(age=age,pointer=pointer,box=list(raw),active=s.read(SHOT)))
            profiles.append(dict(kind=kind,rows=rows));print('profile',kind,[(r['age'],hex(r['pointer'])) for r in rows[:18]])
        s.load(OUT/'combat/natural-260.state');player_pointer=s.read(0x7e0bc8,2)
        boxes={name:list(DATA[pc(0x860000|pointer):pc(0x860000|pointer)+4]) for name,pointer in [('hero',player_pointer),('armor',0xca2b),('body',0xca35)]}
        print('ROM actor boxes',hex(player_pointer),boxes)
        (directory/'reference.json').write_text(json.dumps(dict(isolation='Hitboxes: current/previous actor and shot coordinates plus zero VX anchored; shot direction flag chosen; player/camera unchanged. Braking: only roller VX changed. Contact: only player current/previous coordinates changed. Profiles: only shot current/previous X kept on-screen.',cases=cases,thresholds=thresholds,duration=duration,contact=contact,contact_bounds=contact_bounds,profiles=profiles,boxes=boxes),indent=2)+'\n')
    finally:s.close()
if __name__=='__main__':main()
