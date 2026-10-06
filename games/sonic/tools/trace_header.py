#!/usr/bin/env python3
"""Convert local measured ROM samples into a PC-test-only reference header."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
GAME = Path(__file__).resolve().parents[1]
data = json.loads((ROOT / 'sources/sonic1_md/physics.json').read_text())
out = ['/* Generated from local ROM measurements; never committed or linked into TI code. */']
cases = []
pad = {'LEFT': 2, 'RIGHT': 8, 'DOWN': 4, 'C': 16}
for name, action in data['actions'].items():
    out.append(f'static const RefFrame ref_{name}[] = {{')
    keys = 0
    for row in action['trace']:
        frame = row['frame']
        if str(frame) in action['keys']:
            keys = sum(pad[k] for k in action['keys'][str(frame)])
        fields = [keys, row['x'] >> 16, row['y'] >> 16, row['x'] >> 8 & 255,
                  row['y'] >> 8 & 255, row['vx'], row['vy'], row['ground_speed'],
                  row['status'], row['radius_y']]
        out.append('    {' + ','.join(map(str, fields)) + '},')
    out.append('};')
    cases.append(f'    {{"{name}", ref_{name}, {len(action["trace"])} }},')
out += ['static const RefCase ref_cases[] = {', *cases, '};']
(GAME / 'generated').mkdir(exist_ok=True)
(GAME / 'generated/physics_ref.h').write_text('\n'.join(out) + '\n')
