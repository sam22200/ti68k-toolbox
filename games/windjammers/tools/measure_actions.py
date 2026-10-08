#!/usr/bin/env python3
"""Replay ordinary action timelines from the verified two-human Beach door.

All trials use normal pads, with no RAM writes. Every sampled frame includes
the full reference fingerprint; reports and states remain local/ignored.
"""
import json
from pathlib import Path
import sys

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
sys.path.insert(0, str(ROOT / 'tools/neogeo'))
from check_gameplay import mask, values
from check_reference import frame_fingerprint
from check_rules import EXTRA_FIELDS, PROGRAM_SHA
from neogeorun import NeoGeo, sha

FIELDS = dict(EXTRA_FIELDS)
for port, base in enumerate((0x100800, 0x100880), 1):
    for name, offset, size in (('palette', 3, 1), ('character', 0x22, 2), ('flags20', 0x20, 1),
                              ('flags21', 0x21, 1), ('direction', 0x32, 1),
                              ('animation', 0x34, 2), ('hold_age', 0x3a, 2),
                              ('power', 0x52, 2), ('bonus', 0x4d, 1), ('recoil_speed', 0x3c, 2), ('boundary', 0x43, 1)):
        FIELDS[f'p{port}_{name}'] = (base + offset, size, False)
FIELDS.update(disc_angle=(0x100a30, 2, False), disc_speed=(0x100a34, 2, False))


def main():
    out = GAME / 'generated'
    out.mkdir(exist_ok=True)
    source = ROOT / 'sources/windjammers_neogeo'
    assert sha((source / 'program.be.bin').read_bytes()) == PROGRAM_SHA
    provenance = json.loads((source / 'gameplay/report.json').read_text())['metadata']
    neo = NeoGeo()
    try:
        metadata = neo.metadata()
        for field in ('core_sha256', 'rom_sha256', 'bios_archive_sha256', 'fps', 'options', 'rtc_initial'):
            assert metadata[field] == provenance[field], field

        def sample():
            return {**values(neo), **{k: neo.read(*v) for k, v in FIELDS.items()}}

        # Obtain genuine surfaces for replay, including the duplicate callback
        # immediately after a core-only state load.
        neo.load(source / 'gameplay/serve.state')
        neo.pressed[:] = [0, 0]
        neo.step(); neo.step()
        neo.save(out / 'actions_serve.state')
        serve_surface = neo.frame
        neo.load(out / 'mita_hold.state')
        neo.step(); neo.step()
        neo.save(out / 'actions_mita.state')
        mita_surface = neo.frame
        trials = {}
        for owner, state, surface in ((1, 'actions_serve.state', serve_surface),
                                      (0, 'actions_mita.state', mita_surface)):
            for direction in ('A', 'UP A', 'DOWN A', ''):
                def run():
                    neo.load(out / state)
                    neo.frame = surface
                    initial = sample()
                    rows, hashes = [], []
                    for frame in range(180):
                        pads = [0, 0]
                        if 4 <= frame < 6:
                            pads[owner] = mask(direction)
                        neo.pressed[:] = pads
                        neo.step()
                        rows.append(sample())
                        hashes.append(frame_fingerprint(neo))
                    return dict(owner=owner, direction=direction, initial=initial,
                                rows=rows, hashes=hashes)
                result = run()
                assert run() == result, (owner, direction)
                trials[f'p{owner + 1}_{direction.replace(" ", "_") or "auto"}'] = result
        report = dict(metadata=metadata, fields=FIELDS, program_sha256=PROGRAM_SHA,
                      script_sha256=sha(Path(__file__).read_bytes()), trials=trials)
        (out / 'actions.json').write_text(json.dumps(report, indent=2) + '\n')
        starts, samples = [], []
        for trial in trials.values():
            if not trial['direction']:
                continue
            ident = len(starts)
            origin = trial['initial']
            starts.append((trial['owner'], mask(trial['direction']),
                           *[origin[f'p{port}_{field}'] for port in (1, 2) for field in ('x', 'y')],
                           *[origin[f'p{port}_{field}'] for port in (1, 2)
                             for field in ('hold_age', 'power', 'bonus')]))
            # Ordinary trials remain in their first catch/hold within 110
            # frames; auto-serves have a different measured speed profile.
            for frame, row in enumerate(trial['rows'][:110]):
                samples.append((ident, frame,
                    *[row[f'p{port}_{field}'] for port in (1, 2) for field in ('x', 'y', 'vx', 'vy', 'action')],
                    row['disc_x'], row['disc_y'], row['disc_state']))
        header = ['/* Normal-input original action fixtures; PC tests only. */']
        for title, rows in (('ActionStart action_start', starts), ('ActionRef action_ref', samples)):
            header.append('static const ' + title + '[] = {')
            header += ['  {' + ','.join(map(str, row)) + '},' for row in rows]
            header.append('};')
        (out / 'actions.h').write_text('\n'.join(header) + '\n')
        print(f'Ordinary actions: {len(trials)} normal-input trials, 1440 frames replay identically')
    finally:
        neo.close()


if __name__ == '__main__':
    main()
