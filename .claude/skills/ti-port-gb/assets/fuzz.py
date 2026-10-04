#!/usr/bin/env python3
"""Seeded random key scripts (logic frames) for the trace tests: fuzz.py SEED FRAMES > keys/fuzz_NN.txt
Segments of 4..60 frames, each a random direction (or none) with A held 60 % of the time:
they wander, blow, pop and exit in ways nobody scripts by hand."""
import random, sys
seed, frames = int(sys.argv[1]), int(sys.argv[2])
r = random.Random(seed)
dirs = ['', 'UP', 'DOWN', 'LEFT', 'RIGHT', 'UP LEFT', 'UP RIGHT', 'DOWN LEFT', 'DOWN RIGHT']
print('# fuzz seed %d, %d frames (tools/fuzz.py)' % (seed, frames))
f = 0
while f < frames:
    keys = (r.choice(dirs) + (' A' if r.random() < 0.6 else '')).strip()
    print(('%d %s' % (f, keys)).strip())
    f += r.randint(4, 60)
