#!/usr/bin/env python3
"""Per-map ragged-depth budget, and what an overrunning map has to give up.

fft.c unpacks one view's ragged depth into vbuf, capped at FFT_CORE_DEPTH_MAX, so the
gate is len(extract.ragged_depth(view)) for the rows the chapter keeps. The stream is
    4 * sc_h            one DepthRow per scene row
  + 4 * sum(span)       four planes over each row's even byte span
so a map's cost splits into a term the authoring pass sets (sc_h) and a term the source
geometry sets (the spans). This measures both with the production encoder and, for a map
over the cap, prints the per-row cost so a row cut can be chosen with the numbers in hand
rather than guessed. It decides nothing visual: cropping rows is the extractor's call.

usage: depth_budget.py [MAP...] [--out x/depth-budget/budget.json]
"""
import argparse
import json
import struct
import sys
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE / 'tools'))
import core_audit   # noqa: E402
import extract      # noqa: E402
import map_budget   # noqa: E402

LIMIT, ROWS = map_budget.DEPTH_MAX, map_budget.SC_HMAX
# campaign/CAMPAIGN_PACK_REPORT.md section 6.2, pack I.
PACK1 = [22, 32, 34, 49, 56, 74, 76, 77, 85, 91, 103]


def row_spans(view, y0, rows):
    """Each row's even byte span, exactly as extract.ragged_depth() computes it."""
    m = view[3, y0:y0 + rows] | view[5, y0:y0 + rows]
    out = []
    for y in range(m.shape[0]):
        nz = np.nonzero(m[y])[0]
        out.append(0 if not len(nz) else ((int(nz[-1]) + 2) & ~1) - (int(nz[0]) & ~1))
    return out


