#!/usr/bin/env python3
"""Measured original drop fixtures, generated locally and ignored."""
import json
from pathlib import Path
from reference import OUT

def main():
    cases = json.loads((OUT / 'terrain/drops.json').read_text())['cases']
    interactions = json.loads((OUT / 'terrain/interactions.json').read_text())
    out = ['/* Local ROM measurements, never commit. */',
           'typedef struct { unsigned short y, sub; short vy; unsigned char jump, angle, flutter, timer, cooldown, head; } DropSample;',
           'typedef struct { unsigned short x, y, count; const DropSample *samples; } DropCase;']
    fields = ('y', 'sub', 'vy', 'jump', 'angle', 'flutter', 'phase_timer', 'cooldown', 'head_timer')
    for n, c in enumerate(cases):
        out.append('static const DropSample drop_%d[] = {' % n)
        for s in c['samples']:
            out.append(' {' + ','.join(str(s[k]) for k in fields) + '},')
        out.append('};')
    out.append('static const DropCase drops[] = {')
    for n, c in enumerate(cases):
        out.append(' {%d,%d,%d,drop_%d},' % (c['x'], c['start_y'], len(c['samples']), n))
    out.append('};')
    for name, c in interactions.items():
        out.append('static const DropSample interaction_' + name + '[] = {')
        for s in c['samples']:
            out.append(' {' + ','.join(str(s[k]) for k in fields) + '},')
        out.append('};')
    p = Path(__file__).resolve().parents[1] / 'generated/terrain_ref.h'
    p.parent.mkdir(exist_ok=True)
    p.write_text('\n'.join(out) + '\n')

if __name__ == '__main__':
    main()
