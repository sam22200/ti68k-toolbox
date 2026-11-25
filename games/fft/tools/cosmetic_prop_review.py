#!/usr/bin/env python3
"""The cosmetic maps in the source's own colours: what the depth budget would remove.

depth_budget.py calls a map COSMETIC when it clears FFT_CORE_DEPTH_MAX once every
polygon standing over a decorative tile is gone (FFT's own flag: a tile that may be
crossed but never stopped on, carrying the PROP's height). That is an arithmetic
verdict. Whether the map still reads as itself afterwards is a judgement call, and it
needs the original picture, not four greys: this renders each map's dearest view three
times side by side -- the source as it is, the same with every prop pixel tinted, and
the map without them -- so the removal can be approved or refused per map.

Colour comes from the proven rasterizer run once per RGB555 channel with the palette
masked to that field (map_color_reference.py's technique), never a second renderer.
The prop pixels are exactly where the two depth buffers differ, so a prop hidden
behind geometry is not claimed as a loss.

usage: cosmetic_prop_review.py [MAP...] [--budget J] [--out campaign/x/cosmetic-props]
"""
import argparse
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

HERE = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(HERE / 'tools'))
import core_audit     # noqa: E402
import depth_budget   # noqa: E402
import extract        # noqa: E402
import map_budget     # noqa: E402

CHANNELS = ((0, .299), (5, .587), (10, .114))   # RGB555 field -> extract.render's weight
TINT = np.array([255, 48, 48])                  # what a prop pixel is painted
PAD = 12


def colour_view(polys, tex, pal, rot, nx, nz, sc_h):
    """One view's RGB and depth, the source's own colours."""
    rgb, depth, visible = np.empty((sc_h * extract.K, extract.SC_W * extract.K, 3)), None, None
    for c, (shift, weight) in enumerate(CHANNELS):
        one = (pal & (31 << shift)) | np.where(pal != 0, 0x8000, 0).astype(np.uint16)
        lum, _, _, z = extract.render(polys, tex, one, rot, nx, nz, sc_h, return_depth=True)
        if depth is None:
            depth, visible = z, z > -1e8
        rgb[:, :, c] = np.where(visible, lum / weight, 255)
    small = np.rint(rgb.reshape(sc_h, extract.K, extract.SC_W, extract.K, 3)
                    .mean((1, 3))).clip(0, 255).astype(np.uint8)
    return small, depth, visible


def sheet(panels, labels):
    """The three panels in a row, each captioned, on white."""
    h = max(p.shape[0] for p in panels)
    w = sum(p.shape[1] for p in panels) + PAD * (len(panels) + 1)
    out = np.full((h + 2 * PAD + 14, w, 3), 255, np.uint8)
    x = PAD
    for p in panels:
        out[PAD + 14:PAD + 14 + p.shape[0], x:x + p.shape[1]] = p
        x += p.shape[1] + PAD
    img = Image.fromarray(out)
    d, x = ImageDraw.Draw(img), PAD
    for p, text in zip(panels, labels):
        d.text((x, 2), text, fill=(0, 0, 0))
        x += p.shape[1] + PAD
    return img


def shrink_props(polys, tops, props, nx, nz, scale):
    """Every prop scaled about its own tile centre, at ground level. The budget
    pays for the canvas a prop covers, so a smaller tree is a cheaper tree -- and
    a flat 2D one is not, since it keeps the silhouette we are paying for."""
    out = list(polys)
    for x0, z0, _ in props:
        g = [tops[(z0 + dz) * nx + x0 + dx]
             for dz in (-1, 0, 1) for dx in (-1, 0, 1)
             if 0 <= x0 + dx < nx and 0 <= z0 + dz < nz
             and tops[(z0 + dz) * nx + x0 + dx] is not None]
        c = np.array([(x0 + .5) * 28, -12.0 * (max(g) if g else 0), (z0 + .5) * 28])
        for i in extract.prop_polygons(polys, tops, [(x0, z0, 0)], nx, nz):
            v, uv, pal, page = out[i]
            out[i] = (c + (v - c) * scale, uv, pal, page)
    return out


