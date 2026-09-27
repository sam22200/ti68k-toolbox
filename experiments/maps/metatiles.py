#!/usr/bin/env python3
"""Tile-reuse analysis of the camp-fire map (games/campfire/data.h, read-only): how many bytes the
background would take with 8x8 tiles, flips, 16x16 metatiles and near-duplicate merging, and the
size of the fire frames as deltas against frame N-1 / N-2 (Amiga ANIM idea).
Writes merged-map previews to argv[1] (default: this directory).
Run: tools/pyenv/bin/python experiments/maps/metatiles.py [outdir]
"""
import os
import re
import sys
import numpy as np
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, '..', '..', 'games', 'campfire', 'data.h')
OUT = sys.argv[1] if len(sys.argv) > 1 else HERE
text = open(SRC).read()


def define(name):
    return int(re.search(r'#define %s (\d+)' % name, text).group(1))


def array(name):
    body = re.search(r'%s\[[^=]*=\s*\{(.*?)\};' % re.escape(name), text, re.S).group(1)
    return [int(v, 0) for v in re.findall(r'0x[0-9A-Fa-f]+|\d+', body)]


MW, MH, NT = define('MAP_W'), define('MAP_H'), define('NTILES')
tmap = np.array(array('map'), np.int32).reshape(MH, MW)
words = np.array(array('tiles'), np.int32).reshape(NT, 16, 2)       # per row: dark, light


def levels(dark, light, n):
    """Plane words -> grey levels 0 white .. 3 black (light = bit 0, dark = bit 1)."""
    bits = lambda w: (w[..., None] >> np.arange(n - 1, -1, -1)) & 1
    return bits(light) + 2 * bits(dark)


tiles16 = levels(words[:, :, 0], words[:, :, 1], 16).astype(np.uint8)       # NT x 16 x 16
img = np.zeros((MH * 16, MW * 16), np.uint8)
for ty in range(MH):
    for tx in range(MW):
        img[ty * 16:ty * 16 + 16, tx * 16:tx * 16 + 16] = tiles16[tmap[ty, tx]]
H, W = 224, 256
scene = img[:H, :W]                     # the 16x14 scene without TileMap's black margin


def cut(a, n):
    h, w = a.shape
    return a.reshape(h // n, n, w // n, n).swapaxes(1, 2).reshape(-1, n, n)


def variants(t, flips):
    if not flips:
        return [t]
    return [t, t[:, ::-1], t[::-1, :], t[::-1, ::-1]]


def dedup(blocks, flips=False, thresh=0):
    """Greedy clustering: most frequent blocks first become representatives; a block joins the first
    representative (under any allowed flip) within `thresh` differing pixels. Returns (reps, assign)."""
    keys = [b.tobytes() for b in blocks]
    count = {}
    for k in keys:
        count[k] = count.get(k, 0) + 1
    order = sorted(set(keys), key=lambda k: -count[k])
    first = {k: blocks[keys.index(k)] for k in order}
    reps, rep_of = [], {}
    for k in order:
        b = first[k]
        best = None
        for i, r in enumerate(reps):
            for f, v in enumerate(variants(r, flips)):
                d = int((v != b).sum())
                if d <= thresh and (best is None or d < best[0]):
                    best = (d, i, f)
        if best is None:
            rep_of[k] = (len(reps), 0)
            reps.append(b)
        else:
            rep_of[k] = best[1:]
    return reps, [rep_of[k] for k in keys]


def rebuild(reps, assign, n, shape, flips):
    out = np.zeros(shape, np.uint8)
    cols = shape[1] // n
    for j, (i, f) in enumerate(assign):
        y, x = divmod(j, cols)
        out[y * n:y * n + n, x * n:x * n + n] = variants(reps[i], flips)[f] if flips else reps[i]
    return out


def png(a, name):
    Image.fromarray((255 - a * 85).astype(np.uint8)).resize((a.shape[1] * 2, a.shape[0] * 2), Image.NEAREST) \
        .save(os.path.join(OUT, name))


cells16, cells8 = len(cut(scene, 16)), len(cut(scene, 8))
print('scene %dx%d: %d cells of 16x16, %d of 8x8; campfire.c stores %d tiles = %d B + map %d B'
      % (W, H, cells16, cells8, NT, NT * 64, MW * MH))
print('%-34s %6s %8s %8s %8s %8s  %s' % ('scheme', 'tiles', 'tile B', 'blocks', 'map B', 'total', 'px changed'))
rows = []
for n in (16, 8):
    for flips in (False, True):
        for th in (0, 2, 4, 8, 12):
            if n == 16 and th > 0 and th != 8:
                continue
            reps, assign = dedup(cut(scene, n), flips, th)
            nt = len(reps)
            tile_b = nt * n * n * 2 // 8                         # 2 planes, 1 bit per pixel each
            if n == 16:
                blocks, idx = 0, 2 if nt > 256 or flips else 1  # flip bits need a second byte
                map_b = cells16 * idx
            else:
                # 16x16 metatiles made of 4 (tile, flip) entries, then a byte map of metatiles
                ent = [a for a in assign]
                cols = W // 8
                meta = {}
                for by in range(H // 16):
                    for bx in range(W // 16):
                        q = tuple(ent[(2 * by + dy) * cols + 2 * bx + dx] for dy in (0, 1) for dx in (0, 1))
                        meta.setdefault(q, len(meta))
                blocks = len(meta)
                per = 1 if nt <= 256 and not flips else 2       # entry = tile index (+ 2 flip bits)
                if flips and nt <= 64:
                    per = 1                                     # 6-bit index + 2 flip bits in a byte
                map_b = blocks * 4 * per + cells16 * (1 if blocks <= 256 else 2)
            changed = int((rebuild(reps, assign, n, scene.shape, flips) != scene).sum())
            name = '%dx%d%s thr %d' % (n, n, ' +flips' if flips else '', th)
            print('%-34s %6d %8d %8s %8d %8d  %d' % (name, nt, tile_b, blocks or '-', map_b, tile_b + map_b, changed))
            rows.append((name, n, flips, th, reps, assign))

for name, n, flips, th, reps, assign in rows:
    if (n, flips, th) in ((8, True, 4), (8, True, 8), (8, True, 12)):
        png(rebuild(reps, assign, n, scene.shape, flips), 'merged_8flip_t%d.png' % th)
png(scene, 'original.png')

# ---- fire frames: deltas against frame N-1 and N-2 of the played sequence
NF = define('NFSEQ') if re.search(r'#define NFSEQ', text) else None
seq = array('fire_seq')
fl = [np.array(array('fire_l'))]
nfr = len(re.findall(r'fire_l\w*', text))
fire_l = np.array(array('fire_l'), np.int64)
fire_d = np.array(array('fire_d'), np.int64)
fh = define('FIRE_H')
nf = len(fire_l) // fh
fire_l, fire_d = fire_l.reshape(nf, fh), fire_d.reshape(nf, fh)
raw = nf * fh * 8
print('\nfire: %d distinct frames of 32x%d, sequence %s, raw %d B' % (nf, fh, seq, raw))
for lag in (1, 2):
    tot = 0
    for i in range(len(seq)):
        a, b = seq[i], seq[i - lag]
        n = int((fire_l[a] != fire_l[b]).sum() + (fire_d[a] != fire_d[b]).sum())   # changed longs
        tot += n * 4 + (2 * fh + 7) // 8                        # changed longs + 1 bit per long
    print('delta vs frame N-%d over the sequence: %d B (%d played frames, bitmap + changed longs)'
          % (lag, tot, len(seq)))
