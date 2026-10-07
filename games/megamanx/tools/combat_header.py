#!/usr/bin/env python3
"""Small independent projectile oracles taken directly from Snes9x traces."""
import json
from reference import GAME,OUT
def main():
    original=json.loads((OUT/'measure/reference.json').read_text())
    lines=['/* Original post-frame projectile samples; local generated data. */',
           'typedef struct { unsigned short x,y; short vx; unsigned char xs,kind; } ShotSample;']
    lines.append('static const MotionSample ref_hurt[]={')
    for r in original['damage'][287:377]:
        lines.append('{'+','.join(str(r[k]) for k in ('x','y','vx','vy','xs','ys','state'))+'},')
    lines.append('};')
    for hold in (1,45,110):
        case=next(c for c in original['shots'] if c['hold']==hold)
        start=0 if hold==1 else hold
        lines.append(f'static const ShotSample ref_shot_{hold}[]={{')
        for row in case['samples'][start:start+24]:
            p=row['shots'][0];lines.append('{'+','.join(str(p[k]) for k in ('x','y','vx','xs','kind'))+'},')
        lines.append('};')
    (GAME/'generated/combat_ref.h').write_text('\n'.join(lines)+'\n')
if __name__=='__main__':main()
