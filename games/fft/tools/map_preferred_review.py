#!/usr/bin/env python3
"""A C-first Zeklaus proposal and a separate one-pixel cactus-outline comparison.

Private offline art review only. Borrow existing extraction/actors/cameras;
never change accepted art, terrain, occlusion or shipping resources.
"""
import argparse
import json
from pathlib import Path
import struct

import numpy as np
from PIL import Image, ImageDraw, ImageFont
from scipy import ndimage

import extract
import map_art_review as review
import isometric_style as style
from review_files import save_image, write_changed

CROSS = np.array([[0,1,0],[1,1,1],[0,1,0]],bool)


def source_data(disc, files, info):
    source = info['source']
    extract.MAP = source['map']
    mesh = extract.mesh_blob(disc, files)
    texture_bytes = disc.read(*source['texture_record'][1:])
    assert review.sha(mesh) == source['mesh_sha256']
    assert review.sha(texture_bytes) == source['texture_sha256']
    t = np.frombuffer(texture_bytes,np.uint8)
    texture = np.empty((1024,256),np.uint8)
    texture[:,0::2] = (t & 15).reshape(1024,128)
    texture[:,1::2] = (t >> 4).reshape(1024,128)
    palette = np.array(struct.unpack_from('<256H',mesh,struct.unpack_from('<I',mesh,0x44)[0])).reshape(16,16)
    polygons = extract.polygons(mesh)
    # Explicit source-material classification for this hash-checked map only.
    # Cacti use transparent palette9/page2 vertical cards, not terrain faces.
    props = []
    indices = []
    for i,polygon in enumerate(polygons):
        v,uv,pl,pg = polygon
        span = np.ptp(v,axis=0)
        if uv is not None and pl == 9 and pg == 2 and span[1] == 48 and max(span[0],span[2]) == 14:
            props.append(polygon)
            indices.append(i)
    assert source['map'] == 76 and len(props) == 64, 'Review material classification for a different source mesh'
    return polygons,props,texture,palette,indices


