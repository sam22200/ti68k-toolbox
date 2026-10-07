#!/usr/bin/env python3
"""Wall firing trajectories, control outcomes and adjacent/repeated muzzles."""
import json
from reference import GAME,OUT

def scenario(kind):return 102 if not kind else 119 if kind==1 else 120

def main():
    data=json.loads((OUT/'wall_charge/reference.json').read_text());lines=[
        '/* Generated measured source wall firing fixtures; ignored. */']
    macro=[];count=shots=0
    for case in data['cases']:
        name=case['name'];initial=case['initial_player'];assert not initial['shots']
        assert [initial[k] for k in ('x','y','vx','vy','xs','ys','state','substate')]==[824,386,376,-768,64,128,8,2]
        previous={};lines.append(f'static const DamageSample wall_{name}[]={{')
        for r in case['samples']:
            live={p['slot']:p['kind'] for p in r['shots']};projectiles=['{0}']*3
            for p in r['shots']:
                slot=(p['slot']-0x1228)//64;assert slot<3
                values=[p[k] for k in ('x','y','vx','xs','kind')]+[1,int(previous.get(p['slot'])!=p['kind'])]
                projectiles[slot]='{'+','.join(map(str,values))+'}';shots+=1
            values=[r[k] for k in ('x','y','vx','vy','xs','ys','state','hp')]+[int(bool(r['charge_state'])),r['charge_tier']]
            lines.append('{'+','.join(map(str,values))+',{'+','.join(projectiles)+'}},')
            previous=live;count+=1
        lines.append('};');lines.append(f'static const ComboKeys wall_keys_{name}[]={{')
        keymap={'RIGHT':8,'B':16,'Y':32}
        lines+=['{'+str(f)+','+str(sum(keymap[k] for k in held))+'},' for f,held in case['keys'].items()]
        lines.append('};');macro.append(f'X({name},{scenario(case["kind"])})')
    lines.append('#define WALL_CASES(X) '+' '.join(macro))
    lines.extend(['typedef struct { u16 frame,earlier,x,y; s16 vx; u8 kick,active; u16 shotx,shoty; } WallProbe;',
                  'static const WallProbe wall_probes[]={'])
    for probe in data['probes']:
        r=probe['player'];p=probe['shot']
        values=[probe['release'],probe['earlier'] if probe['earlier'] is not None else 65535,
                r['x'],r['y'],r['vx'],int(probe['kick']),int(p is not None),p['x'] if p else 0,p['y'] if p else 0]
        lines.append('{'+','.join(map(str,values))+'},')
    lines.append('};')
    for case in data['fractions']:
        lines.append(f'static const ShotSample wall_fraction_{case["initial_xs"]}[]={{')
        lines+=['{'+','.join(str(p[k]) for k in ('x','y','vx','xs','kind'))+'},' for p in case['samples']]
        lines.append('};')
    (GAME/'generated/wall_ref.h').write_text('\n'.join(lines)+'\n')
    print(count,'wall hero states,',shots,'projectile samples,',len(data['probes']),'adjacent/repeated birth probes')

if __name__=='__main__':main()
