#!/usr/bin/env python3
"""Convert a ccleste TAS ("b0,b1,...": one PICO-8 button bitfield per frame from the first
frame of 100 m) into a runtime key script (lines "<frame> <keys>", held until the next line).
usage: tas2keys.py TAS FRAMES > keys/x.txt   (bits: 0 left 1 right 2 up 3 down 4 O=A 5 X=B)"""
import sys
NAMES = ['LEFT', 'RIGHT', 'UP', 'DOWN', 'A', 'B']
vals = [int(v) for v in open(sys.argv[1]).read().replace('\n', '').split(',') if v.strip()]
n = int(sys.argv[2])
print('# from a ccleste TAS (test-tas.txt), first %d frames' % n)
prev = None
for f, b in enumerate(vals[:n]):
    if b != prev:
        print(f, ' '.join(NAMES[i] for i in range(6) if b >> i & 1))
        prev = b
