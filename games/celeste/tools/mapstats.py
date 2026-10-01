#!/usr/bin/env python3
"""Map and sprite counts for the Celeste port decisions. usage: mapstats.py XDIR"""
import sys
from collections import Counter
X = sys.argv[1]
m = open(X + '/map.bin', 'rb').read(); f = open(X + '/flags.bin', 'rb').read()
s = open(X + '/sprites.bin', 'rb').read()
cell = lambda rx, ry, tx, ty: m[(ry * 16 + ty) * 128 + rx * 16 + tx]
OBJ = {1: 'spawn', 8: 'key', 11: 'platform', 12: 'platform', 18: 'spring', 20: 'chest', 22: 'balloon',
       23: 'fall_floor', 26: 'fruit', 28: 'fly_fruit', 64: 'fake_wall', 86: 'message', 96: 'big_chest', 118: 'flag'}
SPIKE = {17, 27, 43, 59}
rooms = [(rx, ry) for ry in range(4) for rx in range(8)]
print('flag values used:', sorted(Counter(f).items()))
print('fg layer (flag 3, mask 8) tiles:', [i for i in range(256) if f[i] & 8])
def drawn(t, mask): return t if (f[t] & mask) else 0
for name, mask in [('bg (mask 4)', 4), ('terrain (mask 2)', 2), ('bg+terrain', 6)]:
    tiles, blocks = set(), set()
    for rx, ry in rooms[:31]:
        for ty in range(16):
            for tx in range(16):
                tiles.add(drawn(cell(rx, ry, tx, ty), mask))
        for by in range(8):
            for bx in range(8):
                b = tuple((drawn(cell(rx, ry, 2*bx+i, 2*by+j), 4), drawn(cell(rx, ry, 2*bx+i, 2*by+j), 2))
                          if mask == 6 else drawn(cell(rx, ry, 2*bx+i, 2*by+j), mask) for j in range(2) for i in range(2))
                blocks.add(b)
    print(f'{name}: {len(tiles)} distinct 8x8 tiles, {len(blocks)} distinct 2x2 blocks (16x16 metatiles) over rooms 0-30')
# a cell holding both a bg and a terrain tile is impossible (one tile per cell): bg+terrain = one layer
both = sum(1 for t in range(256) if f[t] & 4 and f[t] & 2)
print('tiles in both bg and terrain layers:', both)
print('\nper room: hazards / objects in the rows a 100-px window cannot show at once')
print('(top band = rows 0-3, y 0..31; bottom band = rows 12-15, y 96..127)')
for rx, ry in rooms[:31]:
    top = Counter(); bot = Counter()
    for ty in range(16):
        for tx in range(16):
            t = cell(rx, ry, tx, ty)
            k = 'spike' if t in SPIKE else OBJ.get(t)
            if not k: continue
            if ty <= 3: top[k] += 1
            if ty >= 12: bot[k] += 1
    print(f'room {ry*8+rx:2d} ({rx},{ry}): top {dict(top)}  bottom {dict(bot)}')
# colours
def spr_cols(n):
    x0, y0 = (n % 16) * 8, (n // 16) * 8
    return Counter(s[(y0 + y) * 128 + x0 + x] for y in range(8) for x in range(8))
groups = {'player 1-7': range(1, 8), 'hair (colour 8 in player)': [], 'objects 8-31': range(8, 32),
          'spikes': SPIKE, 'terrain/bg 32-127': range(32, 128), 'title/logo 128-255': range(128, 256)}
print('\ncolours per sprite group (PICO-8 index: pixels)')
for g, r in groups.items():
    c = Counter()
    for n in r: c += spr_cols(n)
    print(f'  {g}: {dict(sorted(c.items()))}')
for n in range(1, 8):
    print(f'  sprite {n}: {dict(sorted(spr_cols(n).items()))}')
