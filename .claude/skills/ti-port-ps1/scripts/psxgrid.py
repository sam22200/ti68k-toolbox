#!/usr/bin/env python3
"""Measure a rule on a grid: from a save state, place the player at each point of a grid (pokes),
run a few frames, read some variables. Prints one ASCII map per variable.

usage: psxgrid.py DISC.cue STATE --set X:4,Y:4 --read ADDR[:size] [--read ...]
                  --x LO:HI:STEP --y LO:HI:STEP [--frames N] [--fix 16] [--also ADDR:4=X,...]
                  [--settle N --hold KEYS]
--set   the two addresses poked with the grid's x and y (with --fix 16: value << 16, 16.16)
--also  more addresses that receive the same x or y (copies the game keeps: ADDR:4=X), or a
        constant (ADDR:4=#100: the player's z set high, so it falls onto the floor at each point)
--read  variables read after --frames frames; printed as maps (hex, one column per x)
--settle / --hold   run N frames after the pokes, then hold KEYS (LEFT, LEFT+CROSS...) during
        --frames: the reads then show where a move from that point ends (walls, ledges)
The state is reloaded for every point (deterministic). Points the game refuses (it moves the
player back) show as the variable's value where it ended: also print --set's values with
--read to see them.
"""
import argparse, os, sys
sys.path.insert(0, os.path.dirname(os.path.realpath(__file__)))
import psxrun  # noqa: E402

ap = argparse.ArgumentParser()
ap.add_argument('disc'); ap.add_argument('state')
ap.add_argument('--set', required=True); ap.add_argument('--also', default='')
ap.add_argument('--read', action='append', required=True)
ap.add_argument('--x', required=True); ap.add_argument('--y', required=True)
ap.add_argument('--frames', type=int, default=2)
ap.add_argument('--settle', type=int, default=0); ap.add_argument('--hold', default=''); ap.add_argument('--fix', type=int, default=16)
a = ap.parse_args()
out = os.fdopen(os.dup(1), 'w', buffering=1)
os.dup2(os.open(os.devnull, os.O_WRONLY), 1)
psx = psxrun.PSX(a.disc)
blob = open(a.state, 'rb').read()
import ctypes as C
psx.run()
def setsize(s):
    ad, sz = (s.split(':') + ['4'])[:2]
    return int(ad, 16), int(sz)
(xa, xs), (ya, ys) = (setsize(s) for s in a.set.split(','))
also = []
for s in filter(None, a.also.split(',')):
    lhs, which = s.split('=')
    also.append((*setsize(lhs), which.upper()))
reads = [setsize(r) for r in a.read]
rng = lambda s: range(*(int(v, 0) for v in s.split(':')))
xs_, ys_ = list(rng(a.x)), list(rng(a.y))
res = {}
for y in ys_:
    for x in xs_:
        psx.lib.retro_unserialize(C.create_string_buffer(blob, len(blob)), len(blob))
        val = lambda w: x if w == 'X' else y if w == 'Y' else int(w[1:], 16)
        for ad, sz, v in [(xa, xs, x), (ya, ys, y)] + [(ad, sz, val(w)) for ad, sz, w in also]:
            psx.write(ad, v << a.fix if sz == 4 else v, sz)
        psx.pressed = 0
        for _ in range(a.settle):
            psx.run()
        psx.pressed = sum(1 << psxrun.PAD[k] for k in filter(None, a.hold.upper().split('+')))
        for _ in range(a.frames):
            psx.run()
        res[x, y] = [psx.read(ad, sz) for ad, sz in reads]
for i, (ad, sz) in enumerate(reads):
    print('# %08x after %d frames; columns x = %s' % (ad, a.frames, a.x), file=out)
    print('      ' + ' '.join('%4x' % x for x in xs_), file=out)
    for y in ys_:
        print('%5x ' % y + ' '.join('%4x' % (res[x, y][i] & 0xffff) for x in xs_), file=out)
