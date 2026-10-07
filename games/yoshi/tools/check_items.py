#!/usr/bin/env python3
"""Explicit item/physics fields and LCDs on the compiled PC and TI engines."""
import json
from check_terrain import GAME, command, run, values
from check_damage import screen, profile

def main():
    idle=GAME/'x/idle.txt'; idle.write_text('0\n')
    dense=GAME/'x/dense.txt'; dense.write_text('0 RIGHT\n')
    cases=[(60,idle,80),(61,idle,440),(61,GAME/'keys/items-throw.txt',160),
           (62,GAME/'keys/items-spit.txt',80),(62,GAME/'keys/items-swallow.txt',160),
           (63,idle,40),(64,dense,80),(65,GAME/'keys/items-throw.txt',160),(66,dense,80),
           (0,GAME/'keys/actors.txt',1300)]
    cases += [(61,GAME/'keys/aim-lock.txt',90),(67,GAME/'keys/tongue-visible.txt',90),
              (68,GAME/'keys/jump-stomp.txt',100),(69,GAME/'keys/egg-hit.txt',95),
              (70,GAME/'keys/egg-bounce.txt',110),(64,GAME/'keys/aim-dense.txt',80)]
    cases.append((61,GAME/'keys/aim-sweep.txt',80))
    total=0; displays=0
    for scenario,keys,frames in cases:
        pc=values(command([str(GAME/'yjprobe_test'),'--itemtrace',str(keys),str(frames),str(scenario)]))
        ti=values(run('yjitemh.89z',keys,frames,scenario))
        assert len(pc)==133*frames+1,(len(pc),frames)
        assert ti==pc,(scenario,keys,'complete state differs')
        states=[pc[f*133:(f+1)*133] for f in range(frames)]
        if keys.name=='aim-lock.txt':
            assert all((p[3]>>8)&1 and (p[2]>>8)&255==4 for p in states[5:25])
            assert not states[25][3]&256 and (states[30][2]>>8)&255==9
        if scenario==68:
            killed=next(f for f,p in enumerate(states) if not p[108]&255)
            assert min(p[99]>>16 for p in states[:killed])<1850
            assert not states[-1][132]&0xff00
        if scenario==69:
            killed=next(f for f,p in enumerate(states) if not p[108]&255)
            assert killed>=60 and states[56][81]&0xff00 and states[killed][110]>>24==8
        if keys.name=='egg-bounce.txt':
            bounce=next(f for f,p in enumerate(states) if p[81]&255)
            assert states[bounce][80]&0x8000 and states[bounce][81]&0xff00
        total+=frames
        for n in sorted({1,2,4,8,16,32,40,frames//2,frames}):
            if n<=frames: screen(keys,n,scenario); displays+=1
        if scenario in (64,65,66):
            for n in range(1,81): screen(keys,n,scenario); displays+=1
        if scenario in (67,68,69) or keys.name in ('aim-lock.txt','aim-sweep.txt','egg-bounce.txt'):
            for n in range(1,frames+1): screen(keys,n,scenario); displays+=1
        print('PC=TI: scenario',scenario,keys.name,frames,'complete states',flush=True)
    print(total,'complete native PC/TI item states;',displays,'LCD checksums',flush=True)
    profile(dense,80,64,'items_dense')
    profile(GAME/'keys/aim-dense.txt',80,64,'items_aim_dense')
    profile(GAME/'keys/items-throw.txt',160,65,'items_projectiles')
    profile(dense,80,66,'items_sixshots')
    profile(GAME/'keys/actors.txt',1300,0,'items_route')
    (GAME/'x/items-check.json').write_text(json.dumps({'states':total,'screens':displays,'words_per_frame':133},indent=2)+'\n')

if __name__=='__main__': main()
