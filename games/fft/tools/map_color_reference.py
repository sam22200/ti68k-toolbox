#!/usr/bin/env python3
"""Add original-disc palette colors beside a pending four-grey map review.

This is an unlit reconstruction at the TI projection, not an original PS1
screenshot. It preserves all pending candidate images and shipping resources.
"""
import argparse
import json
from pathlib import Path
import struct

import numpy as np
from PIL import Image, ImageDraw, ImageFont

import extract
import map_art_review as review
from review_files import save_image, write_changed


def color_views(disc, report, directory):
    source = report['source']
    mesh = disc.read(*source['mesh_record'][1:])
    texture_bytes = disc.read(*source['texture_record'][1:])
    assert review.sha(mesh) == source['mesh_sha256']
    assert review.sha(texture_bytes) == source['texture_sha256']
    terrain = (directory/'terrain.bin').read_bytes()
    assert review.sha(terrain) == source['terrain']['terrain_sha256']
    t = np.frombuffer(texture_bytes,np.uint8)
    texture = np.empty((1024,256),np.uint8)
    texture[:,0::2] = (t & 15).reshape(1024,128)
    texture[:,1::2] = (t >> 4).reshape(1024,128)
    palette = np.array(struct.unpack_from('<256H',mesh,struct.unpack_from('<I',mesh,0x44)[0])).reshape(16,16)
    polygons = extract.polygons(mesh)
    extract.SC_W = source['canvas_width']
    extract.SC_OY = 64
    extract.SOURCE_CAMERA = True
    width,rows = source['canvas_width'],source['canvas_rows']
    top,end = report['scene_top'],report['scene_top']+report['scene_rows']
    images,crops,checks = [],[],[]
    sprites = {}
    for rot in range(4):
        with np.load(directory/f'raster-{rot}.npz',allow_pickle=False) as cache:
            original = {name:cache[name].copy() for name in cache.files}
        visible = original['depth'] > -1e8
        rgb = np.empty((rows*extract.K,width*extract.K,3),float)
        # Reuse the exact proven polygon/texture rasterizer for each palette
        # channel. Bit15 preserves opaque zero-valued channels without changing
        # the RGB555 values or the source's transparent-zero classification.
        for channel,(shift,weight) in enumerate(((0,.299),(5,.587),(10,.114))):
            one_channel = (palette & (31 << shift)) | np.where(palette != 0,0x8000,0).astype(np.uint16)
            luminance,diagonal,_face,depth = extract.render(polygons,texture,one_channel,rot,
                                                          source['nx'],source['nz'],rows,return_depth=True)
            assert np.array_equal(depth,original['depth']), 'Palette pass changed source geometry/transparency'
            assert np.array_equal(diagonal[extract.K//2::extract.K,extract.K//2::extract.K],original['diagonal'])
            rgb[:,:,channel] = np.where(visible,luminance/weight,255)
        # The three recovered channels must produce the existing unquantized
        # source luminance, apart from the deliberately white exterior pixels.
        luminance = np.where(visible,rgb @ np.array([.299,.587,.114]),extract.BG)
        small_lum = luminance.reshape(rows,extract.K,width,extract.K).mean((1,3))
        assert np.allclose(small_lum,original['luminance'],rtol=0,atol=1e-10)
        small = np.rint(rgb.reshape(rows,extract.K,width,extract.K,3).mean((1,3))).clip(0,255).astype(np.uint8)
        color = small[top:end].copy()
        # Reuse the exact same approved greyscale actor fixtures/cover cutout;
        # these are illustrative scale markers, not original color PS1 actors.
        stencil = np.full(color.shape[:2],255,np.uint8)
        stencil,_ = review.preview_actors(stencil,original,terrain,rot,source['nx'],source['nz'],top,sprites)
        with_actors = color.copy()
        mask = stencil < 4
        with_actors[mask] = review.TONES[stencil[mask],None]
        cx,cy = report['views'][rot]['camera']
        crop = with_actors[cy:cy+100,cx:cx+160]
        save_image(Image.fromarray(color),directory/'reference'/f'view-{rot}.png')
        save_image(Image.fromarray(with_actors),directory/'reference'/f'scene-{rot}.png')
        save_image(Image.fromarray(crop),directory/'reference'/f'lcd-{rot}.png')
        images.append(Image.fromarray(with_actors))
        crops.append(Image.fromarray(crop))
        checks.append(dict(view=rot+1,geometry_and_masks_equal=True,luminance_matches=True,
                           camera=[cx,cy],source_palette_colors=int(np.unique(small.reshape(-1,3),axis=0).shape[0])))
        print(f'Color reference view{rot+1}: exact source geometry/transparency and source luminance verified',flush=True)
    return images,crops,checks


def sheets(directory, images, crops, report):
    font_path = '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
    title = ImageFont.truetype(font_path,20)
    label = ImageFont.truetype(font_path,15)
    width,rows = report['source']['canvas_width'],report['scene_rows']
    full = Image.new('RGB',(2*(width*2+18)+12,2*(rows*2+40)+88),'#efefef')
    draw = ImageDraw.Draw(full)
    draw.text((12,12),'REFERENCE / original disc palette colors / four complete views at 2x',fill='black',font=title)
    draw.text((12,40),'Unlit reconstruction at the same projection; not a PS1 screenshot. Actors are the same TI scale fixtures.',fill='black',font=label)
    for rot,image in enumerate(images):
        x,y = 12+(rot%2)*(width*2+18),82+(rot//2)*(rows*2+40)
        draw.text((x,y),f'VIEW {rot+1}',fill='black',font=label)
        full.paste(image.resize((width*2,rows*2),Image.Resampling.NEAREST),(x,y+24))
    save_image(full,directory/'reference-four-views.png')
    old = Image.open(directory/'choose-A-E.png').convert('RGB')
    combined = Image.new('RGB',(old.width,old.height+240),'#efefef')
    draw = ImageDraw.Draw(combined)
    draw.text((12,8),'ZEKLAUS / original disc colors + A-E / identical160x100 camera cuts at 2x',fill='black',font=title)
    draw.text((12,36),'Reference colors are reconstructed, without original lighting; this is not a PS1 capture.',fill='black',font=label)
    draw.text((12,116),'REFERENCE',fill='black',font=title)
    draw.text((12,150),'Disc palette colors',fill='black',font=label)
    for rot,image in enumerate(crops):
        x = 190+rot*332
        draw.text((x,62),f'VIEW {rot+1}',fill='black',font=label)
        combined.paste(image.resize((320,200),Image.Resampling.NEAREST),(x,86))
    combined.paste(old.crop((0,62,old.width,old.height)),(0,302))
    assert np.array_equal(np.asarray(combined)[302:],np.asarray(old)[62:]), 'Pending A-E comparison changed'
    save_image(combined,directory/'with-reference-A-E.png')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--review',type=Path,default=review.HERE/'campaign/x/zeklaus-review')
    parser.add_argument('--disc',type=Path,default=review.core_audit.DEFAULT_DISC)
    args = parser.parse_args()
    directory = args.review.resolve()
    report = json.loads((directory/'results.json').read_text())
    # Verify preservation of the existing verdict candidates, including dates.
    pending = [directory/'choose-A-E.png',*[directory/(letter+'-four-views.png') for letter,_ in review.PROFILES],
               *directory.glob('?/ffzv*.bin'),*directory.glob('?/ffzv*.89y')]
    before = {str(p):[review.sha(p.read_bytes()),p.stat().st_mtime_ns] for p in pending}
    disc,_ = extract.disc_files(str(args.disc))
    images,crops,checks = color_views(disc,report,directory)
    sheets(directory,images,crops,report)
    assert all([review.sha(Path(p).read_bytes()),Path(p).stat().st_mtime_ns] == record for p,record in before.items())
    evidence = dict(scope=__doc__,views=checks,pending_candidates_preserved=len(before),
                    source_mesh_sha256=report['source']['mesh_sha256'],source_texture_sha256=report['source']['texture_sha256'],
                    script_sha256=review.sha(Path(__file__).read_bytes()),native_runs=0,shipping_changes=False)
    write_changed(directory/'reference.json',(json.dumps(evidence,indent=2)+'\n').encode())
    print(f'Added four original-palette reference views; {len(before)} pending images/banks retain bytes and dates',flush=True)


if __name__ == '__main__':
    main()
