#!/usr/bin/env python3
"""Extract opening collision banks offline; measure unpatched original walking.

The source checkout is only a reading aid. Fixed ROM offsets are checked
against live original RAM, and movement fixtures record actual emulator data.
"""
import argparse
import collections
import json
import struct
from pathlib import Path
from reference import (GBA, ROM, OUTPUT, PLAYER, MAP_BOTTOM, ORIGIN_X, ORIGIN_Y,
                       extract, rom_bytes, raw, position, digest, sha)

GAME = Path(__file__).resolve().parents[1]
W, H, STRIDE = 720, 320, 64


def terrain():
    rom, assets = rom_bytes(), extract()
    pointers = struct.unpack_from('<40I', rom, 0x823c)
    rows = [struct.unpack_from('<16H', rom, p - 0x08000000) for p in pointers]
    ids = struct.unpack('<3969H', assets['map_bottom'])
    types = struct.unpack('<2048H', assets['types_bottom'])
    collision = bytearray([255] * (64 * 32))
    acts = bytearray(64 * 32)
    for y in range(H >> 4):
        for x in range(W >> 4):
            collision[y * 64 + x] = rom[0xb3e80 + types[ids[y * 63 + x] & 0xfff]]
            acts[y * 64 + x] = rom[0xb37a0 + types[ids[y * 63 + x] & 0xfff]]
    shape_for = list(range(16)) + list(rom[0x82dc:0x833c])
    # Unknown collision IDs are rejected rather than treated as walkable.
    assert all(t == 255 or t < len(shape_for) for t in collision)
    shapes = bytes(15 if t == 255 else shape_for[t] for t in collision)
    for host, order in (('mindat.bin', '<'), ('mindat.be.bin', '>')):
        tile_words = []
        for shape in rows:
            for y, bits in enumerate(shape):
                # Terrain is a diagnostic: grey blocked pixels with sparse dark
                # hatching, pale open ground. Geometry is the original mask.
                tile_words.extend((bits & (0x8888 if y & 2 else 0x2222),
                                   bits | (0x0100 if y == 7 else 0)))
        data = (collision + b''.join(struct.pack(order + '16H', *s) for s in rows)
                + shapes + struct.pack(order + '1280H', *tile_words) + acts)
        assert len(data) == 9984
        (GAME / host).write_bytes(data)
    return collision, rows, rom


def point(collision, rows, rom, x, y):
    if not (0 <= x < W and 0 <= y < H):
        return 1
    t = collision[(y >> 4) * 64 + (x >> 4)]
    s = 15 if t == 255 else t if t < 16 else rom[0x82dc + t - 16]
    return (rows[s][y & 15] >> (15 - (x & 15))) & 1


def sample(gba):
    x, y = position(gba)
    assert not (x & 255 or y & 255), 'walking must fit exact Q8.8 units'
    return [x >> 8, y >> 8, gba.read(PLAYER + 0xc),
            gba.read(PLAYER + 0x15), gba.read(PLAYER + 0x2a, 2)]


def place(gba, x, y):
    gba.load(OUTPUT / 'woods.state')
    gba.write(PLAYER + 0x2c, (ORIGIN_X + x) << 16, 4)
    gba.write(PLAYER + 0x30, (ORIGIN_Y + y) << 16, 4)


def route(collision, rows, rom):
    """Plan a conservative centreline offline; validate its actual run below."""
    def clear(x, y):
        return all(not point(collision, rows, rom, x + dx, y + dy)
                   for dx, dy in ((5, 0), (5, -6), (-5, 0), (-5, -6),
                                  (3, 2), (-3, 2), (3, -8), (-3, -8)))
    start, goal = (248, 88), (692, 136)
    todo, prev = collections.deque([start]), {start: None}
    while todo and goal not in prev:
        x, y = todo.popleft()
        for dx, dy in ((4, 0), (0, 4), (0, -4), (-4, 0)):
            n = (x + dx, y + dy)
            if n not in prev and clear(*n):
                prev[n] = (x, y)
                todo.append(n)
    assert goal in prev, 'opening centreline must reach the third view'
    path = [goal]
    while prev[path[-1]] is not None:
        path.append(prev[path[-1]])
    path.reverse()
    # Keep only bends and endpoint.
    return [path[0]] + [p for i, p in enumerate(path[1:-1], 1)
        if (p[0]-path[i-1][0], p[1]-path[i-1][1]) !=
           (path[i+1][0]-p[0], path[i+1][1]-p[1])] + [goal]


BUTTONS = {1: 'UP', 2: 'LEFT', 4: 'DOWN', 8: 'RIGHT'}


def run_trial(gba, trial):
    place(gba, *trial['spawn'])
    states, hashes, surfaces = [], [], []
    for key in trial['keys']:
        gba.step([name for bit, name in BUTTONS.items() if key & bit])
        states.append(sample(gba))
        hashes.append(digest(gba))
        surfaces.append(gba.read(0x03003f80 + 0x12))
    trial['surfaces'] = surfaces
    return states, hashes


