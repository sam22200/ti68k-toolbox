#!/usr/bin/env python3
"""Chrono Trigger camp-fire scene -> TI-89 data (map tiles, character sprites, fire animation).

Input : chrono_fire_camp.gif (636x516, 13 frames at 200 ms; the SNES 256x224 picture scaled by
        2.484 horizontally and 2.246 vertically, plus a 13-row border at the bottom).
Output: ../data.h, and preview PNGs in argv[1] (default: this directory).
Run with tools/pyenv/bin/python (numpy, scipy, pillow).

1. Every frame is box-downscaled back to 256x224; the static layer is the per-pixel median of the
   13 frames (removes the GIF's dithering noise; only the fire moves).
2. Characters: a hand-traced polygon per character (SNES pixels) gives the rough outline; inside
   it, blue ground pixels connected to the polygon's edge are removed (the night ground is
   saturated blue, the characters' outlines are near-black without a blue cast).
3. The map under each character is rebuilt from the same rows of a horizontally shifted patch of
   ground (fine-grained texture: a copy looks natural).
4. 4 grey levels from luminance quantiles of the rebuilt map (40/75/93 %: a dark night scene).
   Sprites get their own quantiles (22/50/78 % of their pixels: full contrast per character) and a
   1-pixel white outline so they stand out on the dark ground (KB rule).
5. The fire is an opaque 32x48 rectangle per frame, drawn with RPLC over the map; identical frames
   are shared.
6. The map is cut into 16x16 grey tiles, deduplicated, padded with black tiles (TileMap reads a
   margin), rows interlaced (dark row, light row): GrayDBuf's dark plane sits before the light one.
"""
import os
import sys
import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = sys.argv[1] if len(sys.argv) > 1 else HERE
W, H = 256, 224

im = Image.open(os.path.join(HERE, 'chrono_fire_camp.gif'))
frames = []
for i in range(im.n_frames):
    im.seek(i)
    f = im.convert('RGB').crop((0, 0, 636, 503)).resize((W, H), Image.BOX)
    frames.append(np.asarray(f).astype(np.float32))
frames = np.stack(frames)
static = np.median(frames, axis=0)

def lum(a):
    return 0.299 * a[..., 0] + 0.587 * a[..., 1] + 0.114 * a[..., 2]

# ------------------------------------------------------------------ characters (drawing order)
POLY = {
    'robo':  [(137, 72), (147, 72), (149, 80), (149, 100), (148, 110), (152, 112), (152, 121), (146, 123),
              (140, 121), (134, 122), (128, 123), (120, 122), (118, 113), (121, 110), (121, 86), (126, 85), (136, 84)],
    'frog':  [(80, 115), (87, 115), (89, 119), (89, 127), (91, 131), (92, 138), (90, 143), (86, 146), (78, 146),
              (74, 143), (73, 136), (75, 130), (78, 127), (78, 119)],
    'ayla':  [(181, 125), (186, 125), (188, 132), (192, 136), (195, 140), (194, 148), (197, 150), (197, 158),
              (194, 162), (180, 162), (177, 158), (177, 151), (180, 148), (179, 140), (181, 134)],
    'marle': [(80, 157), (88, 156), (90, 160), (90, 166), (97, 168), (99, 173), (96, 178), (90, 176), (89, 186),
              (84, 188), (78, 188), (72, 187), (66, 186), (66, 182), (72, 179), (78, 176), (80, 172), (76, 172),
              (75, 166), (76, 160)],
    'crono': [(142, 187), (150, 184), (154, 180), (160, 178), (170, 180), (173, 184), (173, 192), (166, 194),
              (158, 194), (150, 193), (142, 193), (140, 190)],
    'magus': [(235, 122), (244, 122), (247, 127), (246, 134), (248, 140), (248, 152), (247, 160), (240, 160),
              (236, 158), (233, 150), (232, 142), (233, 134), (234, 128)],
}
FIRE = (112, 112, 144, 160)          # x0, y0, x1, y1: 32x48 animated rectangle

r, g, b = static[..., 0], static[..., 1], static[..., 2]
BLUE = (b > r + 12) & (b > g + 8)             # cyan (Crono's clothes) is not ground

def poly_mask(pts):
    img = Image.new('1', (W, H), 0)
    ImageDraw.Draw(img).polygon(pts, fill=1, outline=1)
    return np.asarray(img).astype(bool)

