#!/usr/bin/env python3
"""Source-aligned complete opening routes on the actual compiled 68000."""
import json
from reference import GAME, OUT
from check_original_ti import execute

def main():
    source=json.loads((OUT/'traverse/reference.json').read_text())
    directory=GAME/'x/original';directory.mkdir(exist_ok=True)
    cases=[];total=shots=0
    for case in source['cases']:
        name=case['name'];keys=directory/f'traverse-{name}.txt'
        translate={'B':'A','Y':'B'}
        keys.write_text('\n'.join(str(f)+' '+' '.join(translate.get(k,k) for k in held)
                        for f,held in case['effective_keys'].items())+'\n')
        rows=execute(0,keys,len(case['samples']));count=0;run_birth={};previous={};max_y_error=0
        for f,(ti,r) in enumerate(zip(rows,case['samples'])):
            actual=[ti[i] for i in (0,1,5,6,7,8,9,17)]
            expected=[r[k]&65535 for k in ('x','y','vx','vy','xs','ys','state','hp')]
            assert actual==expected,(name,f,r['source_frame'],actual,expected)
            tier=0 if ti[19]<31 else 3 if ti[19]<101 else 2
            assert bool(ti[19])==bool(r['charge_state']) and (not ti[19] or tier==r['charge_tier']),(name,f,'charge',ti[19],r['charge_tier'])
            e=r['roller'];assert bool(ti[56])==bool(e),(name,f,'roller active')
            births=sum(1<<((p['slot']-0x1228)//64) for p in r['shots'] if previous.get(p['slot'])!=p['kind'])
            actual_births=sum(1<<slot for slot in range(3) if ti[33+slot*8] and not ti[34+slot*8])
            assert births==actual_births,(name,f,'births',actual_births,births)
            if e:
                actual=[ti[i] for i in (52,53,54,62,55,57,64)]
                expected=[e['x'],e['y'],e['xs'],e['vx']&65535,e['hp']&127,r['roller_phase'],r['roller_fuse']]
                assert actual==expected,(name,f,'roller',actual,expected)
            for p in r['shots']:
                slot=(p['slot']-0x1228)//64;assert slot<3
                if previous.get(p['slot'])!=p['kind']:
                    run_birth[p['slot']]=r['state']==4
                actual=ti[28+slot*8:36+slot*8]
                # Native running bob is flattened within one source pixel.
                # Charged XS is unused and differs with wider-view slot reuse.
                error=abs(actual[1]-p['y']);max_y_error=max(max_y_error,error)
                assert actual[5] and actual[0]==p['x'] and error<=int(run_birth[p['slot']]) and actual[2]==p['vx']&65535 and actual[4]==p['kind'] and (p['kind'] or actual[3]==p['xs']), (name,f,'shot',actual,p)
                count+=1
            previous={p['slot']:p['kind'] for p in r['shots']}
        assert rows[-1][21] and rows[-1][24]==1 and rows[-1][17]==16
        total+=len(rows);shots+=count
        cases.append(dict(case=name,source_frames=case['source_frames'],logic_states=len(rows),
                          skipped_source_frames=case['skipped'],projectile_samples=count,
                          max_running_muzzle_y_error=max_y_error,
                          finish=dict(x=rows[-1][0],y=rows[-1][1],hp=rows[-1][17],kills=rows[-1][24])))
        print(name,len(rows),'complete-route hero/roller states and',count,'shot samples pass on68000',flush=True)
    edges=0
    for case in source['ledge']:
        keys=directory/f'ledge-{case["name"]}.txt'
        keys.write_text('\n'.join(str(f)+' '+' '.join(translate.get(k,k) for k in held)
                       for f,held in case['keys'].items())+'\n')
        rows=execute(case['scenario'],keys,len(case['samples']))
        for f,(ti,r) in enumerate(zip(rows,case['samples'])):
            actual=[ti[i] for i in (0,1,5,6,7,8,9,14,13)]
            expected=[r[k]&65535 for k in ('x','y','vx','vy','xs','ys','state','wall')]+[int(bool(r['facing']&64))]
            if r['state'] in (16,18):
                # Native wall banks keep wall-side orientation; wallage selects
                # the source outward gun pose (checked separately in M14).
                actual=actual[:-1];expected=expected[:-1]
            assert actual==expected,(case['name'],f,actual,expected)
        edges+=len(rows)
    keys=directory/'clamp-right.txt';keys.write_text('0 RIGHT\n')
    for n,case in enumerate(source['clamp']):
        ti=execute(126+n,keys,1)[0];r=case['sample']
        actual=[ti[i] for i in (0,1,5,6,7,8,9,14,13)]
        expected=[r[k]&65535 for k in ('x','y','vx','vy','xs','ys','state','wall')]+[int(bool(r['facing']&64))]
        assert actual==expected,('first clamp',n,actual,expected)
        edges+=1
    report=dict(hero_roller_states=total,projectile_samples=shots,cases=cases,
                edge_samples=edges,controlled=source['controlled'],
                clock=source['clock'],isolation=source['isolation'],
                natural_control=source['natural_control'],
                adaptations='Source-active shots checked; one-pixel running muzzle bob for all tiers and unused charged XS disclosed. Wider native camera can keep extra shots; no source lag is inserted into target gameplay. Native facing during wall states denotes art-bank wall side; effective gun/pose direction follows measured wallage, checked in wall oracles.')
    (GAME/'x/traversal-ti.json').write_text(json.dumps(report,indent=2)+'\n')
    print(total,'long-route hero/roller states;',shots,'source-active shots;',edges,'ledge/clamp states match')

if __name__=='__main__':main()
