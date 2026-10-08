#!/usr/bin/env python3
"""No-write A/contact timing, lift/charge and complete immediate-return trials."""
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

TIMING_FIELDS = dict(FIELDS)
for port, base in ((1, 0x100800), (2, 0x100880)):
    TIMING_FIELDS[f'p{port}_charge'] = (base + 0x4e, 2, False)
TIMING_FIELDS.update(z=(0x100a44, 4, True), vz=(0x100a48, 4, True))


def main():
    out = GAME / 'generated'
    program = (ROOT / 'sources/windjammers_neogeo/program.be.bin').read_bytes()
    assert sha(program) == PROGRAM_SHA
    provenance = json.loads((out / 'actions.json').read_text())['metadata']
    neo = NeoGeo()
    trials, doors, surfaces = {}, [], []
    try:
        metadata = neo.metadata()
        for field in ('core_sha256', 'rom_sha256', 'bios_archive_sha256', 'fps', 'options', 'rtc_initial'):
            assert metadata[field] == provenance[field], field
        def sample():
            return {**values(neo), **{k: neo.read(*v) for k, v in TIMING_FIELDS.items()}}
        if '--fixtures-only' in sys.argv:
            capture = json.loads((out / 'timing_capture.json').read_text())
            assert capture['program_sha256'] == PROGRAM_SHA and capture['metadata'] == metadata
            trials, doors = capture['trials'], capture['doors']
        else:
            for receiver, state in ((0, 'actions_serve.state'), (1, 'actions_mita.state')):
                # A normal opposite-side straight serve creates each incoming door.
                neo.load(out / state)
                neo.pressed[:] = [0, 0]
                neo.step(); neo.step()
                neo.save(out / 'timing_prepare.state')
                surface = neo.frame
                rows = []
                for frame in range(100):
                    neo.pressed[:] = [0, 0]
                    neo.pressed[receiver ^ 1] = mask('A') if 4 <= frame < 6 else 0
                    neo.step(); rows.append(sample())
                catch = next(f for f, row in enumerate(rows) if row[f'p{receiver + 1}_action'] == 0x1000)
                neo.load(out / 'timing_prepare.state'); neo.frame = surface
                for frame in range(catch - 18):
                    neo.pressed[:] = [0, 0]
                    neo.pressed[receiver ^ 1] = mask('A') if 4 <= frame < 6 else 0
                    neo.step()
                neo.save(out / f'timing_p{receiver + 1}.state')
                surfaces.append(neo.frame); doors.append(sample())
                cases = [(None, 1, 'A')]
                cases += [(delay, 1, 'A') for delay in (4, 5, 6, 12, 13, 14, 15, 16, 17,
                                                      18, 19, 20, 21, 25, 26, 30, 31, 32, 33, 34)]
                cases += [(delay, held, 'A') for delay, held in ((0, 45), (14, 45), (18, 45), (19, 2))]
                cases += [(delay, 1, direction) for delay in (18, 19, 21, 30, 31, 33, 34)
                          for direction in ('UP A', 'DOWN A')]
                for delay, held, direction in cases:
                    def run():
                        neo.load(out / f'timing_p{receiver + 1}.state'); neo.frame = surfaces[receiver]
                        neo.pressed[:] = [0, 0]
                        initial = sample()
                        rows, hashes = [], []
                        for frame in range(140):
                            neo.pressed[:] = [0, 0]
                            if delay is not None and delay <= frame < delay + held:
                                neo.pressed[receiver] = mask(direction)
                            neo.step(); rows.append(sample()); hashes.append(frame_fingerprint(neo))
                        return dict(receiver=receiver, delay=delay, held=held, direction=direction,
                                    initial=initial, rows=rows, hashes=hashes)
                    result = run()
                    assert result == run(), (receiver, delay, held, direction)
                    trials[f'p{receiver + 1}_{delay}_{held}_{direction.replace(" ", "_")}'] = result
            capture = dict(metadata=metadata, fields=TIMING_FIELDS, program_sha256=PROGRAM_SHA,
                           script_sha256=sha(Path(__file__).read_bytes()), doors=doors, trials=trials)
            (out / 'timing_capture.json').write_text(json.dumps(capture, indent=2) + '\n')
        starts, samples, outcomes = [], [], []
        for trial in trials.values():
            ident = len(starts)
            starts.append((trial['receiver'], 65535 if trial['delay'] is None else trial['delay'],
                           trial['held'], mask(trial['direction'])))
            rows = trial['rows']
            blocked = next((f for f, row in enumerate(rows) if row['disc_state'] == 10), None)
            lifted = next((f for f, row in enumerate(rows) if row['disc_state'] == 14), None)
            released = next((f for f, row in enumerate(rows) if row[f'p{trial["receiver"] + 1}_action'] == 0x1400), None)
            if blocked is not None:
                # Original deflection uses RNG-picked targets and is explicitly
                # not a native equality flight. Check the input/contact prefix
                # and assert its classification separately, without hiding it.
                stop, coverage = blocked, 'prefix_then_block_classification'
                outcomes.append((ident, blocked, 10))
            elif lifted is not None:
                stop = next(f for f in range(lifted + 1, len(rows)) if rows[f]['disc_state'] == 2) + 2
                coverage = 'lift_charge_descent_recapture'
            elif trial['delay'] is None or released not in (trial['delay'], trial['delay'] + trial['held']):
                stop, coverage = 50, 'ordinary_catch_no_repeat_from_held_A'
            elif released is not None:
                flight = next(f for f in range(released + 1, len(rows)) if rows[f]['disc_state'] == 4)
                settled = next((f for f in range(flight + 1, len(rows)) if rows[f][f'p{(trial["receiver"] ^ 1) + 1}_action'] == 0x1004), None)
                if settled is None:
                    stop = next(f for f in range(flight + 1, len(rows)) if
                                rows[f][f'p{(trial["receiver"] ^ 1) + 1}_contact'] or rows[f]['disc_state'] != 4) + 1
                    coverage = 'immediate_diagonal_flight_through_contact_or_miss'
                else:
                    stop, coverage = settled + 2, 'immediate_return_flight_catch_settled'
            else:
                stop, coverage = 50, 'ordinary_catch_no_repeat_from_held_A'
            trial['native_coverage'] = dict(kind=coverage, steps=stop, blocked_frame=blocked)
            for frame, row in enumerate(rows[:stop]):
                actor_fields = []
                for port in (1, 2):
                    actor_fields += [row[f'p{port}_{field}'] for field in
                                     ('x', 'y', 'vx', 'vy', 'action', 'hold_age', 'power', 'bonus', 'charge')]
                    actor_fields.append((row[f'p{port}_flags21'] >> 4) & 1)
                samples.append((ident, frame, *actor_fields,
                    row['disc_x'], row['disc_y'], row['disc_state'], row['disc_vx'], row['disc_vy'],
                    row['z'], row['vz']))
        # The profile IDs are generated by prepare.py from the same CPU image.
        profile_text = (out / 'tables.h').read_text().split('static const WjProfile profiles')[1].split('};')[0]
        profiles = [tuple(map(int, line.split('{')[1].split('}')[0].split(',')))
                    for line in profile_text.splitlines() if line.strip().startswith('{')]
        header = ['/* Normal-input incoming doors, generated locally. */']
        header.append('static const WjTimingDoor timing_doors[2] = {')
        for origin in doors:
            profile = next(i for i, p in enumerate(profiles) if p[:2] == (origin['disc_vx'], origin['disc_vy']))
            row = [origin[f'p{port}_{field}'] for port in (1, 2) for field in ('x', 'y', 'hold_age', 'power', 'bonus')]
            row += [origin['disc_x'], origin['disc_y'], profile]
            header.append('  {' + ','.join(map(str, row)) + '},')
        header.append('};')
        # Ready pose descriptors share Mita's neutral box; Yoo's has a +2 anchor
        # flipped to -2 on the right and half_x18 instead of neutral14.
        for receiver in (0, 1):
            char = 12 if receiver else 20
            table = int.from_bytes(program[0x1c908 + char:0x1c90c + char], 'big')
            header.append('static const u8 ready_window_' + str(receiver) + '[16] = {' +
                          ','.join(map(str, program[table:table + 16])) + '};')
        (out / 'timing_doors.h').write_text('\n'.join(header) + '\n')
        header = ['/* Original timing fixtures, PC only; coverage intervals in timing.json. */']
        for title, rows in (('TimingStart timing_start', starts), ('TimingRef timing_ref', samples),
                            ('TimingOutcome timing_outcome', outcomes)):
            header.append('static const ' + title + '[] = {')
            header += ['  {' + ','.join(map(str, row)) + '},' for row in rows]
            header.append('};')
        (out / 'timing.h').write_text('\n'.join(header) + '\n')
        report = dict(metadata=metadata, fields=TIMING_FIELDS, program_sha256=PROGRAM_SHA,
                      script_sha256=sha(Path(__file__).read_bytes()), capture_script_sha256=capture['script_sha256'], doors=doors,
                      fixture_frames=len(samples), trials=trials,
                      adapted='blocked flight RNG target; classification checked separately')
        (out / 'timing.json').write_text(json.dumps(report, indent=2) + '\n')
        print(f'Timed receptions: {len(trials)} no-write trials, {140 * len(trials)} replay frames, '
              f'{len(samples)} native equality steps, {len(outcomes)} block classifications')
    finally:
        neo.close()


if __name__ == '__main__':
    main()
