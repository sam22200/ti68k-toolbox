#!/usr/bin/env python3
"""Generate PC tests of measured source contact/charge/recovery states."""
import json
from reference import GAME, OUT

def main():
    data=json.loads((OUT/'damage/reference.json').read_text())
    lines=['/* Generated from natural source contact/recovery traces; ignored. */',
           'typedef struct { u16 x,y; s16 vx; u8 xs,kind,active,born; } DamageShot;',
           'typedef struct { u16 x,y; s16 vx,vy; u8 xs,ys,state,hp,charging,tier; DamageShot shots[3]; } DamageSample;']
    for case in data['cases']:
        name=case['name']; previous={p['slot']:p['kind'] for p in case['initial']['shots']}
        lines.append(f'static const DamageSample damage_{name}[]={{')
        for r in case['samples']:
            live={p['slot']:p['kind'] for p in r['shots']}; shots=['{0}']*3
            for p in r['shots']:
                slot=(p['slot']-0x1228)//64; assert slot<3
                values=[p[k] for k in ('x','y','vx','xs','kind')]+[1,int(previous.get(p['slot'])!=p['kind'])]
                shots[slot]='{'+','.join(map(str,values))+'}'
            values=[r[k] for k in ('x','y','vx','vy','xs','ys','state','hp')]+[int(bool(r['charge_state'])),r['charge_tier']]
            lines.append('{'+','.join(map(str,values))+',{'+','.join(shots)+'}},')
            previous=live
        lines.append('};'); lines.append(f'static const ComboKeys damage_keys_{name}[]={{')
        keymap={'RIGHT':8,'LEFT':2,'B':16,'Y':32}
        lines+=['{'+str(f)+','+str(sum(keymap[k] for k in keys))+'},' for f,keys in case['keys'].items()]
        lines.append('};'); lines.append(f'#define DAMAGE_{name.upper()}_SCENARIO {7 if case["state"]=="hit.state" else 16}')
    # Original actor descriptors are invariant over these measured semantic
    # poses; retain the independent motion fixtures as well.
    for case in data['poses']:
        assert all(r['box_pointer']==0xa552 and r['box']==[0,255,6,14] for r in case['samples']),case['name']
    (GAME/'generated/damage_ref.h').write_text('\n'.join(lines)+'\n')
    print(sum(len(c['samples']) for c in data['cases']),'natural damage/recovery states;',sum(len(c['samples']) for c in data['poses']),'player-pose box samples')

if __name__=='__main__': main()
