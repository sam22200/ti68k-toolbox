#!/usr/bin/env python3
"""Original buster capacity/charge timelines on the compiled TI engine."""
import json
from reference import GAME,OUT
from check_original_ti import execute
from combined_header import observations
from capacity_header import cases

def main():
    directory=GAME/'x/original';directory.mkdir(exist_ok=True)
    total=0;frames=0;reports=[]
    for case in cases():
        keys=directory/('capacity-'+case['name']+'.txt')
        keys.write_text(''.join(str(f)+' '+('B' if held else '')+'\n' for f,held in case['keys'].items()))
        rows=execute(101,keys,len(case['samples']));count=0;previous={}
        for f,(ti,r) in enumerate(zip(rows,case['samples'])):
            live={p['slot']:p['kind'] for p in r['shots']}
            # Standing pellets never turn: a backward X jump in a live slot
            # would be reuse. That occurs only outside the rapid prefix.
            births=[((slot-0x1228)//64,kind) for slot,kind in live.items() if previous.get(slot)!=kind]
            native=[(i,ti[32+8*i]) for i in range(3) if ti[33+8*i] and not ti[34+8*i]]
            assert native==births,(case['name'],f,'births',native,births)
            previous=live
            tier=0 if ti[19]<31 else 3 if ti[19]<101 else 2
            assert (ti[19] and tier==r['charge_tier'] if r['charge_state'] else not ti[19]),(case['name'],f,'charge')
        for f,x,y,vx,xs,kind,slot,ytol,checkxs in observations(case):
            ti=rows[f];offset=28+8*slot;got=[ti[offset+i] for i in (0,1,2,3,4)]
            assert ti[offset+5] and got==[x,y,vx&65535,xs,kind],(case['name'],f,slot,got,[x,y,vx,xs,kind])
            count+=1
        total+=count;frames+=len(rows);reports.append(dict(case=case['name'],frames=len(rows),samples=count))
        print(case['name'],len(rows),'updates,',count,'projectile samples and births pass on68000',flush=True)
    # The existing dense isolated door has three live shots and a large
    # charge ready. It must consume release without allocating a fourth.
    idle=directory/'capacity-full.txt';idle.write_text('0\n')
    r=execute(108,idle,1)[0]
    assert r[19]==0 and all(r[33+8*i] and r[34+8*i]>0 for i in range(3))
    probes=json.loads((OUT/'capacity/reference.json').read_text())['cases'][3:]
    assert all(c['samples'][c['hold']-1]['charge_state']==0 and len(c['samples'][c['hold']-1]['shots'])==3 and not any(p['kind'] for p in c['samples'][c['hold']-1]['shots']) for c in probes)
    report=dict(frames=frames,projectile_samples=total,cases=reports,
                full_slot_releases=[31,101],rapid_adaptation='Comparison stops before first source removal/reuse; wider target viewport changes later availability.')
    (GAME/'x/capacity-ti.json').write_text(json.dumps(report,indent=2)+'\n')
    print(frames,'source updates,',total,'projectile samples; full-slot releases suppressed')

if __name__=='__main__':main()
