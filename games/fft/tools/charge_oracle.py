#!/usr/bin/env python3
"""Observe all eight Archer Charges through FFT's real menu and preview.

usage: charge_oracle.py DISC.cue GARILAND.state OUT.json
The state is produced by oracle.py. Only actor statistics/learned abilities and
target statistics/status are injected; targeting and charge commitment use pad
input. Generated JSON/header/screens stay local, beside OUT.json.
"""
import hashlib
import json
import sys
from pathlib import Path

import battle
import oracle


def trial(p, state, index, config):
    p.load(state)
    case, charging = config
    oracle.attack_case(p, case)
    a = oracle.UNITS + oracle.UNIT_SIZE * oracle.ATTACKER
    t = oracle.UNITS + oracle.UNIT_SIZE * oracle.TARGET
    p.write(a + 0x12, 8)             # primary Charge, no secondary list
    p.write(a + 0x13, 0)
    p.write(a + 0x99 + 3 * 3, 255)  # Archer's eight learned actions
    if charging:
        p.write(t + 0x58, p.read(t + 0x58) | 8)
    keys = oracle.taps(oracle.GUTS + 'DOWN:20 ' * index + 'CIRCLE:60 CIRCLE:60 LEFT:40 CIRCLE:60')
    oracle.run(p, len(keys) + 1, keys)
    hit, damage = p.read(oracle.PREVIEW + 2, 2), p.read(oracle.PREVIEW, 2)
    # Commit the selected ability. Read the first charged CT, before another
    # action/turn can advance it. This also verifies the actual selected ID.
    keys = oracle.taps('CIRCLE:60 CIRCLE:60')
    observed = None
    for f in range(len(keys) + 1):
        p.pressed = keys[f] if f < len(keys) else 0
        p.run()
        ct = p.read(a + 0x15D)
        if 0 < ct < 255:
            observed = ct, p.read(a + 0x170, 2)
            break
    assert observed and observed[1] == 0x196 + index, (index, config, observed, hit, damage)
    return dict(ability=0x196 + index, case=case, charging=charging,
                hit=hit, amount=damage, ct=observed[0])


def main():
    disc, state, destination = sys.argv[1:]
    out = Path(destination); out.parent.mkdir(parents=True, exist_ok=True)
    d = battle.Disc(disc)
    # Sword, dagger, bare hands, good/bad compatibility and vulnerable target.
    configs = [((5, 6, 70, 0x13, 0, 0, 0, 0, 2), 0),
               ((5, 8, 70, 1, 0, 0, 0, 0, 2), 0),
               ((5, 6, 70, 255, 0, 0, 0, 0, 2), 0),
               ((7, 6, 70, 0x13, 0, 0, 4, 0, 2), 0),
               ((7, 6, 70, 0x13, 9, 0, 3, 0, 2), 0),
               ((7, 6, 70, 0x13, 0, 0, 4, 0, 2), 1)]
    p = oracle.PSX(disc)
    rows = []
    for index in range(8):
        for config in configs:
            first = trial(p, state, index, config)
            assert first == trial(p, state, index, config), ('replay', first)
            assert first['ct'] == d.at(0x80061044 + index * 2, 1)[0]
            rows.append(first)
        print('Charge', index, 'six twice-replayed source previews/commits', flush=True)
    report = dict(state_sha256=hashlib.sha256(Path(state).read_bytes()).hexdigest(),
                  scope='Original menus/previews/initial committed CT; injected actor stats, learned actions and target Charging. Not a full delayed-hit replay.',
                  rows=rows)
    out.write_text(json.dumps(report, indent=2) + '\n')
    lines = ['/* Generated from the original run; never committed. */',
             'static const struct { u16 ability; u8 pa,sp,br,weapon,az,af,tz,tf,face,charging,hit; u16 amount; u8 ct; } charge_oracle[] = {']
    for row in rows:
        values = [row['ability'], *row['case'], row['charging'], row['hit'], row['amount'], row['ct']]
        lines.append('    {' + ','.join(map(str, values)) + '},')
    lines.append('};\n')
    out.with_suffix('.h').write_text('\n'.join(lines))


if __name__ == '__main__':
    main()
