#!/usr/bin/env python3
"""Capacity/charge projectile fixtures; rapid prefix ends before source cull."""
import json
from reference import GAME, OUT
from combined_header import observations

def cases():
    data=json.loads((OUT/'capacity/reference.json').read_text()); result=[]
    for case in data['cases'][:3]:
        c=dict(case,initial_player=case['initial'])
        if c['name']=='rapid':
            # The wider target camera changes later availability. Check the
            # complete allocation prefix before the first source removal or
            # reuse rather than pretending the two viewport limits are equal.
            previous={};count=len(c['samples'])
            for frame,r in enumerate(c['samples']):
                live={p['slot']:p['x'] for p in r['shots']}
                if any(slot not in live or live[slot]<x for slot,x in previous.items()):count=frame;break
                previous=live
            c['samples']=c['samples'][:count]
        result.append(c)
    return result

def main():
    lines=['/* Generated source allocation/charge fixtures; ignored. */',
           'typedef struct { u8 charging,tier,births; } CapacitySample;']
    for case in cases():
        name=case['name'];keymap={'Y':32}
        lines.append(f'static const ComboKeys capacity_keys_{name}[]={{')
        lines+=['{'+str(f)+','+str(sum(keymap[k] for k in keys))+'},' for f,keys in case['keys'].items()]
        lines.append('};');lines.append(f'static const ComboSample capacity_{name}[]={{')
        samples=list(observations(case))
        lines+=['{'+','.join(map(str,values))+'},' for values in samples];lines.append('};')
        lines.append(f'#define CAPACITY_{name.upper()}_FRAMES {len(case["samples"])}')
        lines.append(f'static const CapacitySample capacity_state_{name}[]={{')
        previous={p['slot']:p['kind'] for p in case['initial']['shots']}
        for r in case['samples']:
            live={p['slot']:p['kind'] for p in r['shots']}
            mask=sum(1<<((slot-0x1228)//64) for slot,kind in live.items() if previous.get(slot)!=kind)
            lines.append('{'+','.join(map(str,[int(bool(r['charge_state'])),r['charge_tier'],mask]))+'},')
            previous=live
        lines.append('};')
        print(name,len(case['samples']),'updates,',len(samples),'projectile samples')
    (GAME/'generated/capacity_ref.h').write_text('\n'.join(lines)+'\n')

if __name__=='__main__':main()
