#!/usr/bin/env python3
"""Generate local mechanics tables/fixtures and replay Mita's ordinary throws.

The angle table is ROM-derived and stays ignored. Fixtures contain original
measurements, not predictions from the native engine. Rebuild the reference
reports with tools/neogeo's gameplay/rules targets when changing core settings.
"""
import json
from pathlib import Path
import sys

GAME = Path(__file__).resolve().parents[1]
ROOT = GAME.parents[1]
sys.path.insert(0, str(ROOT / 'tools/neogeo'))
from check_gameplay import mask, values
from check_reference import frame_fingerprint
from check_rules import PROGRAM_SHA, EXTRA_FIELDS, collision_box
from neogeorun import NeoGeo, sha


def angle(program, dx, dy):
    # Original neutral collision-center differences, doubled by the caller.
    x, y = -2 * dx, -2 * dy
    base, swap = (0xc0, True) if x >= 0 else (0, False)
    if y < 0:
        base, swap = (0x80, False) if x >= 0 else (0x40, True)
    x, y = abs(x), abs(y)
    if swap:
        x, y = y, x
    if x + y < 64:
        x, y = x << 3, y << 3
    return (base + program[0x1927e + ((y & 0x1f8) << 3) + ((x & 0x1f8) >> 3)]) & 255


def keys(original):
    return sum(native for core, native in ((4, 1), (5, 4), (6, 2), (7, 8), (0, 16))
               if original & (1 << core))


