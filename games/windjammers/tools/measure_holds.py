#!/usr/bin/env python3
"""No-write possession-age/return-strength trials, with full deterministic replay."""
import json
from pathlib import Path
import sys

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
sys.path.insert(0, str(ROOT / 'tools/neogeo'))
from check_gameplay import mask, values
from check_reference import frame_fingerprint
from neogeorun import NeoGeo, sha
from measure_actions import FIELDS, PROGRAM_SHA


def main():
    out = GAME / 'generated'
    provenance = json.loads((out / 'actions.json').read_text())['metadata']
    program = ROOT / 'sources/windjammers_neogeo/program.be.bin'
    assert sha(program.read_bytes()) == PROGRAM_SHA
    neo = NeoGeo()
    trials = {}
    try:
        metadata = neo.metadata()
        for field in ('core_sha256', 'rom_sha256', 'bios_archive_sha256', 'fps', 'options', 'rtc_initial'):
            assert metadata[field] == provenance[field], field
        def sample():
            return {**values(neo), **{k: neo.read(*v) for k, v in FIELDS.items()}}
        for owner, state in ((0, 'actions_mita.state'), (1, 'actions_serve.state')):
            neo.load(out / state)
            neo.pressed[:] = [0, 0]
            neo.step(); neo.step()
            door = out / f'holds_p{owner + 1}.state'
            neo.save(door)
            surface = neo.frame
            cases = [(None, '', 600)] + [(delay, direction, 220) for delay in (0, 4, 12, 24)
                       for direction in ('A', 'UP A', 'DOWN A')]
            if owner:
                cases += [(40, direction, 220) for direction in ('A', 'UP A', 'DOWN A')]
            for delay, direction, frames in cases:
                def run():
                    neo.load(door)
                    neo.frame = surface
                    initial = sample()
                    rows, hashes = [], []
                    for frame in range(frames):
                        pads = [0, 0]
                        if delay is not None and delay <= frame < delay + 2:
                            pads[owner] = mask(direction)
                        neo.pressed[:] = pads
                        neo.step()
                        rows.append(sample())
                        hashes.append(frame_fingerprint(neo))
                    return dict(owner=owner, delay=delay, direction=direction, initial=initial,
                                rows=rows, hashes=hashes)
                result = run()
                assert result == run(), (owner, delay, direction)
                trials[f'p{owner + 1}_{delay}_{direction.replace(" ", "_") or "auto"}'] = result
        starts, samples = [], []
        for trial in trials.values():
            ident = len(starts)
            origin = trial['initial']
            starts.append((trial['owner'], 65535 if trial['delay'] is None else trial['delay'],
                           mask(trial['direction']),
                           *[origin[f'p{port}_{field}'] for port in (1, 2)
                             for field in ('x', 'y', 'hold_age', 'power', 'bonus')]))
            # Manual cases compare the entire initial release/flight/catch and
            # first settled hold. Automatic controls compare all 600 steps.
            launched = False
            for frame, row in enumerate(trial['rows']):
                launched |= row['disc_state'] == 4
                samples.append((ident, frame,
                    *[row[f'p{port}_{field}'] for port in (1, 2)
                      for field in ('x', 'y', 'vx', 'vy', 'action', 'hold_age', 'power', 'bonus')],
                    *[row[field] for field in ('disc_x', 'disc_y', 'disc_state', 'disc_vx', 'disc_vy')]))
                receiver = trial['owner'] ^ 1
                if trial['delay'] is not None and launched and row[f'p{receiver + 1}_action'] == 0x1004:
                    break
        report = dict(metadata=metadata, program_sha256=PROGRAM_SHA, fields=FIELDS,
                      script_sha256=sha(Path(__file__).read_bytes()),
                      fixture_frames=len(samples), trials=trials)
        (out / 'holds.json').write_text(json.dumps(report, indent=2) + '\n')
        header = ['/* Normal-input possession fixtures, PC tests only. */']
        for title, rows in (('HoldStart hold_start', starts), ('HoldRef hold_ref', samples)):
            header.append('static const ' + title + '[] = {')
            header += ['  {' + ','.join(map(str, row)) + '},' for row in rows]
            header.append('};')
        (out / 'holds.h').write_text('\n'.join(header) + '\n')
        print(f'Possession: {len(trials)} no-write trials, {sum(len(t["rows"]) for t in trials.values())} '
              f'frames replay identically; {len(samples)} native fixture steps')
    finally:
        neo.close()


if __name__ == '__main__':
    main()
