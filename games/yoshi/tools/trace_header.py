#!/usr/bin/env python3
"""Generate ignored PC-test fixtures from raw original traces, never formulas."""
import json
from pathlib import Path
from reference import OUT


def main():
    data = json.loads((OUT / 'physics.json').read_text())
    output = ['/* Generated locally from measured PAL ROM traces; do not commit. */',
              'typedef struct { unsigned short y, sub; short vy; } JumpSample;']
    for name in ('jump_tap', 'jump_hold'):
        samples = data['cases'][name]['samples'][1:]
        assert len(samples) == 90
        output.append('static const JumpSample ' + name + '[90] = {')
        output.extend('    {%d, %d, %d},' % (s['y'], s['y_sub'], s['vy']) for s in samples)
        output.append('};')
    directory = Path(__file__).resolve().parents[1] / 'generated'
    directory.mkdir(exist_ok=True)
    (directory / 'jump_ref.h').write_text('\n'.join(output) + '\n')


if __name__ == '__main__':
    main()