def review(disc, files, m, name, rot, out, minimal=False, scale=None):
    mesh, nx, nz, maxh = map_budget.load(disc, files, m)
    tex, pal = depth_budget.texture_palette(disc, files, m, mesh)
    polys = extract.polygons(mesh)
    _, tops, props = extract.terrain(mesh, nx, nz)
    if minimal:
        sc = 6 * (nx + nz) + 6 * maxh + 8
        keep, _ = depth_budget.minimal_props(polys, tops, props, tex, pal, nx, nz, sc)
        props = keep or props
    dec = set(extract.prop_polygons(polys, tops, props, nx, nz))
    bare = (shrink_props(polys, tops, props, nx, nz, scale) if scale
            else [q for i, q in enumerate(polys) if i not in dec])
    sc_h = 6 * (nx + nz) + 6 * maxh + 8

    full_rgb, full_z, full_vis = colour_view(polys, tex, pal, rot, nx, nz, sc_h)
    bare_rgb, bare_z, bare_vis = colour_view(bare, tex, pal, rot, nx, nz, sc_h)
    # A prop is only a loss where it is actually the frontmost surface.
    lost = (full_z != bare_z).reshape(sc_h, extract.K, extract.SC_W, extract.K).any((1, 3))
    marked = full_rgb.copy()
    marked[lost] = (.35 * full_rgb[lost] + .65 * TINT).astype(np.uint8)

    seen = (full_vis | bare_vis).reshape(sc_h, extract.K, extract.SC_W, extract.K).any((1, 3))
    rows, cols = np.nonzero(seen.any(1))[0], np.nonzero(seen.any(0))[0]
    box = (slice(rows[0], rows[-1] + 1), slice(cols[0], cols[-1] + 1))
    panels = [full_rgb[box], marked[box], bare_rgb[box]]
    img = sheet(panels, ['source (view %d)' % rot,
                         '%d of %d props, %d polygons, %d px'
                         % (len(props), len(extract.terrain(mesh, nx, nz)[2]),
                            len(dec), int(lost.sum())) if minimal else
                         '%d prop polygons, %d px' % (len(dec), int(lost.sum())),
                         'every prop at %d%%' % (100 * scale) if scale else 'without them'])
    path = out / ('map-%03d-%s.png' % (m, name.lower().replace(' ', '-').replace(',', '')[:28]))
    img.save(path)
    return dict(map=m, name=name, view=rot, prop_tiles=len(props), prop_polygons=len(dec),
                lost_pixels=int(lost.sum()), scene_pixels=int(seen.sum()),
                lost_share=round(float(lost.sum()) / max(int(seen.sum()), 1), 4), image=str(path))


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('maps', nargs='*', type=int)
    ap.add_argument('disc', nargs='?', type=Path, default=core_audit.DEFAULT_DISC)
    ap.add_argument('--budget', type=Path,
                    default=HERE / 'campaign/x/depth-budget/all-maps.json')
    ap.add_argument('--out', type=Path, default=HERE / 'campaign/x/cosmetic-props')
    ap.add_argument('--minimal', action='store_true',
                    help='remove only the fewest props that clear the cap')
    ap.add_argument('--shrink', type=float, metavar='S',
                    help='keep every prop, scaled to S (0.6 = 60%%), instead of removing it')
    args = ap.parse_args()
    budget = json.loads(args.budget.read_text())
    args.maps = args.maps or [int(k) for k in budget['categories']['cosmetic']]
    args.out.mkdir(parents=True, exist_ok=True)
    disc, files = extract.disc_files(str(args.disc))
    report = []
    for m in args.maps:
        e = budget['maps'][str(m)]
        r = review(disc, files, m, e['name'], e.get('dearest_view', 0), args.out,
                   minimal=args.minimal, scale=args.shrink)
        report.append(r)
        print('map %3d %-30s view %d  %4d prop polygons  %6d px (%.1f%% of the scene)  %s'
              % (m, r['name'][:30], r['view'], r['prop_polygons'], r['lost_pixels'],
                 100 * r['lost_share'], Path(r['image']).name), flush=True)
    (args.out / 'review.json').write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
