#!/usr/bin/env python3
"""Windjammers Beach: measured wall sweep, contact boxes, goals and next serve.

This is a host reference oracle, not a native port. Normal rallies use only
inputs; labelled diagnostic trials inject coordinates/velocities before replay.
"""
import argparse
import csv
import json
from pathlib import Path

from check_gameplay import FIELDS, KEYS, mask, values
from check_reference import frame_fingerprint
from neogeorun import DEFAULT_BIOS, DEFAULT_CORE, DEFAULT_ROM, NeoGeo, script, sha, write_file

PROGRAM_SHA = '53dc6e3d48729c74cef82e6b2dfde5af67af88ea445d820c509346677601c641'
EXTRA_FIELDS = dict(
    p1_flags=(0x100800, 1, False), p2_flags=(0x100880, 1, False),
    p1_contact=(0x10081a, 2, False), p2_contact=(0x10089a, 2, False),
    p1_pose=(0x100804, 2, False), p2_pose=(0x100884, 2, False),
    disc_pose=(0x100a04, 2, False), disc_contact=(0x100a1a, 2, False),
    disc_wall=(0x100a1c, 1, False), disc_flags=(0x100a20, 1, False),
    goal_flags=(0x100b81, 1, False), stage=(0x100322, 2, False),
    left_zone=(0x1000a3, 1, False), right_zone=(0x1000a4, 1, False),
    p1_points=(0x100873, 1, False), p2_points=(0x1008f3, 1, False),
    p1_arcade_score=(0x100066, 4, False), p2_arcade_score=(0x10006a, 4, False),
    clock=(0x10008c, 2, False), clock_ticks=(0x10008e, 2, False))


