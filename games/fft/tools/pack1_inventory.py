#!/usr/bin/env python3
"""Does Chapter I fit as one pack? Its actual content, measured, not estimated.

Sizing only. Nothing here authors art, approves a camera, crops a map or changes a
shipping bank: the final game gets a human-supervised map/scene extractor. Every
battle map is read from the local disc and packed through the production R4 view
codec; every narrative room is rastered and ZX0-packed with the same bounded probe
as campaign/x/pack-refinement/measure_scene_compression.py.

Two independent verdicts come out:
  storage  pack content against ceiling - reserve - core (archfree measured the ceiling)
  depth    each map's ragged depth against FFT_CORE_DEPTH_MAX (a runtime buffer, not Flash)

usage: pack1_inventory.py [--maps-json X] [--out Y] [--skip-scenes]
"""
import argparse
import hashlib
import json
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

import numpy as np

HERE = Path(__file__).resolve().parents[1]
ROOT = HERE.parents[1]
sys.path.insert(0, str(HERE / 'tools'))
import core_audit   # noqa: E402
import extract      # noqa: E402
import map_budget   # noqa: E402

# campaign/CAMPAIGN_PACK_REPORT.md section 6.2, pack I.
BATTLE_MAPS = [22, 32, 34, 49, 56, 74, 77, 85, 91, 103]
EXTRA_RANDOM = [76]
SCENE_MAPS = [9, 11, 23, 24, 62, 73, 93, 94, 104]

# Measured elsewhere, reused rather than re-derived.
ARCHIVE_CEILING = 2555826        # experiments/archfree, EM_survey on a clean Titanium
ARCHIVE_PER_VARIABLE = 23        # measured 22.39 / 22.34
RESERVE = 131072
CORE_STORED = 700273             # promoted ABI99 core, stored payload + size words
PACK1_TEXT_CHUNKED = 18432       # referenced event text, 4 KiB ZX0 chunks + directory
PACK1_ACTORS_NATIVE30 = 253813   # x/actor-storage costume-choices.json, pack I, one body each
CORE_ARCHIVED = 694818           # the 36 archived core variables; fftstory is RAM, not archive
CORE_ARCHIVED_VARIABLES = 36
# Shipped Chapter I pilot, packages.json: chapter bytes that are neither the eight map
# views nor the two actor banks, split into what scales per map and what does not.
PILOT_PER_MAP_DATA = 7862        # (294439 - 152787 views - 84455 actors - 41474 chapter) / 2
PILOT_WORLD_MAP = 17674          # ffwm
PILOT_CHAPTER_MISC = 3714        # ffru + ffpack + ffsv + ffgu
DEPTH_MAX = map_budget.DEPTH_MAX


def packed(raw):
    """ZX0 size, round-tripped. Same bounded compressor as the scene probe."""
    with tempfile.TemporaryDirectory() as folder:
        p = Path(folder)
        (p / 'in').write_bytes(raw)
        subprocess.run([str(ROOT / 'tools/bin/zx0'), '-q', '-f', 'in', 'out'],
                       cwd=p, check=True, capture_output=True)
        subprocess.run([str(ROOT / 'tools/bin/dzx0'), '-f', 'out', 'back'],
                       cwd=p, check=True, capture_output=True)
        assert (p / 'back').read_bytes() == raw
        return (p / 'out').stat().st_size


