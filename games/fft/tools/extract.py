#!/usr/bin/env python3
"""Gariland's battlefield from the local Final Fantasy Tactics disc -> map.h and the four views
fftv0.bin .. fftv3.bin, ZX0-packed (lib/zx0pack.py: ~10 KB each from 52 KB), never committed.

usage: extract.py DISC.cue [preview.png]       (writes map.h and fftv*.bin here)
map.h:
Reads MAP/MAP022.GNS (the map's resource records), takes the primary mesh (type 0x2E01) and
decodes its terrain block (pointer at 0x68): x and z counts, then 2 levels x z x x tiles of
8 bytes (byte 0 & 0x3F surface, 2 height, 3 slope height & 0x1F | depth << 5, 4 slope type,
6 flags: bit 6 can't walk). Only level 0 is used (Gariland's level 1 is empty). Format: the
FFT modding community (adamrt/fft_toolkit src/terrain.c), checked on this map (RE_NOTES.md).
Views: the same mesh's textured polygons (pointer at 0x40; texture MAP022.8, 256 x 1024 4-bit;
16 palettes at 0x44) drawn with a depth buffer in the game's projection (24 x 12 tiles, 6 px
per height unit, 4x supersampled), averaged to the TI's pixels and cut into 4 greys by
luminance (thresholds from the whole map). One view = the scene of fft.h (SC_BYTES x SC_H):
light plane, dark plane, then twice (the frontmost depth of each image byte, then the next
one) a plane of depths (the view diagonal u + v of the tile the pixels belong to) and a plane
of the masks of those pixels: a unit is behind them when its own diagonal is smaller (fft.c
cover()). Two depths per byte are exact for 97.5 % of the bytes (Gariland, all views).
"""
import os, struct, sys
import numpy as np
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '../../../.claude/skills/ti-port-ps1/scripts'))
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '../../../lib'))
from psxiso import Disc, walk   # noqa: E402
import zx0pack                  # noqa: E402

MAP = 22                                         # Magic City Gariland
# surface -> (top material, side material) of fft.h (MT_*, SD_*)
SURF = {0x03: ('GRASS', 'DIRT'), 0x0E: ('WATER', 'STONE'), 0x14: ('WOOD', 'WOOD'),
        0x15: ('STONE', 'STONE'), 0x16: ('ROOF', 'HOUSE'), 0x1E: ('TREE', 'LEAF'),
        0x1F: ('BOX', 'WOOD'), 0x21: ('BRICK', 'BRICK'), 0x23: ('WOOD', 'WOOD')}


def edge(sl, shift):
    return (sl >> shift) & 3


def corners(h, st, sl):
    """Heights of the corners (x,z) (x+1,z) (x+1,z+1) (x,z+1). The slope byte holds four 2-bit
    edges, N S W E from the top bits (0 low, 1 half, 2 high); N is +z and E is +x (checked:
    every incline's high side meets the neighbour at h + st)."""
    if not sl:
        return [h] * 4
    n, s, w, e = edge(sl, 6), edge(sl, 4), edge(sl, 2), edge(sl, 0)
    ends = {(0, 0): (s, w), (1, 0): (s, e), (1, 1): (n, e), (0, 1): (n, w)}
    whole = 2 in (n, s, w, e)
    out = []
    for c in ((0, 0), (1, 0), (1, 1), (0, 1)):
        a, b = ends[c]
        high = (2 in (a, b)) if whole else (a == 1 and b == 1)
        out.append(h + st if high else h)
    return out


def disc_files(cue):
    disc = Disc(cue)
    root = disc.sector(16)[156:190]
    files = []
    walk(disc, struct.unpack_from('<I', root, 2)[0], struct.unpack_from('<I', root, 10)[0], '', files)
    return disc, {p: (lba, size) for p, lba, size in files}


def gns_records(disc, files):
    """(type, sector, size) of MAP022.GNS's records until the end record."""
    gns = disc.read(*files['MAP/MAP%03d.GNS' % MAP])
    out = []
    for i in range(0, len(gns) - 20, 20):
        t = struct.unpack('<H', gns[i + 4:i + 6])[0]
        if t == 0x3101:
            break
        out.append((t,) + struct.unpack('<II', gns[i + 8:i + 16]))
    return out


