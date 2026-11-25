#!/usr/bin/env python3
"""The chicken and the frog from the original, at three sizes, for a decision.

BATTLE/OTHER.SPR holds the sprites a transformed unit wears: the chicken, the
frog, the crystal a dead one leaves and the treasure chest. Only the first two
are wanted here.

The question the proposals answer is not the drawing but the scale. The cast is
drawn at 0.6, which takes a 16-pixel original down to about ten, and a ten-pixel
bird on a 160x100 screen is a smudge. So:

  A  0.6, the cast's own scale, faithful
  B  1.0, the original's size, readable but larger than the humans it replaces
  C  0.6 with the interior flattened to two values, so the silhouette carries it

Every variant keeps the project's white outline, the mask dilated by one pixel.

usage: other_actors.py [--out x/status-picto/others.png]
"""
import argparse
import struct
import sys
from pathlib import Path

import numpy as np
from PIL import Image

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE / 'tools'))
import core_audit   # noqa: E402
import extract      # noqa: E402

GREY = [216, 152, 88, 24]           # the project's software-capture palette
BG = 1


def sheet(disc, files):
    """OTHER.SPR as a luminance image and its transparency mask."""
    raw = disc.read(*files['BATTLE/OTHER.SPR'])
    pal = np.array(struct.unpack_from('<256H', raw, 0)).reshape(16, 16)
    lum = ((pal & 31) * 8 * .299 + (pal >> 5 & 31) * 8 * .587 + (pal >> 10 & 31) * 8 * .114)
    a = np.frombuffer(raw[512:], np.uint8)
    px = np.empty(len(a) * 2, np.uint8)
    px[0::2] = a & 15
    px[1::2] = a >> 4
    w = 256
    idx = px[:len(px) // w * w].reshape(-1, w)
    return lum[0][idx], idx != 0          # index 0 is transparent


def assemble(spr, tiles):
    """One OTHER frame from its SHP tile list: palette indices, -1 transparent.

    units.sprite() cannot be reused here: it is specialised for human bodies and
    looks for hair, a face, eyes and skin, which a chicken does not have. Only the
    tile compositing is shared, and the treatment below is the generic one.
    """
    px = np.frombuffer(spr[512:512 + 128 * 256], np.uint8).reshape(256, 128)
    sheet = np.empty((256, 256), np.uint8)
    sheet[:, 0::2] = px & 15
    sheet[:, 1::2] = px >> 4
    cv = np.zeros((128, 128), np.uint8)
    ox, oy = 64, 96
    for sx, sy, x, y, w, h, fx, fy in reversed(tiles):
        t = sheet[y:y + h, x:x + w]
        if fx:
            t = t[:, ::-1]
        if fy:
            t = t[::-1]
        r = cv[oy + sy:oy + sy + h, ox + sx:ox + sx + w]
        r[t > 0] = t[t > 0]
    return cv


def creature(spr, tiles, lum):
    """An OTHER frame at the TI's four greys with the project's white outline."""
    cv = assemble(spr, tiles)
    m = cv > 0
    if not m.any():
        return None, None
    ys, xs = np.nonzero(m)
    cv = cv[ys.min():ys.max() + 1, xs.min():xs.max() + 1]
    m = cv > 0
    g = quantise(np.where(m, lum[np.clip(cv, 0, 15)], 0.0), m)
    return outline(np.where(m, g, 0).astype(np.uint8), m)


def frames(opaque, y0, y1, least=6):
    """Bounding boxes of the sprites on one row band, split on empty columns."""
    band = opaque[y0:y1]
    cols = band.any(0)
    out, x = [], 0
    while x < len(cols):
        if not cols[x]:
            x += 1
            continue
        x1 = x
        while x1 < len(cols) and cols[x1]:
            x1 += 1
        rows = np.nonzero(band[:, x:x1].any(1))[0]
        if len(rows) and x1 - x >= least:
            out.append((x, x1, y0 + int(rows[0]), y0 + int(rows[-1]) + 1))
        x = x1
    return out


def quantise(lum, opaque, levels=4):
    """Luminance to grey indices over the drawn pixels only, darkest = 3."""
    vals = lum[opaque]
    if not len(vals):
        return np.zeros(lum.shape, np.uint8)
    q = np.percentile(vals, np.linspace(100 / levels, 100 - 100 / levels, levels - 1))
    g = (levels - 1 - np.searchsorted(q, lum, side='left')).astype(np.uint8)
    return g


def outline(img, opaque):
    """The project's white halo: the mask dilated by one pixel, drawn white."""
    h, w = opaque.shape
    out = np.zeros((h + 2, w + 2), np.uint8)
    a = np.zeros((h + 2, w + 2), bool)
    big = np.zeros((h + 2, w + 2), bool)
    big[1:-1, 1:-1] = opaque
    halo = big.copy()
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            halo |= np.roll(np.roll(big, dy, 0), dx, 1)
    out[1:-1, 1:-1] = img
    out[halo & ~big] = 0                   # white
    a[halo] = True
    return out, a


def scaled(lum, opaque, box, factor, flat=False):
    x0, x1, y0, y1 = box
    l = lum[y0:y1, x0:x1]
    o = opaque[y0:y1, x0:x1]
    if factor != 1.0:
        h, w = o.shape
        nh, nw = max(1, int(round(h * factor))), max(1, int(round(w * factor)))
        li = Image.fromarray(l.astype(np.uint8)).resize((nw, nh), Image.BILINEAR)
        oi = Image.fromarray((o * 255).astype(np.uint8)).resize((nw, nh), Image.BILINEAR)
        l = np.asarray(li, float)
        o = np.asarray(oi) > 110
    g = quantise(l, o, 3 if flat else 4)
    if flat:
        g = np.where(g >= 2, 3, 1).astype(np.uint8)    # two interior values only
    g = np.where(o, g, 0).astype(np.uint8)
    return outline(g, o)


def blit(dst, src, a, x0, y0):
    for y in range(src.shape[0]):
        for x in range(src.shape[1]):
            if a[y, x] and 0 <= y0 + y < dst.shape[0] and 0 <= x0 + x < dst.shape[1]:
                dst[y0 + y, x0 + x] = src[y, x]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out', type=Path, default=HERE / 'campaign/x/status-picto/others.png')
    ap.add_argument('--scale', type=int, default=7)
    args = ap.parse_args()
    disc, files = extract.disc_files(str(core_audit.DEFAULT_DISC))
    lum, opaque = sheet(disc, files)
    chick = frames(opaque, 10, 34)
    frog = frames(opaque, 58, 84)
    print('chicken frames', len(chick), 'frog frames', len(frog))
    if len(chick) < 4 or len(frog) < 4:
        raise SystemExit('frame bands did not split; adjust the row ranges')
    picks = [('poulet', chick[0]), ('grenouille', frog[0])]
    variants = [('A', .6, False), ('B', 1.0, False), ('C', .6, True)]

    # A human unit beside them: the size question cannot be judged without it.
    import status_glyphs as sg
    body, body_a = sg.unit_frame(HERE / 'fftu.bin')

    cell, W0 = 26, 24
    W = W0 + cell * len(variants) * len(picks)
    H = cell + 12
    canvas = np.full((H, W), BG, np.uint8)
    feet = H - 8
    blit(canvas, body, body_a, 4, feet - sg.UNIT_SH)
    for pi, (name, box) in enumerate(picks):
        for vi, (label, f, flat) in enumerate(variants):
            img, a = scaled(lum, opaque, box, f, flat)
            cx = W0 + (pi * len(variants) + vi) * cell
            blit(canvas, img, a, cx + (cell - img.shape[1]) // 2, feet - img.shape[0])
            sg.text(canvas, label.lower(), cx + cell // 2 - 2, H - 6, 3)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    rgb = np.array(GREY, np.uint8)[canvas]
    Image.fromarray(rgb).resize((W * args.scale, H * args.scale), Image.NEAREST).save(args.out)
    print('wrote', args.out, 'human, then poulet A/B/C and grenouille A/B/C')


if __name__ == '__main__':
    main()
