#!/usr/bin/env python3
"""Battle-map budget census over every map the retail game references (maps.json from
tools/map_layers.py: event and random-encounter maps), against the TI core envelopes:
scene width (SC_W 320 px), scene rows (SC_HMAX), ragged depth bytes (FFT_CORE_DEPTH_MAX)
and the ring-backup footprint. Pass 1 projects every mesh vertex in the four views (fast,
unclipped); pass 2 (--render N or --maps) rasterizes the N largest maps on a widened canvas
like extract.views() to count their ragged depth bytes. Estimates for planning, read from
the local disc: the shipped views also crop and restyle (campaign_scenery.py).

usage: map_budget.py [--render N] [--maps 3,50,51] [--out campaign/x/map-budget.json]
"""
import argparse
import json
import re
import struct
import sys
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE / 'tools'))
import core_audit  # noqa: E402
import extract     # noqa: E402

MAPS = HERE / 'campaign/x/core-profile-deep/maps.json'


def define(path, name):
    return int(re.search(r'^#define ' + name + r' (\d+)', (HERE / path).read_text(), re.M)[1])


SC_HMAX = define('chapters.h', 'SC_HMAX')
DEPTH_MAX = define('core_limits.h', 'FFT_CORE_DEPTH_MAX')


def mesh_record(disc, files, m):
    """The map's primary mesh: the first 0x2E01/0x2F01 record whose polygons are present."""
    extract.MAP = m
    for r in extract.gns_records(disc, files):
        if r[0] in (0x2E01, 0x2F01) and struct.unpack_from('<I', disc.read(r[1], r[2]), 0x40)[0]:
            mesh = disc.read(r[1], r[2])
            if extract.polygons(mesh):
                return r
    raise ValueError('map %d: no primary mesh' % m)


def load(disc, files, m):
    mesh_rec = mesh_record(disc, files, m)
    mesh = disc.read(mesh_rec[1], mesh_rec[2])
    p = struct.unpack('<I', mesh[0x68:0x6C])[0]
    nx, nz = mesh[p], mesh[p + 1]
    maxh = 0
    for i in range(nx * nz):
        b = mesh[p + 2 + 8 * i:p + 10 + 8 * i]
        h, st, sl = b[2], b[3] & 0x1F, b[4]
        maxh = max(maxh, max(extract.corners(h, st, sl if st else 0)))
    return mesh, nx, nz, maxh


def bounds(polys, nx, nz):
    """Unclipped pixel bounds (x0, x1, y0, y1) of each view, extract.render()'s projection."""
    v = np.concatenate([p[0] for p in polys])
    x, z, h = v[:, 0] / 28, v[:, 2] / 28, -v[:, 1] / 12
    out = []
    for rot in range(4):
        vh = nx if rot & 1 else nz
        u, w = [(x, z), (nz - z, x), (nx - x, nz - z), (z, nx - x)][rot]
        sx = (u - w + vh) * 12
        sy = extract.SC_OY + (u + w) * 6 - h * 6
        out.append((float(sx.min()), float(sx.max()), float(sy.min()), float(sy.max())))
    return out