def polygons(mesh):
    """The primary mesh's triangles: (3 x (x, y, z), 3 x (u, v) or None, palette, page);
    quads ABCD split into ABC and BDC (the PS1's order). Untextured polygons are black."""
    p = struct.unpack_from('<I', mesh, 0x40)[0]
    ntt, ntq, nut, nuq = struct.unpack_from('<4H', mesh, p)
    o = p + 8
    def verts(n, k):
        nonlocal o
        a = np.array(struct.unpack_from('<%dh' % (n * k * 3), mesh, o), float).reshape(n, k, 3)
        o += n * k * 6
        return a
    tt, tq, ut, uq = verts(ntt, 3), verts(ntq, 4), verts(nut, 3), verts(nuq, 4)
    o += (ntt * 3 + ntq * 4) * 6                     # normals
    out = []
    for v in tt:
        b = mesh[o:o + 10]; o += 10
        out.append((v, [(b[0], b[1]), (b[4], b[5]), (b[8], b[9])], b[2] & 15, b[6] & 3))
    for v in tq:
        b = mesh[o:o + 12]; o += 12
        uv = [(b[0], b[1]), (b[4], b[5]), (b[8], b[9]), (b[10], b[11])]
        out.append((v[[0, 1, 2]], [uv[0], uv[1], uv[2]], b[2] & 15, b[6] & 3))
        out.append((v[[1, 3, 2]], [uv[1], uv[3], uv[2]], b[2] & 15, b[6] & 3))
    for v in ut:
        out.append((v, None, 0, 0))
    for v in uq:
        out.append((v[[0, 1, 2]], None, 0, 0)); out.append((v[[1, 3, 2]], None, 0, 0))
    return out


K = 4                                           # supersampling
SC_W, SC_OY, BG = 320, 64, 40.0                 # fft.h: SC_BYTES * 8, SC_OY; background marker


def render(polys, tex, pal, rot, nx, nz, sc_h):
    """One view at K x the TI's pixels: luminance (BG outside the map) and the view diagonal
    of the tile each pixel belongs to (-1 outside)."""
    W, H = SC_W * K, sc_h * K
    lum = np.full((H, W), BG); diag = np.full((H, W), -1, np.int16); zb = np.full((H, W), -1e9)
    lpal = (pal & 31) * .299 + (pal >> 5 & 31) * .587 + (pal >> 10 & 31) * .114
    lpal = lpal * 255 / 31
    vh = nx if rot & 1 else nz
    cam = np.array([1.0, 1.0, 2 * 12 / 28])     # towards the camera in (u, v, height in tiles)
    for v, uv, pl, pg in polys:
        x, z, h = v[:, 0] / 28, v[:, 2] / 28, -v[:, 1] / 12
        u, w = [(x, z), (nz - z, x), (nx - x, nz - z), (z, nx - x)][rot]
        sx = (u - w + vh) * 12 * K; sy = (SC_OY + (u + w) * 6 - h * 6) * K; d = u + w + 2 * h
        # the tile of a face: its centre pushed into the solid (a wall belongs to the tile
        # behind it, whose side it is)
        p3 = np.stack([u, w, h * 12 / 28], 1)
        n = np.cross(p3[1] - p3[0], p3[2] - p3[0])
        if np.dot(n, cam) < 0: n = -n
        n = n / (np.linalg.norm(n) or 1)
        c = p3.mean(0) - .05 * n
        td = int(np.floor(c[0])) + int(np.floor(c[1]))
        x0, x1 = max(int(np.floor(sx.min())), 0), min(int(np.ceil(sx.max())) + 1, W)
        y0, y1 = max(int(np.floor(sy.min())), 0), min(int(np.ceil(sy.max())) + 1, H)
        if x0 >= x1 or y0 >= y1:
            continue
        X, Y = np.meshgrid(np.arange(x0, x1) + .5, np.arange(y0, y1) + .5)
        (ax, bx, cx), (ay, by, cy) = sx, sy
        den = (by - cy) * (ax - cx) + (cx - bx) * (ay - cy)
        if abs(den) < 1e-9:
            continue
        l0 = ((by - cy) * (X - cx) + (cx - bx) * (Y - cy)) / den
        l1 = ((cy - ay) * (X - cx) + (ax - cx) * (Y - cy)) / den
        l2 = 1 - l0 - l1
        m = (l0 >= -1e-6) & (l1 >= -1e-6) & (l2 >= -1e-6)
        D = l0 * d[0] + l1 * d[1] + l2 * d[2]
        m &= D > zb[y0:y1, x0:x1]
        if uv is None:
            col = np.zeros(X.shape)
        else:
            U = np.clip((l0 * uv[0][0] + l1 * uv[1][0] + l2 * uv[2][0]).astype(int), 0, 255)
            V = np.clip((l0 * uv[0][1] + l1 * uv[1][1] + l2 * uv[2][1]).astype(int), 0, 255) + pg * 256
            idx = tex[V, U]
            m &= pal[pl][idx] != 0                     # colour 0: transparent
            col = lpal[pl][idx]
        zb[y0:y1, x0:x1][m] = D[m]; lum[y0:y1, x0:x1][m] = col[m]; diag[y0:y1, x0:x1][m] = td
    return lum, diag