def main():
    out = GAME / 'generated'
    out.mkdir(exist_ok=True)
    source = ROOT / 'sources/windjammers_neogeo'
    program = (source / 'program.be.bin').read_bytes()
    assert sha(program) == PROGRAM_SHA
    gameplay = json.loads((source / 'gameplay/report.json').read_text())
    rules = json.loads((source / 'rules/report.json').read_text())
    for field in ('core_sha256', 'rom_sha256', 'bios_archive_sha256', 'fps', 'options', 'rtc_initial'):
        assert gameplay['metadata'][field] == rules['metadata'][field], f'reference provenance mismatch: {field}'
    # Validate the independent angle model against running-ROM contact labels.
    contacts = []
    for name, trial in rules['trials'].items():
        if not name.startswith('contact_'):
            continue
        port = 0 if trial['initial']['p1_x'] >> 16 == 80 else 1
        actor, row = f'p{port + 1}', trial['rows'][0]
        box = collision_box(program, row[actor + '_pose'])
        dx = (row['disc_x'] >> 16) - (row[actor + '_x'] >> 16)
        dy = (row['disc_y'] >> 16) - (row[actor + '_y'] >> 16)
        contact = row[actor + '_contact']
        if contact:
            assert angle(program, dx - box['x'], dy - box['y']) == contact & 255, name
        contacts.append((port, dx, dy, bool(row[actor + '_flags'] & 16), contact))

    neo = NeoGeo()
    try:
        assert neo.metadata()['core_sha256'] == gameplay['metadata']['core_sha256'], 'regenerate reference states'
        neo.load(source / 'gameplay/serve.state')
        # State load may duplicate the first presentation; no fingerprint/image
        # is requested until the genuine normal preparation supplies a surface.
        for f in range(80):
            neo.pressed[:] = [0, mask('A') if 4 <= f < 6 else 0]
            neo.step()
        hold = values(neo)
        assert hold['p1_action'] == 0x1004
        neo.save(out / 'mita_hold.state')
        surface = neo.frame
        mita = {}
        for direction in ('A', 'UP A', 'DOWN A'):
            def run():
                neo.load(out / 'mita_hold.state')
                neo.frame = surface
                rows, hashes = [], []
                for f in range(160):
                    neo.pressed[:] = [mask(direction) if 4 <= f < 6 else 0, 0]
                    neo.step()
                    rows.append({**values(neo), **{k: neo.read(*v) for k, v in EXTRA_FIELDS.items()}})
                    hashes.append(frame_fingerprint(neo))
                return dict(initial=hold, rows=rows, hashes=hashes)
            first = run()
            assert run() == first, direction
            mita[direction] = first
        (out / 'mita_reference.json').write_text(json.dumps(dict(metadata=neo.metadata(), trials=mita), indent=2) + '\n')
    finally:
        neo.close()

    # Ordinary velocity/substep/recoil profiles are generated offline. Retain
    # the six M1 indices so old diagnostic scenario doors remain compatible.
    def speed(port, hold):
        char = 12 if port else 20
        minimum = int.from_bytes(program[0x28d44 + (char >> 1):0x28d46 + (char >> 1)], 'big')
        if hold >= 64:
            return 96
        if hold < minimum:
            return 288  # Special-action trigger; native ordinary fallback only.
        address = 0x28d50 + (char << 3) + ((hold >> 2) << 1)
        return int.from_bytes(program[address:address + 2], 'big')

    pairs = [(0, 147), (1, 174)]
    lookup = []
    for port in (0, 1):
        row = []
        for hold in range(65):
            pair = (port, speed(port, hold))
            if pair not in pairs:
                pairs.append(pair)
            row.append(pairs.index(pair) * 3)
        lookup.append(row)
    profiles = []
    for port, scalar in pairs:
        for direction in range(3):
            vx = scalar * (2048 if not direction else 1448) * (-1 if port else 1)
            vy = scalar * 1448 * (-1 if direction == 1 else 1) if direction else 0
            selected = vy if (vy & 0xffffffff) > abs(vx) else abs(vx)
            count = abs(selected >> 16) + 1
            def step(v):
                word = (v >> 5) & 65535
                if word >= 32768:
                    word -= 65536
                return (abs(word) // count) * (-32 if word < 0 else 32)
            profiles.append((vx, vy, step(vx), step(vy), count, scalar))
    assert len(profiles) < 256
    table = ['/* Generated locally from the verified Windjammers CPU image. */',
             '#define WJ_PROFILE_COUNT ' + str(len(profiles)),
             'static const u8 collision_angles[4096] = {']
    raw = program[0x1927e:0x1927e + 4096]
    for start in range(0, len(raw), 32):
        table.append('  ' + ','.join(str(v) for v in raw[start:start + 32]) + ',')
    table += ['};', 'static const WjProfile profiles[WJ_PROFILE_COUNT] = {']
    table += ['  {' + ','.join(str(v) for v in p) + '},' for p in profiles]
    table.append('};')
    table.append('static const u8 hold_profiles[2][65] = {')
    table += ['  {' + ','.join(map(str, row)) + '},' for row in lookup]
    table.append('};')
    table.append('static const u8 reflected[WJ_PROFILE_COUNT] = {')
    table.append('  ' + ','.join(str(i if i % 3 == 0 else i + (1 if i % 3 == 1 else -1))
                               for i in range(len(profiles))))
    table.append('};')
    table.append('static const WjRecoil recoil[WJ_PROFILE_COUNT][10] = {')
    for vx, vy, _sx, _sy, _count, scalar in profiles:
        table.append('  {' + ','.join('{' + str((vx // scalar) * (scalar >> level)) + ',' +
                                    str((vy // scalar) * (scalar >> level)) + '}'
                                    for level in range(10)) + '},')
    table.append('};')
    (out / 'tables.h').write_text('\n'.join(table) + '\n')

    fixtures = ['/* Original execution fixtures, PC tests only. */',
                '#define WJ_TEST_PROFILE_COUNT ' + str(len(profiles)),
                'static const ContactRef contact_ref[] = {']
    fixtures += ['  {' + ','.join(str(int(v)) for v in p) + '},' for p in contacts]
    fixtures.append('};')
    walls = []
    for name, trial in rules['trials'].items():
        if name.startswith('wall_') and abs(trial['initial']['disc_vx']) == 0x3d830:
            before, after = trial['initial'], trial['rows'][0]
            walls.append((4 if before['disc_vy'] < 0 else 5,
                          before['disc_x'], before['disc_y'], after['disc_x'], after['disc_y']))
    fixtures.append('static const WallRef wall_ref[] = {')
    fixtures += ['  {' + ','.join(str(v) for v in p) + '},' for p in walls]
    fixtures.append('};')
    goal_refs = []
    for name, trial in rules['trials'].items():
        if not name.startswith('goal_p'):
            continue
        before = trial['initial']
        port = int(name[6]) - 1
        awarded = next(row for row in trial['rows'] if row[f'p{port + 1}_points'])
        goal_refs.append((port, awarded['frame'], awarded[f'p{port + 1}_points'],
                          before['disc_x'], before['disc_y'], before['p1_y'], before['p2_y'],
                          awarded['disc_x'], awarded['disc_y']))
    fixtures.append('static const GoalRef goal_ref[] = {')
    fixtures += ['  {' + ','.join(str(v) for v in p) + '},' for p in goal_refs]
    fixtures.append('};')
    # Player walking is compared field-for-field; no other actor is predicted.
    movement = []
    for name, trial in gameplay['trials'].items():
        if not name.startswith(('p1_', 'p2_')) or 'poke' in name:
            continue
        port = int(name[1]) - 1
        direction = name[3:].split('_')
        held = sum({'UP': 1, 'DOWN': 4, 'LEFT': 2, 'RIGHT': 8}[k] for k in direction)
        reverse = ((held & 1) << 2) | ((held & 4) >> 2) | ((held & 2) << 2) | ((held & 8) >> 2)
        for f, row in enumerate(trial['rows']):
            movement.append((port, f, held if f < 80 else reverse if f >= 84 else 0,
                             *[row[f'p{port + 1}_{k}'] for k in ('x', 'y', 'vx', 'vy')]))
    fixtures.append('static const MoveRef movement_ref[] = {')
    fixtures += ['  {' + ','.join(str(v) for v in p) + '},' for p in movement]
    fixtures.append('};')
    shots, launch_info = [], []
    for port, launches in ((1, {k: gameplay['trials']['disc_' + k.replace(' ', '_')] for k in mita}),
                           (0, mita)):
        for direction, trial in launches.items():
            rows = trial['rows']
            first = next(f for f, row in enumerate(rows) if row['disc_state'] == 4)
            catch = next(f for f, row in enumerate(rows) if f > first and (
                row['disc_state'] != 4 or row.get('p1_points', 0) or row.get('p2_points', 0)))
            origin = trial.get('initial', gameplay['serve'])
            shot_id = len(launch_info)
            launch_info.append((port, mask(direction), first, catch, rows[catch]['disc_state'],
                                origin['p1_x'], origin['p1_y'], origin['p2_x'], origin['p2_y']))
            # Native equality stops before adapted catch knockback.
            for f, row in enumerate(rows[:catch]):
                shots.append((shot_id, f, row['disc_x'], row['disc_y'], row['disc_vx'], row['disc_vy'],
                              row['disc_state'], row[f'p{port + 1}_action']))
    for title, values_ in (('ShotStart shot_start', launch_info), ('ShotRef shot_ref', shots)):
        fixtures.append('static const ' + title + '[] = {')
        fixtures += ['  {' + ','.join(str(v) for v in p) + '},' for p in values_]
        fixtures.append('};')
    (out / 'reference.h').write_text('\n'.join(fixtures) + '\n')
    (out / 'provenance.json').write_text(json.dumps(dict(program_sha256=PROGRAM_SHA,
        gameplay_report_sha256=sha((source / 'gameplay/report.json').read_bytes()),
        rules_report_sha256=sha((source / 'rules/report.json').read_bytes()),
        native_prepare_sha256=sha(Path(__file__).read_bytes()),
        movement_frames=len(movement), shot_frames=len(shots), contact_cases=len(contacts),
        wall_cases=len(walls), goal_cases=len(goal_refs)), indent=2) + '\n')
    print(f'Native fixtures: {len(movement)} movement frames, {len(shots)} ordinary shot frames, '
          f'{len(contacts)} contact labels; 480 extra Mita frames replay identically')


if __name__ == '__main__':
    main()
