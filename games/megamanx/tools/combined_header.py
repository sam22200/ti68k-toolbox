#!/usr/bin/env python3
"""Generate coupled-shot oracles from original post-frame samples."""
import json
from reference import GAME,OUT
SCENARIOS={'start':101,'wall':102,'ledge':125,'medium_ledge':143,'large_ledge':144}
def observations(case):
    seen={}
    for f,r in enumerate(case['samples']):
        live={p['slot'] for p in r['shots']}
        seen={slot:v for slot,v in seen.items() if slot in live}
        for p in r['shots']:
            slot=(p['slot']-0x1228)//64
            if p['slot'] not in seen or p['kind']!=seen[p['slot']][0]:
                previous=case['initial_player'] if not f else case['samples'][f-1]
                running=r['state']==4 or (r['state'] in (0,6,8) and previous['state']==4)
                seen[p['slot']]=(p['kind'],running)
            birth=seen[p['slot']][1]
            if slot>=3:raise ValueError('oracle needs more than target shot capacity')
            # All source-active shots are compared. Additional native shots
            # can persist after source removal with the wider target camera.
            ytol=int(birth and (p['kind']==0 or case.get('boundary_bob',False)))
            checkxs=int(not (p['kind'] and birth))
            yield [f,p['x'],p['y'],p['vx'],p['xs'],p['kind'],slot,ytol,checkxs]
def main():
    cases=json.loads((OUT/'combined/reference.json').read_text())['cases']
    lines=['/* Generated original combined-action observations; ignored. */',
        'typedef struct { unsigned short frame,x,y; short vx; unsigned char xs,kind,slot,ytol,checkxs; } ComboSample;',
        'typedef struct { unsigned short frame,keys; } ComboKeys;']
    for case in cases:
        name=case['name'];keymap={'RIGHT':8,'LEFT':2,'B':16,'Y':32}
        lines.append(f'static const ComboKeys keys_{name}[]={{')
        lines+=['{'+str(f)+','+str(sum(keymap[k] for k in keys))+'},' for f,keys in case['keys'].items()]
        lines.append('};');lines.append(f'static const ComboSample ref_combo_{name}[]={{')
        count=0
        for values in observations(case):
            lines.append('{'+','.join(map(str,values))+'},');count+=1
        lines.append('};');lines.extend([f'#define COMBO_{name.upper()}_FRAMES {len(case["samples"])}',
            f'#define COMBO_{name.upper()}_PRECHARGE {case["precharge"]}',
            f'#define COMBO_{name.upper()}_SCENARIO {SCENARIOS[case["initial_state"]]}'])
        print(name,count,'coupled samples')
    (GAME/'generated/combined_ref.h').write_text('\n'.join(lines)+'\n')
if __name__=='__main__':main()
