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

**The two ends are not symmetric, and camera_clamp.h says why.** A unit is drawn
on the SCREEN, not into the scene: `draw_unit_covered(f.x - st.camx, f.y -
st.camy, ...)`. So the scene only bounds the scenery, and what bounds the unit is
how far the camera may scroll:

    if (*y < -CAM_MT) *y = -CAM_MT;       // CAM_MT 39, virtual blank rows
    if (*y > SC_H - RT_H) *y = SC_H - RT_H;

**Above the scene there are 39 virtual blank rows** -- "body, smaller round
pointer and six pixels of headroom", which is exactly UNIT_SH 26 + FOE_H 7 + 6 --
so a head up to 39 rows above the first scene row is drawn in full. **Below the
scene there is nothing.** A row cut off the top is free until the 39 are spent; a
row cut off the bottom takes ground a unit stands on, and that unit is then drawn
off the scene with no way to scroll to it.

Gariland calibrates it: its band runs 21 rows above its scene, inside the 39, and
nothing below. That is a clean map, and the rule reproduces it.

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
CAM_MT = 39                        # camera_clamp.h: virtual blank rows above the scene
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
        # above, the first CAM_MT rows are free; below, none are
        clip_above = max(0, (best[1] - need_lo) - CAM_MT) if best else None
        clip_below = max(0, need_hi - (best[1] + best[0] - 1)) if best else None

        # What the map already loses at full height, before any crop: several of
        # these maps stand units more than CAM_MT above their own scene.
        inherent = max(0, -need_lo - CAM_MT)
        # The window that loses NOTHING BELOW, whatever it costs: its bottom must
        # hold the band, and among those the one clipping least above. Reported
        # even when it is over the cap, because that number is the ask. SC_HMAX
        # alone can make it impossible: Slums in Dorter needs 207 rows of 202.
        keep_all = None
        for h in range(min(depth_budget.ROWS, content), 0, -1):
            cands = [(max(0, (o - need_lo) - CAM_MT), cost(o, h), o)
                     for o in range(0, content - h + 1) if o + h > need_hi]
            if cands:
                ca, c, o = min(cands)
                keep_all = (h, o, c, ca)
                break

        row = dict(map=m, name=e['name'], worst=e['worst'], over=e['over'],
                   content_rows=content, unit_band=[need_lo, need_hi], unit_rows=need_rows,
                   sc_h=best and best[0], offset=best and best[1],
                   worst_after=best and best[2],
                   clipped_above=clip_above, clipped_below=clip_below,
                   inherent_above=inherent,
                   clean=best is not None and not clip_above and not clip_below,
                   lossless_sc_h=keep_all and keep_all[0],
                   lossless_offset=keep_all and keep_all[1],
                   lossless_worst=keep_all and keep_all[2],
                   lossless_clipped_above=keep_all and keep_all[3],
                   lossless_over=keep_all and keep_all[2] - depth_budget.LIMIT)
        report.append(row)
        print('map %3d %-24s content %3d  unit band %3d..%3d (%3d rows)  '
              'crop sc_h %s at +%s -> %s (%s spare)  clips %s above, %s below'
              % (m, e['name'][:24], content, need_lo, need_hi, need_rows,
                 best and best[0], best and best[1], best and best[2],
                 best and depth_budget.LIMIT - best[2], clip_above, clip_below),
              flush=True)
        print('        %d row(s) already lost above at full height; adding nothing needs %s'
              % (inherent,
                 'sc_h %d at +%d -> %d, %d more clipped above, %s' % (
                     keep_all[0], keep_all[1], keep_all[2], keep_all[3],
                     'FITS with %d spare' % (depth_budget.LIMIT - keep_all[2])
                     if keep_all[2] <= depth_budget.LIMIT
                     else 'OVER THE CAP BY %d' % (keep_all[2] - depth_budget.LIMIT))
                 if keep_all else 'no window at all (SC_HMAX %d)' % depth_budget.ROWS),
              flush=True)

        # the picture: the source and the two windows, side by side
        rot = e.get('dearest_view', 0)
        v = views[rot]
        light = np.unpackbits(v[0], axis=1)[:, :extract.SC_W]
        dark = np.unpackbits(v[1], axis=1)[:, :extract.SC_W]
        grey = (255 - 85 * (light + 2 * dark)).astype(np.uint8)
        base = np.repeat(grey[lo:hi, :, None], 3, 2).copy()

        def shaded(win):
            img = base.copy()
            if win:
                drop = np.ones(content, bool)
                drop[win[1]:win[1] + win[0]] = False
                img[drop] = (.45 * img[drop] + .55 * LOST).astype(np.uint8)
            return img

        labels = ['source (view %d), blue = the rows a unit needs' % rot,
                  'the budget crop: sc_h %s, clips %s above %s below'
                  % (best and best[0], clip_above, clip_below),
                  'nothing lost below: sc_h %s, %s'
                  % (keep_all and keep_all[0],
                     ('fits, %d spare' % (depth_budget.LIMIT - keep_all[2]))
                     if keep_all and keep_all[2] <= depth_budget.LIMIT else
                     ('over by %d' % (keep_all[2] - depth_budget.LIMIT)) if keep_all
                     else 'impossible, SC_HMAX %d' % depth_budget.ROWS)]
        panels = [base, shaded(best), shaded(keep_all)]
        PAD = 10
        w = sum(pp.shape[1] for pp in panels) + PAD * (len(panels) + 1)
        sheet = np.full((content + 2 * PAD + 14, w, 3), 255, np.uint8)
        x = PAD
        for pp in panels:
            sheet[PAD + 14:PAD + 14 + content, x:x + pp.shape[1]] = pp
            x += pp.shape[1] + PAD
        pic = Image.fromarray(sheet)
        d = ImageDraw.Draw(pic)
        x = PAD
        for pp, text in zip(panels, labels):
            d.text((x, 2), text, fill=(0, 0, 0))
            d.rectangle([x, PAD + 14 + max(0, need_lo), x + extract.SC_W - 1,
                         PAD + 14 + min(content - 1, need_hi)], outline=BAND)
            x += pp.shape[1] + PAD
        pic.save(args.out / ('map-%03d-crop.png' % m))

    (args.out / 'crop.json').write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
