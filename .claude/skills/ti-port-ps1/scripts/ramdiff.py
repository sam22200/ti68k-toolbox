#!/usr/bin/env python3
"""Compare main-RAM dumps (psxrun.py --dump) to find a game variable by experiment.

usage: ramdiff.py A.bin B.bin [C.bin ...] [--same X.bin ...] [--range LO-HI] [--max N]
Prints the addresses (KSEG0, 80xxxxxx) whose byte differs between every consecutive pair of the
positional dumps (A != B, B != C, ...) and is equal in every --same dump compared with A: e.g.
A = standing, B = after holding RIGHT 10 frames, C = 10 more frames, --same = standing 20
frames (rules out timers and animation counters). Values are printed per dump, so a coordinate
that grows A < B < C shows at once; 16-bit words show as two neighbouring bytes.
"""
import argparse

ap = argparse.ArgumentParser()
ap.add_argument('dumps', nargs='+')
ap.add_argument('--same', nargs='*', default=[])
ap.add_argument('--range', default='0-1fffff')
ap.add_argument('--max', type=int, default=200)
a = ap.parse_args()
d = [open(f, 'rb').read() for f in a.dumps]
same = [open(f, 'rb').read() for f in a.same]
lo, hi = (int(x, 16) & 0x1fffff for x in a.range.split('-'))
n = 0
for i in range(lo, min(hi + 1, len(d[0]))):
    if all(d[k][i] != d[k + 1][i] for k in range(len(d) - 1)) and all(s[i] == d[0][i] for s in same):
        print('%08x  %s' % (0x80000000 + i, ' '.join('%02x' % x[i] for x in d)))
        n += 1
        if n >= a.max:
            print('... (--max)')
            break
