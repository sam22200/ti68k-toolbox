#!/usr/bin/env python3
"""The world map's bank (fftmap, campaign/worldmap.c) from the local FFT disc -> ffwm.bin (never
committed). The user chose a simplified geography: Ivalice's land and sea from the original map,
without the parchment's lines and lettering, our own markers, roads and labels.

usage: world_assets.py DISC.cue OUT.bin [preview.png]
Sources (layouts named after adamrt/fft_decomp, checked here on the disc and the original run):
- WORLD/WLDTEX.TM2: the map texture, a stream of 2048-byte sectors loaded into VRAM, each a
  12-byte header (type: 1/2 the last sector of a rectangle, x, y, words left in the row) then
  1010 words; each rectangle starts with its 256-colour palette. Its four 8-bit pieces make the
  map: 240 + 256 columns, 241 + 132 rows; each piece's first 8 columns are not map (filled from
  the next column here).
- WORLD/WLDCORE.BIN (loaded at 0x80067000): the 43 locations' map positions and dot kinds
  (s_world_map_dot_source_data, 0x2ddfc: s16 x and kind bytes, then y), the 48 routes
  (g_world_map_route_table at 0x2e008: count, start, end, cost, then count points of 12 bytes,
  x and y packed in the first word). A map position + (263, 186) is its texture pixel (Gariland
  and Mandalia on the original's screen).
- WORLD/WORLD.BIN: the locations' names (0x8e1a0, FFT's text, 0xFE between them).
Land is told from sea by the texture's blue against its red (ochre land, grey-green sea),
filtered (median 9), opened and closed, specks below 60 pixels dropped; drawn with 16-pixel
periodic motifs (greys()), the rarest of the coast's tiles merged into their nearest down to 255.
Layout, big-endian: "FWM1", u16 offsets: tiles, cells, locations, routes, story; the map's
width and height in tiles; tiles in ExtGraph's TileMap order (16 dark / light u16 pairs);
cells u8; locations 43 x (s16 x, y texture pixels, kind 1 town / 2 field, name offset);
routes 48 x (start, end, cost, points, offset of its points: s16 x, y); story: u8 count, then
6 bytes each (location, story flag + 1 that shows it, u16 battle door, flag + 1 it sets, 0). Strings NUL-terminated ASCII after them.
"""
from pathlib import Path
import struct
import sys

import numpy as np
from PIL import Image
from scipy import ndimage

import extract
import management_data

WLDCORE = 0x80067000
DOTS, ROUTES, NAMES = 0x2ddfc, 0x2e008, 0x8e1a0
ORIGIN = (263, 186)
TILES_MAX = 255
# Chapter I as far as it is ripped: after Gariland's victory (flag 1) its town and Mandalia
# Plains, the next battle, show, as on the original's map then. Per location: the story flag + 1
# that shows it, the battle program's door it enters (fftbat; 0 none: not ripped yet) and the
# flag + 1 that battle's victory sets (then the location is a town or a field again).
STORY = [(6, 2, 200, 2), (24, 2, 0, 0)]


def s16(v):
    return v - 0x10000 if v & 0x8000 else v


def texture(disc, files):
    b = disc.read(*files['WORLD/WLDTEX.TM2'])
    P = 1010
    secs = [(struct.unpack_from('<I4H', b, o), np.frombuffer(b[o + 12:o + 12 + 2 * P], '<u2'))
            for o in range(0, len(b), 2048)]
    groups, cur = [], []
    for s in secs:
        cur.append(s)
        if s[0][0] in (1, 2):
            groups.append(cur); cur = []
    pieces = []
    for g in groups[:4]:
        (_, _, _, cw, _), d0 = g[0]
        clut = d0[:cw].astype(int)
        (_, x1, y1, w1, _), _ = g[1]
        (_, x2, y2, _, _), _ = g[2]
        W = (P - (x2 - x1)) // (y2 - y1)
        words = np.concatenate([d0[cw:]] + [d for _, d in g[1:]])
        H = len(words) // W
        idx = words[:H * W].reshape(H, W).view(np.uint8).reshape(H, W * 2)
        rgb = np.stack([(clut & 31) << 3, (clut >> 5 & 31) << 3, (clut >> 10 & 31) << 3], 1)
        pieces.append(rgb[idx].astype(float))
    top = np.concatenate([pieces[0][:241], pieces[1][:241]], 1)
    bottom = np.concatenate([pieces[2][:132], pieces[3][:132]], 1)
    m = np.concatenate([top, bottom], 0)
    for x0 in (0, 240):
        m[:, x0:x0 + 8] = m[:, x0 + 8:x0 + 9]
    return m


def land_mask(m):
    q = ndimage.median_filter(m[..., 2] / np.maximum(m[..., 0], 1), size=9)
    land = ndimage.binary_closing(ndimage.binary_opening(q < 0.47, iterations=2), iterations=2)
    for mask, keep in ((land, True), (~land, False)):
        lab, n = ndimage.label(mask)
        sizes = ndimage.sum(mask, lab, range(1, n + 1))
        small = np.isin(lab, 1 + np.where(sizes < 60)[0])
        land = land & ~small if keep else land | small
    return land


def motif(shape, points):
    """A 16-pixel periodic motif: aligned on the tiles, the open sea's and land's tiles repeat."""
    p = np.zeros((16, 16), bool)
    for x, y in points:
        p[y % 16, x % 16] = True
    yy, xx = np.mgrid[:shape[0], :shape[1]]
    return p[yy % 16, xx % 16]


WAVE = [(0, 0), (1, -1), (2, -1), (3, 0), (4, 0), (5, -1)]     # a small '~'


