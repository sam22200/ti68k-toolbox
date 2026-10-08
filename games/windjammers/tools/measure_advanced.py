#!/usr/bin/env python3
"""Repeat normal-input ROM lobs/specials/counters; export bounded C fixtures.

No reference RAM writes. A normal lob's native jitter is conditioned on its
observed target; this proves the flight model, not the Neo Geo global PRNG.
Post-goal celebrations, rear deflection and CPU decisions stay separate.
"""
import json
from pathlib import Path
import sys

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
sys.path[:0] = [str(ROOT / 'tools/neogeo')]
from check_gameplay import mask, values
from check_reference import frame_fingerprint
from neogeorun import NeoGeo, sha
from measure_timing import TIMING_FIELDS
from measure_actions import PROGRAM_SHA


def export(capture):
    program = (ROOT / 'sources/windjammers_neogeo/program.be.bin').read_bytes()
    profiles = (GAME / 'generated/tables.h').read_text().split('static const WjProfile profiles')[1].split('};')[0]
    vectors = [tuple(map(int, line.split('{')[1].split('}')[0].split(',')))[:2]
               for line in profiles.splitlines() if line.strip().startswith('{')]
    starts, samples, scripts, reports = [], [], [], []
    for ident, trial in enumerate(capture['trials']):
        initial, rows = trial['initial'], trial['rows']
        owner = trial['owner']
        launched = 0 if trial['kind'] == 'charge_control' else next(f for f, r in enumerate(rows) if r['disc_state'] in (4, 22, 38, 42, 44) and
                        (trial['kind'] == 'lob' or f > trial['delay']))
        # Include the complete air landing, or the first reception/push. Counter
        # cases include their returned shot until the next contact/goal boundary.
        if trial['kind'] == 'charge_control':
            stop = 106
        elif trial['counter'] is not None:
            released = next((f for f in range(trial['counter'], len(rows))
                             if rows[f][f'p{2-owner}_action'] in (0x1400, 0x140c, 0x1410, 0x141c)), None)
            if released is None:
                stop = next(f for f in range(launched+1, len(rows)) if rows[f]['disc_state'] == 24)
            else:
                relaunched = next(f for f in range(released + 1, len(rows)) if rows[f]['disc_state'] in (4, 22, 38, 42, 44))
                stop = next(f for f in range(relaunched + 1, len(rows))
                            if rows[f]['disc_state'] not in (4, 22, 38, 42, 44))
                if rows[stop]['disc_state'] == 2:
                    stop -= 1  # return flight through sampled contact
        else:
            stop = next(f for f in range(launched + 1, len(rows))
                        if rows[f]['disc_state'] in (24, 28, 18) or
                        (rows[f]['disc_state'] == 2 and rows[f-1]['disc_state'] == 2))
        # Goal rendering/score sequencing is the previously documented native
        # adaptation. Compare up to entry, not its celebration frame.
        if rows[stop]['disc_state'] == 18:
            stop -= 1
        scored = next((f for f in range(launched+1, stop+1) if
                       rows[f]['p1_points'] != initial['p1_points'] or rows[f]['p2_points'] != initial['p2_points']), None)
        if scored is not None:
            stop = scored - 1
        jitter = 0
        if trial['kind'] == 'lob' and rows[launched]['disc_state'] == 22:
            birth = rows[launched]
            angle = 192 if owner else 64
            if trial['direction'] == 'UP': angle = 224 if owner else 32
            if trial['direction'] == 'DOWN': angle = 160 if owner else 96
            import struct
            lookup = struct.unpack('>64H', program[0x28176:0x281f6])
            target = struct.unpack('>7H', program[0x28244:0x28252])
            # Formation's Y uses the actor's integer center plus four-pixel
            # vector; the fractional actor coordinate is retained by the disc.
            formed_y = rows[launched-1][f'p{owner+1}_y']
            formed_y += -0x2d400 if trial['direction'] == 'UP' else 0x2d400 if trial['direction'] == 'DOWN' else 0
            row = ((formed_y >> 16) - 64) // 20
            direction = program[0x1d76e + (angle >> 4)] // 2
            jitter = (birth['target_y'] >> 16) - target[lookup[row * 8 + direction] // 2]
            assert 0 <= jitter <= 31, jitter
        actor = [initial[f'p{port}_{field}'] for port in (1, 2)
                 for field in ('x', 'y', 'hold_age', 'power', 'bonus')]
        profile = vectors.index((initial['disc_vx'], initial['disc_vy'])) if initial['disc_state'] == 4 else 0
        starts.append((owner, initial['disc_state'], *actor, initial['disc_x'], initial['disc_y'], profile, jitter, len(scripts), stop + 1))
        for f, row in enumerate(rows[:stop+1]):
            scripts.append(trial['inputs'][f])
            values_ = [ident, f]
            for port in (1, 2):
                values_ += [row[f'p{port}_{field}'] for field in ('x', 'y', 'vx', 'vy', 'action', 'hold_age', 'power', 'bonus', 'charge')]
                values_.append((row[f'p{port}_flags21'] >> 4) & 1)
            values_ += [row[k] for k in ('disc_x', 'disc_y', 'disc_state', 'disc_vx', 'disc_vy', 'z', 'vz')]
            samples.append(values_)
        reports.append(dict(name=trial['name'], steps=stop+1, launch=launched,
                            boundary=rows[stop]['disc_state'], jitter=jitter,
                            coverage='charge_ignores_early_buttons_movement_recapture' if trial['kind'] == 'charge_control' else
                                     'automatic_release_before_late_B' if trial['kind'] == 'lob' and rows[launched]['disc_state'] == 4 else
                                     'normal_input_to_landing_or_reception_push' if trial['counter'] is None else 'normal_input_through_counter_and_return'))
    header = ['/* Original advanced execution fixtures; PC only. */']
    for title, rows_ in (('AdvancedStart advanced_start', starts), ('AdvancedRef advanced_ref', samples), ('AdvancedInput advanced_input', scripts)):
        header.append('static const ' + title + '[] = {')
        header += ['  {' + ','.join(map(str, row)) + '},' for row in rows_]
        header.append('};')
    (GAME / 'generated/advanced.h').write_text('\n'.join(header) + '\n')
    (GAME / 'generated/advanced.json').write_text(json.dumps(dict(metadata=capture['metadata'], program_sha256=PROGRAM_SHA,
        script_sha256=sha(Path(__file__).read_bytes()), reference_frames=sum(len(t['rows']) for t in capture['trials']),
        native_steps=len(samples), cases=reports, conditioned='normal lob target jitter; no assertion of global PRNG equality',
        excluded='native goal entry/celebration geometry; idle reuse of hold counter; special-lob 100c/1010 pose selector (reaction family and physics checked)'), indent=2) + '\n')
    print(f'Advanced reference: {len(starts)} repeated no-write trials, {len(samples)} native fixture steps', flush=True)


def main():
    out = GAME / 'generated'
    if '--fixtures-only' in sys.argv:
        export(json.loads((out / 'advanced_capture.json').read_text())); return
    neo = NeoGeo()
    trials = []
    fields = dict(TIMING_FIELDS)
    fields['target_y'] = (0x100a54, 4, False)
    try:
        metadata = neo.metadata()
        assert sha((ROOT / 'sources/windjammers_neogeo/program.be.bin').read_bytes()) == PROGRAM_SHA
        def sample(): return {**values(neo), **{k: neo.read(*v) for k, v in fields.items()}}
        for owner in (0, 1):
            cases = [('lob', delay, direction, None, False) for delay in (4, 24, 56) for direction in ('', 'UP', 'DOWN')]
            cases += [('special', delay, direction, None, False) for delay in (98, 108, 128) for direction in ('', 'UP', 'DOWN')]
            cases += [('superlob', 98, direction, None, miss) for direction in ('', 'UP', 'DOWN') for miss in (False, True)]
            cases += [('special', 98, '', (143 if owner == 0 else 161) + offset, owner == 1) for offset in (0, 1)]
            cases += [('charge_control', delay, direction, None, False) for delay,direction in ((30,'RIGHT'),(30,'A'),(30,'B'),(60,'RIGHT'),(99,'RIGHT'))]
            if '--charge-only' in sys.argv: cases = [case for case in cases if case[0] == 'charge_control']
            for kind, delay, direction, counter, miss in cases:
                state = f'timing_p{owner+1}.state' if kind != 'lob' else 'actions_serve.state' if owner else 'actions_mita.state'
                neo.load(out / state); neo.pressed[:] = [0, 0]
                neo.step(); neo.step()
                neo.save(out / 'advanced_prepare.state'); surface = neo.frame
                def run():
                    neo.load(out / 'advanced_prepare.state'); neo.frame = surface; neo.pressed[:] = [0, 0]
                    initial, rows, inputs, hashes = sample(), [], [], []
                    for frame in range(300):
                        neo.pressed[:] = [0, 0]
                        if kind != 'lob' and frame == 12: neo.pressed[owner] = mask('A')
                        if kind == 'charge_control':
                            if delay <= frame < delay + 3: neo.pressed[owner] = mask(direction)
                        elif frame == delay: neo.pressed[owner] = mask(direction + (' B' if kind in ('lob', 'superlob') else ' A'))
                        if miss and frame < 25: neo.pressed[owner ^ 1] = mask('UP')
                        if counter is not None and frame == counter: neo.pressed[owner ^ 1] |= mask('A')
                        inputs.append(tuple(neo.pressed)); neo.step(); rows.append(sample()); hashes.append(frame_fingerprint(neo))
                    return dict(owner=owner, kind=kind, direction=direction, delay=delay, counter=counter, miss=miss,
                                name=f'{owner}_{kind}_{delay}_{direction}_{counter}_{miss}', initial=initial, rows=rows, inputs=inputs, hashes=hashes)
                trial = run(); assert trial == run(), trial['name']; trials.append(trial)
                print('replayed ' + trial['name'], flush=True)
        if '--charge-only' in sys.argv:
            previous = json.loads((out / 'advanced_capture.json').read_text())
            assert previous['metadata'] == metadata
            trials = [t for t in previous['trials'] if t['kind'] != 'charge_control'] + trials
        capture = dict(metadata=metadata, fields=fields, program_sha256=PROGRAM_SHA, script_sha256=sha(Path(__file__).read_bytes()), trials=trials)
        (out / 'advanced_capture.json').write_text(json.dumps(capture) + '\n')
        export(capture)
    finally:
        neo.close()


if __name__ == '__main__': main()
