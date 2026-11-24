#!/usr/bin/env python3
"""Three status-pictogram proposals shown above a real unit, ally and enemy.

Writes an animated GIF cycling negative / positive / charging so the three sets can
be compared at a glance. The 0.3 s pause is a review aid only; in game the pictures
alternate once a second.

The original's own vocabulary, read from EVENT/FRAME.BIN: statuses are speech
bubbles carrying a symbol (". . ." Silence, a heart for Charm, glasses for Dark,
"DW"/"DA" for Don't Move/Don't Act), charging is an animated five-point star, and
the death counter is a numbered bubble -- which this game already draws. Those
icons are about 14x11; a picto here has to read above a 16-pixel unit in four
greys, so each proposal trades fidelity against legibility differently.

usage: status_picto_preview.py [--out x/status-picto/proposals.gif] [--scale 6]
"""
import argparse
from pathlib import Path

import numpy as np
from PIL import Image

HERE = Path(__file__).resolve().parents[1]
UNIT_SH, UNIT_W = 26, 16
# (light, dark) -> grey index, as the engine packs its two planes
SHADE = {(0, 0): 0, (1, 0): 1, (0, 1): 2, (1, 1): 3}
GREY = [216, 152, 88, 24]              # the project's software-capture palette
BG = 1                                 # mid grey scenery behind the unit

# Body value, outline value. 'w' white, 'g' light grey, 'd' dark grey, 'k' black.
VALUE = {'w': (0, 0), 'g': (1, 0), 'd': (0, 1), 'k': (1, 1)}

PROPOSALS = {
    'A bulle FFT': {
        # The original's language: a light bubble with a dark symbol and a tail.
        'neg': ('wk', (' ####### ',
                       '#########',
                       '##ooooo##',
                       '###ooo###',
                       '####o####',
                       '#########',
                       '  ###    ')),
        'pos': ('wk', (' ####### ',
                       '#########',
                       '####o####',
                       '###ooo###',
                       '##ooooo##',
                       '#########',
                       '  ###    ')),
        # charging keeps the original's bare star, no bubble
        'chg': ('kw', ('..#..',
                       '#####',
                       '.###.',
                       '.###.',
                       '#...#')),
    },
    'B forme pleine': {
        # Polarity carried by the fill, the strongest signal at this size.
        'neg': ('kw', ('#####', '#####', ' ### ', ' ### ', '  #  ')),
        'pos': ('wk', ('  #  ', ' ### ', ' ### ', '#####', '#####')),
        # dark grey, not light: a light grey disc vanishes on mid-grey scenery
        'chg': ('dw', (' ### ', '#####', '#####', '#####', ' ### ')),
    },
    'C signe': {
        'neg': ('kw', ('#####', '#####', 'ooooo', '#####', '#####')),
        'pos': ('wk', ('##o##', '##o##', 'ooooo', '##o##', '##o##')),
        'chg': ('kw', ('..#..', '#####', '.###.', '.###.', '#...#')),
    },
}
ORDER = ['neg', 'pos', 'chg']
LABEL = {'neg': 'negatif', 'pos': 'positif', 'chg': 'charge'}


def dilate(g):
    h, w = len(g), len(g[0])
    out = [[False] * w for _ in range(h)]
    for y in range(h):
        for x in range(w):
            if g[y][x]:
                for dy in (-1, 0, 1):
                    for dx in (-1, 0, 1):
                        if 0 <= y + dy < h and 0 <= x + dx < w:
                            out[y + dy][x + dx] = True
    return out


def picto(spec):
    """(grey image, alpha) of one pictogram: body, its outline, transparent around."""
    (body, outline), shape = spec
    # '#' body, 'o' a symbol inside it in the outline's value, ' ' transparent
    g = [[c in '#o' for c in row] for row in shape]
    o = [[c == 'o' for c in row] for row in shape]
    h, w = len(g), len(g[0])
    pad = [[False] * (w + 2) for _ in range(h + 2)]
    sym = [[False] * (w + 2) for _ in range(h + 2)]
    for y in range(h):
        for x in range(w):
            pad[y + 1][x + 1] = g[y][x]
            sym[y + 1][x + 1] = o[y][x]
    halo = dilate(pad)
    img = np.zeros((h + 2, w + 2), np.uint8)
    a = np.zeros((h + 2, w + 2), bool)
    for y in range(h + 2):
        for x in range(w + 2):
            if sym[y][x]:
                img[y, x] = SHADE[VALUE[outline]]; a[y, x] = True
            elif pad[y][x]:
                img[y, x] = SHADE[VALUE[body]]; a[y, x] = True
            elif halo[y][x]:
                img[y, x] = SHADE[VALUE[outline]]; a[y, x] = True
    return img, a


def diamond():
    """The enemy marker exactly as art.h foe_gfx draws it: black, white outline."""
    shape = ('  #  ', ' ### ', '#####', ' ### ', '  #  ')
    return picto((('k', 'w'), shape))


