#!/usr/bin/env python3
"""Gariland's battlefield from the local Final Fantasy Tactics disc -> map.h, tiles.bin and the four views
fftv0.bin .. fftv3.bin, ZX0-packed (lib/zx0pack.py: ~10 KB each from 52 KB), never committed.

usage: extract.py DISC.cue [preview.png]       (writes map.h, tiles.bin and fftv*.bin here)
map.h and tiles.bin (the Tile array, 12 bytes a tile, read from ffti: interface.py):
Reads MAP/MAP022.GNS (the map's resource records), assembles the mesh (mesh_blob: the
primary record, type 0x2E01, plus any chunk it leaves to a state record) and
decodes its terrain block (pointer at 0x68): x and z counts, then 2 levels x z x x tiles of
8 bytes (byte 0 & 0x3F surface, 2 height, 3 slope height & 0x1F | depth << 5, 4 slope type,
6 flags: bit 0 blocked). Only level 0 is used (Gariland's level 1 is empty). Format: the
FFT modding community (adamrt/fft_toolkit src/terrain.c), checked on this map (RE_NOTES.md).
Views: the same mesh's textured polygons (pointer at 0x40; texture MAP022.8, 256 x 1024 4-bit;
16 palettes at 0x44) drawn with a depth buffer in the game's projection (24 x 12 tiles, 6 px
per height unit, 4x supersampled), averaged to the TI's pixels and cut into 4 greys by
luminance (thresholds from the whole map). One view = the scene of fft.h (SC_BYTES x SC_H):
light plane, dark plane, then twice (the frontmost depth of each image byte, then the next
one) a plane of depths (the view diagonal u + v of the tile the pixels belong to) and a plane
of the masks of those pixels: a unit is behind them when its own diagonal is smaller (fft.c
cover()). Two depths per byte are exact for 97.5 % of the bytes (Gariland, all views).
The same pass gives each tile's top and four walls a shade for the rotation frames (tiles.bin):
the mean grey of its pixels over the four views, rounded.
"""
import os, struct, sys
import numpy as np
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '../../../.claude/skills/ti-port-ps1/scripts'))
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '../../../lib'))
from psxiso import Disc, walk   # noqa: E402
import zx0pack                  # noqa: E402

MAP = 22                                         # Magic City Gariland


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


def mesh_blob(disc, files):
    """The map's mesh, with every chunk its primary record leaves empty filled in.

    FFT spreads one mesh over the map's GNS records: the primary (type 0x2E01, or the
    first 0x2F01 for the many maps that carry no 0x2E01) holds the day/clear state, and
    each later state record (0x2F01/0x3001, one per time and weather) fills only the
    header pointers it overrides, inheriting the rest. Entrance to Death City (map 53)
    is the one map that leans on this for its geometry: its primary mesh gives the
    texture palettes (0x44) and the terrain (0x68) but leaves the polygon pointer
    (0x40) at 0, and its 1062 polygons live in the next record. Read alone, that
    primary draws nothing at all and the whole map measures as empty.

    The donor record is appended whole and the pointer rebased onto the copy, so every
    reader downstream still works on a single blob. For the 93 other maps the primary
    carries all three chunks and the blob is that record's bytes, byte for byte.
    """
    recs = [r for r in gns_records(disc, files) if r[0] in (0x2E01, 0x2F01, 0x3001)]
    want = [t for t in (0x2E01, 0x2F01, 0x3001) if any(r[0] == t for r in recs)][0]
    first = next(i for i, r in enumerate(recs) if r[0] == want)
    mesh = bytearray(disc.read(*recs[first][1:]))
    for off in (0x40, 0x44, 0x68):                   # geometry, texture palettes, terrain
        if struct.unpack_from('<I', mesh, off)[0]:
            continue
        for r in recs[first + 1:]:
            donor = disc.read(*r[1:])
            p = struct.unpack_from('<I', donor, off)[0]
            if p:
                struct.pack_into('<I', mesh, off, len(mesh) + p)
                mesh += donor
                break
    return bytes(mesh)


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



