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


def map_views(disc, files, m):
    rec, records = map_budget.mesh_record(disc, files, m), extract.gns_records
    _, nx, nz, maxh = map_budget.load(disc, files, m)
    extract.gns_records = lambda d, f: [(0x2E01,) + rec[1:]] + [r for r in records(d, f) if r[0] != 0x2E01]
    try:
        raw, _ = extract.views(disc, files, nx, nz, maxh)
    finally:
        extract.gns_records = records
    return [np.frombuffer(v, np.uint8).reshape(6, -1, 40) for v in raw], maxh


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('maps', nargs='*', type=int, default=PACK1)
    ap.add_argument('disc', nargs='?', type=Path, default=core_audit.DEFAULT_DISC)
    ap.add_argument('--out', type=Path, default=HERE / 'campaign/x/depth-budget/budget.json')
    args = ap.parse_args()
    names = json.loads((HERE / 'campaign/x/core-audit/map-viewer-names.json').read_text())
    disc, files = extract.disc_files(str(args.disc))
    report = dict(schema='FFT_DEPTH_BUDGET_1', limit=LIMIT, sc_hmax=ROWS, maps={})

    for m in args.maps:
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
                '  need %d; tallest fitting scene %s rows (lose %s: %s above, %s below) -> %s'
                % (entry['need_bytes'], entry['max_fitting_sc_h'], entry['rows_to_lose'],
                   entry['rows_lost_above'], entry['rows_lost_below'], entry['fitting_worst']))
        print('map %3d %-24s sc_h %3d worst %5d %s%s' % (
            m, entry['name'][:24], rows, worst,
            'ok, %d spare' % (LIMIT - worst) if entry['fits'] else 'OVER +%d' % entry['over'], tail),
            flush=True)

    over = [k for k, v in report['maps'].items() if not v['fits']]
    report['over_limit'] = over
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, indent=2) + '\n')
    print('\n%d of %d maps over the %d-byte cap: %s' % (len(over), len(args.maps), LIMIT, over or 'none'))


if __name__ == '__main__':
    main()