masks = {}
for k, pts in POLY.items():
    p = poly_mask(pts)
    ground = BLUE & p
    lab, n = ndimage.label(ground)
    edge = p & ~ndimage.binary_erosion(p)
    touching = set(np.unique(lab[edge & ground])) - {0}
    m = p & ~np.isin(lab, list(touching))
    m = ndimage.binary_opening(m, structure=np.ones((2, 2))) | (m & ndimage.binary_erosion(p, iterations=2))
    lab, n = ndimage.label(m)
    if n > 1:
        sizes = ndimage.sum(m, lab, range(1, n + 1))
        m = np.isin(lab, [i + 1 for i, s in enumerate(sizes) if s >= 6])
    masks[k] = ndimage.binary_fill_holes(m)

# ------------------------------------------------------------------ map without characters
occupied = np.zeros((H, W), bool)
for m in masks.values():
    occupied |= m
occupied[FIRE[1]:FIRE[3], FIRE[0]:FIRE[2]] = True
plate = static.copy()
for k, m in masks.items():
    ys, xs = np.nonzero(m)
    y0, y1, x0, x1 = ys.min(), ys.max() + 1, xs.min(), xs.max() + 1
    w = x1 - x0
    best = None
    for off in [o for o in range(-3 * w, 3 * w + 1) if abs(o) > w]:
        if x0 + off < 0 or x1 + off > W:
            continue
        bad = occupied[y0:y1, x0 + off:x1 + off].sum()
        if best is None or bad < best[0]:
            best = (bad, off)
    g1 = ndimage.binary_dilation(m, iterations=1)[y0:y1, x0:x1]
    plate[y0:y1, x0:x1][g1] = static[y0:y1, x0 + best[1]:x1 + best[1]][g1]

# ------------------------------------------------------------------ grey levels
TH = [np.percentile(lum(plate), p) for p in (40, 75, 93)]

def quant(a):
    return np.digitize(lum(a), TH).astype(np.uint8)      # 0 = black .. 3 = white

qmap = quant(plate)
qstatic = quant(static)

def planes_of(q):
    """0 black .. 3 white -> (light, dark) plane bits with ExtGraph's colours:
    white (0,0), light grey (1,0), dark grey (0,1), black (1,1)."""
    lvl = 3 - q
    return (lvl == 1) | (lvl == 3), (lvl == 2) | (lvl == 3)

def rows(bits, n):
    out = []
    for row in bits:
        v = 0
        for i, bit in enumerate(row):
            if bit:
                v |= 1 << (n - 1 - i)
        out.append(v)
    return out

c = ['// Generated by tools/extract.py from chrono_fire_camp.gif: do not edit.',
     '// Scene %dx%d SNES pixels; grey thresholds %s.' % (W, H, [int(t) for t in TH])]

# ---- map
MW, MH = 18, 16                       # 16x14 scene tiles + 2 black columns and rows of margin
full = np.zeros((MH * 16, MW * 16), np.uint8)
full[:H, :W] = qmap
tiles, index = [], {}
tmap = np.zeros((MH, MW), np.uint8)
for ty in range(MH):
    for tx in range(MW):
        t = full[ty * 16:ty * 16 + 16, tx * 16:tx * 16 + 16]
        key = t.tobytes()
        if key not in index:
            index[key] = len(tiles)
            tiles.append(t)
        tmap[ty, tx] = index[key]
assert len(tiles) < 256
c.append('#define MAP_W %d\n#define MAP_H %d\n#define SCENE_W %d\n#define SCENE_H %d\n#define NTILES %d'
         % (MW, MH, W, H, len(tiles)))
c.append('static unsigned char map[MAP_H][MAP_W] = {')
c += ['  {' + ','.join(str(v) for v in tmap[ty]) + '},' for ty in range(MH)]
c.append('};')
c.append('static const unsigned short tiles[NTILES][32] = {   // rows interlaced: dark, light')
for t in tiles:
    L, D = planes_of(t)
    L, D = rows(L, 16), rows(D, 16)
    c.append('  {' + ','.join('0x%04X,0x%04X' % (D[i], L[i]) for i in range(16)) + '},')
c.append('};')