def terrain(mesh, nx, nz):
    """Per tile and level: walkability and standable top, over BOTH terrain levels.

    walk 0 blocked, 1 ordinary, 2 crossed but never stopped on. FFT gives a walk-2
    tile the standing geometry's own height, not the ground's: Sweegy's trees are
    tiles of height 17-18 over ground near 4.

    Two things that flag does NOT say, both learned the hard way (c2-pipeline.md):

    - a tile with no second level carries eight zero bytes, and reading that as an
      ordinary tile of height 0 collapses the ground under everything. Map 3 has
      117 absent level-1 entries of 117, Chapel of Orbonne 116 of 140.
    - walk 2 is not decoration, it is "you cannot stop here", and you cannot stop
      on a WALL either. So a flagged tile is decoration when it RISES above the
      ordinary ground around it -- one level with the ground carries nothing, and
      Araguay flags two such tiles beside a trunk -- and when its 4-connected
      component among the rising ones is FREE-STANDING: a wall is the room's
      enclosure and runs along the map's edge, furniture stands in the middle.
      A single tile is always free-standing, since three of Araguay's five trees
      grow at the very edge of the map.

      That splits the Chapel of Orbonne's one 33-tile component spanning 13x10 on
      the border, and the Book Storage's 10x2 back wall, from the Book Storage's
      three 2x1 bookcases in the middle of the room and Sweegy's five lone trees.
    """
    p0 = struct.unpack('<I', mesh[0x68:0x6C])[0]
    n = nx * nz
    walk = [0] * n
    top = [None] * n
    flagged = {}
    for level in (0, 1):
        base = p0 + 2 + 8 * n * level
        for i in range(n):
            b = mesh[base + 8 * i:base + 8 * i + 8]
            if not any(b):            # this tile has no such level
                continue
            w = 0 if b[6] & 0x01 else 2 if b[6] & 0x02 else 1
            h = max(corners(b[2], b[3] & 0x1F, b[4] if (b[3] & 0x1F) else 0))
            if w == 2:
                flagged[(i % nx, i // nx)] = h
            elif w == 1 and (top[i] is None or h > top[i]):
                top[i] = h
                walk[i] = 1
    rising = {}
    for (x, z), h in flagged.items():
        near = [top[(z + dz) * nx + x + dx]
                for dz in (-1, 0, 1) for dx in (-1, 0, 1)
                if 0 <= x + dx < nx and 0 <= z + dz < nz
                and top[(z + dz) * nx + x + dx] is not None]
        if near and h > max(near):
            rising[(x, z)] = h
    props = []
    for comp in components(rising):
        edge = any(x in (0, nx - 1) or z in (0, nz - 1) for x, z in comp)
        if len(comp) > 1 and edge:
            continue                  # the room's own enclosure, not furniture
        props += [(x, z, rising[(x, z)]) for x, z in comp]
    return walk, top, props


def components(cells):
    """The 4-connected components of a set of (x, z) tiles."""
    seen, out = set(), []
    for c in cells:
        if c in seen:
            continue
        stack, comp = [c], []
        seen.add(c)
        while stack:
            x, z = stack.pop()
            comp.append((x, z))
            for dx, dz in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                q = (x + dx, z + dz)
                if q in cells and q not in seen:
                    seen.add(q)
                    stack.append(q)
        out.append(comp)
    return out


def prop_polygons(polys, top, props, nx, nz, grow=0.75):
    """Every polygon standing over a decorative tile, trunk and overhanging canopy.

    Removing part of a prop saves nothing: the rest draws the same silhouette and no
    span moves (measured on Sweegy, 48 of 144 tree polygons removed changed the stream
    by zero bytes).

    The ground under the prop is kept: only geometry above the HIGHEST standable
    neighbour goes. FFT gives the prop tile the prop's own height, so the ground
    under it has to be guessed from around it, and the lowest neighbour guesses too
    low -- it took the ground out from under Sweegy's and Araguay's trees and left
    white holes in the floor. The highest neighbour leaves no stump on either. A
    hole that survives this is in the source: the Book Storage's floor under a
    bookcase has no polygon of its own, the object's base draws it.
    """
    out = []
    for x0, z0, _ in props:
        ground = [top[(z0 + dz) * nx + x0 + dx]
                  for dz in (-1, 0, 1) for dx in (-1, 0, 1)
                  if 0 <= x0 + dx < nx and 0 <= z0 + dz < nz
                  and top[(z0 + dz) * nx + x0 + dx] is not None]
        floor = max(ground) if ground else 0
        cx, cz = x0 + .5, z0 + .5
        for i, q in enumerate(polys):
            v = q[0]
            x, z = v[:, 0].mean() / 28, v[:, 2].mean() / 28
            if abs(x - cx) <= grow and abs(z - cz) <= grow and (-v[:, 1] / 12).max() > floor:
                out.append(i)
    return sorted(set(out))


# FFT leaves the skirt under a map's terrain untextured, and polygons() draws an
# untextured polygon black. The PS1's camera never shows it; our four orientations
# do, and on a cliff map it fills the screen -- Zirekile Falls reads as 13,410
# pixels of black with its waterfall a sliver at the top. Dropping it is approved
# art direction (2026-11-25): it frees more depth than every crop put together
# (20 of 27 structural maps fit on it alone), and a large black mass is dead
# weight on a 160x100 four-grey LCD. The playable surface is untouched; what
# changes is that a map no longer sits on a solid block.
DROP_UNDERSIDE = True


# Props the authoring pass has approved dropping, as the tiles they stand on.
# A map over FFT_CORE_DEPTH_MAX whose excess is carried by free-standing scenery
# gives up the FEWEST props that clear it, never all of them: what a prop costs
# is whether it widens a row's span, so one inside the silhouette is free.
# Each line is a decision reviewed on the picture (cosmetic_prop_review.py
# --minimal) and measured by depth_budget.minimal_props(). Dropping these tiles'
# geometry is the only thing this does; no other map is touched.
# Once the underside goes, almost nothing else has to. Roof of Riovanes (24
# spare), Murond Holy Place (1,272) and Araguay Woods (2,160) keep every prop
# they had, so their entries are gone: the bell towers and all five trunks stay.
PROPS_DROPPED = {
    74: [(9, 10)],     # Sweegy Woods: its tallest tree alone, 19,000 -> 16,272 (2,216 spare)
}


def dropped_props(mesh, nx, nz, m=None):
    """Polygon indices this map's approved prop drops remove, [] for every other map."""
    tiles = PROPS_DROPPED.get(MAP if m is None else m)
    if not tiles:
        return []
    _, top, props = terrain(mesh, nx, nz)
    by_tile = {(x, z): (x, z, h) for x, z, h in props}
    missing = [t for t in tiles if t not in by_tile]
    if missing:
        raise ValueError('map %d: no prop on tile(s) %s' % (MAP if m is None else m, missing))
    return prop_polygons(polygons(mesh), top, [by_tile[t] for t in tiles], nx, nz)


K = 4                                           # supersampling
SC_W, SC_OY, BG = 320, 64, 40.0                 # fft.h: SC_BYTES * 8, SC_OY; background marker
SOURCE_CAMERA = False                          # historical controls retain their archived views


def render(polys, tex, pal, rot, nx, nz, sc_h, return_depth=False):
    """One view at K x the TI's pixels: luminance (BG outside the map), the view diagonal of
    the tile each pixel belongs to (-1 outside) and its face: world tile * 5 + 0 top, 1 +x,
    2 -x, 3 +z, 4 -z wall (-1 outside the grid)."""
    W, H = SC_W * K, sc_h * K
    lum = np.full((H, W), BG); diag = np.full((H, W), -1, np.int16); zb = np.full((H, W), -1e9)
    face = np.full((H, W), -1, np.int16)
    lpal = (pal & 31) * .299 + (pal >> 5 & 31) * .587 + (pal >> 10 & 31) * .114
    lpal = lpal * 255 / 31
    vh = nx if rot & 1 else nz
    cam = np.array([1.0, 1.0, 2 * 12 / 28])     # towards the camera in (u, v, height in tiles)
    for v, uv, pl, pg in polys:
        x, z, h = v[:, 0] / 28, v[:, 2] / 28, -v[:, 1] / 12
        u, w = [(x, z), (nz - z, x), (nx - x, nz - z), (z, nx - x)][rot]
        sx = (u - w + vh) * 12 * K
        if SOURCE_CAMERA: sx = SC_W * K - sx
        sy = (SC_OY + (u + w) * 6 - h * 6) * K; d = u + w + 2 * h
        # the tile of a face: its centre pushed into the solid (a wall belongs to the tile
        # behind it, whose side it is)
        p3 = np.stack([u, w, h * 12 / 28], 1)
        n = np.cross(p3[1] - p3[0], p3[2] - p3[0])
        if np.dot(n, cam) < 0: n = -n
        n = n / (np.linalg.norm(n) or 1)
        c = p3.mean(0) - .05 * n
        ut, wt = int(np.floor(c[0])), int(np.floor(c[1]))
        td = ut + wt
        xt, zt = [(ut, wt), (wt, nz - 1 - ut), (nx - 1 - ut, nz - 1 - wt), (nx - 1 - wt, ut)][rot]
        fc = -1
        if 0 <= xt < nx and 0 <= zt < nz:
            f = 0
            if abs(n[2]) < .2:                         # a wall: its outward normal in the world
                dx, dz = [(n[0], n[1]), (n[1], -n[0]), (-n[0], -n[1]), (-n[1], n[0])][rot]
                f = (1 if dx > 0 else 2) if abs(dx) > abs(dz) else (3 if dz > 0 else 4)
            fc = (zt * nx + xt) * 5 + f
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
        face[y0:y1, x0:x1][m] = fc
    return (lum, diag, face, zb) if return_depth else (lum, diag, face)


def views(disc, files, nx, nz, maxh, preview=None, quantize=None, decorate=None):
    """Pack four views. An optional offline quantizer changes colours only;
    coverage/depth packing and the default Gariland conversion stay identical.
    """
    recs = gns_records(disc, files)
    tex_rec = next(r for r in recs if r[0] == 0x1701)              # the first: day, no weather
    t = np.frombuffer(disc.read(tex_rec[1], tex_rec[2]), np.uint8)
    tex = np.empty((1024, 256), np.uint8)
    tex[:, 0::2] = (t & 15).reshape(1024, 128); tex[:, 1::2] = (t >> 4).reshape(1024, 128)
    mesh = mesh_blob(disc, files)
    pal = np.array(struct.unpack_from('<256H', mesh, struct.unpack_from('<I', mesh, 0x44)[0])).reshape(16, 16)
    polys = polygons(mesh)
    # Both index sets are taken against the same unfiltered list.
    drop = set(dropped_props(mesh, nx, nz))
    if DROP_UNDERSIDE:
        drop |= {i for i, q in enumerate(polys) if q[1] is None}
    quant_polys = polys                      # see the percentiles below
    if drop:
        polys = [q for i, q in enumerate(polys) if i not in drop]
    sc_h = 6 * (nx + nz) + 6 * maxh + 8                            # fft.h SC_H
    out, q = [], None
    gsum, gcnt = np.zeros(nx * nz * 5), np.zeros(nx * nz * 5)
    for rot in range(4):
        rendered = render(polys, tex, pal, rot, nx, nz, sc_h, return_depth=decorate is not None)
        lum, diag, face = rendered[:3]
        small = lum.reshape(sc_h, K, SC_W, K).mean((1, 3))
        bg = (lum == BG).reshape(sc_h, K, SC_W, K).mean((1, 3)) > .5
        if quantize is None:
            if q is None:                                          # the whole map, view 0
                # The thresholds are percentiles of the view's own pixels, so
                # removing the underside would re-quantise the WHOLE map: on
                # Gariland the roofs went black. Measure them on the geometry
                # the approved C2 treatment was reviewed against, and let the
                # removal change only what is drawn.
                ql = render(quant_polys, tex, pal, rot, nx, nz, sc_h)[0] \
                    if quant_polys is not polys else lum
                qs = ql.reshape(sc_h, K, SC_W, K).mean((1, 3))
                qbg = (ql == BG).reshape(sc_h, K, SC_W, K).mean((1, 3)) > .5
                q = np.percentile(qs[~qbg], [25, 55, 82])
            g = np.where(small > q[2], 0, np.where(small > q[1], 1, np.where(small > q[0], 2, 3)))
        else:
            g = quantize(small, bg)
        g[bg] = 0                                                  # white around the map (the user's pick over light grey)
        if decorate is not None:
            g = decorate(g, bg, rendered[3], rot, polys, tex, pal)
            g[bg] = 0
        fc = face[K // 2::K, K // 2::K]                            # each face's mean grey
        seen = (fc >= 0) & ~bg
        gsum += np.bincount(fc[seen], g[seen], nx * nz * 5); gcnt += np.bincount(fc[seen], None, nx * nz * 5)
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
    # shades of the rotation frames (art.py turn_pat): the mean grey rounded, 0..3; a face never
    # seen takes its tile's seen walls, else its top
    shade = np.where(gcnt > 0, np.rint(gsum / np.maximum(gcnt, 1)), -1).reshape(nx * nz, 5)
    for t in shade:
        walls = t[1:][t[1:] >= 0]
        t[0] = t[0] if t[0] >= 0 else 3
        t[1:][t[1:] < 0] = np.rint(walls.mean()) if len(walls) else t[0]
    return out, shade.astype(int)


def rowxor_masks(raw):
    """The former chapter views (V2/V3): each occlusion mask row (planes 3 and 5) XORed with
    the row above before ZX0; ragged_depth() replaced them."""
    p = np.frombuffer(raw, np.uint8).reshape(6, -1, 40).copy()
    for k in (3, 5):
        p[k, 1:] ^= p[k, :-1].copy()
    return p.tobytes()


def unrowxor_masks(raw):
    """rowxor_masks() undone (the former V2/V3 banks, converted by view_split.py)."""
    p = np.frombuffer(raw, np.uint8).reshape(6, -1, 40).copy()
    for k in (3, 5):
        for y in range(1, p.shape[1]):
            p[k, y] ^= p[k, y - 1]
    return p.tobytes()


def ragged_depth(raw, row_bytes=40):
    """The depth/mask planes of a 6-plane view as a chapter view's depth stream, unpacked by
    fft.c depth_rows(): a 4-byte record per scene row (u16 0, filled with the row's offset by
    the decoder; u8 x0, u8 w: the even byte span holding the row's mask bits), then each of the
    four planes as its rows' spans one after the other, every mask byte XORed with the byte
    above where both rows' spans cover it (~6 KB less than XORed full rows after ZX0). Bytes
    outside the span are never read (their masks are zero): about half of a full view's RAM."""
    p = np.frombuffer(raw, np.uint8).reshape(6, -1, row_bytes)[2:]
    spans = []
    for y in range(p.shape[1]):
        nz = np.nonzero(p[1, y] | p[3, y])[0]
        x0 = int(nz[0]) & ~1 if len(nz) else 0
        spans.append((x0, ((int(nz[-1]) + 2) & ~1) - x0 if len(nz) else 0))
    out = [bytearray() for _ in range(4)]
    for y, (x0, w) in enumerate(spans):
        for k in range(4):
            seg = p[k, y, x0:x0 + w].copy()
            if k & 1 and y:
                px0, pw = spans[y - 1]
                lo, hi = max(x0, px0), min(x0 + w, px0 + pw)
                if lo < hi:
                    seg[lo - x0:hi - x0] ^= p[k, y - 1, lo:hi]
            out[k] += seg.tobytes()
    return b''.join(bytes((0, 0, x0, w)) for x0, w in spans) + b''.join(out)


def unragged_depth(stream, rows, row_bytes=40):
    """ragged_depth() back to four full planes (zero outside each row's span)."""
    p = np.zeros((4, rows, row_bytes), np.uint8)
    spans = [(stream[4 * y + 2], stream[4 * y + 3]) for y in range(rows)]
    plane = sum(w for _, w in spans)
    assert len(stream) == 4 * rows + 4 * plane, 'ragged depth stream size'
    for k in range(4):
        o = 4 * rows + k * plane
        for y, (x0, w) in enumerate(spans):
            p[k, y, x0:x0 + w] = np.frombuffer(stream[o:o + w], np.uint8)
            if k & 1 and y:
                px0, pw = spans[y - 1]
                lo, hi = max(x0, px0), min(x0 + w, px0 + pw)
                if lo < hi:
                    p[k, y, lo:hi] ^= p[k, y - 1, lo:hi]
            o += w
    return p.tobytes()


def view_bank(tag, colours, stroke, depth, row_bytes=40):
    """R4(40-byte rows)/R5(44): raw colors/mask, compressed ragged depth. Legacy callers
    supply R2/R3 ZX0 colour streams; normalize them before serialization."""
    if tag in (b'R2', b'R3'):
        colours = unzx0(colours)
        stroke = unzx0(stroke) if stroke else b''
    assert row_bytes in (40,44)
    assert len(colours) % (2*row_bytes) == 0 and (not stroke or 2*len(stroke) == len(colours))
    return ((b'R5' if row_bytes==44 else b'R4') + len(colours).to_bytes(2, 'big') + len(stroke).to_bytes(2, 'big')
            + len(depth).to_bytes(2, 'big') + colours + stroke + zx0pack.pack(depth))


def pack_view(raw):
    """R4 view: the two colour planes (copied straight into the scene buffer) and the
    ragged depth stream."""
    plane = len(raw) // 6
    return view_bank(b'R4', raw[:2 * plane], b'', ragged_depth(raw))


def archive_view(bank):
    """Convert a legacy bank losslessly, retaining depth stream and contour bytes.
    Already normalized input is byte-identical; no disc or artwork regeneration."""
    tail=b''
    if bank[-10:-6] in (b'FC01',b'FC02'):
        original,offset,count=struct.unpack('>3H',bank[-6:])
        tail=bank[offset:offset+count];bank=bank[:original]
    if bank[:2] in (b'R4',b'R5'):
        out=bank
    else:
        assert bank[:2] in (b'R2',b'R3'), 'not a ragged view'
        a,b=struct.unpack_from('>2H',bank,2)
        colours=unzx0(bank[8:8+a]);mask=unzx0(bank[8+a:8+a+b]) if b else b''
        assert len(colours)%80==0 and (not mask or 2*len(mask)==len(colours))
        out=b'R4'+struct.pack('>2H',len(colours),len(mask))+bank[6:8]+colours+mask+bank[8+a+b:]
    if tail:
        n=len(out);off=(n+1)&~1
        out+=bytes(off-n)+tail+b'FC02'+struct.pack('>3H',n,off,len(tail))
    assert len(out)<=65512, 'view plus AMS trailer exceeds a variable'
    return out


def unzx0(stream):
    import tempfile
    with tempfile.TemporaryDirectory() as d:
        src = os.path.join(d, 'i'); dst = os.path.join(d, 'o')
        open(src, 'wb').write(stream)
        import subprocess
        subprocess.run([zx0pack.DZX0, '-f', src, dst], check=True, capture_output=True)
        return open(dst, 'rb').read()


def read_view(bank, row_bytes=None):
    """A chapter view bank -> (tag, colours, mask, four full depth/mask planes).
    Colours/mask are raw for R4 and ZX0 streams for legacy R2/R3. Ignore the contour tail."""
    if bank[-10:-6] in (b'FC01', b'FC02'):
        bank = bank[:int.from_bytes(bank[-6:-4], 'big')]
    assert bank[:2] in (b'R2', b'R3', b'R4', b'R5'), 'not a ragged chapter view'
    a, b, n = (int.from_bytes(bank[i:i + 2], 'big') for i in (2, 4, 6))
    if row_bytes is None:row_bytes=44 if bank[:2]==b'R5' else 40
    depth = unzx0(bank[8 + a + b:])
    assert len(depth) == n, 'depth size field'
    rows = (a if bank[:2] in (b'R4',b'R5') else len(unzx0(bank[8:8 + a]))) // (2 * row_bytes)
    return bank[:2], bank[8:8 + a], bank[8 + a:8 + a + b], unragged_depth(depth, rows, row_bytes)


def view_info(bank, max_rows=202, color_max=16544, depth_max=18488):
    """Cold packaging counterpart of core_view_rows; no decompression required."""
    if len(bank)<9 or bank[:2] not in (b'R4',b'R5'):
        raise ValueError('R4/R5 archive colors required')
    colors,mask,depth=struct.unpack_from('>3H',bank,2)
    stride=44 if bank[:2]==b'R5' else 40
    rows,remainder=divmod(colors,2*stride)
    if (not rows or rows>max_rows or remainder or colors>color_max or
        (mask and mask!=colors//2) or depth<4*rows or depth>depth_max or depth&3 or
        8+colors+mask>=len(bank)):
        raise ValueError('View exceeds color/row/depth bounds or is truncated')
    return dict(rows=rows,row_bytes=stride,color_bytes=colors,depth_bytes=depth)


def main():
    disc, files = disc_files(sys.argv[1])
    mesh = mesh_blob(disc, files)
    p = struct.unpack('<I', mesh[0x68:0x6C])[0]
    nx, nz = mesh[p], mesh[p + 1]
    tiles = []
    for i in range(nx * nz):
        b = mesh[p + 2 + 8 * i:p + 10 + 8 * i]
        surf, h, st, sl, flags = b[0] & 0x3F, b[2], b[3] & 0x1F, b[4], b[6]
        c = corners(h, st, sl if st else 0)
        # FFT's blocked bit (none at Gariland: its canals and chimneys are stood on, measured on the
        # original); an untargetable tile (its trees) is crossed, not stopped on: 2
        walk_ok = 0 if flags & 0x01 else 2 if flags & 0x02 else 1
        tiles.append((c, walk_ok, 2 * h + (st if sl else 0), int(surf == 0x0E)))   # 0x0E: water
    maxh = max(max(c) for c, *_ in tiles)
    raw, shade = views(disc, files, nx, nz, maxh, sys.argv[2] if len(sys.argv) > 2 else None)
    f = open('map.h', 'w')
    def print(*a):
        f.write(' '.join(a) + '\n')
    print('// Generated by tools/extract.py from the local FFT disc (MAP%03d, Magic City Gariland):' % MAP)
    print('// never committed. Corners (x,z) (x+1,z) (x+1,z+1) (x,z+1) in height units, shades of the')
    print('// top and the +x -x +z -z walls (turn_pat), walkable (2: crossed only), standing height in half\n// units, water.')
    print('#define MAP_W %d\n#define MAP_H %d   // the tiles (Tile): tiles.bin, in ffti (interface.py)' % (nx, nz))
    f.close()
    open('tiles.bin', 'wb').write(bytes(v for i in range(nx * nz) for v in (*tiles[i][0], *shade[i], *tiles[i][1:])))
    from concurrent.futures import ThreadPoolExecutor             # the packer is slow: 4 at once
    with ThreadPoolExecutor(4) as ex:
        for r, data in enumerate(ex.map(zx0pack.pack, raw)):
            open('fftv%d.bin' % r, 'wb').write(data)


if __name__ == '__main__':
    main()
