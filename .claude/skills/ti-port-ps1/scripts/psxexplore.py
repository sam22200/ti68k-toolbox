#!/usr/bin/env python3
"""Explore a room by real moves and find the way out: a breadth-first search from a save state
whose nodes are save states (in memory), keyed by the player's position rounded to a grid.
Every node tries 8 directions held for --step frames. A step whose position jumps further than
walking allows (a door, a ladder's end, a scene change) is an exit: its key script is printed
and its state saved. Teleporting (pokes) is not used: it leaves the game's collision lookups
stale, the player then cannot move.

usage: psxexplore.py DISC.cue STATE --pos X,Y[,Z] [--step 8] [--grid 8] [--jump 48]
                     [--max 3000] [--out exit] [--target X,Y] [--keys-out FILE]
--pos     addresses of the player's x, y (and z) as 32-bit 16.16 words (the master copies)
--jump    a step moving more than this many pixels is an exit (default 48)
--fade    a step after which the screen's mean brightness fell below this fraction of the
          first step's is an exit too (doors fade out before the player moves; default 0.5)
--settle  after each step, also run N frames without input before the tests: a door walks
          the player in and fades over ~30 frames, which a short step alone does not see
          (Alundra: --settle 40); the node itself keeps the state from before the settle
--target  stop at the first node within --grid of this position instead of an exit
--keys-out write the key script (psxrun.py --keys format) of the winning path
--out     prefix for the saved state (<out>.state) and a screenshot (<out>.png)
--jumps   also try every direction with CROSS (jumps)
--all     do not stop at the first exit: save every one (<out>_N.state/.png/.txt, one per
          destination cell) and keep searching (a house whose stairs come first)
--map     PNG of the positions reached (one pixel per grid cell, grey = z): what the search covered
Prints the number of nodes explored and the path.
"""
import argparse, ctypes as C, os, sys
from collections import deque

sys.path.insert(0, os.path.dirname(os.path.realpath(__file__)))
import psxrun  # noqa: E402

DIRS = [('UP',), ('DOWN',), ('LEFT',), ('RIGHT',), ('UP', 'LEFT'), ('UP', 'RIGHT'),
        ('DOWN', 'LEFT'), ('DOWN', 'RIGHT')]

ap = argparse.ArgumentParser()
ap.add_argument('disc'); ap.add_argument('state')
ap.add_argument('--pos', required=True)
ap.add_argument('--step', type=int, default=8)
ap.add_argument('--grid', type=int, default=8)
ap.add_argument('--jump', type=int, default=48)
ap.add_argument('--fade', type=float, default=0.5)
ap.add_argument('--settle', type=int, default=0)
ap.add_argument('--max', type=int, default=3000)
ap.add_argument('--target')
ap.add_argument('--out', default='exit')
ap.add_argument('--keys-out')
ap.add_argument('--all', action='store_true', help='save every exit, keep searching')
ap.add_argument('--jumps', action='store_true', help='also try each direction with CROSS')
ap.add_argument('--map', help='PNG of the visited positions (x right, y down, brighter = higher z)')
a = ap.parse_args()

out = os.fdopen(os.dup(1), 'w', buffering=1)
os.dup2(os.open(os.devnull, os.O_WRONLY), 1)
psx = psxrun.PSX(a.disc)
blob = open(a.state, 'rb').read()
psx.run()
pos_addr = [int(s, 16) for s in a.pos.split(',')]
moves = DIRS + ([d + ('CROSS',) for d in DIRS] if a.jumps else [])
size = psx.lib.retro_serialize_size()


def save():
    buf = C.create_string_buffer(size)
    psx.lib.retro_serialize(buf, size)
    return buf


def load(buf):
    psx.lib.retro_unserialize(buf, size)


def pos():
    return tuple(psx.read(ad, 4, True) / 65536 for ad in pos_addr)


def bright():
    import numpy as np
    data, w, h, pitch = psx.frame
    return float(np.frombuffer(data, np.uint8).mean())


