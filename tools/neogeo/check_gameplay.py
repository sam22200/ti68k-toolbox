#!/usr/bin/env python3
"""Windjammers original: cold-boot serve door, walking and ordinary disc traces.

No TI engine or ROM code translation. Ordinary trials contain no RAM writes;
the separately labelled coordinate injections validate the candidate fields.
"""
import argparse
import csv
import json
from pathlib import Path
import subprocess
import sys

from check_reference import frame_fingerprint
from neogeorun import DEFAULT_BIOS, DEFAULT_CORE, DEFAULT_ROM, NeoGeo, PAD, script, sha, write_file

KEYS = Path(__file__).parent / 'keys/windjammers_serve.txt'
FIELDS = {f'{name}_{field}': (base + offset, size, signed)
          for name, base in (('p1', 0x100800), ('p2', 0x100880), ('disc', 0x100a00))
          for field, offset, size, signed in (
              ('x', 6, 4, False), ('y', 10, 4, False),
              ('vx', 40, 4, True), ('vy', 44, 4, True),
              ('render_x', 20, 2, False), ('render_y', 22, 2, False))}
FIELDS.update(p1_action=(0x100824, 2, False), p2_action=(0x1008a4, 2, False),
              disc_state=(0x100a22, 2, False))
DIRECTIONS = {'UP': (0, -1), 'DOWN': (0, 1), 'LEFT': (-1, 0), 'RIGHT': (1, 0),
              'UP LEFT': (-1, -1), 'UP RIGHT': (1, -1),
              'DOWN LEFT': (-1, 1), 'DOWN RIGHT': (1, 1)}


def mask(keys):
    return sum(1 << PAD[k] for k in keys.split())


def values(neo):
    return {name: neo.read(address, size, signed) for name, (address, size, signed) in FIELDS.items()}


