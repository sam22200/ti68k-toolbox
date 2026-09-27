#!/usr/bin/env python3
"""Debug view of a room: original picture on the logic grid (x = a/9 + 1) with cell codes.
Usage: tools/roomsheet.py out.png ROOM [ROOM...]   (red = wall, green = door, blue = trigger)"""
import os, sys
from PIL import Image, ImageDraw
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from murparse import Room
PICS = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'ffa_en', 'pictures')
Z = 6
tiles = []
for u in map(int, sys.argv[2:]):
    r = Room(u)
    src = Image.open(os.path.join(PICS, 'dec%d.png' % u)).convert('RGB')
    c = Image.new('RGB', ((r.w) * 9 * Z, (r.h + 1) * 9 * Z), (60, 60, 90))
    c.paste(src.resize((src.width * Z, src.height * Z), Image.NEAREST), (9 * Z, 0))
    d = ImageDraw.Draw(c)
    for y in range(r.h):
        for x in range(r.w):
            v = r.grid[y][x]; X, Y = x * 9 * Z, y * 9 * Z
            d.rectangle([X, Y, X + 9 * Z - 1, Y + 9 * Z - 1], outline=(200, 200, 255))
            if v == 0: col = (255, 0, 0); s = ''
            elif 2 < v < 500: col = (0, 170, 0); s = 'D%d' % v
            elif v >= 500 or v <= -2: col = (0, 0, 255); s = '%g' % float(v)
            else: continue
            if v == 0: d.line([X + 20, Y + 20, X + 34, Y + 34], fill=col, width=2)
            else: d.text((X + 3, Y + 18), s, fill=col)
            d.text((X + 2, Y + 2), '%d,%d' % (x, y), fill=(150, 150, 150))
    d.text((4, r.h * 9 * Z + 10), 'room %d' % u, fill=(255, 255, 0))
    tiles.append(c)
W = max(t.width for t in tiles); H = sum(t.height for t in tiles)
sh = Image.new('RGB', (W, H)); y = 0
for t in tiles: sh.paste(t, (0, y)); y += t.height
sh.save(sys.argv[1])
