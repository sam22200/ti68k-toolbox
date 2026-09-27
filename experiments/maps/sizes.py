#!/usr/bin/env python3
"""Sizes of the 256x16 level grid (level.bin, written by the host build of levels.c) as a raw
grid, column-major PackBits RLE, LZ4 and ZX0 (encoders from ../compress/tools/pack.py), against
its object list (the size printed by levels).
Run from experiments/maps: gcc -O2 -o levels levels.c && ./levels && ../../tools/pyenv/bin/python sizes.py
"""
import os
import sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'compress', 'tools'))
import pack  # noqa: E402

g = open(os.path.join(HERE, 'level.bin'), 'rb').read()
W, H = 256, 16
cols = bytes(g[y * W + x] for x in range(W) for y in range(H))
print('grid raw        %5d' % len(g))
print('RLE rows        %5d' % len(pack.rle_pack(g)))
print('RLE columns     %5d' % len(pack.rle_pack(cols)))
print('LZ4 rows        %5d' % len(pack.lz4_pack(g)))
print('LZ4 columns     %5d' % len(pack.lz4_pack(cols)))
zx0 = dict(pack.FORMATS)['zx0']
print('ZX0 rows        %5d' % len(zx0(g)))
print('ZX0 columns     %5d' % len(zx0(cols)))
