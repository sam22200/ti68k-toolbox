#!/usr/bin/env python3
"""Five offline four-view map proposals from one local disc extraction.

Full-width source geometry remains visible. The current320px crop is audited
separately, never silently accepted as playable. No shipping banks are changed.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct

import numpy as np
from PIL import Image, ImageDraw, ImageFont

import chapter_packages as packages
import core_audit
import extract
import isometric_style as style
import map_budget
from review_files import save_image, write_changed

HERE = Path(__file__).resolve().parents[1]
TONES = np.array([255, 170, 85, 0], np.uint8)
PROFILES = [('A', 'Source textures'), ('B', 'Contrast + ledges'),
            ('C', 'Light sand'), ('D', 'Soft texture + relief'), ('E', 'Ink')]


def sha(data):
    return hashlib.sha256(data).hexdigest()


def gray_image(grey):
    return Image.fromarray(TONES[grey]).convert('RGB')


def raster(disc, files, number, out):
    """Decode one primary mesh and texture; reuse only a hash-matched raster cache."""
    extract.MAP = number
    record = map_budget.mesh_record(disc, files, number)
    mesh, nx, nz, maxh = map_budget.load(disc, files, number)
    records = extract.gns_records(disc, files)
    texture_record = next(r for r in records if r[0] == 0x1701)
    texture_bytes = disc.read(*texture_record[1:])
    texture = np.empty((1024, 256), np.uint8)
    t = np.frombuffer(texture_bytes, np.uint8)
    texture[:, 0::2] = (t & 15).reshape(1024,128)
    texture[:, 1::2] = (t >> 4).reshape(1024,128)
    palettes = np.array(struct.unpack_from('<256H',mesh,struct.unpack_from('<I',mesh,0x44)[0])).reshape(16,16)
    polygons = extract.polygons(mesh)
    bounds = map_budget.bounds(polygons, nx, nz)
    assert min(b[0] for b in bounds) >= 0, 'This review needs a documented left projection offset'
    extract.SC_W = int(np.ceil((max(b[1] for b in bounds)+8)/16))*16
    extract.SOURCE_CAMERA = True
    rows = int(np.ceil(max(b[3] for b in bounds))) + 8
    key = dict(map=number,mesh=sha(mesh),texture=sha(texture_bytes),width=extract.SC_W,rows=rows,
               renderer=sha(Path(extract.__file__).read_bytes()),source_camera=True)
    key_path = out/'raster-key.json'
    cached = key_path.exists() and json.loads(key_path.read_text()) == key
    all_views = []
    for rot in range(4):
        cache = out/f'raster-{rot}.npz'
        if cached and cache.exists():
            with np.load(cache,allow_pickle=False) as source:
                view = {n:source[n].copy() for n in source.files}
        else:
            lum, diag, face, depth = extract.render(polygons,texture,palettes,rot,nx,nz,rows,return_depth=True)
            k,w = extract.K,extract.SC_W
            view = dict(luminance=lum.reshape(rows,k,w,k).mean((1,3)),
                        background=(lum == extract.BG).reshape(rows,k,w,k).mean((1,3)) > .5,
                        diagonal=diag[k//2::k,k//2::k], face=face[k//2::k,k//2::k], depth=depth)
            np.savez_compressed(cache,**view)
        all_views.append(view)
        print(f'map{number:03d} source-handed view{rot+1}: raster ready',flush=True)
    write_changed(key_path,(json.dumps(key,indent=2)+'\n').encode())
    terrain = core_audit.terrain(mesh, f'MAP{number:03d}')
    terrain_bytes = mesh[terrain['terrain_offset']:terrain['terrain_offset']+4098]
    write_changed(out/'terrain.bin',terrain_bytes)
    source = dict(map=number,mesh_record=record,mesh_sha256=sha(mesh),texture_record=texture_record,
                  texture_sha256=sha(texture_bytes),triangles=len(polygons),nx=nx,nz=nz,max_height=maxh,
                  canvas_width=extract.SC_W,canvas_rows=rows,unreflected_bounds=bounds,terrain=terrain)
    return all_views, polygons, terrain_bytes, source


def quantize(luminance, thresholds):
    return (3 - np.searchsorted(thresholds,luminance,side='left')).astype(np.uint8)


def variants(view, thresholds, edges, rot, nx, nz):
    lum,bg,face = view['luminance'],view['background'],view['face']
    strong = thresholds[1] + (thresholds-thresholds[1])*.65
    original = quantize(lum,thresholds)
    contrast = quantize(lum,strong)
    light = quantize(lum,np.percentile(lum[~bg],[10,34,65]))
    # Suppress small texture noise within the original polygon/material faces.
    # No silhouette blur, rescale, terrain edit or generated scenery is involved.
    soft = lum.copy()
    for label in np.unique(face[face >= 0]):
        mask = (face == label) & ~bg
        if mask.any():
            soft[mask] = .3*lum[mask] + .7*np.median(lum[mask])
    soft[(face >= 0) & (face % 5 == 0)] += 8
    soft[(face >= 0) & (face % 5 != 0)] -= 8
    relief = quantize(soft,strong)
    ink = np.where(lum > thresholds[1],0,3).astype(np.uint8)
    results = [original,contrast,light,relief,ink]
    mask = None
    for i in range(len(results)):
        results[i][bg] = 0
        if i:
            results[i], outline = style.outline(results[i],view['depth'],edges,rot,nx,nz)
            if mask is None:
                mask = outline
            else:
                assert np.array_equal(mask,outline)
            results[i][bg] = 0
    return results


def depth_planes(diagonal):
    """Reuse the extractor's two-front-depth rule at the current width."""
    rows,width = diagonal.shape
    d = diagonal.copy()
    layers = []
    for _ in range(2):
        nearest = d.reshape(rows,width//8,8).max(2)
        front = (d == np.repeat(nearest,8,1)) & (d >= 0)
        layers += [np.maximum(nearest,0).astype(np.uint8),np.packbits(front,axis=1)]
        d = np.where(front,-1,d)
    return b''.join(p.tobytes() for p in layers)


def frame(logical, cache):
    """Borrow only the needed frames from the existing physical actor banks."""
    mapping = (HERE/'campaign/data/ffga.bin').read_bytes()
    key = struct.unpack_from('>H',mapping,2*logical)[0]
    if key not in cache:
        index = key & 0x7fff
        for name in ('ffca','ffcb','ffcc','ffcd','ffce','ffcf'):
            path = HERE/'campaign/data'/(name+'.bin')
            if not path.exists():
                raise ValueError('Missing shared actor frame')
            bank = path.read_bytes()
            count = struct.unpack_from('>H',bank)[0]
            if index >= count:
                index -= count
                continue
            offset = struct.unpack_from('>H',bank,2+2*index)[0]
            end = struct.unpack_from('>H',bank,4+2*index)[0] if index+1 < count else len(bank)-2-2*count
            raw = extract.unzx0(bank[2+2*count+offset:2+2*count+end])
            planes = np.unpackbits(np.frombuffer(raw,np.uint8).reshape(4,26,2),axis=2)
            if key & 0x8000:
                planes = planes[:,:,::-1]
            cache[key] = planes
            break
    return cache[key]


def world_point(x,z,h,rot,nx,nz):
    return style.project(np.array([[28*x,-12*h,28*z]],float),rot,nx,nz)[0]


def preview_actors(grey, view, terrain, rot, nx, nz, top, cache):
    """Scale/contrast fixtures, explicitly not an authored Zeklaus encounter."""
    pixels = grey.copy()
    placed = []
    # Fixed tactical identities/locations, reused in all20 proposals.
    for gfx,x,z in ((0,4,8),(2,6,8),(5,5,10)):
        cell = terrain[2+8*(z*nx+x):10+8*(z*nx+x)]
        assert not cell[6] & 3
        h = sum(extract.corners(cell[2],cell[3]&31,cell[4] if cell[3]&31 else 0))/4
        sx,sy,_depth = world_point(x+.5,z+.5,h,rot,nx,nz)
        u,v = [(x,z),(nz-1-z,x),(nx-1-x,nz-1-z),(z,nx-1-x)][rot]
        placed.append((u+v,sx,sy-top,gfx,(rot+0)&3))
    for tile_diagonal,sx,sy,gfx,direction in sorted(placed):
        sprite = frame(gfx*44+(5 if direction&1 else 0)+(22 if direction&2 else 0),cache)
        x0,y0 = int(sx)-8,int(sy)-21
        for yy in range(26):
            y = y0+yy
            if not 0 <= y < pixels.shape[0]:
                continue
            for xx in range(16):
                x = x0+xx
                if not 0 <= x < pixels.shape[1] or sprite[2,yy,xx]:
                    continue
                # Original per-pixel tile diagonals give a cutout for the review.
                # This is not the native two-depth/interior silhouette renderer.
                if view['diagonal'][y+top,x] > tile_diagonal:
                    continue
                pixels[y,x] = sprite[0,yy,xx]+2*sprite[1,yy,xx]
    return pixels,placed


def sheets(out, pictures, screens, width, rows):
    font_path = '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
    title_font = ImageFont.truetype(font_path,20)
    small_font = ImageFont.truetype(font_path,15)
    # One sheet with all20 actual160x100 crops at exactly2x nearest-neighbour.
    atlas = Image.new('RGB',(4*332+190,5*240+70),(239,239,239))
    draw = ImageDraw.Draw(atlas)
    draw.text((12,12),'ZEKLAUS / 5 styles x 4 views / 160x100 crops at 2x',fill='black',font=title_font)
    for row,(label,name) in enumerate(PROFILES):
        y = 62+row*240
        draw.text((12,y+55),label,fill='black',font=title_font)
        # Keep labels outside the calculator picture.
        for line,word in enumerate(name.split(' + ')):
            draw.text((12,y+90+line*22),word,fill='black',font=small_font)
        for rot in range(4):
            x = 190+rot*332
            draw.text((x,y),f'VIEW {rot+1}',fill='black',font=small_font)
            atlas.paste(gray_image(screens[label,rot]).resize((320,200),Image.Resampling.NEAREST),(x,y+24))
    save_image(atlas,out/'choose-A-E.png')
    # Full map sheets preserve the wider source bounds, same scale in all variants.
    for label,name in PROFILES:
        image = Image.new('RGB',(2*(width*2+18)+12,2*(rows*2+40)+64),'#efefef')
        draw = ImageDraw.Draw(image)
        draw.text((12,12),f'{label} / {name} / complete source map at 2x',fill='black',font=title_font)
        for rot in range(4):
            x = 12+(rot%2)*(width*2+18)
            y = 58+(rot//2)*(rows*2+40)
            draw.text((x,y),f'VIEW {rot+1}',fill='black',font=small_font)
            image.paste(gray_image(pictures[label,rot]).resize((width*2,rows*2),Image.Resampling.NEAREST),(x,y+24))
        save_image(image,out/f'{label}-four-views.png')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--map',type=int,default=76)
    parser.add_argument('--disc',type=Path,default=core_audit.DEFAULT_DISC)
    parser.add_argument('--out',type=Path,default=HERE/'campaign/x/zeklaus-review')
    args = parser.parse_args()
    out = args.out.resolve()
    out.mkdir(parents=True,exist_ok=True)
    disc,files = extract.disc_files(str(args.disc))
    views,polygons,terrain,source = raster(disc,files,args.map,out)
    nx,nz = source['nx'],source['nz']
    used = [np.flatnonzero((~v['background']).any(1)) for v in views]
    top = max(0,min(int(y[0]) for y in used)-4)
    end = min(source['canvas_rows'],max(int(y[-1]) for y in used)+5)
    rows,width = end-top,source['canvas_width']
    thresholds = np.percentile(views[0]['luminance'][~views[0]['background']],[25,55,82])
    edges = style.ledges(polygons)
    pictures,screens,cache,stats = {},{},{},[]
    for rot,view in enumerate(views):
        variants_ = variants(view,thresholds,edges,rot,nx,nz)
        # Full-width extraction is preserved. Current core coordinates are exactly
        # the rightmost320 columns of this source-handed wide canvas.
        left = width-320
        diagonal = view['diagonal'][top:end,left:]
        depths = depth_planes(diagonal)
        ragged = extract.ragged_depth(bytes(80*rows)+depths)
        compressed = extract.zx0pack.pack(ragged)
        assert extract.unragged_depth(ragged,rows) == depths
        outside = []
        for z in range(nz):
            for x in range(nx):
                cell = terrain[2+8*(z*nx+x):10+8*(z*nx+x)]
                if cell[6]&3:
                    continue
                point = world_point(x+.5,z+.5,cell[2],rot,nx,nz)
                if not left <= point[0] < width:
                    outside.append([x,z])
        camera = None
        for i,(label,_) in enumerate(PROFILES):
            grey = variants_[i][top:end]
            folder = out/label
            folder.mkdir(exist_ok=True)
            write_changed(folder/f'view-{rot}.planes',style.pack(grey))
            save_image(gray_image(grey),folder/f'view-{rot}.png')
            with_actors,placed = preview_actors(grey,view,terrain,rot,nx,nz,top,cache)
            pictures[label,rot] = with_actors
            if camera is None:
                center = np.mean([[p[1],p[2]] for p in placed],axis=0)
                camera = [max(0,min(width-160,int(center[0])-80)),max(0,min(rows-100,int(center[1])-62))]
            cx,cy = camera
            screens[label,rot] = with_actors[cy:cy+100,cx:cx+160]
            save_image(gray_image(screens[label,rot]),folder/f'lcd-{rot}.png')
            # Review-only R4 candidate sizes use exact current core320px coordinates.
            # These banks are not added to a chapter or accepted as playable crops.
            colors = style.pack(grey[:,left:])
            bank = b'R4'+struct.pack('>3H',len(colors),0,len(ragged))+colors+compressed
            write_changed(folder/f'ffzv{rot}.bin',bank)
            variable = packages.Variable(f'ffzv{rot}',0x1c,3,bank+b'\0dat\0\xf8')
            write_changed(folder/f'ffzv{rot}.89y',packages.group_bytes([variable],single=True))
            tag,actual,mask,back = extract.read_view(bank)
            assert tag == b'R4' and actual == colors and mask == b'' and back == depths
        stats.append(dict(view=rot+1,rows=rows,current_crop_left=left,depth_bytes=len(ragged),
                          compressed_depth_bytes=len(compressed),bank_bytes=len(bank),
                          full_map_color_bytes=2*(width//8)*rows,current_color_bytes=80*rows,
                          current_crop_excludes_stoppable_centers=outside,camera=camera,depth_sha256=sha(depths)))
        print(f'view{rot+1}: R4 candidate {len(bank)} bytes, depth {len(ragged)}, excluded centers {outside}',flush=True)
    sheets(out,pictures,screens,width,rows)
    depth_cap = map_budget.DEPTH_MAX
    report = dict(schema='FFT_MAP_ART_REVIEW_1',source=source,profiles=PROFILES,thresholds=thresholds.tolist(),
                  scene_top=top,scene_rows=rows,views=stats,units='Borrowed approved human sprites; illustrative fixtures, not source encounter',
                  source_camera=True,full_map_preserved=True,styles_share_depth=True,styles_share_camera=True,
                  nodes=nx*nz,axis=max(nx,nz),rows_fit=rows<=map_budget.SC_HMAX,
                  candidate_depth_fits=all(s['depth_bytes']<=depth_cap for s in stats),depth_capacity=depth_cap,
                  width_requires_adaptation=any(s['current_crop_excludes_stoppable_centers'] for s in stats),
                  next='Await visual choice, then adapt shared framing and validate real PC integration; native checks only if needed',
                  scope='Offline extraction/art/data-envelope review only. No CPU/installed RAM/real battle certification or shipping changes.',
                  script_sha256=sha(Path(__file__).read_bytes()))
    write_changed(out/'results.json',(json.dumps(report,indent=2)+'\n').encode())
    print(json.dumps({k:report[k] for k in ('nodes','axis','scene_rows','candidate_depth_fits','width_requires_adaptation','scope')},indent=2),flush=True)


if __name__ == '__main__':
    main()
