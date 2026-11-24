#!/usr/bin/env python3
"""Mark source sand/rock interfaces, retaining the pending cactus/texture proposals."""
import argparse
from collections import defaultdict
import json
from pathlib import Path
import tempfile

import numpy as np
from PIL import Image

import extract
import isometric_style as style
import map_art_review as review
import map_preferred_review as preferred
from review_files import save_image, write_changed


def material_edges(polygons):
    adjacency = defaultdict(list)
    for v,uv,palette,_page in polygons:
        n = np.cross(v[1]-v[0],v[2]-v[0])
        length = np.linalg.norm(n)
        # The existing ledge treatment already covers top/wall boundaries.
        # Add only top/top interfaces; equal-material tessellation stays invisible.
        if uv is None or not length or abs(n[1])/length <= .5 or palette in (9,15):
            continue
        material = 'sand' if palette in (0,1) else 'rock'
        for a,b in zip(v,np.roll(v,-1,axis=0)):
            edge = tuple(sorted((tuple(a.astype(int)),tuple(b.astype(int)))))
            adjacency[edge].append(material)
    selected = [np.array(e,float) for e,materials in adjacency.items() if set(materials)=={'sand','rock'}]
    assert len(selected) == 52, 'Recheck source material classification for this map'
    return selected


def read_grey(path):
    pixels = np.asarray(Image.open(path).convert('L'))
    assert set(np.unique(pixels)).issubset({0,85,170,255})
    return ((255-pixels)//85).astype(np.uint8)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--review',type=Path,default=review.HERE/'campaign/x/zeklaus-review')
    parser.add_argument('--preferred',type=Path,default=review.HERE/'campaign/x/zeklaus-preferred')
    parser.add_argument('--out',type=Path,default=review.HERE/'campaign/x/zeklaus-materials')
    parser.add_argument('--disc',type=Path,default=review.core_audit.DEFAULT_DISC)
    args = parser.parse_args()
    root,prior,out = args.review.resolve(),args.preferred.resolve(),args.out.resolve()
    assert out not in (root,prior)
    out.mkdir(parents=True,exist_ok=True)
    before = {str(p):[review.sha(p.read_bytes()),p.stat().st_mtime_ns] for folder in (root,prior)
              for p in folder.rglob('*') if p.is_file() and p.suffix != '.ti-view-decision'}
    info = json.loads((root/'results.json').read_text())
    disc,_ = extract.disc_files(str(args.disc))
    polygons,*_rest = preferred.source_data(disc,info)
    edges = material_edges(polygons)
    source = info['source']
    extract.SC_W = source['canvas_width']
    extract.SC_OY = 64
    extract.SOURCE_CAMERA = True
    top,end = info['scene_top'],info['scene_top']+info['scene_rows']
    terrain = (root/'terrain.bin').read_bytes()
    crops,full,stats = {},{'C2':[],'F2':[]},[]
    actors = {}
    # Also exercise the shared sheet defaults against their existing exact PNG.
    default_crops = {(key,rot):Image.open(prior/key/f'lcd-{rot}.png').convert('RGB')
                     for key in ('F0','F1') for rot in range(4)}
    original_full = []
    for rot in range(4):
        with np.load(root/f'raster-{rot}.npz',allow_pickle=False) as cache:
            view = {name:cache[name].copy() for name in cache.files}
        ring = np.asarray(Image.open(prior/f'cactus-ring-{rot}.png').convert('L')) != 0
        f1 = read_grey(prior/'F1'/f'view-{rot}.png')
        c1 = read_grey(root/'C'/f'view-{rot}.png')
        c1[ring] = 0
        cx,cy = info['views'][rot]['camera']
        original,_ = review.preview_actors(f1,view,terrain,rot,source['nx'],source['nz'],top,actors)
        original_full.append(review.gray_image(original))
        crops['F1',rot] = default_crops['F1',rot]
        per_view = {}
        for key,base in (('F2',f1),('C2',c1)):
            expanded = np.zeros(view['luminance'].shape,np.uint8)
            expanded[top:end] = base
            bordered,mask = style.outline(expanded,view['depth'],edges,rot,source['nx'],source['nz'])
            mask &= ~view['background']
            # Cactus rings win when a material edge meets a prop silhouette.
            mask[top:end] &= ~ring
            result = expanded.copy()
            result[mask] = bordered[mask]
            assert np.array_equal(result[~mask],expanded[~mask])
            result = result[top:end]
            folder = out/key
            folder.mkdir(exist_ok=True)
            write_changed(folder/f'view-{rot}.planes',style.pack(result))
            save_image(review.gray_image(result),folder/f'view-{rot}.png')
            rendered,_ = review.preview_actors(result,view,terrain,rot,source['nx'],source['nz'],top,actors)
            full[key].append(review.gray_image(rendered))
            crops[key,rot] = full[key][-1].crop((cx,cy,cx+160,cy+100))
            save_image(crops[key,rot],folder/f'lcd-{rot}.png')
            per_view[key] = dict(changed_pixels=int(np.count_nonzero(result != base)),
                                 material_edge_pixels=int(mask[top:end].sum()),changes_outside_edge_mask=0)
        stats.append(dict(view=rot+1,styles=per_view,camera=[cx,cy],depth_unchanged=True))
        print('Material boundaries view'+str(rot+1)+': '+json.dumps(per_view),flush=True)
    labels = [('Reference','Disc palette colors'),('C','C / original'),('C2','C / cactus + materials'),
              ('F1','My mix / cactus only'),('F2','My mix / both outlines')]
    preferred.comparison(out,root,crops,labels,'material-comparison.png',
                         'ZEKLAUS / sand-stone boundaries + cactus outlines / same cameras at 2x')
    for key,pictures in full.items():
        preferred.full_sheet(out,pictures,info,key+'-four-views.png',
                             key+' / white cactus contour + material boundaries / full map at 2x')
    with tempfile.TemporaryDirectory(prefix='sheet-check-',dir=out) as temporary:
        folder = Path(temporary)
        preferred.comparison(folder,root,default_crops)
        preferred.full_sheet(folder,original_full,info)
        for name in ('C-A-my-proposal.png','F1-four-views.png'):
            assert (folder/name).read_bytes() == (prior/name).read_bytes(), 'Shared default sheet changed'
    assert all([review.sha(Path(p).read_bytes()),Path(p).stat().st_mtime_ns] == record for p,record in before.items())
    report = dict(schema='FFT_ZEKLAUS_MATERIAL_REVIEW_1',source_mesh_sha256=source['mesh_sha256'],
                  source_top_materials={'sand_palettes':[0,1],'rock_palettes':[2,3,4],'excluded_props':[9,15]},
                  top_material_interface_edges=len(edges),views=stats,old_review_files_preserved=len(before),
                  shared_default_sheets_identical=True,source_geometry_and_depth_unchanged=True,
                  scope='Offline pending art only: F2 adds sand/rock interfaces to F1; C2 uses C with cactus and material contours. No TI/runtime/shipping change.',
                  width_adaptation_still_required=True,script_sha256=review.sha(Path(__file__).read_bytes()))
    write_changed(out/'results.json',(json.dumps(report,indent=2)+'\n').encode())


if __name__ == '__main__':
    main()
