#!/usr/bin/env python3
"""PC fixtures taken directly from controlled original PAL measurements."""
import json
from pathlib import Path
from reference import OUT

FIELDS = ('x', 'x_sub', 'y', 'y_sub', 'vx', 'vy', 'flutter', 'phase_timer',
          'cooldown', 'jump', 'skid')


def main():
    data = json.loads((OUT / 'movement.json').read_text())
    out = ['/* Local ROM measurements; generated, never commit. */',
           'typedef struct { unsigned short x, x_sub, y, sub; short vx, vy;',
           ' unsigned char flutter, timer, cooldown, jump, skid; } MovementSample;',
           'typedef struct { const char *name; const MovementSample *samples;',
           ' const unsigned short *keys; unsigned frames; } MovementCase;']
    for name, case in data['cases'].items():
        samples = case['samples'][1:]
        out.append('static const MovementSample ref_' + name + '[] = {')
        out.extend(' {' + ','.join(str(s[k]) for k in FIELDS) + '},' for s in samples)
        out.append('};')
        events = dict(case['input'])
        keys = ''
        out.append('static const unsigned short keys_' + name + '[] = {')
        for frame in range(len(samples)):
            keys = events.get(frame, keys)
            out.append(' ' + (' | '.join('K_A' if key == 'B' else 'K_' + key
                                       for key in keys.split()) or '0') + ',')
        out.append('};')
    out.append('static const MovementCase movement_cases[] = {')
    out.extend(' {"%s", ref_%s, keys_%s, %d},' %
               (name, name, name, len(case['samples']) - 1)
               for name, case in data['cases'].items())
    out.append('};')
    path = Path(__file__).resolve().parents[1] / 'generated/movement_ref.h'
    path.parent.mkdir(exist_ok=True)
    path.write_text('\n'.join(out) + '\n')


if __name__ == '__main__':
    main()