def views(disc, files, nx, nz, maxh, preview=None):
    recs = gns_records(disc, files)
    tex_rec = next(r for r in recs if r[0] == 0x1701)              # the first: day, no weather
    mesh_rec = next(r for r in recs if r[0] == 0x2E01)
    t = np.frombuffer(disc.read(tex_rec[1], tex_rec[2]), np.uint8)
    tex = np.empty((1024, 256), np.uint8)
    tex[:, 0::2] = (t & 15).reshape(1024, 128); tex[:, 1::2] = (t >> 4).reshape(1024, 128)
    mesh = disc.read(mesh_rec[1], mesh_rec[2])
    pal = np.array(struct.unpack_from('<256H', mesh, struct.unpack_from('<I', mesh, 0x44)[0])).reshape(16, 16)
    polys = polygons(mesh)
    sc_h = 6 * (nx + nz) + 6 * maxh + 8                            # fft.h SC_H
    out, q = [], None
    for rot in range(4):
        lum, diag = render(polys, tex, pal, rot, nx, nz, sc_h)
        small = lum.reshape(sc_h, K, SC_W, K).mean((1, 3))
        bg = (lum == BG).reshape(sc_h, K, SC_W, K).mean((1, 3)) > .5
        if q is None:                                              # the whole map, view 0
            q = np.percentile(small[~bg], [25, 55, 82])
        g = np.where(small > q[2], 0, np.where(small > q[1], 1, np.where(small > q[0], 2, 3)))
        g[bg] = 1
        d = diag[K // 2::K, K // 2::K]                             # each pixel's centre
        bits = 0x80 >> (np.arange(SC_W) & 7)
        def pack(a):
            return np.bitwise_or.reduceat(np.where(a, bits, 0), np.arange(0, SC_W, 8), axis=1).astype(np.uint8)
        layers = []
        for _ in range(2):                                         # the two frontmost depths
            thr = d.reshape(sc_h, SC_W // 8, 8).max(2)
            front = (d == np.repeat(thr, 8, 1)) & (d >= 0)
            layers += [np.maximum(thr, 0).astype(np.uint8), pack(front)]
            d = np.where(front, -1, d)
        out.append(b''.join(a.tobytes() for a in [pack(g & 1), pack(g >> 1)] + layers))
        if preview:
            from PIL import Image
            Image.fromarray(np.array([255, 170, 85, 0], np.uint8)[g]).save(preview.replace('.png', '%d.png' % rot))
    return out


def main():
    disc, files = disc_files(sys.argv[1])
    _, sector, size = [r for r in gns_records(disc, files) if r[0] == 0x2E01][-1]
    mesh = disc.read(sector, size)
    p = struct.unpack('<I', mesh[0x68:0x6C])[0]
    nx, nz = mesh[p], mesh[p + 1]
    tiles = []
    for i in range(nx * nz):
        b = mesh[p + 2 + 8 * i:p + 10 + 8 * i]
        surf, h, st, sl, flags = b[0] & 0x3F, b[2], b[3] & 0x1F, b[4], b[6]
        top, side = SURF[surf]
        c = corners(h, st, sl if st else 0)
        walk_ok = not (flags & 0x40) and surf not in (0x1E, 0x21)   # trees, chimneys: blocks
        tiles.append((c, top, side, walk_ok, 2 * h + (st if sl else 0)))
    f = open('map.h', 'w')
    def print(*a):
        f.write(' '.join(a) + '\n')
    print('// Generated by tools/extract.py from the local FFT disc (MAP%03d, Magic City Gariland):' % MAP)
    print('// never committed. Corners (x,z) (x+1,z) (x+1,z+1) (x,z+1) in height units, top and side')
    print('// materials, walkable, standing height in half units.')
    print('#define MAP_W %d\n#define MAP_H %d' % (nx, nz))
    print('static const Tile map_tiles[MAP_W * MAP_H] = {')
    for z in range(nz):
        row = tiles[z * nx:(z + 1) * nx]
        print('    ' + ' '.join('{{%d,%d,%d,%d},MT_%s,SD_%s,%d,%d},' % (*c, t, s, w, st)
                                 for c, t, s, w, st in row))
    print('};')
    f.close()
    maxh = max(max(c) for c, *_ in tiles)
    from concurrent.futures import ThreadPoolExecutor             # the packer is slow: 4 at once
    raw = views(disc, files, nx, nz, maxh, sys.argv[2] if len(sys.argv) > 2 else None)
    with ThreadPoolExecutor(4) as ex:
        for r, data in enumerate(ex.map(zx0pack.pack, raw)):
            open('fftv%d.bin' % r, 'wb').write(data)


if __name__ == '__main__':
    main()
