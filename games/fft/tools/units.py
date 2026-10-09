#!/usr/bin/env python3
"""The units' sprites from the local Final Fantasy Tactics disc -> units.h, fftu.bin (never committed).

usage: units.py DISC.cue units.h [preview.png]   (also writes fftu.bin next to units.h)
FFT's battle sprites (format: FFTPatcher's ShishiSpriteEditor and TacticsTemplateG, checked on
this disc, RE_NOTES.md):
- BATTLE/<name>.SPR: 16 palettes of 16 BGR555 colours (colour 0 transparent), then the sheet,
  256 pixels wide, 4 bits (low nibble = left pixel); the rows used here are raw.
- BATTLE/TYPE1.SHP: frames made of up to 8 tiles of the sheet. u32 pointers at 8 + 4 i
  (relative to 0x40A); a frame is byte0 (bits 0-2: tiles - 1), byte1, then per tile s8 x, s8 y
  (from the unit's anchor, near the feet), u16: sheet x / 8 (bits 0-4), y / 8 (5-9), size
  index (10-13), flip x (14), flip y (15). Drawn last to first.
- BATTLE/TYPE1.SEQ: the battle idle is frames 11 10 9 10 11 12 13 12 facing the camera and
  16 15 14 15 16 17 18 17 facing away (6 8 10 8 ticks at 60 Hz), the walk the same frames at
  2 4 6 4 ticks. FFT draws two directions (front: down-left, back: up-left) and mirrors them
  for the other two.
Each of frames 9..18 is composed, scaled by SCALE (0.6: the unit's height against the map's
tiles and doors in FFT, README.md; each TI pixel takes the source pixels whose centre falls in
it, opaque when half of them are, its grey the vote of their greys, FFT's own black line
excluded), cut to 16 columns (a hand or a
ponytail tip in 4 frames of 40), then given a black line on its silhouette's edge and a white
outline (visibility rule). Greys by each palette colour's luminance (0-31): <= 7 black, <= 16
dark, <= 22 light, else white; colour 1 (FFT's line) black. The face is made one grey lighter and the
eyes are placed on their own (below: scaling merged them into a bar or lost one).
"""
import os, struct, sys
import numpy as np
from scipy import ndimage
sys.path.insert(0, os.path.dirname(__file__))
import extract                  # noqa: E402  (disc access)

CHARS = [('RAMZA', 'RAMUZA'), ('DELITA', 'DILY'), ('AGRIAS', 'AGURI'), ('THIEF', 'THIEF_M')]
FRAMES = range(9, 19)           # TYPE1: 9-13 front, 14-18 back
SIZES = [(8, 8), (16, 8), (16, 16), (16, 24), (24, 8), (24, 16), (24, 24), (32, 8), (32, 16),
         (32, 24), (32, 32), (32, 40), (40, 16), (40, 32), (48, 48), (56, 56)]
SCALE = 0.6
W, H, FOOT = 16, 26, 21         # the TI sprite: 16 x 26, anchor at column 8, row FOOT
SKIN = (13, 14, 15)             # the face's palette colours (the same ramp in all four sheets)
SKIN_STEP, EYE_H = 1, 2         # the face one grey lighter, the eyes two rows tall


def shp_frames(d):
    out, i = [], 0
    while True:
        p = struct.unpack_from('<I', d, 8 + 4 * i)[0]
        if i and not p:
            return out
        o = 0x40A + p
        tiles = []
        for j in range((d[o] & 7) + 1):
            sx, sy, fl = struct.unpack_from('<bbH', d, o + 2 + 4 * j)
            w, h = SIZES[fl >> 10 & 15]
            tiles.append((sx, sy, (fl & 31) * 8, (fl >> 5 & 31) * 8, w, h, fl >> 14 & 1, fl >> 15 & 1))
        out.append(tiles)
        i += 1


