#!/usr/bin/env python3
"""A structural map's crop: the rows it may lose, and the rows a unit needs.

A map whose excess is terrain can only give up scene rows. That is cheap at the
top (sky) and expensive at the bottom (ground the player walks on), and it has a
hard floor nobody should guess at: a unit standing on a tile occupies
UNIT_SH = 26 rows with its feet on UNIT_FOOT = 21, so the kept window must hold
[feet - 21, feet + 4] for EVERY walkable tile in EVERY one of the four views, or
a unit standing there is drawn clipped.

This measures that band, compares it with the tallest window the budget allows,
and renders the dearest view with the lost rows shaded and the band outlined, so
the crop is approved on the picture and not on the arithmetic alone.

**The band is not a gate, it is a measurement, and the reason is Gariland.** That
map ships and is played, and it already breaks the rule: one of its walkable
roofs at height 10, seen from view 1, puts a unit's feet on the very first lit
row, so 21 of that unit's rows are above everything the bank holds. Whatever the
engine does about it, it is what shipping already tolerates. So this tool reports
how far a window clips the band and leaves the verdict to a person, and the
calibration it still needs is a runtime look at a unit standing on that roof.

usage: crop_review.py [MAP...] [--out campaign/x/crop-review]
"""
import argparse
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE / 'tools'))
import core_audit      # noqa: E402
import depth_budget    # noqa: E402
import extract         # noqa: E402
import map_budget      # noqa: E402

UNIT_SH, UNIT_FOOT = 26, 21        # units.h
LOST = np.array([255, 70, 70])     # rows the crop drops
BAND = (40, 120, 255)              # the rows a unit needs


def feet_rows(mesh, nx, nz, rot):
    """The scene row a unit's feet land on, per walkable tile, in extract's canvas.

    extract.render() puts a vertex at sy = SC_OY + (u + w) * 6 - h * 6; a unit stands
    at the tile's centre, so u + w gains one.
    """
    _, top, _ = extract.terrain(mesh, nx, nz)
    out = []
    for i, h in enumerate(top):
        if h is None:
            continue
        x, z = i % nx, i // nx
        u, w = [(x, z), (nz - 1 - z, x), (nx - 1 - x, nz - 1 - z), (z, nx - 1 - x)][rot]
        out.append(extract.SC_OY + (u + w + 1) * 6 - h * 6)
    return out


def main():
    budget = json.loads((HERE / 'campaign/x/depth-budget/all-maps.json').read_text())
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('maps', nargs='*', type=int,
                    default=[int(k) for k in budget['categories']['structural']])
    ap.add_argument('disc', nargs='?', type=Path, default=core_audit.DEFAULT_DISC)
    ap.add_argument('--out', type=Path, default=HERE / 'campaign/x/crop-review')
    args = ap.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    disc, files = extract.disc_files(str(args.disc))
    report = []

    for m in args.maps:
        e = budget['maps'][str(m)]
        mesh, nx, nz, maxh = map_budget.load(disc, files, m)
        views, _ = depth_budget.map_views(disc, files, m)
        lit = np.array([v[:2].any((0, 2)) | v[3].any(1) | v[5].any(1) for v in views]).any(0)
        used = np.nonzero(lit)[0]
        lo, hi = int(used[0]), int(used[-1]) + 1
        content = hi - lo
        span = [depth_budget.row_spans(v, lo, content) for v in views]

        def cost(y0, rows):
            return max(4 * rows + 4 * sum(sp[y0:y0 + rows]) for sp in span)

        # the band a unit needs, over all four views, in content-relative rows
        need_lo, need_hi = 10**9, -10**9
        for rot in range(4):
            for f in feet_rows(mesh, nx, nz, rot):
                need_lo = min(need_lo, f - UNIT_FOOT)
                need_hi = max(need_hi, f - UNIT_FOOT + UNIT_SH - 1)
        need_lo, need_hi = need_lo - lo, need_hi - lo
        need_rows = need_hi - need_lo + 1

        # The tallest window that fits; among those of that height, the one that
        # clips the band least. Below first: a row cut off the top takes the top of
        # a head, a row cut off the bottom takes ground a unit stands on, so the
        # unit is drawn off the scene entirely. Gariland ships at 21 above, 0 below.
        best = None
        for h in range(min(depth_budget.ROWS, content), 0, -1):
            fitting = [(o, cost(o, h)) for o in range(0, content - h + 1)
                       if cost(o, h) <= depth_budget.LIMIT]
            if not fitting:
                continue
            o, c = min(fitting, key=lambda t: (max(0, need_hi - (t[0] + h - 1)),
                                               max(0, t[0] - need_lo), t[1]))
            best = (h, o, c)
            break
        clip_above = max(0, best[1] - need_lo) if best else None
        clip_below = max(0, need_hi - (best[1] + best[0] - 1)) if best else None

        row = dict(map=m, name=e['name'], worst=e['worst'], over=e['over'],
                   content_rows=content, unit_band=[need_lo, need_hi], unit_rows=need_rows,
                   sc_h=best and best[0], offset=best and best[1],
                   worst_after=best and best[2],
                   clipped_above=clip_above, clipped_below=clip_below,
                   clean=best is not None and not clip_above and not clip_below)
        report.append(row)
        print('map %3d %-24s content %3d  unit band %3d..%3d (%3d rows)  '
              'crop sc_h %s at +%s -> %s (%s spare)  clips %s above, %s below'
              % (m, e['name'][:24], content, need_lo, need_hi, need_rows,
                 best and best[0], best and best[1], best and best[2],
                 best and depth_budget.LIMIT - best[2], clip_above, clip_below),
              flush=True)

        # the picture: the dearest view, lost rows shaded, the unit band outlined
        rot = e.get('dearest_view', 0)
        v = views[rot]
        g = np.frombuffer(np.packbits(np.unpackbits(v[0]).reshape(-1, 320)), np.uint8)
        light = np.unpackbits(v[0], axis=1)[:, :extract.SC_W]
        dark = np.unpackbits(v[1], axis=1)[:, :extract.SC_W]
        grey = (255 - 85 * (light + 2 * dark)).astype(np.uint8)
        img = np.repeat(grey[lo:hi, :, None], 3, 2).copy()
        if best:
            keep = slice(best[1], best[1] + best[0])
            mask = np.ones(content, bool)
            mask[keep] = False
            img[mask] = (.45 * img[mask] + .55 * LOST).astype(np.uint8)
        pic = Image.fromarray(img)
        d = ImageDraw.Draw(pic)
        d.rectangle([0, need_lo, extract.SC_W - 1, need_hi], outline=BAND)
        pic.save(args.out / ('map-%03d-crop.png' % m))

    (args.out / 'crop.json').write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
