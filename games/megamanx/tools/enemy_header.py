#!/usr/bin/env python3
"""Original roller timelines: real movement, damage states and removal times."""
import json
from reference import GAME,OUT
def observations(case):
    for frame,row in enumerate(case['samples']):
        enemy=next((e for e in row['enemies'] if e['kind']==21),None)
        raw=bytes.fromhex(row['roller_raw'])
        # Culling uses the chosen native viewport. Explicit deaths are checked,
        # but extra native actors after source viewport removal are permitted.
        if enemy or case['name'] not in ('no_fire','single_tap'):
            yield [frame,bool(enemy),enemy['x'] if enemy else 0,enemy['y'] if enemy else 0,
                   enemy['xs'] if enemy else 0,enemy['vx'] if enemy else 0,
                   raw[0x27]&127,raw[1],raw[0x34],row['hp']]
def main():
    data=json.loads((OUT/'enemy/reference.json').read_text());lines=[
        '/* Generated natural source enemy oracles; ignored. */',
        'typedef struct { u16 frame,active,x,y,xs; s16 vx; u16 hp,phase,fuse,player_hp; } EnemySample;']
    for case in data['cases']:
        name=case['name'];samples=list(observations(case));keys=case['keys']
        lines.append(f'static const EnemySample enemy_{name}[]={{')
        lines+= ['{'+','.join(str(int(v)) for v in values)+'},' for values in samples];lines.append('};')
        keymap={'RIGHT':8,'LEFT':2,'B':16,'Y':32}
        lines.append(f'static const ComboKeys enemy_keys_{name}[]={{')
        lines+= ['{'+str(f)+','+str(sum(keymap[k] for k in values))+'},' for f,values in keys.items()];lines.append('};')
        lines.extend([f'#define ENEMY_{name.upper()}_FRAMES {len(case["samples"])}',
            f'#define ENEMY_{name.upper()}_SCENARIO {17 if name=="charged_release" else 18 if name=="medium_release" else 16}'])
        print(name,len(samples),'enemy observations')
    (GAME/'generated/enemy_ref.h').write_text('\n'.join(lines)+'\n')
if __name__=='__main__':main()