def terrain(mesh, nx, nz):
    """Per tile and level: walkability and standable top, over BOTH terrain levels.

    walk 0 blocked, 1 ordinary, 2 crossed but never stopped on. FFT marks a purely
    decorative prop with walk 2 and gives the tile the PROP's own height, not the
    ground's: Sweegy's four trees are tiles of height 17-18 over ground near 4.
    That flag, not geometry analysis, is what identifies decoration.
    """
    p0 = struct.unpack('<I', mesh[0x68:0x6C])[0]
    n = nx * nz
    walk = [0] * n
    top = [None] * n
    props = []
    for level in (0, 1):
        base = p0 + 2 + 8 * n * level
        for i in range(n):
            b = mesh[base + 8 * i:base + 8 * i + 8]
            w = 0 if b[6] & 0x01 else 2 if b[6] & 0x02 else 1
            h = max(extract.corners(b[2], b[3] & 0x1F, b[4] if (b[3] & 0x1F) else 0))
            if w == 2:
                props.append((i % nx, i // nx, h))
            elif w == 1 and (top[i] is None or h > top[i]):
                top[i] = h
                walk[i] = 1
    return walk, top, props


def prop_polygons(polys, top, props, nx, nz, grow=0.75):
    """Every polygon standing over a decorative tile, trunk and overhanging canopy.

    Removing part of a prop saves nothing: the rest draws the same silhouette and no
    span moves (measured on Sweegy, 48 of 144 tree polygons removed changed the stream
    by zero bytes). The ground under the prop is kept: only geometry above the lowest
    standable neighbour goes.
    """
    out = []
    for x0, z0, _ in props:
        ground = [top[(z0 + dz) * nx + x0 + dx]
                  for dz in (-1, 0, 1) for dx in (-1, 0, 1)
                  if 0 <= x0 + dx < nx and 0 <= z0 + dz < nz
                  and top[(z0 + dz) * nx + x0 + dx] is not None]
        floor = min(ground) if ground else 0
        cx, cz = x0 + .5, z0 + .5
        for i, q in enumerate(polys):
            v = q[0]
            x, z = v[:, 0].mean() / 28, v[:, 2].mean() / 28
            if abs(x - cx) <= grow and abs(z - cz) <= grow and (-v[:, 1] / 12).max() > floor:
                out.append(i)
    return sorted(set(out))


def map_views(disc, files, m):
    rec, records = map_budget.mesh_record(disc, files, m), extract.gns_records
    _, nx, nz, maxh = map_budget.load(disc, files, m)
    extract.gns_records = lambda d, f: [(0x2E01,) + rec[1:]] + [r for r in records(d, f) if r[0] != 0x2E01]
    try:
        raw, _ = extract.views(disc, files, nx, nz, maxh)
    finally:
        extract.gns_records = records
    return [np.frombuffer(v, np.uint8).reshape(6, -1, 40) for v in raw], maxh


def texture_palette(disc, files, m, mesh):
    extract.MAP = m                       # gns_records reads the module's current map
    recs = extract.gns_records(disc, files)
    tr = next(r for r in recs if r[0] == 0x1701)
    t = np.frombuffer(disc.read(tr[1], tr[2]), np.uint8)
    tex = np.empty((1024, 256), np.uint8)
    tex[:, 0::2] = (t & 15).reshape(1024, 128)
    tex[:, 1::2] = (t >> 4).reshape(1024, 128)
    pal = np.array(struct.unpack_from('<256H', mesh, struct.unpack_from('<I', mesh, 0x44)[0])).reshape(16, 16)
    return tex, pal


def depth_of(polys, tex, pal, nx, nz, sc_h):
    """Best window's worst view for a polygon set, same accounting as the main pass."""
    spans, lits = [], []
    for rot in range(4):
        lum, diag, _ = extract.render(polys, tex, pal, rot, nx, nz, sc_h)
        K, W = extract.K, extract.SC_W
        bg = (lum == extract.BG).reshape(sc_h, K, W, K).mean((1, 3)) > .5
        d = diag[K // 2::K, K // 2::K]
        bits = 0x80 >> (np.arange(W) & 7)
        masks = []
        for _ in range(2):
            thr = d.reshape(sc_h, W // 8, 8).max(2)
            front = (d == np.repeat(thr, 8, 1)) & (d >= 0)
            masks.append(np.bitwise_or.reduceat(np.where(front, bits, 0),
                                                np.arange(0, W, 8), axis=1).astype(np.uint8))
            d = np.where(front, -1, d)
        mk = masks[0] | masks[1]
        row = []
        for y in range(sc_h):
            nzc = np.nonzero(mk[y])[0]
            row.append(0 if not len(nzc) else ((int(nzc[-1]) + 2) & ~1) - (int(nzc[0]) & ~1))
        spans.append(row)
        lits.append(~bg.all(1) | mk.any(1))
    used = np.nonzero(np.array(lits).any(0))[0]
    if not len(used):
        return 0, 0
    lo, hi = int(used[0]), int(used[-1]) + 1
    content = hi - lo
    h = min(ROWS, content)
    return min(max(4 * h + 4 * sum(sp[lo + o:lo + o + h]) for sp in spans)
               for o in range(content - h + 1)), content


# The game's optional content, from the guide's own grouping (fft_guides: the
# battle compendium's "5) Optional Battles", and the walkthrough's Move-Find
# Item tables, which separate Deep Dungeon [17.tad], Colliery [17.tac] and
# Nelveska [17.tan] from the story chapters). A chapter package never ships
# these, so they do not belong in a budget census until the story fits.
OPTIONAL_DEEP = ('nogias', 'terminate', 'delta', 'valkyries', 'mlapan',
                 'tiger', 'bridge', 'voyage', 'horror', 'end')


def optional_map(name):
    """The side quest a map belongs to, or None for story and random battles."""
    n = name.strip().lower()
    if n in OPTIONAL_DEEP:
        return 'deep dungeon'
    if n.startswith('colliery underground') or 'passage in goland' in n:
        return 'colliery'
    if 'nelveska' in n or 'nevelska' in n:
        return 'nelveska temple'
    # Zarghidas is NOT here: the compendium lists Cloud's battle under the
    # optional ones, but the same area also takes random battles, so the map is
    # reached without the side quest. Only maps that exist for nothing else are
    # left out, which is why each of the three above appears in the compendium
    # under "5) Optional Battles" and nowhere else.
    return None


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('maps', nargs='*', type=int, default=PACK1)
    ap.add_argument('disc', nargs='?', type=Path, default=core_audit.DEFAULT_DISC)
    ap.add_argument('--out', type=Path, default=HERE / 'campaign/x/depth-budget/budget.json')
    ap.add_argument('--include-optional', action='store_true',
                    help='count the Deep Dungeon, Colliery, Nelveska and Zarghidas maps too')
    args = ap.parse_args()
    names = json.loads((HERE / 'campaign/x/core-audit/map-viewer-names.json').read_text())
    disc, files = extract.disc_files(str(args.disc))
    report = dict(schema='FFT_DEPTH_BUDGET_1', limit=LIMIT, sc_hmax=ROWS, maps={})

    for m in args.maps:
        # One unusable map must not take the whole census down: some records draw
        # nothing and the quantiser has no pixels to work from.
        try:
            views, maxh = map_views(disc, files, m)
            lit = np.array([v[:2].any((0, 2)) | v[3].any(1) | v[5].any(1) for v in views]).any(0)
            used = np.nonzero(lit)[0]
            lo, hi = int(used[0]), int(used[-1]) + 1
            content = hi - lo
            # One window for all four views, as the chapter keeps one sc_h/sc_top per map.
            # Per-row span cost of each view, once; a window is then a slice of these.
            span = [row_spans(v, lo, content) for v in views]

            def cost(y0, rows):
                return max(4 * rows + 4 * sum(sp[y0:y0 + rows]) for sp in span)

            sc_h = min(ROWS, content)
            best = min(((cost(o, sc_h), o) for o in range(0, content - sc_h + 1)), key=lambda t: t[0])
            worst, y0 = best
            rows = sc_h
            per_view = [4 * rows + 4 * sum(sp[y0:y0 + rows]) for sp in span]
            entry = dict(name=names[str(m)], maxh=maxh, content_rows=content, sc_h=rows,
                         window_offset=y0 - lo, per_view=per_view, worst=worst,
                         span_bytes=[t - 4 * rows for t in per_view], over=worst - LIMIT,
                         fits=worst <= LIMIT)
            if worst > LIMIT:
                # Why is it over? FFT flags a decorative prop as a tile that may be crossed
                # but never stopped on, and gives that tile the prop's own height. Remove
                # every polygon standing over such a tile and re-measure: if that clears the
                # cap the map is cosmetic, otherwise its height is terrain and only a
                # shorter scene works.
                mesh, nx, nz, _ = map_budget.load(disc, files, m)
                tex, pal = texture_palette(disc, files, m, mesh)
                polys = extract.polygons(mesh)
                _, tops, props = terrain(mesh, nx, nz)
                dec = set(prop_polygons(polys, tops, props, nx, nz))
                sc_h_full = 6 * (nx + nz) + 6 * maxh + 8
                bare = (depth_of([q for i, q in enumerate(polys) if i not in dec],
                                 tex, pal, nx, nz, sc_h_full)[0] if dec else worst)
                entry.update(cause='cosmetic' if bare <= LIMIT else 'structural',
                             prop_tiles=[dict(x=x, z=z, height=h) for x, z, h in props],
                             prop_polygons=len(dec), total_polygons=len(polys),
                             depth_without_props=bare, saved_by_props=worst - bare)
                # The actionable number: the tallest scene that fits, over every window of
                # that height, worst view counted. Shrinking is what a row cut really does;
                # sliding a full-height window is not enough.
                fit = None
                for h in range(sc_h - 1, 0, -1):
                    cand = min(((cost(o, h), o) for o in range(0, content - h + 1)), key=lambda t: t[0])
                    if cand[0] <= LIMIT:
                        fit = (h, cand[1], cand[0])
                        break
                w = int(np.argmax(per_view))
                rc = [4 + 4 * s for s in span[w][y0:y0 + rows]]
                entry.update(dearest_view=w, need_bytes=worst - LIMIT,
                             max_fitting_sc_h=fit and fit[0], fitting_window_offset=fit and fit[1],
                             fitting_worst=fit and fit[2], rows_to_lose=fit and (sc_h - fit[0]),
                             rows_lost_above=fit and fit[1], rows_lost_below=fit and (content - fit[1] - fit[0]),
                             row_cost_top20=round(float(np.mean(rc[:20]))),
                             row_cost_middle20=round(float(np.mean(rc[rows // 2 - 10:rows // 2 + 10]))),
                             row_cost_bottom20=round(float(np.mean(rc[-20:]))),
                             row_cost=rc)
            report['maps'][str(m)] = entry
            tail = ('' if entry['fits'] else
                    '  %s: %d prop tiles, %d polygons -> %d (%+d). Tallest fitting scene %s rows '
                    '(lose %s: %s above, %s below) -> %s'
                    % (entry['cause'].upper(), len(entry['prop_tiles']), entry['prop_polygons'],
                       entry['depth_without_props'], LIMIT - entry['depth_without_props'],
                       entry['max_fitting_sc_h'], entry['rows_to_lose'],
                       entry['rows_lost_above'], entry['rows_lost_below'], entry['fitting_worst']))
            print('map %3d %-24s sc_h %3d worst %5d %s%s' % (
                m, entry['name'][:24], rows, worst,
                'ok, %d spare' % (LIMIT - worst) if entry['fits'] else 'OVER +%d' % entry['over'], tail),
                flush=True)
        except Exception as exc:
            report['maps'][str(m)] = dict(error='%s: %s' % (type(exc).__name__, exc))
            print('map %3d SKIPPED (%s)' % (m, type(exc).__name__), flush=True)
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_text(json.dumps(report, indent=2) + '\n')   # resumable

    ok = {k: v for k, v in report['maps'].items() if 'error' not in v}
    if not args.include_optional:
        skipped = {k: optional_map(v['name']) for k, v in ok.items() if optional_map(v['name'])}
        report['optional_skipped'] = skipped
        ok = {k: v for k, v in ok.items() if k not in skipped}
        print('\nside quests left out of the counts (%d): %s'
              % (len(skipped), ', '.join(sorted(set(skipped.values())))))
    over = [k for k, v in ok.items() if not v['fits']]
    report['over_limit'] = over
    report['categories'] = {c: [k for k, v in ok.items()
                                if (v.get('cause') or 'fits') == c]
                            for c in ('fits', 'cosmetic', 'structural')}
    report['categories']['unreadable'] = [k for k, v in report['maps'].items() if 'error' in v]
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, indent=2) + '\n')
    print()
    for c, v in report['categories'].items():
        print('%-11s %2d maps  %s' % (c, len(v), ' '.join(v)))


if __name__ == '__main__':
    main()
