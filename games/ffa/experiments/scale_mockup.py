#!/usr/bin/env python3
"""Base-unit benchmark mock-up: room 8 (hero bedroom) with one original 9-px cell = N px, N in 8/16/32.
The original picture is upscaled (nearest) as a stand-in for the new tiles, the hero is drawn at
N x 1.5N; the 160x100 view is centred on the hero. Output: scale_<N>.png (x4) + scale_all.png."""
import sys
from PIL import Image
ROOT = '../../../ffa_en/pictures/'
G = {'.': None, 'w': (255,)*3, 'l': (170,)*3, 'd': (85,)*3, 'b': (0,)*3}
HERO16 = [                       # 16x24, facing down (placeholder hero for the mock-up)
    ".....bbbbbb.....",
    "....bddddddb....",
    "...bddddddddb...",
    "..bdddlddlddddb.",
    "..bddlllllldddb.",
    "..bdlwwllwwldb..",
    "..bdlbwllwbldb..",
    "...blllllllb....",
    "....bllllbbb....",
    "...bbbdddbbb....",
    "..bwwbddddbwwb..",
    ".bwwlbdwwdblwwb.",
    ".bwlbddwwddblwb.",
    ".blbbdddddddbblb",
    ".bllbddddddbllb.",
    "..bbbbbbbbbbbb..",
    "...bdddbbdddb...",
    "...bddb..bddb...",
    "...bddb..bddb...",
    "...bllb..bllb...",
    "...bddb..bddb...",
    "..bbbbb..bbbbb..",
    "..bdddb..bdddb..",
    "..bbbbb..bbbbb..",
]
def hero(n):
    g = [[G[c] for c in r] for r in HERO16]
    if n == 8:   # halve: keep the darker of each 2x2 (thin lines survive)
        g = [[min((g[2*y+j][2*x+i] for j in (0,1) for i in (0,1) if g[2*y+j][2*x+i]), default=None)
              for x in range(8)] for y in range(12)]
    if n == 32:
        g = [[g[y//2][x//2] for x in range(32)] for y in range(48)]
    im = Image.new('RGBA', (len(g[0]) + 2, len(g) + 2), (0, 0, 0, 0))
    for y, r in enumerate(g):            # white outline (mask dilated by one pixel)
        for x, c in enumerate(r):
            if c:
                for j in (0, 1, 2):
                    for i in (0, 1, 2):
                        if im.getpixel((x + i, y + j))[3] == 0:
                            im.putpixel((x + i, y + j), (255, 255, 255, 255))
    for y, r in enumerate(g):
        for x, c in enumerate(r):
            if c: im.putpixel((x + 1, y + 1), c + (255,))
    return im
def mock(n, cell=(3, 4)):
    bg = Image.open(ROOT + 'dec8.png').convert('L')
    bg = bg.point(lambda v: 170 if v == 0 else 255).convert('RGB')   # black lines -> light grey stand-in
    w, h = bg.width * n // 9, bg.height * n // 9
    world = bg.resize((w, h), Image.NEAREST)
    hx, hy = cell[0] * n, cell[1] * n
    hs = hero(n)
    world.paste(hs, (hx - 1, hy + n - hs.height + 1), hs)
    cx = min(max(hx + n // 2 - 80, 0), max(w - 160, 0)); cy = min(max(hy + n // 2 - 50, 0), max(h - 100, 0))
    view = Image.new('RGB', (160, 100), (0, 0, 0))
    view.paste(world.crop((cx, cy, min(cx + 160, w), min(cy + 100, h))), (0, 0))
    return view, (w, h)
if __name__ == '__main__':
    tiles = []
    for n in (8, 16, 32):
        v, size = mock(n)
        print('N=%2d world %dx%d = %.1f x %.1f screens, hero %dx%d = %.0f%% of the screen height, 16x16 tiles/room %d'
              % (n, size[0], size[1], size[0] / 160, size[1] / 100, n, n * 3 // 2, 150 * n / 100, (size[0] // 16 + 1) * (size[1] // 16 + 1)))
        v.resize((640, 400), Image.NEAREST).save('scale_%d.png' % n)
        tiles.append(v)
    sh = Image.new('RGB', (160 * 3 + 8, 100), (255, 0, 0))
    for k, v in enumerate(tiles): sh.paste(v, (k * 164, 0))
    sh.resize((sh.width * 3, 300), Image.NEAREST).save('scale_all.png')