def greys(land):
    """Light-grey land with a sparse dark stipple, dark-grey sea with light waves, shallows
    checkered light along the coast (3 pixels), a black coast (the user's simplified geography,
    then "un peu de motif")."""
    h, w = land.shape
    pad = np.zeros(((h + 15) // 16 * 16, (w + 15) // 16 * 16), bool)
    pad[:h, :w] = land
    yy, xx = np.mgrid[:pad.shape[0], :pad.shape[1]]
    coast = pad & ~ndimage.binary_erosion(pad, border_value=1)
    near = ndimage.distance_transform_edt(~pad) <= 3
    g = np.where(pad, 1, 2)
    g[~pad & motif(pad.shape, [(2 + x, 3 + y) for x, y in WAVE] + [(10 + x, 11 + y) for x, y in WAVE])] = 1
    g[~pad & near & ((xx + yy) % 2 == 0)] = 1
    g[pad & motif(pad.shape, [(5, 6), (13, 14), (1, 12), (9, 2)])] = 2
    g[coast] = 3
    return g


def tiles_of(g):
    ty, tx = g.shape[0] // 16, g.shape[1] // 16
    cells = [g[y * 16:y * 16 + 16, x * 16:x * 16 + 16] for y in range(ty) for x in range(tx)]
    uniq, index = {}, []
    for c in cells:
        k = c.tobytes()
        if k not in uniq:
            uniq[k] = len(uniq)
        index.append(uniq[k])
    tiles = [np.frombuffer(k, g.dtype).reshape(16, 16) for k in uniq]
    count = np.bincount(index, minlength=len(tiles))
    alias = list(range(len(tiles)))
    live = set(range(len(tiles)))
    while len(live) > TILES_MAX:               # the rarest tile into its nearest: a coast pixel
        a = min(live, key=lambda t: count[t])
        b = min((t for t in live if t != a), key=lambda t: int((tiles[t] != tiles[a]).sum()))
        live.discard(a); count[b] += count[a]
        alias = [b if v == a else v for v in alias]
    order = sorted(live)
    renum = {t: i for i, t in enumerate(order)}
    return [tiles[t] for t in order], bytes(renum[alias[i]] for i in index), tx, ty


def tile_bytes(t):
    out = bytearray()
    for row in t:
        light = sum(1 << (15 - x) for x in range(16) if row[x] & 1)
        dark = sum(1 << (15 - x) for x in range(16) if row[x] & 2)
        out += struct.pack('>HH', dark, light)
    return bytes(out)


def main():
    cue, out = sys.argv[1], Path(sys.argv[2])
    disc, files = extract.disc_files(cue)
    core = disc.read(*files['WORLD/WLDCORE.BIN'])
    world = disc.read(*files['WORLD/WORLD.BIN'])
    m = texture(disc, files)
    land = land_mask(m)
    g = greys(land)
    tiles, cells, tw, th = tiles_of(g)
    names = management_data.names(world, NAMES, 43)
    assert (names[6], names[24]) == ('Magic City Gariland', 'Mandalia Plains'), names[:8]
    locs = []
    for i in range(43):
        a, y = struct.unpack_from('<II', core, DOTS + 8 * i)
        locs.append((s16(a & 0xFFFF) + ORIGIN[0], s16(y & 0xFFFF) + ORIGIN[1], (a >> 16) & 255))
    routes = []
    for r in range(48):
        p = struct.unpack_from('<I', core, ROUTES + 4 * r)[0] - WLDCORE
        n, s, e, cost = core[p:p + 4]
        pts = []
        for k in range(n):
            v = struct.unpack_from('<I', core, p + 4 + 12 * k)[0]
            pts.append((s16(v & 0xFFFF) + ORIGIN[0], s16(v >> 16) + ORIGIN[1]))
        near = lambda a, b: abs(a[0] - b[0]) <= 3 and abs(a[1] - b[1]) <= 3   # (Bariaus Hill's: 2 off)
        assert near(pts[0], locs[s]) and near(pts[-1], locs[e]), ('route ends', r)
        routes.append((s, e, cost, pts))
    body = bytearray(b'FWM1' + bytes(10) + struct.pack('>BB', tw, th))
    offs = []

    def section():
        if len(body) & 1: body.append(0)
        offs.append(len(body))

    section()
    for t in tiles: body += tile_bytes(t)
    section(); body += cells
    section(); loc_at = len(body); body += bytes(8 * 43)
    section(); route_at = len(body); body += bytes(6 * 48)
    section(); body += bytes([len(STORY)]) + b''.join(struct.pack('>BBHBx', *e) for e in STORY)
    for r, (s, e, cost, pts) in enumerate(routes):
        if len(body) & 1: body.append(0)
        struct.pack_into('>4BH', body, route_at + 6 * r, s, e, cost, len(pts), len(body))
        for x, y in pts: body += struct.pack('>hh', x, y)
    for i, (x, y, kind) in enumerate(locs):
        struct.pack_into('>hhBBH', body, loc_at + 8 * i, x, y, kind, 0, len(body))
        body += names[i].encode('ascii') + b'\0'
    struct.pack_into('>5H', body, 4, *offs)
    assert len(body) < 65000
    out.write_bytes(bytes(body))
    if len(sys.argv) > 3:
        Image.fromarray(np.array([255, 170, 85, 0], np.uint8)[g]).save(sys.argv[3])
    print('%s: %d bytes (%d tiles, %dx%d cells, 43 locations, 48 routes)' % (out.name, len(body), len(tiles), tw, th))


if __name__ == '__main__':
    main()
