#!/usr/bin/env python3
"""Independent long-route motion and roller oracles for portable C tests."""
import json
from reference import GAME, OUT

def main():
    source=json.loads((OUT/'traverse/reference.json').read_text());cases=source['cases']
    lines=['/* Generated original long-route observations; ignored. */',
           'typedef struct { MotionSample motion; u8 hp,charging,tier; '
           'u8 active,ehp,phase,fuse; u16 ex,ey; s16 evx; u8 exs,births; } TraverseSample;']
    keymap={'RIGHT':8,'LEFT':2,'B':16,'Y':32}
    for case in cases:
        name=case['name']
        lines.append(f'static const ComboKeys traverse_keys_{name}[]={{')
        lines += ['{'+str(f)+','+str(sum(keymap[k] for k in keys))+'},'
                  for f,keys in case['effective_keys'].items()]
        lines.append('};')
        lines.append(f'static const TraverseSample traverse_{name}[]={{')
        previous={};shot_rows=[];running={}
        for f,r in enumerate(case['samples']):
            motion=','.join(str(r[k]) for k in ('x','y','vx','vy','xs','ys','state'))
            e=r['roller']
            live={p['slot']:p['kind'] for p in r['shots']};births=0
            for p in r['shots']:
                slot=(p['slot']-0x1228)//64
                if previous.get(p['slot'])!=p['kind']:
                    births|=1<<slot;running[p['slot']]=r['state']==4
            values=[r['hp'],int(bool(r['charge_state'])),r['charge_tier'],
                    int(bool(e)),(e['hp']&127) if e else 0,r['roller_phase'],
                    r['roller_fuse'],e['x'] if e else 0,e['y'] if e else 0,
                    e['vx'] if e else 0,e['xs'] if e else 0,births]
            lines.append('{{'+motion+'},'+','.join(map(str,values))+'},')
            for p in r['shots']:
                shot_rows.append([f,
                                  p['x'],p['y'],p['vx'],p['xs'],p['kind'],
                                  (p['slot']-0x1228)//64,int(running[p['slot']]),int(not p['kind'])])
            previous=live
        lines.append('};')
        lines.append(f'static const ComboSample traverse_shots_{name}[]={{')
        lines += ['{'+','.join(map(str,values))+'},' for values in shot_rows]
        lines.append('};')
    lines.append('typedef struct { MotionSample motion; u8 wall,facing; } EdgeSample;')
    for case in source['ledge']:
        lines.append(f'static const EdgeSample ledge_{case["name"]}[]={{')
        for r in case['samples']:
            motion=','.join(str(r[k]) for k in ('x','y','vx','vy','xs','ys','state'))
            lines.append('{{'+motion+'},'+str(r['wall'])+','+str(int(bool(r['facing']&64)))+'},')
        lines.append('};')
        lines.append(f'static const ComboKeys ledge_keys_{case["name"]}[]={{')
        lines += ['{'+str(f)+','+str(sum(keymap[k] for k in keys))+'},'
                  for f,keys in case['keys'].items()]
        lines.append('};')
    lines.append('static const EdgeSample first_clamp[]={')
    for case in source['clamp']:
        r=case['sample'];motion=','.join(str(r[k]) for k in ('x','y','vx','vy','xs','ys','state'))
        lines.append('{{'+motion+'},'+str(r['wall'])+','+str(int(bool(r['facing']&64)))+'},')
    lines.append('};')
    lines.append('#define LEDGE_CASES(X) '+' '.join('X('+c['name']+','+str(c['scenario'])+')' for c in source['ledge']))
    (GAME/'generated/traversal_ref.h').write_text('\n'.join(lines)+'\n')

if __name__ == '__main__':
    main()