def bounded(value, low, high):
    # OBSERVED: clamp the integer word, retain the updated fractional word.
    return (min(high, max(low, value >> 16)) << 16) | (value & 65535)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bios', type=Path, default=DEFAULT_BIOS)
    parser.add_argument('--rom', type=Path, default=DEFAULT_ROM)
    parser.add_argument('--core', type=Path, default=DEFAULT_CORE)
    parser.add_argument('--out', type=Path, default=Path('sources/windjammers_neogeo/gameplay'))
    parser.add_argument('--door-only', action='store_true', help=argparse.SUPPRESS)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    if not args.door_only:
        # FBNeo cannot safely unload/reinitialize a real Neo Geo driver in one
        # process. Independent cold boots need independent core address spaces.
        subprocess.run([sys.executable, str(Path(__file__).resolve()), '--door-only',
                        '--bios', str(args.bios), '--rom', str(args.rom), '--core', str(args.core),
                        '--out', str(args.out / 'cold_boot_check')], check=True)
    neo = NeoGeo(args.rom, args.bios, args.core)
    trials, surfaces = {}, {}
    try:
        assert neo.status()['bios_crc32'] == '91b64be3', 'serve script requires the measured MVS v6 BIOS'
        keys = script(KEYS)
        def boot():
            for frame in range(4351):
                if frame in keys:
                    neo.pressed[:] = keys[frame]
                neo.step()
        boot()
        first_door = frame_fingerprint(neo)
        write_file(args.out / 'fingerprint.json', json.dumps(first_door, indent=2) + '\n')
        neo.snapshot(args.out / 'cold_boot_snapshot')
        if args.door_only:
            return
        assert first_door == json.loads((args.out / 'cold_boot_check/fingerprint.json').read_text()), \
            'two independent fresh cold boots must reach the same door'
        door = values(neo)
        assert (door['p1_x'], door['p2_x'], door['p1_y'], door['p2_y']) == (
            40 << 16, 280 << 16, 138 << 16, 138 << 16)
        assert (door['p1_action'], door['p2_action'], door['disc_state']) == (0, 0x1004, 2)
        assert door['disc_x'] == door['p2_x'] and door['disc_y'] == door['p2_y']
        neo.image().save(args.out / 'serve.png')
        neo.snapshot(args.out / 'serve')

        def save(name):
            neo.save(args.out / (name + '.state'))
            # libretro may duplicate the first restored frame (NULL video callback).
            # Its frontend's last presented surface is separate from the core state.
            surfaces[name] = neo.frame

        def restore(name):
            neo.load(args.out / (name + '.state'))
            neo.frame = surfaces[name]

        def run(name, count, inputs, inject=None):
            rows, hashes = [], []
            if inject:
                inject()
            for frame in range(count):
                neo.pressed[:] = inputs(frame)
                neo.step()
                rows.append(values(neo))
                hashes.append(frame_fingerprint(neo))
            return dict(rows=rows, hashes=hashes, diagnostic_writes=bool(inject))

        def trial(name, state, count, inputs, inject=None):
            restore(state)
            first = run(name, count, inputs, inject)
            restore(state)
            assert run(name, count, inputs, inject) == first, f'{name}: full replay differs'
            trials[name] = first
            with (args.out / (name + '.csv')).open('w') as stream:
                writer = csv.DictWriter(stream, fieldnames=['frame', *FIELDS])
                writer.writeheader()
                writer.writerows(dict(frame=i, **row) for i, row in enumerate(first['rows']))
            return first['rows']

        save('serve')
        # Uninterrupted vs restored sequence, including the cached duplicate surface.
        rally_input = lambda f: [mask('UP') if f < 8 else 0, mask('A') if 4 <= f < 6 else 0]
        uninterrupted = run('rally', 160, rally_input)
        restore('serve')
        assert run('rally', 160, rally_input) == uninterrupted
        trials['rally'] = uninterrupted

        # Ordinary P2 throw unlocks him; the preparation uses no diagnostic writes.
        restore('serve')
        run('prepare_free', 40, lambda f: [0, mask('A') if 4 <= f < 6 else 0])
        assert values(neo)['p2_action'] == 0
        save('free')

        for port, state, axial, diagonal, xbounds in (
                (0, 'serve', 0x28000, 0x1c480, (27, 141)),
                (1, 'free', 0x24000, 0x19740, (180, 292))):
            actor = f'p{port + 1}'
            for direction, (dx, dy) in DIRECTIONS.items():
                def inputs(f):
                    result = [0, 0]
                    # Walk, release, then reverse: clamp and instant stop/reversal.
                    result[port] = mask(direction) if f < 80 else (
                        mask(' '.join({'UP': 'DOWN', 'DOWN': 'UP', 'LEFT': 'RIGHT',
                                       'RIGHT': 'LEFT'}[k] for k in direction.split())) if f >= 84 else 0)
                    return result
                restore(state)
                previous = values(neo)
                rows = trial(actor + '_' + direction.replace(' ', '_'), state, 92, inputs)
                speed = diagonal if dx and dy else axial
                for f, row in enumerate(rows):
                    sign = 1 if f < 80 else (-1 if f >= 84 else 0)
                    vx, vy = dx * speed * sign, dy * speed * sign
                    assert (row[actor + '_vx'], row[actor + '_vy']) == (vx, vy), (actor, direction, f)
                    assert row[actor + '_x'] == bounded(previous[actor + '_x'] + vx, *xbounds)
                    assert row[actor + '_y'] == bounded(previous[actor + '_y'] + vy, 76, 188)
                    previous = row

        launches = {}
        for direction, release, velocity, bounces in (
                ('A', 16, (-0x57000, 0), []),
                ('UP A', 16, (-0x3d830, -0x3d830), [32, 66]),
                ('DOWN A', 12, (-0x3d830, 0x3d830), [27, 61])):
            rows = trial('disc_' + direction.replace(' ', '_'), 'serve', 80,
                         lambda f: [0, mask(direction) if 4 <= f < 6 else 0])
            first = next(f for f, row in enumerate(rows) if row['disc_state'] == 4)
            assert first == release and (rows[first]['disc_vx'], rows[first]['disc_vy']) == velocity
            actual_bounces = [f for f in range(1, len(rows)) if
                              rows[f]['disc_state'] == rows[f-1]['disc_state'] == 4 and
                              rows[f]['disc_vy'] != rows[f-1]['disc_vy']]
            assert actual_bounces == bounces
            for f in range(first + 1, len(rows)):
                before, after = rows[f - 1], rows[f]
                if before['disc_state'] == after['disc_state'] == 4 and f not in bounces:
                    assert after['disc_x'] - before['disc_x'] == after['disc_vx']
                    assert after['disc_y'] - before['disc_y'] == after['disc_vy']
            launches[direction] = dict(release_frame=first, vx=velocity[0], vy=velocity[1],
                                       rebound_frames=actual_bounces)

        # Independently reposition both players and the in-flight disc. The renderer
        # copies integer coordinates; ROM execution consumes the injected velocities.
        for port, state in ((0, 'serve'), (1, 'free')):
            base, actor, x = 0x100800 + port * 128, f'p{port + 1}', 80 if port == 0 else 230
            def inject():
                neo.write(base + 6, x << 16, 4)
                neo.write(base + 10, 110 << 16, 4)
            rows = trial(actor + '_coordinate_poke', state, 3, lambda f: [0, 0], inject)
            assert (rows[-1][actor + '_render_x'], rows[-1][actor + '_render_y']) == (x, 110)
            neo.image().save(args.out / (actor + '_coordinate_poke.png'))

        restore('serve')
        run('prepare_flight', 21, lambda f: [0, mask('UP A') if 4 <= f < 6 else 0])
        save('flight')
        def disc_poke():
            for offset, value in ((6, 210 << 16), (10, 110 << 16), (40, -0x20000), (44, 0x10000)):
                neo.write(0x100a00 + offset, value, 4)
        rows = trial('disc_coordinate_velocity_poke', 'flight', 4, lambda f: [0, 0], disc_poke)
        for f, row in enumerate(rows):
            assert (row['disc_x'], row['disc_y']) == ((210 - 2 * (f + 1)) << 16, (111 + f) << 16)
        neo.image().save(args.out / 'disc_coordinate_velocity_poke.png')

        report = dict(metadata=neo.metadata(), boot_frames=4351, keys_sha256=sha(KEYS.read_bytes()),
                      serve=door, field_map=FIELDS, launches=launches,
                      identical_fresh_cold_boots=2,
                      uninterrupted_replay_frames=160, frontend_duplicate_surface_seeded=True,
                      trials=trials, native_port=False,
                      covered_by_check_rules=['exact ordinary wall positions',
                                              'neutral contact boxes/classification', 'Beach points/next serve'],
                      unresolved=['dash/lob/charged/special actions', 'other characters/courts',
                                  'full animation/collision-angle model', 'round/time-out ending'])
        write_file(args.out / 'report.json', json.dumps(report, indent=2) + '\n')
        print(f'Windjammers: cold serve, 16 movement trials, 3 disc trials, 3 diagnostic trials; '
              f'{sum(len(t["rows"]) for t in trials.values())} full replay frames passed')
        print('Original measurements only; see check_rules.py for wall/contact/goal checks. Native TI engine remains ahead.')
    finally:
        neo.close()


if __name__ == '__main__':
    main()