def ragged(disc, files, m, nx, nz, b):
    """Ragged depth bytes per view (extract.ragged_depth) on a canvas holding the whole map,
    rows cropped to the content of all four views (the chapter's SC_TOP/SC_H)."""
    pad = 8
    left = -8 * np.ceil((pad - min(0.0, min(x0 for x0, _, _, _ in b))) / 8)   # whole bytes
    width = max(x1 for _, x1, _, _ in b) - left + pad
    top = min(y0 for _, _, y0, _ in b) - pad
    rows = int(np.ceil(max(y1 for _, _, _, y1 in b) - top + pad))
    old = extract.SC_W, extract.SC_OY
    extract.SC_W = int(np.ceil(width / 16)) * 16
    extract.SC_OY = old[1] - top
    extract.MAP = m
    try:
        recs = extract.gns_records(disc, files)
        tex_rec = next(r for r in recs if r[0] == 0x1701)
        mesh_rec = next(r for r in recs if r[0] in (0x2E01, 0x2F01))
        t = np.frombuffer(disc.read(tex_rec[1], tex_rec[2]), np.uint8)
        tex = np.empty((1024, 256), np.uint8)
        tex[:, 0::2] = (t & 15).reshape(1024, 128); tex[:, 1::2] = (t >> 4).reshape(1024, 128)
        mesh = disc.read(mesh_rec[1], mesh_rec[2])
        pal = np.array(struct.unpack_from('<256H', mesh, struct.unpack_from('<I', mesh, 0x44)[0])).reshape(16, 16)
        polys = extract.polygons(mesh)
        views, used = [], []
        K, W = extract.K, extract.SC_W
        for rot in range(4):
            lum, diag, _ = extract.render(polys, tex, pal, rot, nx, nz, rows)
            bg = (lum == extract.BG).reshape(rows, K, W, K).mean((1, 3)) > .5
            d = diag[K // 2::K, K // 2::K]
            bits = 0x80 >> (np.arange(W) & 7)
            def pack(a):
                return np.bitwise_or.reduceat(np.where(a, bits, 0), np.arange(0, W, 8), axis=1).astype(np.uint8)
            masks = []
            for _ in range(2):
                thr = d.reshape(rows, W // 8, 8).max(2)
                front = (d == np.repeat(thr, 8, 1)) & (d >= 0)
                masks.append(pack(front))
                d = np.where(front, -1, d)
            views.append(masks[0] | masks[1])
            used.append(np.nonzero((~bg).any(1))[0])
        if not any(len(u) for u in used):
            return None                                 # nothing drawn (no textured polygon)
        r0 = min(u[0] for u in used if len(u)); r1 = max(u[-1] for u in used if len(u)) + 1
        lb = int(-left) // 8
        def size(rows_, cols):
            total = 0
            for y in rows_:
                nzc = np.nonzero(cols(y))[0]
                if len(nzc):
                    x0 = int(nzc[0]) & ~1
                    total += ((int(nzc[-1]) + 2) & ~1) - x0
            return 4 * len(rows_) + 4 * total
        full = [size(range(r0, r1), lambda y: mk[y]) for mk in views]
        # the TI canvas: x in [0, 320) as extract.views() places it, the top content row first,
        # at most SC_HMAX rows (the chapter crops the rest)
        shipped = [size(range(r0, min(r1, r0 + SC_HMAX)), lambda y: mk[y][lb:lb + 40]) for mk in views]
        return full, shipped, r1 - r0, extract.SC_W
    finally:
        extract.SC_W, extract.SC_OY = old


def archive(disc, files, m):
    """Estimated archive bytes of the map's four R2 views (extract.views() default four greys,
    rows cropped to the first SC_HMAX content rows, extract.pack_view): ZX0 is slow, about 10 s
    a map."""
    mesh, nx, nz, maxh = load(disc, files, m)
    rec, records = mesh_record(disc, files, m), extract.gns_records
    # extract.views() reads the first 0x2E01 record: hand it this map's primary mesh
    extract.gns_records = lambda d, f: [(0x2E01,) + rec[1:]] + [r for r in records(d, f) if r[0] != 0x2E01]
    try:
        raw, _ = extract.views(disc, files, nx, nz, maxh)
    finally:
        extract.gns_records = records
    total = []
    for v in raw:
        p = np.frombuffer(v, np.uint8).reshape(6, -1, 40)
        used = np.nonzero(p[:2].any((0, 2)) | p[3].any(1) | p[5].any(1))[0]
        r0 = int(used[0]) if len(used) else 0
        total.append(len(extract.pack_view(p[:, r0:r0 + SC_HMAX].tobytes())))
    return total


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('disc', nargs='?', type=Path, default=core_audit.DEFAULT_DISC)
    ap.add_argument('--render', type=int, default=0, help='rasterize the N largest maps')
    ap.add_argument('--maps', default='', help='also rasterize these map numbers')
    ap.add_argument('--archive', default='', help='estimate the archive bytes of these maps\' views')
    ap.add_argument('--out', type=Path, default=HERE / 'campaign/x/map-budget.json')
    args = ap.parse_args()
    disc, files = extract.disc_files(str(args.disc))
    rows = []
    for entry in json.loads(MAPS.read_text())['maps']:
        m = entry['map']
        mesh, nx, nz, maxh = load(disc, files, m)
        b = bounds(extract.polygons(mesh), nx, nz)
        width = max(x1 - min(x0, 0.0) for x0, x1, _, _ in b)
        scene_rows = int(np.ceil(max(y1 for *_, y1 in b) - min(y0 for _, _, y0, _ in b)))
        rows.append(dict(map=m, nx=nx, nz=nz, maxh=maxh, width_px=round(width, 1),
                         fits_width=all(x0 >= 0 and x1 <= 320 for x0, x1, _, _ in b),
                         scene_rows=scene_rows, fits_rows=scene_rows <= SC_HMAX))
    rows.sort(key=lambda r: -(r['scene_rows'] * r['width_px']))
    chosen = {r['map'] for r in rows[:args.render]} | {int(m) for m in args.maps.split(',') if m}
    for r in rows:
        if r['map'] not in chosen:
            continue
        mesh, nx, nz, _ = load(disc, files, r['map'])
        b = bounds(extract.polygons(mesh), nx, nz)
        got = ragged(disc, files, r['map'], nx, nz, b)
        if got is None:
            r.update(depth_bytes=None); print('map %3d: nothing drawn' % r['map']); continue
        full, sizes, content_rows, canvas = got
        r.update(depth_bytes=sizes, depth_max=max(sizes), fits_depth=max(sizes) <= DEPTH_MAX,
                 uncropped_depth_bytes=full, content_rows=content_rows, canvas_px=canvas)
        print('map %3d %2dx%-2d h%-2d rows %3d width %5.1f shipped depth %s uncropped %s' % (
            r['map'], nx, nz, r['maxh'], content_rows, r['width_px'], sizes, full), flush=True)
    for m in [int(m) for m in args.archive.split(',') if m]:
        r = next(r for r in rows if r['map'] == m)
        r['view_archive_bytes'] = archive(disc, files, m)
        print('map %3d views %s = %d archive bytes' % (m, r['view_archive_bytes'], sum(r['view_archive_bytes'])), flush=True)
    if not args.render and not args.maps and args.out.exists():   # keep the rendered depth figures
        old = {r['map']: r for r in json.loads(args.out.read_text())['rows']}
        for r in rows:
            r.update({k: v for k, v in old.get(r['map'], {}).items() if k not in r})
    report = dict(schema='FFT_MAP_BUDGET_1', sc_w=320, sc_hmax=SC_HMAX, depth_max=DEPTH_MAX,
                  maps=len(rows), over_width=[r['map'] for r in rows if not r['fits_width']],
                  over_rows=[r['map'] for r in rows if not r['fits_rows']],
                  over_depth=[r['map'] for r in rows if r.get('fits_depth') is False],
                  rows=rows)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, indent=1, default=int) + '\n')
    print('maps', len(rows), 'over width', report['over_width'])
    print('over rows', report['over_rows'])
    print('over depth', report['over_depth'], '->', args.out)


if __name__ == '__main__':
    main()