def sprite(spr, tiles):
    """One frame at the TI's size: greys 0-3, -1 transparent, H x W."""
    pal = np.frombuffer(spr[:32], '<u2').astype(int)
    lum = (pal & 31) * .299 + (pal >> 5 & 31) * .587 + (pal >> 10 & 31) * .114
    grey = np.where(lum <= 7, 3, np.where(lum <= 16, 2, np.where(lum <= 22, 1, 0)))
    grey[1] = 3
    px = np.frombuffer(spr[512:512 + 128 * 256], np.uint8).reshape(256, 128)
    sheet = np.empty((256, 256), np.uint8)
    sheet[:, 0::2] = px & 15; sheet[:, 1::2] = px >> 4
    cv = np.zeros((128, 128), np.uint8); ox, oy = 64, 96          # the anchor
    for sx, sy, x, y, w, h, fx, fy in reversed(tiles):
        t = sheet[y:y + h, x:x + w]
        t = t[:, ::-1] if fx else t
        t = t[::-1] if fy else t
        r = cv[oy + sy:oy + sy + h, ox + sx:ox + sx + w]
        r[t > 0] = t[t > 0]
    o = cv > 0
    p = np.pad(o, 1)
    inner = o & p[:-2, 1:-1] & p[2:, 1:-1] & p[1:-1, :-2] & p[1:-1, 2:]
    top = np.nonzero(o.sum(1) > 2)[0][0]                            # the hair (a 1-2 px tuft aside)
    face = np.zeros_like(o); face[top + 5:top + 14] = True         # from the eyebrows to the chin
    skin = np.isin(cv, SKIN) & face
    # the eyes: FFT's line colour inside the skin, rows 7-10 under the hair's top, one or two
    # 8-connected groups of 2-4 pixels (the far one partly hidden: the face is seen 3/4)
    sk = np.pad(skin | (cv == 2), 1)                                # skin or the eyes' glint
    near = sk[:-2, 1:-1] | sk[2:, 1:-1] | sk[1:-1, :-2] | sk[1:-1, 2:]
    eye = (cv == 1) & inner & near
    eye[:top + 7] = False; eye[top + 11:] = False
    lab, k = ndimage.label(eye, np.ones((3, 3)))
    groups = sorted(range(1, k + 1), key=lambda i: -(lab == i).sum())[:2]
    # every source pixel into its TI cell
    ys, xs = np.mgrid[:128, :128]
    ty = np.floor((ys + .5 - oy) * SCALE).astype(int) + FOOT
    tx = np.floor((xs + .5 - ox) * SCALE).astype(int) + W // 2
    ok = (ty >= 0) & (ty < H) & (tx >= 1) & (tx < W - 1)            # room for the outline
    assert not (o & (ty < 1)).any() and not (o & (ty >= H - 1)).any(), 'a frame does not fit 26 rows'
    cell = ty * W + tx
    def acc(m):
        return np.bincount(cell[ok & m], minlength=H * W).reshape(H, W)
    n, opq, skins = acc(np.ones_like(o)), acc(o), acc(skin)
    hist = np.stack([acc(o & (cv != 1) & (grey[cv] == g)) for g in range(4)], 2)
    out = np.full((H, W), -1)
    m = (opq * 2 >= n) & (n > 0)
    g = np.where(hist.sum(2) > 0, hist.argmax(2), 3)
    g = np.where(skins * 2 >= hist.sum(2).clip(1), np.maximum(g - SKIN_STEP, 0), g)  # lighter skin
    out[m] = g[m]
    p = np.pad(m, 1)
    edge = m & ~(p[:-2, 1:-1] & p[2:, 1:-1] & p[1:-1, :-2] & p[1:-1, 2:])
    out[edge] = 3                                                   # black line
    # each eye placed on its own: its group's centre column, its top row; the second one at
    # least two columns from the first (a skin pixel between: halving merged them in a bar)
    pos = []
    for i in groups:
        yy, xx = np.nonzero(lab == i)
        pos.append([int(np.floor((yy.min() + .5 - oy) * SCALE)) + FOOT,
                    int(np.floor((xx.mean() + .5 - ox) * SCALE)) + W // 2])
    pos.sort(key=lambda e: e[1])
    if len(pos) == 2 and pos[1][1] - pos[0][1] < 2:
        pos[1][1] = pos[0][1] + 2
    for y, x in pos:
        for r in range(EYE_H):
            if m[y + r, x] and not edge[y + r, x]:
                out[y + r, x] = 3
    ring = (p[:-2, 1:-1] | p[2:, 1:-1] | p[1:-1, :-2] | p[1:-1, 2:]) & ~m
    out[ring] = 0                                                   # white outline
    return out


def main():
    disc, files = extract.disc_files(sys.argv[1])
    shp = shp_frames(disc.read(*files['BATTLE/TYPE1.SHP']))
    sprites = []
    for _, name in CHARS:
        spr = disc.read(*files['BATTLE/%s.SPR' % name])
        sprites.append([sprite(spr, shp[k]) for k in FRAMES])
    f = open(sys.argv[2], 'w')
    f.write('// Generated by tools/units.py from the local FFT disc (BATTLE/*.SPR, TYPE1.SHP): never\n'
            '// committed. The frames are in the data file fftu (fftu.bin, big-endian u16 rows; on\n'
            '// the TI the archived variable read in place): per unit UNIT_FRAMES frames, 0-4 facing\n'
            '// the camera (FFT TYPE1 9-13, looking down-left), 5-9 facing away (14-18, up-left); per\n'
            '// frame light, dark, mask (1 = transparent) rows, 16 wide, the anchor (the tile centre\n'
            '// under the feet) at column 8, row UNIT_FOOT.\n')
    f.write('enum { %s, UNIT_GFX_N };\n' % ', '.join('UG_' + c for c, _ in CHARS))
    f.write('#define UNIT_FRAMES %d\n#define UNIT_SH %d\n#define UNIT_FOOT %d\n' % (len(FRAMES), H, FOOT))
    f.close()
    out = bytearray()
    bits = 1 << (15 - np.arange(W))
    for frames in sprites:
        for s in frames:
            l = [(bits * ((s[y] >= 0) & ((s[y] & 1) > 0))).sum() for y in range(H)]
            d = [(bits * ((s[y] >= 0) & ((s[y] & 2) > 0))).sum() for y in range(H)]
            k = [(bits * (s[y] < 0)).sum() for y in range(H)]
            out += struct.pack('>%dH' % (3 * H), *(l + d + k))
    open(os.path.join(os.path.dirname(sys.argv[2]) or '.', 'fftu.bin'), 'wb').write(out)
    if len(sys.argv) > 3:
        from PIL import Image
        lcd = np.array([[208, 214, 190], [150, 158, 135], [90, 96, 80], [30, 34, 28]], np.uint8)
        rows = []
        for frames in sprites:
            tiles = []
            for s in frames + [s[:, ::-1] for s in frames]:
                im = np.tile(lcd[1], (H, W, 1)); im[s >= 0] = lcd[s[s >= 0]]
                tiles.append(np.pad(im, ((2, 2), (2, 2), (0, 0)), constant_values=255))
            rows.append(np.concatenate(tiles, 1))
        img = np.concatenate(rows)
        Image.fromarray(img).resize((img.shape[1] * 4, img.shape[0] * 4), Image.NEAREST).save(sys.argv[3])


if __name__ == '__main__':
    main()
