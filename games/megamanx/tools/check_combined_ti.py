#!/usr/bin/env python3
"""All source-active coupled-shot samples on the actual compiled 68000."""
import json
from reference import GAME,OUT
from check_original_ti import execute
from combined_header import observations,SCENARIOS
def main():
    cases=json.loads((OUT/'combined/reference.json').read_text())['cases'];total=0;results=[];boundary_states=0
    directory=GAME/'x/original';directory.mkdir(exist_ok=True)
    translate={'B':'A','Y':'B'}
    for case in cases:
        hold=case['precharge'];lines=['0 B'] if hold else []
        lines+= [str(int(f)+hold)+' '+' '.join(translate.get(k,k) for k in keys) for f,keys in case['keys'].items()]
        keys=directory/(case['name']+'.txt');keys.write_text('\n'.join(lines)+'\n')
        rows=execute(SCENARIOS[case['initial_state']],keys,len(case['samples'])+hold)[hold:]
        if case.get('boundary_bob'):
            previous={p['slot']:p['kind'] for p in case['initial_shots']}
            for f,(ti,r) in enumerate(zip(rows,case['samples'])):
                actual=[ti[i] for i in (0,1,5,6,7,8,9,17)]
                expected=[r[k]&65535 for k in ('x','y','vx','vy','xs','ys','state','hp')]
                assert actual==expected,(case['name'],f,'boundary hero',actual,expected)
                tier=0 if ti[19]<31 else 3 if ti[19]<101 else 2
                assert bool(ti[19])==bool(r['charge_state']) and (not ti[19] or tier==r['charge_tier']),(case['name'],f,'boundary charge')
                births=sum(1<<((p['slot']-0x1228)//64) for p in r['shots'] if previous.get(p['slot'])!=p['kind'])
                actual_births=sum(1<<slot for slot in range(3) if ti[33+slot*8] and not ti[34+slot*8])
                assert births==actual_births,(case['name'],f,'boundary births',births,actual_births)
                previous={p['slot']:p['kind'] for p in r['shots']}
            boundary_states+=len(rows)
        errors=[];count=0
        for f,x,y,vx,xs,kind,slot,ytol,checkxs in observations(case):
            ti=rows[f];p=ti[28+slot*8:36+slot*8];dx=p[0]-x;dy=p[1]-y
            assert p[5] and dx==0 and abs(dy)<=ytol and p[2]==vx&65535 and p[4]==kind and (not checkxs or p[3]==xs), (case['name'],f,p,(x,y,vx,xs,kind,ytol,checkxs))
            errors.append(abs(dy));count+=1
        total+=count;results.append(dict(case=case['name'],samples=count,max_x_error=0,max_y_error=max(errors),
            running_pellet_y_tolerance=1,charged_running_fraction='unused; native viewport culling adaptation'))
        results[-1]['charged_boundary_y_tolerance']=int(case.get('boundary_bob',False))
        print(case['name'],count,'source-active shot samples pass on68000',flush=True)
    report=dict(samples=total,boundary_hero_states=boundary_states,cases=results,comparison='All source-active trajectories; running pellet and new run-boundary muzzle bob flattened within one original pixel, legacy charged tolerances retained; unused running charged fractions excluded.')
    (GAME/'x/combined-ti.json').write_text(json.dumps(report,indent=2)+'\n');print(total,'coupled projectile samples passed')
if __name__=='__main__':main()
