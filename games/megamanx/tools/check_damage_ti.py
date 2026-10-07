#!/usr/bin/env python3
"""Compare natural source contact, recovery and weapons on the TI binary."""
import json
from reference import GAME, OUT
from check_original_ti import execute

def main():
    data = json.loads((OUT/'damage/reference.json').read_text())
    directory = GAME/'x/original'; directory.mkdir(exist_ok=True)
    total=0; shot_total=0; reports=[]
    for case in data['cases']:
        keys=directory/('damage-'+case['name']+'.txt')
        translate={'B':'A','Y':'B'}
        keys.write_text(''.join(str(f)+' '+' '.join(translate.get(k,k) for k in held)+'\n' for f,held in case['keys'].items()))
        rows=execute(7 if case['state']=='hit.state' else 16, keys, len(case['samples']))
        previous_shots={p['slot']:p['kind'] for p in case['initial']['shots']}
        for frame,(ti,r) in enumerate(zip(rows,case['samples'])):
            got=[ti[i] for i in (0,1,5,6,7,8,9,17)]
            expected=[r[k]&65535 for k in ('x','y','vx','vy','xs','ys','state','hp')]
            assert got==expected,(case['name'],frame,got,expected)
            tier=0 if ti[19]<31 else 3 if ti[19]<101 else 2
            assert (ti[19] and tier==r['charge_tier'] if r['charge_state'] else ti[19]==0),(case['name'],frame,'charge tier',ti[19],r['charge_tier'])
            live={p['slot']:p['kind'] for p in r['shots']}
            births=[((slot-0x1228)//64,kind) for slot,kind in live.items() if previous_shots.get(slot)!=kind]
            native_births=[(i,ti[32+8*i]) for i in range(3) if ti[33+8*i] and not ti[34+8*i]]
            assert native_births==births,(case['name'],frame,'shot births',native_births,births)
            previous_shots=live
            for p in r['shots']:
                slot=(p['slot']-0x1228)//64; offset=28+8*slot
                got=[ti[offset+i] for i in (0,1,2,4)]
                expected=[p[k]&65535 for k in ('x','y','vx','kind')]
                assert ti[offset+5] and got==expected,(case['name'],frame,'shot',slot,got,expected)
                if not p['kind']: assert ti[offset+3]==p['xs'],(case['name'],frame,'normal fraction')
                shot_total+=1
            # Birth/no-birth checks also cover suppressed presses and releases.
            # Wider native view permits later culling than the original.
            if frame and not r['shots'] and case['samples'][frame-1]['state']==14 and r['state'] in (0,14):
                assert not any(ti[33+8*i] for i in range(3)),(case['name'],frame,'shot during recoil')
        total+=len(rows); reports.append(dict(case=case['name'],frames=len(rows)))
        print(case['name'],len(rows),'contact/recovery states pass on68000',flush=True)
    report=dict(states=total,projectile_samples=shot_total,cases=reports,
                fields='Hero position/fractions/VX/VY/state/HP, charge tier, projectile births and source-active X/Y/VX/kind (normal fraction exact; charged fraction unused/canonical).')
    (GAME/'x/damage-ti.json').write_text(json.dumps(report,indent=2)+'\n')
    print(total,'original contact/recovery states and',shot_total,'projectile samples pass')

if __name__=='__main__': main()