def measure(collision, rows, rom):
    fixtures = GAME / 'fixtures'
    fixtures.mkdir(exist_ok=True)
    gba = GBA(ROM)
    trials = []
    try:
        gba.load(OUTPUT / 'woods.state')
        live = raw(gba, MAP_BOTTOM + 0x2004, 4096)
        assert all(collision[y*64+x] == live[y*64+x]
                   for y in range(H >> 4) for x in range(W >> 4))
        act_live = raw(gba, MAP_BOTTOM + 0xb004, 4096)
        assert all((GAME / 'mindat.bin').read_bytes()[7936+y*64+x] == act_live[y*64+x]
                   for y in range(H >> 4) for x in range(W >> 4))
        # Eight directions, no input, conflicting axes and obstacle contacts.
        for spawn in ((32, 88), (120, 88), (244, 120), (392, 48), (544, 48), (580, 140)):
            for key in (0, 1, 2, 4, 8, 3, 9, 6, 12, 5, 10, 15):
                trials.append({'spawn': list(spawn), 'keys': [key] * 90})
        bends = route(collision, rows, rom)
        place(gba, *bends[0])
        inputs = []
        for tx, ty in bends[1:]:
            for _ in range(500):
                x, y, action, *_ = sample(gba)
                assert action == 1, ('route left normal walking', x/256, y/256, action)
                dx, dy = tx * 256 - x, ty * 256 - y
                if abs(dx) <= 160 and abs(dy) <= 160:
                    break
                # Planned cardinal bends; correct accumulated <1px fractions.
                key = (8 if dx > 0 else 2) if abs(dx) > abs(dy) else (4 if dy > 0 else 1)
                gba.step([BUTTONS[key]])
                inputs.append(key)
            else:
                raise AssertionError(('stuck route bend', tx, ty, sample(gba)))
        trials.append({'name': 'opening', 'spawn': list(bends[0]), 'keys': inputs})
        for i, trial in enumerate(trials):
            states, hashes = run_trial(gba, trial)
            again, replay_hashes = run_trial(gba, trial)
            assert states == again and hashes == replay_hashes, ('original replay', i)
            # Room transitions and automatic surface actions are outside M1.
            # Keep their preceding normal-walking timeline and record the cut.
            cut = next((n for n, s in enumerate(states) if s[2] != 1 or
                        trial['surfaces'][n] in (18, 19) or
                        not (8*256 <= s[0] < (W-8)*256 and
                             12*256 <= s[1] < (H-4)*256)), len(states))
            if cut != len(states):
                trial['excluded_tail'] = {'first_frame': cut, 'state': states[cut],
                                          'surface': trial['surfaces'][cut]}
                trial['keys'] = trial['keys'][:cut]
                states = states[:cut]
            assert states, ('invalid trial spawn', i)
            trial['states'] = states
            trial['surfaces'] = trial['surfaces'][:len(states)]
        metadata = {'rom_sha256': sha(rom), 'core': gba.identity,
                    'sample': 'post-frame, Q8.8 x/y, action, direction, collision bits',
                    'spawn_pokes': ['player.x Q16.16', 'player.y Q16.16'],
                    'checked_collision_cells': 900, 'replays': 2, 'bends': bends,
                    'trials': len(trials), 'frames': sum(len(t['keys']) for t in trials),
                    'route_end': trials[-1]['states'][-1]}
        (fixtures / 'traversal.json').write_text(json.dumps(trials) + '\n')
        (fixtures / 'measurement.json').write_text(json.dumps(metadata, indent=2) + '\n')
        # Compact text fixtures consumed by the portable C test, no JSON runtime.
        with (fixtures / 'traversal.txt').open('w') as f:
            f.write(str(len(trials)) + '\n')
            for t in trials:
                f.write(f"{t['spawn'][0]} {t['spawn'][1]} {len(t['keys'])}\n")
                for key, state in zip(t['keys'], t['states']):
                    f.write(' '.join(map(str, [key] + state)) + '\n')
        # TI scripts hold an input for two source steps. The original route
        # uses 60Hz changes; native route validation uses this resampled replay.
        keys = inputs[::2]
        keydir = GAME / 'keys'
        keydir.mkdir(exist_ok=True)
        with (keydir / 'opening.txt').open('w') as f:
            old = None
            for n, k in enumerate(keys):
                if k != old:
                    f.write(f'{n} {BUTTONS[k]}\n')
                    old = k
            f.write(f'{len(keys)}\n')
        print(f"Walking: {len(trials)} trials, {metadata['frames']} frames replayed twice; route {len(inputs)} frames to {sample(gba)[:2]}")
    finally:
        gba.close()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--measure', action='store_true')
    args = parser.parse_args()
    c, rows, rom = terrain()
    if args.measure:
        measure(c, rows, rom)
    print('Opening: offline 720x320 collision bank, host and TI endian variants')
