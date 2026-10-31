#!/usr/bin/env python3
"""Read Mandalia's scenario, ENTD and original deployment limit before porting it.

Usage: mandalia_preflight.py DISC.cue [--state GARILAND_DEPLOYMENT.state]
Without --state, replay oracle.boot's existing Academy/recruitment recipe.
Reports and original state remain local under campaign/x/mandalia.
This reads a deployment table, not a played Mandalia encounter.
"""
import argparse
import hashlib
import json
from pathlib import Path

import battle
import extract
import oracle

HERE = Path(__file__).resolve().parents[1]
OUT = HERE / 'campaign/x/mandalia'


def inspect(disc, state=None):
    d = battle.Disc(disc)
    disk, files = extract.disc_files(disc)
    attack = disk.read(*files['EVENT/ATTACK.OUT'])
    scenarios = []
    for at in range(0x801CF938 - 0x801BF000, len(attack) - 23, 24):
        b = attack[at:at + 24]
        event = int.from_bytes(b[:2], 'little')
        if not event:
            break
        if 15 <= event <= 23:
            scenarios.append(dict(event=event, map=b[2],
                                  entd=int.from_bytes(b[7:9], 'little'),
                                  squad=int.from_bytes(b[9:11], 'little')))
    assert len(scenarios) == 9, scenarios
    assert all((r['map'], r['entd'], r['squad']) == (85, 0x185, 257)
               for r in scenarios), scenarios
    units = []
    for slot in range(16):
        b = d.entd(0x185, slot)
        # Empty ENTD slots keep default Brave/equipment bytes; they are not zero-filled.
        if b[3] == 0 and b[10] == 0:
            assert b[32] == 255, b.hex()
            continue
        units.append(dict(slot=slot, job=b[10], level=b[3]))
    assert [r['job'] for r in units] == [4, 7, 0x53, 0x4A, 0x4A, 0x4A, 0x4A, 0x67], units

    p = oracle.PSX(disc)
    if state:
        p.load(str(state))
    else:
        oracle.boot(p)
    # ATTACK's globals overlap later BATTLE data. Reject a battle-turn state;
    # only a live deployment overlay can supply this pointer and record.
    assert oracle.var(p, oracle.EVENT) == 9
    pointer = p.read(0x801CD270, 4)
    assert 0x80000000 <= pointer <= 0x80200000 - 258 * 12, hex(pointer)
    record = bytes(p.read(pointer + 257 * 12 + i) for i in range(12))
    assert record[9] == 85 and record[8] == 4, record.hex()
    report = dict(
        scope='Original Gariland deployment table plus disc Mandalia scenario/ENTD; '
              'not observed Mandalia active units or a full playthrough.',
        recipe='oracle.boot: new game, Academy event skip, recruits join, cancel save prompt',
        state_sha256=hashlib.sha256(Path(state).read_bytes()).hexdigest() if state else None,
        scenarios=scenarios, entd_units=units,
        squad=dict(id=257, map=record[9], limit=record[8],
                   bitmap=int.from_bytes(record[:4], 'little'),
                   center=list(record[4:6]), flags=record[7], record_hex=record.hex()),
        declared_maximum=len(units) + record[8],
        remaining='Observe conditional active units and four-player deployment in Mandalia '
                  'before changing native capacity; never discard a source unit to fit eleven.')
    OUT.mkdir(parents=True, exist_ok=True)
    if not state:
        p.save(str(OUT / 'gariland-deployment.state'))
    (OUT / 'preflight.json').write_text(json.dumps(report, indent=2) + '\n')
    print('Mandalia: MAP085, ENTD185, squad257; 8 ENTD records + 4 player slots = '
          '12 declared maximum. Conditional active-unit observation remains required.')
    return report


def main():
    ap = argparse.ArgumentParser(__doc__)
    ap.add_argument('disc')
    ap.add_argument('--state', type=Path)
    args = ap.parse_args()
    inspect(args.disc, args.state)


if __name__ == '__main__':
    main()
