#!/usr/bin/env python3
"""Actual-game ISO comparison: original, stronger contrast, contrast plus height edges."""
import csv
import hashlib
import io
import json
import subprocess

from PIL import Image,ImageDraw
from texture_review import HERE,normalize
from perspective_demo import gif
from isometric_style import OUT

FLAGS = ['-DFFT_EXPLORATION','-DFFT_FIXED','-DFFT_CINEMA','-DFFT_TEXTURE_REVIEW','-DFFT_TEXTURE_PREVIEW']


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    binary = OUT/'capture-pc'
    subprocess.run(['cc','-std=gnu99','-O1','-g',*FLAGS,'-o',str(binary),
                    'tools/isometric_style_capture.c','../../runtime/core/rt_core.c',
                    '../../runtime/platform-sw/rt_sw.c'],cwd=HERE,check=True)
    labels = ['ORIGINAL','CONTRAST','CONTRAST + HEIGHT EDGES']
    images = []; reference = None
    for style,label in enumerate(labels):
        folder = OUT/f'style-{style}'; folder.mkdir(exist_ok=True)
        text = subprocess.check_output([str(binary),str(style),str(folder)],cwd=HERE,text=True)
        (folder/'frames.csv').write_text(text); rows = list(csv.DictReader(io.StringIO(text)))
        assert len(rows) == 144
        if reference is None:
            reference = rows
        else:
            assert rows == reference
        images.append([normalize(folder/f'{n:03d}.png') for n in range(144)])
    comparisons = []
    for n in range(144):
        canvas = Image.new('RGB',(1920,440),'white'); draw = ImageDraw.Draw(canvas)
        for style,label in enumerate(labels):
            draw.text((style*640+4,3),label,fill='black')
            canvas.paste(images[style][n].resize((640,400),Image.Resampling.NEAREST),(style*640,20))
        draw.text((4,425),'Actual C game / same actors, inputs and depths / PC review at 20 FPS',fill='black')
        comparisons.append(canvas)
    gif(OUT/'isometric-contrast-edges.gif',comparisons,[50]*144)
    sheet = Image.new('RGB',(1920,1760),'white')
    for row,n in enumerate([8,56,80,104]):
        sheet.paste(comparisons[n],(0,row*440))
        comparisons[n].save(OUT/f'comparison-rot{row}.png')
    sheet.save(OUT/'comparison-four-views.png')
    assets = json.loads((OUT/'assets.json').read_text())
    manifest = dict(platform='Actual PC game; no TiEmu run or hardware timing claim.',
                    frames_per_style=144,styles=labels,assets=assets,
                    checks='Exact input/camera/RNG logs across styles; unchanged units and tactical mode; original four occlusion planes checked on every settled frame; key 8 cycles styles.',
                    sources={p:hashlib.sha256((HERE/p).read_bytes()).hexdigest() for p in
                             ['fft.c','texture_preview.inc','tools/isometric_style.py','tools/isometric_style_capture.c','tools/isometric_review.py','../../runtime/platform-sw/rt_sw.c']})
    (OUT/'manifest-pc.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print('432 actual game frames checked; identical actors/RNG/input/camera; original occlusion data preserved.')


if __name__ == '__main__':
    main()