def wall_step(x, y, vx, vy):
    """ROM 02AB42..02AC10, for ordinary flight away from side/corner cells.

    The unsigned comparison with a possibly negative VY is deliberate. DIVS
    truncates toward zero after signed-word extraction, quantizing to 1/2048 px.
    The caller's full integration survives unless a swept step hits a wall.
    """
    ax, selected = abs(vx), vy
    if ax < 65536:
        selected = abs(selected)
        if selected < 65536:
            yy = y + vy
            if yy >> 16 < 72:
                return x + vx, (72 << 16) | (yy & 65535), True
            if yy >> 16 >= 200:
                return x + vx, (199 << 16) | (yy & 65535), True
            return x + vx, yy, False
    if (selected & 0xffffffff) <= ax:
        selected = ax
    count = abs(selected >> 16) + 1

    def quantized(v):
        word = (v >> 5) & 65535
        if word >= 32768:
            word -= 65536
        return (abs(word) // count) * (-32 if word < 0 else 32)

    sx, sy = quantized(vx), quantized(vy)
    xx, yy = x, y
    for _ in range(count):
        xx, yy = xx + sx, yy + sy
        if yy >> 16 < 72:
            return xx, (72 << 16) | (yy & 65535), True
        if yy >> 16 >= 200:
            return xx, (199 << 16) | (yy & 65535), True
    return x + vx, y + vy, False


def collision_box(program, pose):
    def read(address, size, signed=False):
        return int.from_bytes(program[address:address + size], 'big', signed=signed)
    table = read(0x78000, 4)
    pointer = read(table + read(0x78004 + 2 * pose, 2), 4)
    return dict(pointer=pointer, arc_start=read(pointer, 1), arc_end=read(pointer + 1, 1),
                x=read(pointer + 8, 2, True), y=read(pointer + 10, 2, True),
                half_x=read(pointer + 12, 2), half_y=read(pointer + 14, 2))


def bcd(value):
    assert value & 15 <= 9 and value >> 4 <= 9
    return (value >> 4) * 10 + (value & 15)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bios', type=Path, default=DEFAULT_BIOS)
    parser.add_argument('--rom', type=Path, default=DEFAULT_ROM)
    parser.add_argument('--core', type=Path, default=DEFAULT_CORE)
    parser.add_argument('--program', type=Path, default=Path('sources/windjammers_neogeo/program.be.bin'))
    parser.add_argument('--out', type=Path, default=Path('sources/windjammers_neogeo/rules'))
    args = parser.parse_args()
    program = args.program.read_bytes()
    assert sha(program) == PROGRAM_SHA, 'oracle requires the measured CPU-order Windjammers image'
    args.out.mkdir(parents=True, exist_ok=True)
    neo = NeoGeo(args.rom, args.bios, args.core)
    surfaces, trials = {}, {}
    try:
        assert neo.status()['bios_crc32'] == '91b64be3'
        keys = script(KEYS)
        for frame in range(4351):
            if frame in keys:
                neo.pressed[:] = keys[frame]
            neo.step()
        assert (neo.read(0x100322, 2), neo.read(0x100a22, 2), neo.read(0x10008c, 2)) == (1, 2, 0x27)
        assert (neo.read(0x100873), neo.read(0x1008f3)) == (0, 0)
        assert (neo.read(0x1000a3), neo.read(0x1000a4)) == (1, 1)

        def sample():
            return {**values(neo), **{k: neo.read(*v) for k, v in EXTRA_FIELDS.items()}}

        def save(name):
            neo.save(args.out / (name + '.state'))
            surfaces[name] = neo.frame

        def restore(name):
            neo.load(args.out / (name + '.state'))
            neo.frame = surfaces[name]

        def advance(count, inputs):
            for f in range(count):
                neo.pressed[:] = inputs(f)
                neo.step()

        def trial(name, state, count, inputs=lambda f: [0, 0], writes=()):
            def run():
                restore(state)
                for address, value, size in writes:
                    neo.write(address, value, size)
                initial = sample()
                rows, hashes = [], []
                for f in range(count):
                    pads = inputs(f)
                    neo.pressed[:] = pads
                    neo.step()
                    rows.append(dict(frame=f, p1_keys=pads[0], p2_keys=pads[1], **sample()))
                    hashes.append(frame_fingerprint(neo))
                return dict(initial=initial, rows=rows, hashes=hashes,
                            diagnostic_writes=list(writes))
            first = run()
            assert run() == first, f'{name}: complete frame replay differs'
            trials[name] = first
            with (args.out / (name + '.csv')).open('w') as stream:
                writer = csv.DictWriter(stream, fieldnames=first['rows'][0])
                writer.writeheader()
                writer.writerows(first['rows'])
            return first['rows']

        save('serve')
        for name, count, inputs in (
                ('left_flight', 31, lambda f: [0, mask('A') if 4 <= f < 6 else 0]),
                ('right_flight', 111, lambda f: [mask('A') if 85 <= f < 87 else 0,
                                                mask('A') if 4 <= f < 6 else 0]),
                ('up_flight', 21, lambda f: [0, mask('UP A') if 4 <= f < 6 else 0])):
            restore('serve')
            advance(count, inputs)
            assert neo.read(0x100a22, 2) == 4
            save(name)

        rebounds = {}
        for direction, expected in (('UP A', [32, 66]), ('DOWN A', [27, 61])):
            rows = trial('ordinary_' + direction.replace(' ', '_'), 'serve', 80,
                         lambda f: [0, mask(direction) if 4 <= f < 6 else 0])
            contacts = []
            for before, after in zip(rows, rows[1:]):
                if before['disc_state'] == after['disc_state'] == 4:
                    x, y, hit = wall_step(before['disc_x'], before['disc_y'],
                                          before['disc_vx'], before['disc_vy'])
                    assert (after['disc_x'], after['disc_y']) == (x, y), (direction, after['frame'])
                    assert after['disc_vy'] == (-before['disc_vy'] if hit else before['disc_vy'])
                    assert after['disc_vx'] == before['disc_vx']
                    if hit:
                        contacts.append(after['frame'])
            assert contacts == expected
            rebounds[direction] = contacts

        # Fractional boundary tests, including the low-speed direct-contact path.
        wall_count = 0
        for vx, speed in ((-0x3d830, 0x3d830), (-0x28000, 0xc000),
                          (0x8000, 0x2c000), (0x8000, 0x4000)):
            for sign, edge in ((-1, 72), (1, 200)):
                for offset in (-0x10001, -1, 0, 1, 0x8000, 0x10001, 0x38000):
                    x, y, vy = (160 << 16) + 12345, (edge << 16) + offset, speed * sign
                    name = f'wall_{wall_count:03d}'
                    writes = [(0x100a00 + o, v, 4) for o, v in ((6, x), (10, y), (40, vx), (44, vy))]
                    after = trial(name, 'up_flight', 1, writes=writes)[0]
                    xx, yy, hit = wall_step(x, y, vx, vy)
                    assert (after['disc_x'], after['disc_y']) == (xx, yy), (name, after, (xx, yy, hit))
                    wall_count += 1

        # Resolve boxes from the pose selected by the original. This checks overlap
        # and its arc classification, not a replacement animation or atan2 model.
        contact_count, classifications = 0, {'none': 0, 'catch': 0, 'deflect': 0}
        contact_examples = {}
        for port, state, x in ((0, 'left_flight', 80), (1, 'right_flight', 230)):
            actor, base = f'p{port + 1}', 0x100800 + port * 128
            points = [(dx, dy) for dx in (-23, -22, -21, -1, 0, 1, 21, 22, 23)
                      for dy in (-28, -27, -24, -23, 0, 19, 20, 23, 24)]
            # Fractions on both entities must not expand the integer boxes.
            for dx, dy, pf, df in [(*p, 0, 0) for p in points] + [
                    (-22, 0, 65535, 1), (22, 0, 1, 65535), (0, 24, 65535, 1)]:
                writes = [(base + 6, (x << 16) + pf, 4), (base + 10, (130 << 16) + pf, 4),
                          (0x100a06, ((x + dx) << 16) + df, 4),
                          (0x100a0a, ((130 + dy) << 16) + df, 4),
                          (0x100a28, 0, 4), (0x100a2c, 0, 4)]
                rows = trial(f'contact_{contact_count:03d}', state, 2, writes=writes)
                first, second = rows
                pbox = collision_box(program, first[actor + '_pose'])
                dbox = collision_box(program, first['disc_pose'])
                flipped = bool(first[actor + '_flags'] & 16)
                px = (first[actor + '_x'] >> 16) + (-pbox['x'] if flipped else pbox['x'])
                py = (first[actor + '_y'] >> 16) + pbox['y']
                overlap = (abs(px - ((first['disc_x'] >> 16) + dbox['x'])) <= pbox['half_x'] + dbox['half_x']
                           and abs(py - ((first['disc_y'] >> 16) + dbox['y'])) <= pbox['half_y'] + dbox['half_y'])
                code = first[actor + '_contact'] >> 8
                assert bool(code) == overlap, (port, dx, dy, pbox, first)
                assert code in (0, 4, 8)
                if code:
                    angle = first[actor + '_contact'] & 255
                    start, end = pbox['arc_start'], pbox['arc_end']
                    if flipped:
                        start, end = (-end) & 255, (-start) & 255
                    expected = 4 if ((angle - start) & 255) <= ((end - start) & 255) else 8
                    assert code == expected, (port, dx, dy, angle, pbox)
                    assert second['disc_state'] == (2 if code == 4 else 12)
                    assert first['disc_state'] == 4, 'contact is consumed on the next frame'
                label = {0: 'none', 4: 'catch', 8: 'deflect'}[code]
                classifications[label] += 1
                contact_examples.setdefault(f'{actor}_{label}', dict(dx=dx, dy=dy, box=pbox,
                                                                     collision=first[actor + '_contact']))
                contact_count += 1
        assert all(classifications.values())

        crossing_count = 0
        for state, edge, vx, side in (('left_flight', 16, -0x4000, 'left'),
                                      ('right_flight', 304, 0x4000, 'right')):
            for offset in (-1, 0, 1, 65535):
                destination = (edge << 16) + offset
                writes = [(0x10080a, 80 << 16, 4), (0x10088a, 80 << 16, 4),
                          (0x100a06, destination - vx, 4), (0x100a0a, 138 << 16, 4),
                          (0x100a28, vx, 4), (0x100a2c, 0, 4)]
                row = trial(f'crossing_{side}_{crossing_count:02d}', state, 1, writes=writes)[0]
                crossed = destination >> 16 < 16 if side == 'left' else destination >> 16 >= 304
                assert row['disc_x'] == destination
                assert bool(row['disc_flags'] & 4) == crossed
                assert bool(row['goal_flags'] & 8) == crossed
                crossing_count += 1

        goals = {}
        for port, state, x in ((0, 'right_flight', 294), (1, 'left_flight', 26)):
            for y in ((119 << 16) + 65535, 120 << 16, (167 << 16) + 65535, 168 << 16,
                      88 << 16, 180 << 16, (135 << 16) + 65535, 152 << 16):
                award = 5 if 120 <= y >> 16 < 168 else 3
                defender_y = 180 if y >> 16 < 144 else 80
                writes = [(0x10080a + i * 128, defender_y << 16, 4) for i in (0, 1)]
                writes += [(0x100a06, x << 16, 4), (0x100a0a, y, 4)]
                name = f'goal_p{port + 1}_{y:08x}'
                rows = trial(name, state, 20, writes=writes)
                scored = next(f for f, row in enumerate(rows) if row[f'p{port + 1}_points'])
                assert bcd(rows[-1][f'p{port + 1}_points']) == award
                assert rows[-1][f'p{2 - port}_points'] == 0
                assert rows[-1][f'p{port + 1}_arcade_score'] == (award << 16)
                goals[name] = dict(y=y, award=award, score_frame=scored)

        # Normal missed shot: only movement/buttons, no artificial state changes.
        rows = trial('ordinary_goal_and_serve', 'serve', 260,
                     lambda f: [mask('UP') if f < 20 else 0, mask('A') if 4 <= f < 6 else 0])
        goal_frame = next(f for f, row in enumerate(rows) if row['p2_points'])
        reset_frame = next(f for f, row in enumerate(rows) if f > goal_frame and row['disc_state'] == 0)
        ballboy_frame = next(f for f, row in enumerate(rows) if f > goal_frame and row['disc_state'] == 20)
        serve_frame = next(f for f, row in enumerate(rows) if f > goal_frame and row['disc_state'] == 2)
        entry_frame = next(f for f, row in enumerate(rows) if row['goal_flags'] & 8)
        goal_animation_frame = next(f for f, row in enumerate(rows) if row['disc_state'] == 18)
        assert (entry_frame, goal_frame, goal_animation_frame, reset_frame, ballboy_frame, serve_frame) == (
            60, 60, 62, 154, 210, 238)
        assert (rows[serve_frame]['p1_action'], rows[serve_frame]['p2_action']) == (0x1004, 0)
        assert all(row['p2_points'] == 5 and row['p1_points'] == 0 for row in rows[goal_frame:])
        assert rows[serve_frame]['clock'] < rows[goal_frame]['clock'], 'clock continues during this pause'
        neo.image().save(args.out / 'next_serve.png')
        # The other side's unmodified return throw: move Yoo clear of the shot.
        opposite = trial('ordinary_opposite_goal_and_serve', 'right_flight', 250,
                         lambda f: [0, mask('UP') if f < 20 else 0])
        opposite_goal = next(f for f, row in enumerate(opposite) if row['p1_points'])
        opposite_serve = next(f for f, row in enumerate(opposite) if f > opposite_goal and row['disc_state'] == 2)
        assert (opposite_goal, opposite_serve) == (41, 220)
        assert (opposite[opposite_serve]['p1_action'], opposite[opposite_serve]['p2_action']) == (0, 0x1004)
        assert all(row['p1_points'] == 5 and row['p2_points'] == 0 for row in opposite[opposite_goal:])
        report = dict(metadata=neo.metadata(), program_sha256=PROGRAM_SHA, keys_sha256=sha(KEYS.read_bytes()),
                      boot_frames=4351, fields={**FIELDS, **EXTRA_FIELDS}, rebounds=rebounds,
                      wall_diagnostics=wall_count, contact_diagnostics=contact_count,
                      goal_crossing_diagnostics=crossing_count,
                      contact_classifications=classifications, contact_examples=contact_examples,
                      goal_diagnostics=goals, ordinary_goal=dict(entry_frame=entry_frame, goal_frame=goal_frame,
                      animation_frame=goal_animation_frame, reset_frame=reset_frame,
                      ballboy_frame=ballboy_frame, serve_frame=serve_frame, award=5, next_owner='p1'),
                      ordinary_opposite_goal=dict(goal_frame=opposite_goal, serve_frame=opposite_serve,
                                                  award=5, next_owner='p2'),
                      trials=trials, frontend_duplicate_surface_seeded=True, native_port=False,
                      scope='Beach, Mita/Yoo, ordinary ground flight and neutral poses',
                      unresolved=['collision-angle model and full animation selection', 'corner/lob/special collisions',
                                  'other courts/characters', 'match/round ending', 'native TI adaptation'])
        write_file(args.out / 'report.json', json.dumps(report, indent=2) + '\n')
        count = sum(len(t['rows']) for t in trials.values())
        print(f'Windjammers rules: 4 natural rebounds, {wall_count} wall / {contact_count} contact / '
              f'{crossing_count} crossing / {len(goals)} goal diagnostics, 2 normal goal/serve sequences; '
              f'{count} exact replay frames passed')
    finally:
        neo.close()


if __name__ == '__main__':
    main()