def cactus_masks(props,texture,palette,view,info,rot):
    source = info['source']
    rows,width,k = source['canvas_rows'],source['canvas_width'],extract.K
    _,_diagonal,_face,depth = extract.render(props,texture,palette,rot,source['nx'],source['nz'],rows,return_depth=True)
    present = depth > -1e8
    visible = present & np.isclose(depth,view['depth'],rtol=0,atol=1e-9)
    mask = visible.reshape(rows,k,width,k).sum((1,3)) >= 4
    local_depth = np.where(visible,depth,-1e9).reshape(rows,k,width,k).max((1,3))
    ring = ndimage.binary_dilation(mask,structure=CROSS) & ~mask & ~view['background']
    neighbour = ndimage.maximum_filter(np.where(mask,local_depth,-1e9),footprint=CROSS)
    # Do not draw a halo over a closer intervening source surface.
    foreground = view['depth'][k//2::k,k//2::k] > neighbour + .15
    ring &= ~foreground
    assert not (ring & mask).any()
    assert not (ring & view['background']).any()
    assert not (ring & foreground).any()
    return mask,ring


def fonts():
    path = '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
    return ImageFont.truetype(path,20),ImageFont.truetype(path,15)


def comparison(out,root,images,labels=None,filename='C-A-my-proposal.png',heading=None):
    title,small = fonts()
    if labels is None:
        labels = [('Reference','Disc palette colors'),('A','Source textures'),('C','Light sand'),
                  ('F0','My mix / no outline'),('F1','My mix / white outline')]
    atlas = Image.new('RGB',(1518,len(labels)*240+70),'#efefef')
    draw = ImageDraw.Draw(atlas)
    draw.text((12,12),heading or 'ZEKLAUS / C-first proposal + cactus outline / same cameras at 2x',fill='black',font=title)
    for row,(key,name) in enumerate(labels):
        y = 62+row*240
        draw.text((12,y+54),key,fill='black',font=title)
        for i,part in enumerate(name.split(' / ')):
            draw.text((12,y+90+22*i),part,fill='black',font=small)
        for rot in range(4):
            if key == 'Reference':
                picture = Image.open(root/'reference'/f'lcd-{rot}.png').convert('RGB')
            elif key in ('A','C'):
                picture = Image.open(root/key/f'lcd-{rot}.png').convert('RGB')
            else:
                picture = images[key,rot]
            x = 190+rot*332
            draw.text((x,y),f'VIEW {rot+1}',fill='black',font=small)
            atlas.paste(picture.resize((320,200),Image.Resampling.NEAREST),(x,y+24))
    save_image(atlas,out/filename)


def detail_sheet(out,details):
    title,small = fonts()
    atlas = Image.new('RGB',(4*270+12,4*290+80),'#efefef')
    draw = ImageDraw.Draw(atlas)
    draw.text((12,12),'CACTUS / same64x64 patch / 4x pixels / one-pixel outside contour',fill='black',font=title)
    labels = ('Disc colors','C before','F0 no outline','F1 white outline')
    for rot,patches in enumerate(details):
        y = 64+rot*290
        for col,(name,image) in enumerate(zip(labels,patches)):
            x = 12+col*270
            draw.text((x,y),f'VIEW {rot+1} / {name}',fill='black',font=small)
            atlas.paste(image.resize((256,256),Image.Resampling.NEAREST),(x,y+24))
    save_image(atlas,out/'cactus-before-after.png')


def full_sheet(out,pictures,info,filename='F1-four-views.png',heading=None):
    title,small = fonts()
    width,rows = info['source']['canvas_width'],info['scene_rows']
    atlas = Image.new('RGB',(2*(width*2+18)+12,2*(rows*2+40)+64),'#efefef')
    draw = ImageDraw.Draw(atlas)
    draw.text((12,12),heading or 'F1 / C-first texture + white cactus outline / complete map at 2x',fill='black',font=title)
    for rot in range(4):
        x,y = 12+(rot%2)*(width*2+18),58+(rot//2)*(rows*2+40)
        draw.text((x,y),f'VIEW {rot+1}',fill='black',font=small)
        atlas.paste(pictures[rot].resize((width*2,rows*2),Image.Resampling.NEAREST),(x,y+24))
    save_image(atlas,out/filename)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--review',type=Path,default=review.HERE/'campaign/x/zeklaus-review')
    parser.add_argument('--out',type=Path,default=review.HERE/'campaign/x/zeklaus-preferred')
    parser.add_argument('--disc',type=Path,default=review.core_audit.DEFAULT_DISC)
    args = parser.parse_args()
    root,out = args.review.resolve(),args.out.resolve()
    out.mkdir(parents=True,exist_ok=True)
    assert root != out
    info = json.loads((root/'results.json').read_text())
    preserved = {str(p):[review.sha(p.read_bytes()),p.stat().st_mtime_ns] for p in root.rglob('*')
                 if p.is_file() and p.suffix != '.ti-view-decision'}
    disc,files = extract.disc_files(str(args.disc))
    polygons,props,texture,palette,indices = source_data(disc,files,info)
    source = info['source']
    extract.SC_W = source['canvas_width']
    extract.SC_OY = 64
    extract.SOURCE_CAMERA = True
    top,end = info['scene_top'],info['scene_top']+info['scene_rows']
    terrain = (root/'terrain.bin').read_bytes()
    edges = style.ledges(polygons)
    crops,full,details,stats = {},[],[],[]
    actors = {}
    for rot in range(4):
        with np.load(root/f'raster-{rot}.npz',allow_pickle=False) as cache:
            view = {name:cache[name].copy() for name in cache.files}
        thresholds = np.percentile(view['luminance'][~view['background']],[14,42,73])
        plain = review.quantize(view['luminance'],thresholds)
        plain[view['background']] = 0
        plain,_ = style.outline(plain,view['depth'],edges,rot,source['nx'],source['nz'])
        plain[view['background']] = 0
        cactus,ring = cactus_masks(props,texture,palette,view,info,rot)
        outlined = plain.copy()
        outlined[ring] = 0
        assert np.array_equal(plain[~ring],outlined[~ring])
        assert set(np.unique(outlined)).issubset({0,1,2,3})
        cx,cy = info['views'][rot]['camera']
        rendered = {}
        for key,pixels in (('F0',plain),('F1',outlined)):
            pixels = pixels[top:end]
            folder = out/key
            folder.mkdir(exist_ok=True)
            write_changed(folder/f'view-{rot}.planes',style.pack(pixels))
            save_image(review.gray_image(pixels),folder/f'view-{rot}.png')
            image,_ = review.preview_actors(pixels,view,terrain,rot,source['nx'],source['nz'],top,actors)
            rendered[key] = review.gray_image(image)
            crops[key,rot] = rendered[key].crop((cx,cy,cx+160,cy+100))
            save_image(crops[key,rot],folder/f'lcd-{rot}.png')
        full.append(rendered['F1'])
        # Each row compares the same largest visible cactus, not arbitrary crops.
        labels,n = ndimage.label(cactus[top:end],structure=CROSS)
        count = np.bincount(labels.ravel())
        count[0] = 0
        biggest = labels == int(count.argmax())
        yy,xx = np.nonzero(biggest)
        x = max(0,min(source['canvas_width']-64,int((xx.min()+xx.max())/2)-32))
        y = max(0,min(info['scene_rows']-64,int((yy.min()+yy.max())/2)-32))
        box = (x,y,x+64,y+64)
        details.append([Image.open(root/'reference'/f'view-{rot}.png').convert('RGB').crop(box),
                        Image.open(root/'C'/f'view-{rot}.png').convert('RGB').crop(box),
                        review.gray_image(plain[top:end]).crop(box),review.gray_image(outlined[top:end]).crop(box)])
        save_image(Image.fromarray((cactus[top:end]*255).astype(np.uint8)),out/f'cactus-mask-{rot}.png')
        save_image(Image.fromarray((ring[top:end]*255).astype(np.uint8)),out/f'cactus-ring-{rot}.png')
        stats.append(dict(view=rot+1,thresholds=thresholds.tolist(),visible_cactus_pixels=int(cactus.sum()),
                          ring_pixels=int(ring.sum()),changed_pixels=int(np.count_nonzero(plain != outlined)),
                          changes_outside_ring=0,camera=[cx,cy],detail_box=box,depth_unchanged=True))
        print(f'F view{rot+1}: cactus pixels{int(cactus.sum())}, outline pixels{int(ring.sum())}',flush=True)
    comparison(out,root,crops)
    detail_sheet(out,details)
    full_sheet(out,full,info)
    assert all([review.sha(Path(p).read_bytes()),Path(p).stat().st_mtime_ns] == record for p,record in preserved.items())
    report = dict(schema='FFT_ZEKLAUS_PREFERRED_REVIEW_1',preference='User prefers C, then A; no final selection yet',
                  proposal='F0: C-first14/42/73 luminance percentiles with existing geometric ledges; F1 adds white1px cactus contour',
                  source_mesh_sha256=source['mesh_sha256'],cactus_polygon_indices=indices,views=stats,
                  source_review_files_preserved=len(preserved),old_candidates_changed=False,
                  scope='Offline candidate only; same geometry, scale, cameras, terrain and depths. No runtime shader/native tests/shipping changes.',
                  width_adaptation_still_required=True,script_sha256=review.sha(Path(__file__).read_bytes()))
    write_changed(out/'results.json',(json.dumps(report,indent=2)+'\n').encode())


if __name__ == '__main__':
    main()
