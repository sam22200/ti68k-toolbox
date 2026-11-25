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
import units        # noqa: E402

GREY = [216, 152, 88, 24]           # the project's software-capture palette
BG = 1


# OTHER.SPR is the one sheet with 512 colours, not 256: thirty-two palettes of
# sixteen, five for each of the chicken, the frog, the chest and the crystal. Its
# pixels therefore start at 0x400, not at the 0x200 every unit sheet uses. Read it
# at 0x200 and the whole sheet slides up four rows: the creatures lose their feet
# and gain two rows of palette bytes read as pixels.
SHEET_BASE = 0x400


def sheet(disc, files):
    """OTHER.SPR as a luminance image and its transparency mask."""
    raw = disc.read(*files['BATTLE/OTHER.SPR'])
    pal = np.array(struct.unpack_from('<256H', raw, 0)).reshape(16, 16)
    lum = ((pal & 31) * 8 * .299 + (pal >> 5 & 31) * 8 * .587 + (pal >> 10 & 31) * 8 * .114)
    a = np.frombuffer(raw[SHEET_BASE:], np.uint8)
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
    px = np.frombuffer(spr[SHEET_BASE:SHEET_BASE + 128 * 256], np.uint8).reshape(256, 128)
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


UNIT_W_PX, UNIT_SH, UNIT_FOOT = 16, 26, 21


def frame_bytes(img, a):
    """One creature frame as the engine's 208-byte record, feet on the ground row.

    Four planes of 26 big-endian u16 rows: light, dark, mask (set bit is
    transparent) and the inner mask the hit flash uses, which is the body without
    its white outline, exactly as a human frame carries it.
    """
    h, w = img.shape
    top = UNIT_FOOT + 1 - h
    inner = a.copy()
    inner[img == 0] = False                      # the halo is white: not body
    planes = [[0] * UNIT_SH for _ in range(4)]
    for r in range(UNIT_SH):
        planes[2][r] = 0xFFFF                    # transparent by default
    for y in range(h):
        r = top + y
        if not 0 <= r < UNIT_SH:
            continue
        for x in range(w):
            bx = (UNIT_W_PX - w) // 2 + x
            if not 0 <= bx < UNIT_W_PX or not a[y, x]:
                continue
            bit = 1 << (15 - bx)
            v = img[y, x]
            if v & 1:
                planes[0][r] |= bit
            if v & 2:
                planes[1][r] |= bit
            planes[2][r] &= ~bit                 # opaque here
            if inner[y, x]:
                planes[3][r] |= bit
    out = bytearray()
    for pl in planes:
        for v in pl:
            out += bytes(((v >> 8) & 0xFF, v & 0xFF))
    return bytes(out)


# One shape table serves every object on the sheet: the sheet-Y offset picks
# which. The sheet is a grid of slot by drawing mode. The slot is the object,
# y = 0 chicken, y = 48 frog, y = 96 crystal, and the chest at x = 224 takes the
# first two slots for its two sides (frames 15 and 16 name y = 0 and y = 48
# themselves). The mode is a further +24, and it is never named by OTHER.SHP, so
# the engine applies it: the +24 drawing is the same object with its vertical
# extent gone - the chest keeps its lid and its base outline and loses the eight
# rows of wall between them, the creatures lose the legs that reach down. That is
# the game's second, higher camera pitch, which the PS1 cannot produce by scaling
# because a sprite quad takes the camera's X and Z zoom but not its Y. The
# crystal, a floating diamond with no vertical face and no ground contact, is the
# one object with no +24 twin. Our camera has a single pitch, so we rip the +0
# drawings.
SHEET_Y = {'chicken': 0, 'frog': 48}
# Facing is NOT the strip: OTHER.SEQ pairs every sequence with a copy five frames
# later (1 2 3 against 6 7 8, over and over), and rendering 1-5 beside 6-10 shows
# the same creature turned. So frames 1-5 face the camera and 6-10 face away,
# exactly the convention units.py records for the human bodies, with the engine's
# mirror bit giving left from right.
FRONT, BACK = 1, 6
IDLE = 4                        # frames kept per facing


def header(out, spr, shapes, lum, idle=IDLE):
    """creature_art.h: the chicken and the frog, idle frames front then back.

    Frames FRONT.. face the camera and BACK.. face away, read off OTHER.SEQ:
    every sequence there has a twin five frames later, and the two render as the
    same creature turned.
    """
    text = ['// Generated by tools/other_actors.py from BATTLE/OTHER.SPR and OTHER.SHP:',
            '// the chicken and the frog a transformed unit wears. Never edit by hand and',
            '// never commit it: this is extracted artwork. Four planes of 26 big-endian',
            '// u16 rows per frame, feet on the ground row.',
            '#ifndef FFT_CREATURE_ART_H', '#define FFT_CREATURE_ART_H', '',
            '#define CREATURE_IDLE %d          // frames per facing' % idle,
            '#define CREATURE_BACK CREATURE_IDLE']
    for name, dy in SHEET_Y.items():
        recs = []
        for first in (FRONT, BACK):
            for i in range(first, first + idle):
                tiles = [(sx, sy, x, y + dy, w, h, fx, fy)
                         for sx, sy, x, y, w, h, fx, fy in shapes[i]]
                img, a = creature(spr, tiles, lum)
                if img is None:
                    raise SystemExit('%s frame %d at y%d is empty' % (name, i, dy))
                recs.append(frame_bytes(img, a))
        text.append('static const u8 %s_gfx[2 * CREATURE_IDLE * %d] = {'
                    % (name, 4 * UNIT_SH * 2))
        for n, r in enumerate(recs):
            text.append('    /* %s %d */' % ('front' if n < idle else 'back ', n % idle))
            for off in range(0, len(r), 16):
                text.append('    ' + ' '.join('0x%02X,' % b for b in r[off:off + 16]))
        text.append('};')
    text += ['', '#endif', '']
    Path(out).write_text('\n'.join(text))
    print('wrote', out, len(SHEET_Y), 'creatures,', 2 * idle, 'frames each')


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out', type=Path, default=HERE / 'campaign/x/status-picto/others.png')
    ap.add_argument('--scale', type=int, default=7)
    ap.add_argument('--header', type=Path,
                    help='write the engine header there instead of the preview')
    args = ap.parse_args()
    disc, files = extract.disc_files(str(core_audit.DEFAULT_DISC))
    if args.header:
        raw = disc.read(*files['BATTLE/OTHER.SPR'])
        pal = np.array(struct.unpack_from('<256H', raw, 0)).reshape(16, 16)
        plum = ((pal & 31) * 8 * .299 + (pal >> 5 & 31) * 8 * .587
                + (pal >> 10 & 31) * 8 * .114)[0]
        header(args.header, raw, units.shp_frames(disc.read(*files['BATTLE/OTHER.SHP'])), plum)
        return
    lum, opaque = sheet(disc, files)
    chick = frames(opaque, 6, 30)
    frog = frames(opaque, 54, 78)
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