def key(p):
    return tuple(int(v // a.grid) for v in p[:2]) + ((int(p[2] // 8),) if len(p) > 2 else ())


load(C.create_string_buffer(blob, len(blob)))
for _ in range(2):
    psx.run()
load(C.create_string_buffer(blob, len(blob)))
p0 = pos()
b0 = None
start = save()
seen = {key(p0)}
zs = {}
q = deque([(start, p0, [])])
target = tuple(int(v, 16) for v in a.target.split(',')) if a.target else None
n = 0
found = None
exits = set()


def script_of(path):
    lines, f = [], 0
    for mv in path:
        lines.append('%d %s' % (f, ' '.join(mv)))
        f += a.step
    return '\n'.join(lines + ['%d' % f]) + '\n'


while q and n < a.max:
    st, p, path = q.popleft()
    for mv in moves:
        load(st)
        psx.pressed = sum(1 << psxrun.PAD[k] for k in mv)
        for _ in range(a.step):
            psx.run()
        psx.pressed = 0
        np_ = pos()
        n += 1
        if a.settle:
            after_step = save()
            for _ in range(a.settle):
                psx.run()
            sp = pos()
            if max(abs(sp[0] - np_[0]), abs(sp[1] - np_[1])) > a.jump:
                np_ = sp
        b = bright()
        b0 = b0 or b
        fade = b < b0 * a.fade
        dist = max(abs(np_[0] - p[0]), abs(np_[1] - p[1]))
        newpath = path + [mv]
        if dist > a.jump or fade or (target and abs(np_[0] - target[0]) <= a.grid and abs(np_[1] - target[1]) <= a.grid):
            if a.all:
                if key(np_) not in exits:
                    exits.add(key(np_))
                    for _ in range(60):
                        psx.run()
                    stem = '%s_%d' % (a.out, len(exits))
                    psx.save(stem + '.state'); psx.image().save(stem + '.png')
                    open(stem + '.txt', 'w').write(script_of(newpath))
                    print('exit %d: (%.0f, %.0f) -> (%.0f, %.0f)%s' % (len(exits), p[0], p[1], np_[0], np_[1],
                          ' fade' if fade else ''), file=out)
                continue
            found = (newpath, p, np_)
            break
        k = key(np_)
        if k not in seen:
            seen.add(k)
            zs[k] = np_[2] if len(np_) > 2 else 0
            q.append((after_step if a.settle else save(), np_, newpath))
    if found:
        break
print('explored %d moves, %d positions' % (n, len(seen)), file=out)
if a.map and zs:
    from PIL import Image
    xs = [k[0] for k in zs]; ys = [k[1] for k in zs]
    img = Image.new('RGB', (max(xs) - min(xs) + 1, max(ys) - min(ys) + 1), (60, 0, 0))
    zmax = max(zs.values()) or 1
    for k, z in zs.items():
        g = int(40 + 215 * z / zmax)
        img.putpixel((k[0] - min(xs), k[1] - min(ys)), (g, g, g))
    img = img.resize((img.width * 8, img.height * 8), Image.NEAREST)
    img.save(a.map)
    import json
    json.dump([[k[0] * a.grid, k[1] * a.grid, z] for k, z in zs.items()], open(a.map + '.json', 'w'))
    print('map: x %x..%x, y %x..%x (grid %d)' % (min(xs) * a.grid, max(xs) * a.grid, min(ys) * a.grid,
                                                max(ys) * a.grid, a.grid), file=out)
if a.all:
    sys.exit(0 if exits else 1)
if not found:
    print('no exit found', file=out)
    sys.exit(1)
path, before, after = found
print('exit after %d steps: (%.0f, %.0f) -> (%.0f, %.0f)' % (len(path), before[0], before[1], after[0], after[1]), file=out)
for _ in range(60):        # let the transition run, then keep the state and a picture
    psx.run()
psx.save(a.out + '.state')
psx.image().save(a.out + '.png')
lines, f = [], 0
for mv in path:
    lines.append('%d %s' % (f, ' '.join(mv)))
    f += a.step
lines.append('%d' % f)
script = '\n'.join(lines) + '\n'
if a.keys_out:
    open(a.keys_out, 'w').write(script)
print(script, file=out, end='')
