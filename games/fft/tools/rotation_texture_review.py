#!/usr/bin/env python3
"""Offline visual feasibility study: original textures through actual F1/F5 rotations.

C supplies affine mesh projection, winning faces, depth-tested actors and real key/state
updates. Python samples original UVs for the review only; it is not TI runtime texturing.
"""
import hashlib
import json
from pathlib import Path
import subprocess

import numpy as np
from PIL import Image, ImageDraw
from attack_texture import load_scene
from perspective_demo import gif
from texture_review import HERE, TONES, normalize

OUT = HERE/'x/rotation-textures'


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    source = OUT/'frames'; source.mkdir(exist_ok=True)
    binary = OUT/'capture-pc'
    subprocess.run(['cc','-std=gnu99','-O1','-g','-DFFT_EXPLORATION','-DFFT_FIXED','-DFFT_CINEMA',
                    '-DFFT_TEXTURE_REVIEW','-o',str(binary),'tools/rotation_texture_capture.c',
                    '../../runtime/core/rt_core.c','../../runtime/platform-sw/rt_sw.c'],cwd=HERE,check=True)
    log = subprocess.check_output([str(binary),str(source)],cwd=HERE,text=True)
    (source/'frames.jsonl').write_text(log)
    rows = [json.loads(line) for line in log.splitlines()]
    faces, tex, palettes, thresholds, strong, water_faces = load_scene()
    lum_pal = ((palettes & 31)*.299+((palettes >> 5) & 31)*.587+((palettes >> 10) & 31)*.114)*255/31
    offsets = [-.375,-.125,.125,.375]
    pictures = []; samples = []; coverage = []
    for row in rows:
        n = row['frame']; base = np.array(normalize(source/f'{n:03d}.png').convert('L'))
        image = base.copy()
        if row['turn']:
            vertices = np.array(row['vertices'],float)
            labels = np.frombuffer((source/f'{n:03d}.faces').read_bytes(),'>u2').reshape(100,160)
            actors = np.unpackbits(np.frombuffer((source/f'{n:03d}.actors').read_bytes(),np.uint8).reshape(100,20),axis=1) != 0
            count_pixels = 0
            for i,(indices,uv,palette,page) in enumerate(faces):
                if uv is None:
                    continue
                yy,xx = np.nonzero((labels == i+1) & ~actors)
                if not len(xx):
                    continue
                a,b,c = vertices[indices,:2]; uv = np.array(uv,float)
                matrix = np.stack([b-a,c-a],axis=1)
                assert abs(np.linalg.det(matrix)) > 0
                inv = np.linalg.inv(matrix)
                accum = np.zeros(len(xx)); count = np.zeros(len(xx),int)
                for oy in offsets:
                    for ox in offsets:
                        vw = (np.stack([xx+.5+ox,yy+.5+oy],axis=1)-a)@inv.T
                        coord = uv[0]+vw[:,0,None]*(uv[1]-uv[0])+vw[:,1,None]*(uv[2]-uv[0])
                        u,v = np.floor(coord).astype(int).T
                        index = tex[np.clip(v,0,255)+256*page,np.clip(u,0,255)]
                        opaque = palettes[palette,index] != 0
                        accum += np.where(opaque,lum_pal[palette,index],0); count += opaque
                valid = count > 0; luminosity = accum/np.maximum(count,1)
                q = thresholds if water_faces[i] else strong
                grey = np.where(luminosity > q[2],0,np.where(luminosity > q[1],1,np.where(luminosity > q[0],2,3)))
                if water_faces[i]:
                    grey = np.clip(grey,1,2)
                image[yy[valid],xx[valid]] = TONES[grey[valid]]
                count_pixels += int(valid.sum())
            assert count_pixels > 2000
            assert np.array_equal(image[actors],base[actors])
            assert np.array_equal(image[labels == 0],base[labels == 0])
            coverage.append(count_pixels)
        canvas = Image.new('RGB',(640,440),'white'); draw = ImageDraw.Draw(canvas)
        direction = 'F5: CLOCKWISE' if n < 76 else 'F1: COUNTERCLOCKWISE'
        draw.text((4,3),'TEXTURED ISOMETRIC ROTATION / '+direction,fill='black')
        canvas.paste(Image.fromarray(image).resize((640,400),Image.Resampling.NEAREST),(0,20))
        draw.text((4,425),'Offline PC texture review - 20 FPS preview, not TI performance',fill='black')
        pictures.append(canvas)
        if row['turn'] and row['sample'] == 1 and n < 76:
            samples.append(canvas)
    assert len(rows) == 160 and len(coverage) == 24
    assert rows[-1]['rot'] == rows[0]['rot'] and rows[-1]['turn'] == 0
    gif(OUT/'f1-f5-textured.gif',pictures,[50]*len(pictures))
    sheet = Image.new('RGB',(1280,880),'white')
    for i,img in enumerate(samples):
        sheet.paste(img,((i%2)*640,(i//2)*440))
    sheet.save(OUT/'review.png')
    manifest = dict(scope='PC offline visual feasibility study; no TI texturing code or FPS claim.',
                    real_game_frames=len(rows),textured_rotation_frames=len(coverage),
                    keys='Four F5 quarter-turns then four F1 quarter-turns; real game_update.',
                    checks='Units, RNG and tactical mode unchanged; returns to original rotation; actor pixels and sky preserved.',
                    source_uv_faces=len(faces),textured_pixels_min=min(coverage),textured_pixels_max=max(coverage),
                    data_policy='No persistent full-screen rotation bank; generated frames are local review artifacts only.',
                    sources={name:hashlib.sha256((HERE/name).read_bytes()).hexdigest() for name in
                             ['tools/rotation_texture_capture.c','tools/rotation_texture_review.py','tools/attack_texture.py']})
    (OUT/'manifest-pc.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(json.dumps(manifest,indent=2))


if __name__ == '__main__':
    main()