def unit_frame(path, index=0):
    raw = np.frombuffer(Path(path).read_bytes(), '>u2')
    f = raw[index * 4 * UNIT_SH:(index + 1) * 4 * UNIT_SH]
    light, dark, mask = f[:UNIT_SH], f[UNIT_SH:2 * UNIT_SH], f[2 * UNIT_SH:3 * UNIT_SH]
    img = np.zeros((UNIT_SH, UNIT_W), np.uint8)
    a = np.zeros((UNIT_SH, UNIT_W), bool)
    for r in range(UNIT_SH):
        for x in range(UNIT_W):
            bit = 1 << (15 - x)
            if mask[r] & bit:                      # mask bit set: transparent
                continue
            img[r, x] = SHADE[(1 if light[r] & bit else 0, 1 if dark[r] & bit else 0)]
            a[r, x] = True
    return img, a


def blit(dst, src, alpha, x0, y0):
    h, w = src.shape
    for y in range(h):
        for x in range(w):
            if alpha[y, x] and 0 <= y0 + y < dst.shape[0] and 0 <= x0 + x < dst.shape[1]:
                dst[y0 + y, x0 + x] = src[y, x]


def glyphs():
    """A 3x5 pixel alphabet, enough for the labels under each column."""
    f = {
        'a': ('010', '101', '111', '101', '101'), 'b': ('110', '101', '110', '101', '110'),
        'c': ('011', '100', '100', '100', '011'), 'e': ('111', '100', '110', '100', '111'),
        'f': ('111', '100', '110', '100', '100'), 'g': ('011', '100', '101', '101', '011'),
        'h': ('101', '101', '111', '101', '101'), 'i': ('111', '010', '010', '010', '111'),
        'l': ('100', '100', '100', '100', '111'), 'n': ('101', '111', '111', '101', '101'),
        'o': ('111', '101', '101', '101', '111'), 'p': ('110', '101', '110', '100', '100'),
        'r': ('110', '101', '110', '101', '101'), 's': ('011', '100', '010', '001', '110'),
        't': ('111', '010', '010', '010', '010'), 'u': ('101', '101', '101', '101', '111'),
        'v': ('101', '101', '101', '101', '010'), 'z': ('111', '001', '010', '100', '111'),
        ' ': ('000', '000', '000', '000', '000'), 'A': ('010', '101', '111', '101', '101'),
        'B': ('110', '101', '110', '101', '110'), 'C': ('011', '100', '100', '100', '011'),
    }
    return f


def text(dst, s, x0, y0, shade=3):
    f = glyphs()
    for ch in s:
        g = f.get(ch, f[' '])
        for y in range(5):
            for x in range(3):
                if g[y][x] == '1' and 0 <= y0 + y < dst.shape[0] and 0 <= x0 + x < dst.shape[1]:
                    dst[y0 + y, x0 + x] = shade
        x0 += 4
    return x0


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--units', type=Path, default=HERE / 'fftu.bin')
    ap.add_argument('--out', type=Path, default=HERE / 'campaign/x/status-picto/proposals.gif')
    ap.add_argument('--scale', type=int, default=6)
    ap.add_argument('--frame', type=int, default=0)
    args = ap.parse_args()

    body, body_a = unit_frame(args.units, args.frame)
    dia, dia_a = diamond()

    cell_w, cell_h = 56, 52
    W = cell_w * len(PROPOSALS)
    H = cell_h + 8
    frames = []
    for kind in ORDER:
        canvas = np.full((H, W), BG, np.uint8)
        for col, (name, sets) in enumerate(PROPOSALS.items()):
            pic, pic_a = picto(sets[kind])
            ox = col * cell_w
            # the unit's feet sit at the same row in both halves
            feet = cell_h - 10
            for half, enemy in ((0, False), (1, True)):
                ux = ox + 6 + half * 26
                blit(canvas, body, body_a, ux, feet - UNIT_SH)
                top = feet - UNIT_SH - 2          # just above the head
                if enemy:
                    # diamond shifted left, pictogram right, their edges touching
                    dw, pw = dia.shape[1], pic.shape[1]
                    total = dw + pw - 2           # the two outlines share a column
                    bx = ux + UNIT_W // 2 - total // 2
                    blit(canvas, dia, dia_a, bx, top - dia.shape[0])
                    blit(canvas, pic, pic_a, bx + dw - 1, top - pic.shape[0])
                else:
                    blit(canvas, pic, pic_a, ux + UNIT_W // 2 - pic.shape[1] // 2,
                         top - pic.shape[0])
            text(canvas, name.split()[0].lower() + ' ' + name.split()[1][:6], ox + 3, H - 7, 3)
        text(canvas, LABEL[kind], W - 4 * len(LABEL[kind]) - 2, 2, 3)
        rgb = np.array(GREY, np.uint8)[canvas]
        frames.append(Image.fromarray(rgb).resize((W * args.scale, H * args.scale), Image.NEAREST))
    args.out.parent.mkdir(parents=True, exist_ok=True)
    frames[0].save(args.out, save_all=True, append_images=frames[1:], duration=300, loop=0)
    print('wrote', args.out, '%dx%d x%d, %d frames' % (W, H, args.scale, len(frames)))


if __name__ == '__main__':
    main()
