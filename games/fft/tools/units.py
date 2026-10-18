#!/usr/bin/env python3
"""The units' sprites from the local Final Fantasy Tactics disc -> units.h, fftu.bin (never committed).

usage: units.py DISC.cue units.h [preview.png]   (also writes fftu.bin next to units.h)
The crystal and the chest the KO units become: BATTLE/OTHER.SPR (obj_sprite).
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

# the Gariland battle's sheets (tools/battle.py names them): sheet, frame file
CHARS = [('RAMZA', 'RAMUZA', 'TYPE1'), ('DELITA', 'DILY', 'TYPE1'), ('SQUIRE_M', 'MINA_M', 'TYPE1'),
         ('SQUIRE_F', 'MINA_W', 'TYPE1'), ('CHEMIST_M', 'ITEM_M', 'TYPE2'), ('CHEMIST_F', 'ITEM_W', 'TYPE2')]
# TYPE1 and TYPE2 alike: the idle 9-13 front, 14-18 back; then each pose front, back (TYPE1.SEQ:
# hit 19 / 20 (animations 66-69), the swing's wind-up 27 / 29 and its blow 31 / 33 (52-57),
# kneeling 21 / 22 (HP low), lying KO 23 / 24 (the end of 102-105), the throw 28 / 30 after the
# same wind-up (58-59: Throw Stone, seen on the original run headless))
FRAMES = list(range(9, 19)) + [19, 20, 27, 29, 31, 33, 21, 22, 23, 24, 28, 30]
LIFT = {23: 4, 24: 4}            # lying: drawn 4 rows higher to fit (still on its tile)
SIZES = [(8, 8), (16, 8), (16, 16), (16, 24), (24, 8), (24, 16), (24, 24), (32, 8), (32, 16),
         (32, 24), (32, 32), (32, 40), (40, 16), (40, 32), (48, 48), (56, 56)]
SCALE = 0.6
W, H, FOOT = 16, 26, 21         # the TI sprite: 16 x 26, anchor at column 8, row FOOT
SKIN = (13, 14, 15)             # the face's palette colours (the same ramp in all six sheets)
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


def sprite(spr, tiles, lift=0):
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
    ty = np.floor((ys + .5 - oy) * SCALE).astype(int) + FOOT - lift
    tx = np.floor((xs + .5 - ox) * SCALE).astype(int) + W // 2
    ok = (ty >= 1) & (ty < H) & (tx >= 1) & (tx < W - 1)            # room for the outline (a raised
                                                                    # hand's top row cut in 7 swing frames)
    assert not (o & (ty >= H - 1)).any(), 'a frame does not fit 26 rows'
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


# the battle's objects (BATTLE/OTHER.SPR, palette 0), boxes of the sheet (x, y, w, h): a crystal (one
# of its eight turning images; no OTHER.SHP frame composes it) and a closed chest
CRYSTAL = (98, 97, 13, 26)
# Complete closed-chest artwork, excluding the stray rows above it. The former
# y=5,h=19 crop lost the front bottom corner (the source extends through row 27).
CHEST = (225, 6, 23, 22)
CHEST_GREYS = [3, 3, 0, 2, 1, 1, 0, 3, 2, 1, 2, 1, 0, 3, 3, 0]
OBJ_LIFT = 6                     # source rows above the anchor: the crystal floats, the chest's
                                 # front corner (6 below FFT's anchor) fits the sprite's rows


def obj_sprite(spr, box, scale=SCALE, greys=None):
    """An object at the TI's size: the units' scaling and greys, no face, a black line and a
    white outline; anchored at the tile centre (row FOOT)."""
    pal = np.frombuffer(spr[:32], '<u2').astype(int)
    lum = (pal & 31) * .299 + (pal >> 5 & 31) * .587 + (pal >> 10 & 31) * .114
    grey = np.where(lum <= 7, 3, np.where(lum <= 16, 2, np.where(lum <= 22, 1, 0)))
    if greys:
        grey = np.array(greys)
    n = (len(spr) - 512) // 128
    px = np.frombuffer(spr[512:512 + 128 * n], np.uint8).reshape(n, 128)
    sheet = np.empty((n, 256), np.uint8)
    sheet[:, 0::2] = px & 15; sheet[:, 1::2] = px >> 4
    cv = np.zeros((128, 128), np.uint8); ox, oy = 64, 96
    x, y, w, h = box                                # bottom centre on the anchor, lifted
    cv[oy - h - OBJ_LIFT:oy - OBJ_LIFT, ox - w // 2:ox - w // 2 + w] = sheet[y:y + h, x:x + w]
    o = cv > 0
    ys, xs = np.mgrid[:128, :128]
    ty = np.floor((ys + .5 - oy) * scale).astype(int) + FOOT
    tx = np.floor((xs + .5 - ox) * scale).astype(int) + W // 2
    ok = (ty >= 1) & (ty < H - 1) & (tx >= 1) & (tx < W - 1)
    assert not (o & ~ok).any(), 'an object does not fit'
    cell = ty * W + tx
    acc = lambda m: np.bincount(cell[ok & m], minlength=H * W).reshape(H, W)
    cnt, opq = acc(np.ones_like(o)), acc(o)
    hist = np.stack([acc(o & (cv != 1) & (grey[cv] == g)) for g in range(4)], 2)
    out = np.full((H, W), -1)
    m = (opq * 2 >= cnt) & (cnt > 0)
    g = np.where(hist.sum(2) > 0, hist.argmax(2), 3)
    out[m] = g[m]
    p = np.pad(m, 1)
    out[m & ~(p[:-2, 1:-1] & p[2:, 1:-1] & p[1:-1, :-2] & p[1:-1, 2:])] = 3
    out[(p[:-2, 1:-1] | p[2:, 1:-1] | p[1:-1, :-2] | p[1:-1, 2:]) & ~m] = 0
    return out


def chest_sprite(spr):
    """The actual OTHER.SPR closed chest, sampled intact to 14x14, palette 0.

    Preserve the source lid, metal straps, lock and side; do not grow a black inner
    contour over them. Only the external white visibility outline is added. The
    bottom rests at the common FOOT anchor instead of floating above its shadow.
    All work is offline; the packed frame and runtime drawing remain unchanged.
    """
    from PIL import Image
    packed = np.frombuffer(spr[512:], np.uint8).reshape(-1, 128)
    sheet = np.empty((len(packed), 256), np.uint8)
    sheet[:, ::2] = packed & 15; sheet[:, 1::2] = packed >> 4
    x, y, w, h = CHEST
    source = sheet[y:y+h, x:x+w]
    small = np.array(Image.fromarray(source).resize((14, 14), Image.Resampling.NEAREST))
    # A majority of bright source metal in a cell wins over one nearest wood pixel:
    # keep the original narrow straps visible at calculator resolution.
    for row in range(14):
        for col in range(14):
            cell = source[row*h//14:(row+1)*h//14, col*w//14:(col+1)*w//14]
            if np.isin(cell, (2, 6, 15)).sum() >= (cell.size + 1) // 2:
                small[row, col] = 2
    out = np.full((H, W), -1)
    out[FOOT-14:FOOT, 1:15] = np.where(small, np.array(CHEST_GREYS)[small], -1)
    mask = out >= 0; p = np.pad(mask, 1)
    outline = (p[:-2, 1:-1] | p[2:, 1:-1] | p[1:-1, :-2] | p[1:-1, 2:]) & ~mask
    out[outline] = 0
    return out


def main():
    disc, files = extract.disc_files(sys.argv[1])
    shp = {t: shp_frames(disc.read(*files['BATTLE/%s.SHP' % t])) for t in ('TYPE1', 'TYPE2')}
    sprites = []
    for _, name, t in CHARS:
        spr = disc.read(*files['BATTLE/%s.SPR' % name])
        sprites.append([sprite(spr, shp[t][k], LIFT.get(k, 0)) for k in FRAMES])
    spr = disc.read(*files['BATTLE/OTHER.SPR'])
    objs = [obj_sprite(spr, box=CRYSTAL), chest_sprite(spr)]
    # Preserve the two legacy object slots, then complete the eight source rotation phases.
    objs += [obj_sprite(spr, box=(2 + 16 * k, 97, 13, 26)) for k in (7, 0, 1, 2, 3, 4, 5)]
    # Two offline lifts; shadows are added afterwards at the unchanged ground anchor.
    for lift in (1, 2):
        raised = np.full((H, W), -1)
        raised[:-lift] = objs[1][lift:]
        assert not (objs[1][:lift] >= 0).any(), 'chest lift clips its outline'
        objs.append(raised)
    f = open(sys.argv[2], 'w')
    f.write('// Generated by tools/units.py from the local FFT disc (BATTLE/*.SPR, TYPE1.SHP, TYPE2.SHP): never\n'
            '// committed. The frames are in the data file fftu (fftu.bin, big-endian u16 rows; on\n'
            '// the TI the archived variable read in place): per unit UNIT_FRAMES frames, 0-4 facing\n'
            '// the camera (FFT TYPE1 9-13, looking down-left), 5-9 facing away (14-18, up-left), then\n'
            '// the poses P_*, each facing the camera then away, then all of them mirrored; per frame\n'
            '// light, dark, mask (1 = transparent) rows, the shadow under the feet (art.py) included,\n'
            '// then the inside (the opaque pixels eroded twice, and the shadow\'s own: what a cover\n'
            '// takes away, the contour kept), 16 wide, the anchor (the tile centre under the feet)\n'
            '// at column 8, row UNIT_FOOT.\n')
    f.write('enum { %s, UNIT_GFX_N };\n' % ', '.join('UG_' + c for c, _, _ in CHARS))
    f.write('enum { P_HIT = 10, P_WIND = 12, P_BLOW = 14, P_KNEEL = 16, P_KO = 18, P_THROW = 20 };\n')
    f.write('#define UNIT_FRAMES %d\n#define UNIT_SH %d\n#define UNIT_FOOT %d\n' % (len(FRAMES), H, FOOT))
    f.write('#define UNIT_FB (8 * UNIT_SH)          // bytes per frame\n')
    f.write('// Objects: crystal phase 0, chest, crystal phases 1..7, chest lifts 1..2.\n'
            'enum { OBJ_CRYSTAL, OBJ_CHEST, OBJ_CHEST_UP1 = 9, OBJ_CHEST_UP2, OBJ_N };\n'
            '#define CRYSTAL_FRAMES 8\n')
    f.close()
    out = bytearray()
    bits = 1 << (15 - np.arange(W))

    def erode(k, r):                    # row r's opaque pixels whose four neighbours are opaque
        o = ~k[r] & 0xFFFF
        return o & (o << 1) & 0xFFFF & (o >> 1) & ~k[r - 1] & ~k[r + 1] & 0xFFFF

    def inside(k):                      # eroded twice, rows 2 .. H - 3 (fft.c draw_unit_covered)
        e = [0] * H
        for r in range(2, H - 2):
            a, b, d = erode(k, r - 1), erode(k, r), erode(k, r + 1)
            e[r] = b & (b << 1) & 0xFFFF & (b >> 1) & a & d
        return e

    from art import SHADOW              # the shadow under the feet (rows FOOT - 1 .. FOOT + 1)
    shadow = [[c != '.' for c in r.ljust(W, '.')] for r in SHADOW.strip('\n').split('\n')]
    for frames in sprites + [objs]:
        for s in frames + ([s[:, ::-1] for s in frames] if frames is not objs else []):   # mirrored
            e = inside([(bits * (s[y] < 0)).sum() for y in range(H)])   # the unit's own mask
            s = s.copy()
            under = [0] * H                 # the shadow's pixels left uncovered by the unit
            for r, row in enumerate(shadow):
                y = FOOT - 1 + r
                for x in range(W):
                    if row[x] and s[y, x] < 0:
                        s[y, x] = 2; under[y] |= 0x8000 >> x
            l = [(bits * ((s[y] >= 0) & ((s[y] & 1) > 0))).sum() for y in range(H)]
            d = [(bits * ((s[y] >= 0) & ((s[y] & 2) > 0))).sum() for y in range(H)]
            k = [(bits * (s[y] < 0)).sum() for y in range(H)]
            out += struct.pack('>%dH' % (4 * H), *(l + d + k + [a | b for a, b in zip(e, under)]))
    open(os.path.join(os.path.dirname(sys.argv[2]) or '.', 'fftu.bin'), 'wb').write(out)
    if len(sys.argv) > 3:
        from PIL import Image
        lcd = np.array([[208, 214, 190], [150, 158, 135], [90, 96, 80], [30, 34, 28]], np.uint8)
        rows = []
        for frames in sprites + [objs + [objs[0]] * (2 * len(FRAMES) - len(objs))]:
            tiles = []
            for s in frames + ([s[:, ::-1] for s in frames] if len(frames) == len(FRAMES) else []):
                im = np.tile(lcd[1], (H, W, 1)); im[s >= 0] = lcd[s[s >= 0]]
                tiles.append(np.pad(im, ((2, 2), (2, 2), (0, 0)), constant_values=255))
            rows.append(np.concatenate(tiles, 1))
        img = np.concatenate(rows)
        Image.fromarray(img).resize((img.shape[1] * 4, img.shape[0] * 4), Image.NEAREST).save(sys.argv[3])


if __name__ == '__main__':
    main()