# ---- characters, cut into 32-pixel-wide strips
c.append('typedef struct { short x, y, h; const unsigned long *l, *d, *m; } SPR;')
spr_list = []
spr_prev = np.full((H, W), -1, np.int16)
for k, m in masks.items():
    mp = np.pad(m, 1)
    ring = ndimage.binary_dilation(mp) & ~mp          # 1-px white outline
    shape = mp | ring
    Ls = lum(static)[m]                               # the sprite's own contrast range
    th = [np.percentile(Ls, q) for q in (22, 50, 78)]
    lv = np.pad(np.digitize(lum(static), th).astype(np.uint8), 1)
    lv[ring] = 3
    ys, xs = np.nonzero(shape)
    y0, y1, x0, x1 = ys.min(), ys.max() + 1, xs.min(), xs.max() + 1
    for sx in range(x0, x1, 32):
        sh = shape[y0:y1, sx:min(sx + 32, x1)]
        q = lv[y0:y1, sx:min(sx + 32, x1)]
        L, D = planes_of(q)
        L &= sh; D &= sh
        nm = '%s%d' % (k, (sx - x0) // 32)
        h = y1 - y0
        c.append('static const unsigned long %s_l[%d] = {%s};' % (nm, h, ','.join('0x%08X' % v for v in rows(L, 32))))
        c.append('static const unsigned long %s_d[%d] = {%s};' % (nm, h, ','.join('0x%08X' % v for v in rows(D, 32))))
        c.append('static const unsigned long %s_m[%d] = {%s};' % (nm, h, ','.join('0x%08X' % (~v & 0xFFFFFFFF) for v in rows(sh, 32))))
        spr_list.append('{%d,%d,%d,%s_l,%s_d,%s_m}' % (sx - 1, y0 - 1, h, nm, nm, nm))   # -1: padding
        view = spr_prev[y0 - 1:y1 - 1, sx - 1:sx - 1 + sh.shape[1]]
        view[sh] = q[sh]
c.append('#define NSPR %d' % len(spr_list))
c.append('static const SPR sprites[NSPR] = {\n  ' + ',\n  '.join(spr_list) + '};')

# ---- fire
fx0, fy0, fx1, fy1 = FIRE
fire, seq = [], []
for f in frames:
    q = quant(f[fy0:fy1, fx0:fx1])
    for i, g2 in enumerate(fire):
        if (g2 == q).all():
            seq.append(i); break
    else:
        seq.append(len(fire)); fire.append(q)
c.append('#define FIRE_X %d\n#define FIRE_Y %d\n#define FIRE_H %d\n#define NFIRE %d\n#define NFSEQ %d'
         % (fx0, fy0, fy1 - fy0, len(fire), len(seq)))
for plane, nm in ((0, 'fire_l'), (1, 'fire_d')):
    c.append('static const unsigned long %s[NFIRE][FIRE_H] = {' % nm)
    c += ['  {' + ','.join('0x%08X' % v for v in rows(planes_of(q)[plane], 32)) + '},' for q in fire]
    c.append('};')
c.append('static const unsigned char fire_seq[NFSEQ] = {' + ','.join(map(str, seq)) + '};')
open(os.path.join(HERE, '..', 'data.h'), 'w').write('\n'.join(c) + '\n')

# ------------------------------------------------------------------ previews
pal = np.array([[40, 50, 40], [95, 110, 90], [155, 170, 145], [205, 220, 195]], np.uint8)
big = lambda a: Image.fromarray(pal[a]).resize((W * 3, H * 3), Image.NEAREST)
comp = qmap.copy()
comp[fy0:fy1, fx0:fx1] = fire[seq[0]]
comp[spr_prev >= 0] = spr_prev[spr_prev >= 0]
big(qmap).save(os.path.join(OUT, 'prev_map.png'))
big(comp).save(os.path.join(OUT, 'prev_scene.png'))
mk = (static // 3).astype(np.uint8)
for m in masks.values():
    mk[m] = static[m].astype(np.uint8)
Image.fromarray(mk).resize((W * 3, H * 3), Image.NEAREST).save(os.path.join(OUT, 'prev_masks.png'))
Image.fromarray(plate.astype(np.uint8)).resize((W * 3, H * 3), Image.NEAREST).save(os.path.join(OUT, 'prev_plate.png'))
print('tiles %d, sprite strips %d, fire frames %d, sequence %s' % (len(tiles), len(spr_list), len(fire), seq))