def scene_rows(disc, files, number, name, widths=(160, 256, 320), rotations=(0, 2)):
    """One narrative room's six picture planes per direction and width, ZX0-packed.

    Lifted from the accepted pack-refinement probe so the figures stay comparable:
    source-texture light-tone greys, all six planes, central crop as a sizing proxy.
    """
    extract.SOURCE_CAMERA = True
    extract.MAP = number
    mesh, nx, nz, _ = map_budget.load(disc, files, number)
    records = extract.gns_records(disc, files)
    texture = disc.read(*next(r[1:] for r in records if r[0] == 0x1701))
    t = np.frombuffer(texture, np.uint8)
    tex = np.empty((1024, 256), np.uint8)
    tex[:, ::2] = (t & 15).reshape(1024, 128)
    tex[:, 1::2] = (t >> 4).reshape(1024, 128)
    pal = np.frombuffer(mesh, '<u2', 256, struct.unpack_from('<I', mesh, 0x44)[0]).reshape(16, 16)
    polys = extract.polygons(mesh)
    bounds = map_budget.bounds(polys, nx, nz)
    extract.SC_W = int(np.ceil((max(b[1] for b in bounds) + 8) / 16)) * 16
    height = int(np.ceil(max(b[3] for b in bounds))) + 8
    out = []
    for rot in rotations:
        lum, diag, _ = extract.render(polys, tex, pal, rot, nx, nz, height)
        k, w = extract.K, extract.SC_W
        small = lum.reshape(height, k, w, k).mean((1, 3))
        bg = (lum == extract.BG).reshape(height, k, w, k).mean((1, 3)) > .5
        q = np.percentile(small[~bg], [10, 34, 65])
        grey = (3 - np.searchsorted(q, small, side='left')).astype(np.uint8)
        grey[bg] = 0
        depth = diag[k // 2::k, k // 2::k]
        cy, cx = np.mean(np.argwhere(~bg), axis=0)
        for width in widths:
            canvas = np.zeros((100, width), np.uint8)
            z = np.full((100, width), -1, np.int16)
            x0 = max(0, int(cx - width / 2) // 8 * 8)
            y0 = max(0, min(height - 100, int(cy - 50)))
            crop = grey[y0:y0 + 100, x0:x0 + width]
            d = depth[y0:y0 + 100, x0:x0 + width]
            canvas[:crop.shape[0], :crop.shape[1]] = crop
            z[:d.shape[0], :d.shape[1]] = d

            def bits(data):
                return np.packbits(data.astype(np.uint8), axis=1)

            planes = [bits(canvas & 1), bits(canvas >> 1)]
            for _ in range(2):
                thr = z.reshape(100, width // 8, 8).max(2)
                front = (z == np.repeat(thr, 8, 1)) & (z >= 0)
                planes += [np.maximum(thr, 0).astype(np.uint8), bits(front)]
                z = np.where(front, -1, z)
            data = np.array(planes, np.uint8)
            out.append(dict(map=number, name=name, rotation=rot, width=width, height=100,
                            raw_bytes=int(data.size), zx0=packed(data.tobytes()),
                            sha256=hashlib.sha256(data.tobytes()).hexdigest()))
        print('scene map %3d %-34s direction %d packed' % (number, name, rot), flush=True)
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('disc', nargs='?', type=Path, default=core_audit.DEFAULT_DISC)
    ap.add_argument('--maps-json', type=Path, required=True,
                    help='map_budget.py output covering every pack I battle map')
    ap.add_argument('--out', type=Path, default=HERE / 'campaign/x/pack1-inventory/inventory.json')
    ap.add_argument('--scenes-json', type=Path,
                    help='reuse a previous scene run instead of re-rendering (10 min)')
    ap.add_argument('--skip-scenes', action='store_true')
    args = ap.parse_args()
    names = json.loads((HERE / 'campaign/x/core-audit/map-viewer-names.json').read_text())
    budget = {r['map']: r for r in json.loads(args.maps_json.read_text())['rows']}

    maps = []
    for number in BATTLE_MAPS + EXTRA_RANDOM:
        r = budget[number]
        views = r.get('view_archive_bytes')
        depth = r.get('depth_bytes')
        if not views or not depth:
            raise SystemExit(f'map {number} has no measured views/depth: rerun map_budget.py')
        maps.append(dict(map=number, name=names[str(number)], story=number in BATTLE_MAPS,
                         view_archive_bytes=views, stored_bytes=sum(views),
                         depth_bytes=depth, depth_max=max(depth),
                         fits_depth=max(depth) <= DEPTH_MAX,
                         width_px=r['width_px'], content_rows=r.get('content_rows')))

    scenes = []
    if args.scenes_json:
        cached = json.loads(args.scenes_json.read_text())
        by_map = {}
        for row in cached:
            by_map.setdefault(row['map'], []).append(row)
        missing = [m for m in SCENE_MAPS if m not in by_map]
        if missing:
            raise SystemExit(f'cached scenes miss maps {missing}')
        scenes = [row for m in SCENE_MAPS for row in by_map[m]]
    elif not args.skip_scenes:
        disc, files = extract.disc_files(str(args.disc))
        for number in SCENE_MAPS:
            scenes += scene_rows(disc, files, number, names[str(number)])

    report = dict(schema='FFT_PACK1_INVENTORY_1', pack='I',
                  scope='Sizing only: measured source geometry/textures through the production '
                        'view codec and the accepted scene probe. No authored C2 art, no camera '
                        'choice, no crop approval, no shipping bank change.',
                  archive=dict(ceiling=ARCHIVE_CEILING, reserve=RESERVE, core_stored=CORE_STORED,
                               per_variable=ARCHIVE_PER_VARIABLE),
                  maps=maps, scenes=scenes)

    report['map_totals'] = dict(
        count=len(maps), stored_bytes=sum(m['stored_bytes'] for m in maps),
        over_depth=[m['map'] for m in maps if not m['fits_depth']], depth_max=DEPTH_MAX)
    if scenes:
        per_room = {}
        for row in scenes:
            # One direction per scene is the authoring default; which one is an art
            # decision, so size on the dearer of the two measured directions.
            per_room.setdefault(row['width'], {}).setdefault(row['map'], []).append(row['zx0'])
        report['scene_totals'] = {str(w): dict(
            rooms=len(v), worst_direction_bytes=sum(max(z) for z in v.values()),
            cheaper_direction_bytes=sum(min(z) for z in v.values()),
            per_room_max=max(max(z) for z in v.values()))
            for w, v in per_room.items()}

    scenes_320 = report.get('scene_totals', {}).get('320', {}).get('worst_direction_bytes')
    measured = dict(battle_map_views=report['map_totals']['stored_bytes'],
                    scene_pictures=scenes_320, actor_frames=PACK1_ACTORS_NATIVE30,
                    event_text=PACK1_TEXT_CHUNKED)
    # Carried from the shipped Chapter I pilot rather than measured for the new maps:
    # its chapter bytes that are neither views nor actor banks, split per map and per
    # chapter (packages.json of the ABI99 release).
    carried = dict(map_geometry_and_descriptors=PILOT_PER_MAP_DATA * len(maps),
                   world_map=PILOT_WORLD_MAP, rumors_pack_save=PILOT_CHAPTER_MISC)
    variables = 4 * len(maps) + len(SCENE_MAPS) + CORE_ARCHIVED_VARIABLES + 20
    total = (CORE_ARCHIVED + sum(v for v in measured.values() if v) + sum(carried.values())
             + ARCHIVE_PER_VARIABLE * variables)
    budget = ARCHIVE_CEILING - RESERVE
    report['verdict'] = dict(
        budget_after_reserve=budget, core_archived_today=CORE_ARCHIVED,
        measured=measured, carried_from_pilot=carried,
        estimated_archive_headers=ARCHIVE_PER_VARIABLE * variables,
        total=total, margin=budget - total, fits=total <= budget,
        not_counted=['flow and dialogue choreography beyond the raw event text',
                     'encounters, objectives and deployment for the eleven battles',
                     'chapter portraits still missing from the shared bank',
                     'the cold text reader and its indexes',
                     'the ten optional adapted effects',
                     'C2 fidelity: measured views are a default-four-grey proxy, within about '
                     '10% of the shipped pilot views either way'],
        note='The margin is what has to absorb the list above. Every figure assumes an '
             'otherwise empty Titanium; run experiments/archfree on the target calculator.')
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({k: report[k] for k in ('map_totals', 'scene_totals', 'verdict')
                      if k in report}, indent=2))


if __name__ == '__main__':
    main()
