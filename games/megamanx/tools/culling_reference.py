#!/usr/bin/env python3
"""Confirm exact source projectile culling edges with one-coordinate probes."""
from reference import *
from combat_reference import actors

def main():
    directory=OUT/'culling';directory.mkdir(exist_ok=True)
    s=SNES(ROM);cases=[]
    try:
        for kind,hold,flight,leftward in [(k,h,f,l) for k,h,f in [(0,1,8),(1,31,12),(3,101,10)] for l in (False,True)]:
            s.load(OUT/'start.state')
            if leftward:run(s,['LEFT']);run(s,[])
            for f in range(hold):run(s,['Y'])
            for f in range(flight):run(s,[])
            state=directory/f'kind-{kind}-{"left" if leftward else "right"}.state';s.save(state)
            initial=actors(s,0x1228,0x1428)
            assert len(initial)==1 and initial[0]['kind']==kind
            assert s.read(CAMX,2)==0 and s.read(CAMY,2)==256
            samples=[]
            for position in list(range(-72,-15))+list(range(271,338)):
                s.load(state)
                # Only this projectile's X is changed. Player/camera/clock and
                # its other fields are restored from the same natural flight.
                s.write(0x7e122d,position&65535,2)
                run(s,[])
                live=actors(s,0x1228,0x1428)
                sample=dict(before=position,active=bool(live),after=live[0]['x'] if live else None)
                integrated=s.read(0x7e122d,2,True)
                run(s,[]);following=actors(s,0x1228,0x1428)
                sample.update(second_active=bool(following),second_after=following[0]['x'] if following else None)
                if kind:
                    assert sample['second_active']==(-32<=integrated<288),(kind,leftward,position,sample)
                else:
                    assert sample['active']==(-32<=integrated<288),(kind,leftward,position,sample)
                samples.append(sample)
            left=[r['before'] for r in samples if r['before']<0 and r['second_active']]
            right=[r['before'] for r in samples if r['before']>0 and r['second_active']]
            active=[r['after'] if r['after']<32768 else r['after']-65536 for r in samples if r['active']]
            case=dict(kind=kind,leftward=leftward,initial=initial[0],samples=samples,
                      first_active_left=min(left),last_active_right=max(right),
                      min_active_after=min(active),max_active_after=max(active))
            cases.append(case);print(kind,leftward,'inclusive culling edges',min(left),max(right),'post',min(active),max(active))
        result=dict(isolation='Restore natural flight; change only projectile X at 7E122D before each update. No player/camera/timing anchoring.',
                    verified_rule='Normal removal follows integration; charged removal reacts one frame later to an injected coordinate. Both bounds are [-32,288), relative to the stationary 256-pixel source camera.',samples=sum(len(c['samples']) for c in cases),cases=cases)
        (directory/'reference.json').write_text(json.dumps(result,indent=2)+'\n')
    finally:s.close()
if __name__=='__main__':main()
