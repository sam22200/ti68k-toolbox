#!/usr/bin/env python3
"""Which maps can keep the untextured underside, and what it costs them.

FFT leaves the skirt under a map's terrain untextured; `extract.DROP_UNDERSIDE`
drops it everywhere because it was the lever that brought 20 of 27 structural
maps under `FFT_CORE_DEPTH_MAX`. A map that never needed that lever should keep
its skirt -- it reads better -- so this measures, per map and on the window that
ships (`extract.MAP_CROP`, `extract.PROPS_DROPPED`):

  * the worst view's ragged depth with the skirt dropped and with it kept,
    against FFT_CORE_DEPTH_MAX: this is the gate, a RAM gate;
  * with --archive, the four views' packed archive bytes both ways. The colour
    planes are raw and always SC_HMAX rows, so only the ZX0 depth grows.

`extract.KEEP_UNDERSIDE` is this census's verdict; re-run it after changing a
map's crop, its dropped props or the depth cap.

usage: underside_census.py [MAP...] [--archive] [--out x/underside/census.json]
"""
import argparse
import json
import sys
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE / 'tools'))
import core_audit      # noqa: E402
import depth_budget    # noqa: E402
import extract         # noqa: E402
import map_budget      # noqa: E402

ALL_MAPS = HERE / 'campaign/x/depth-budget/all-maps.json'


def depth(disc, files, m):
    """(worst view's ragged depth, scene rows, content rows) on the shipping window."""
    views, _ = depth_budget.map_views(disc, files, m)
    lit = np.array([v[:2].any((0, 2)) | v[3].any(1) | v[5].any(1) for v in views]).any(0)
    used = np.nonzero(lit)[0]
    lo, content = int(used[0]), int(used[-1]) + 1 - int(used[0])
    span = [depth_budget.row_spans(v, lo, content) for v in views]

    def cost(y0, rows):
        return max(4 * rows + 4 * sum(sp[y0:y0 + rows]) for sp in span)

    if m in extract.MAP_CROP:
        rows, off = extract.MAP_CROP[m]
        return cost(off, rows), rows, content
    rows = min(depth_budget.ROWS, content)
    return min(cost(o, rows) for o in range(content - rows + 1)), rows, content


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('maps', nargs='*', type=int,
                    help='default: every map of the depth census')
    ap.add_argument('disc', nargs='?', type=Path, default=core_audit.DEFAULT_DISC)
    ap.add_argument('--archive', action='store_true',
                    help="also pack the four views both ways (ZX0, about 20 s a map)")
    ap.add_argument('--out', type=Path, default=HERE / 'campaign/x/underside/census.json')
    args = ap.parse_args()

    maps = args.maps or sorted(int(k) for k in json.loads(ALL_MAPS.read_text())['maps'])
    names = json.loads((HERE / 'campaign/x/core-audit/map-viewer-names.json').read_text())
    disc, files = extract.disc_files(str(args.disc))
    limit, shipping = depth_budget.LIMIT, set(extract.KEEP_UNDERSIDE)
    report = dict(schema='FFT_UNDERSIDE_CENSUS_1', limit=limit, maps={})

    for m in maps:
        row = dict(name=names[str(m)])
        for keep, key in ((False, 'dropped'), (True, 'kept')):
            extract.KEEP_UNDERSIDE = set(maps) if keep else set()
            try:
                d, rows, content = depth(disc, files, m)
                row[key] = dict(depth=d, sc_h=rows, content_rows=content)
                if args.archive:
                    row[key]['archive'] = sum(map_budget.archive(disc, files, m))
            except Exception as e:                      # one unreadable map, not the census
                row[key] = dict(error='%s: %s' % (type(e).__name__, e))
        k = row['kept'].get('depth')
        row['fits'] = isinstance(k, int) and k <= limit
        report['maps'][m] = row
        d0, a0 = row['dropped'].get('depth'), row['dropped'].get('archive')
        a1 = row['kept'].get('archive')
        print('%3d %-30s depth %6s -> %6s  %s%s' % (
            m, row['name'][:30], d0, k,
            ('KEEP, %d spare' % (limit - k)) if row['fits'] else
            ('over +%d' % (k - limit) if isinstance(k, int) else k),
            ('  archive %s -> %s (%+d)' % (a0, a1, a1 - a0))
            if isinstance(a0, int) and isinstance(a1, int) else ''), flush=True)

    extract.KEEP_UNDERSIDE = shipping
    fits = sorted(m for m, r in report['maps'].items() if r['fits'])
    report['keep_underside'] = fits
    print('\n%d of %d maps keep the underside: %s'
          % (len(fits), len(maps), ', '.join(str(m) for m in fits)))
    disagree = sorted(set(fits) ^ (shipping & set(maps)))
    print('extract.KEEP_UNDERSIDE %s' % ('agrees' if not disagree else
          'DISAGREES on %s' % ', '.join(str(m) for m in disagree)))
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, indent=1) + '\n')
    print('wrote %s' % args.out)


if __name__ == '__main__':
    main()
