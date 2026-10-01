#!/usr/bin/env python3
"""Seeded random key scripts (runtime format) for the trace tests: they reach states nobody
scripts by hand. usage: fuzz.py N FRAMES OUTDIR  -> OUTDIR/fuzz_00.txt .. fuzz_N-1.txt"""
import os, random, sys
n, frames, out = int(sys.argv[1]), int(sys.argv[2]), sys.argv[3]
for k in range(n):
    rnd = random.Random(1000 + k)
    lines, f = ['# fuzz.py seed %d' % (1000 + k)], 0
    while f < frames:
        keys = []
        h = rnd.random()
        if h < 0.45: keys.append('RIGHT')
        elif h < 0.75: keys.append('LEFT')
        v = rnd.random()
        if v < 0.2: keys.append('UP')
        elif v < 0.3: keys.append('DOWN')
        if rnd.random() < 0.35: keys.append('A')
        if rnd.random() < 0.15: keys.append('B')
        lines.append('%d %s' % (f, ' '.join(keys)))
        f += rnd.choice([1, 2, 3, 4, 6, 8, 12, 20, 30])
    open(os.path.join(out, 'fuzz_%02d.txt' % k), 'w').write('\n'.join(lines) + '\n')
