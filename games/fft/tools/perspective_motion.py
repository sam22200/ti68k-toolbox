#!/usr/bin/env python3
"""Reviewed contrast, restored old canal, and actual low-poly camera transitions on PC."""
import csv
import hashlib
import io
import json
from pathlib import Path
import subprocess
import sys

from PIL import Image, ImageDraw
from perspective_demo import gif
from texture_review import HERE, VIEWS, normalize

OUT = HERE/'x/perspective-motion'


def main():
    subprocess.run([sys.executable, 'tools/texture_contrast_review.py', *sys.argv[1:]], cwd=HERE, check=True)
    OUT.mkdir(parents=True, exist_ok=True)
    binary = OUT/'capture-pc'
    subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-DFFT_EXPLORATION', '-DFFT_FIXED', '-DFFT_CAMERA_MOTION',
                    '-o', str(binary), 'tools/perspective_motion_capture.c',
                    '../../runtime/core/rt_core.c', '../../runtime/platform-sw/rt_sw.c'], cwd=HERE, check=True)
    frames_dir = OUT/'frames'
    frames_dir.mkdir(exist_ok=True)
    background = HERE/'x/scenery-contrast/close-front/contrast-plus/contrast.bin'
    log = subprocess.check_output([str(binary), str(frames_dir), str(background)], cwd=HERE, text=True)
    (OUT/'frames.csv').write_text(log)
    rows = list(csv.DictReader(io.StringIO(log)))
    assert len(rows) == 136
    assert sum(r['transition'] == '1' for r in rows) == 17
    assert sum(r['transition'] == '2' for r in rows) == 17
    frames = [normalize(frames_dir/f'{n:03d}.png') for n in range(136)]
    # Uniform 20 FPS makes the 17 geometry steps easy to inspect; no timing claim.
    gif(OUT/'entry-exit.gif', frames, [50]*136)
    enlarged = [im.resize((640, 400), Image.Resampling.NEAREST) for im in frames]
    gif(OUT/'entry-exit-large.gif', enlarged, [50]*136)
    picks = [12, 16, 20, 24, 28, 32, 44, 88, 92, 96, 100, 104, 120]
    strip = Image.new('RGB', (320*len(picks), 220), 'white')
    draw = ImageDraw.Draw(strip)
    for col, n in enumerate(picks):
        draw.text((col*320+4, 4), f'FRAME {n} / STEP {rows[n]["step"]}', fill='black')
        strip.paste(frames[n].resize((320, 200), Image.Resampling.NEAREST), (col*320, 20))
    strip.save(OUT/'steps.png')
    before_root = OUT/'before-water'
    if all((before_root/v/'008.png').exists() for v in VIEWS):
        comparison = []
        for n in range(32):
            image = Image.new('RGB', (640, 880), 'white'); draw = ImageDraw.Draw(image)
            for row, view in enumerate(VIEWS):
                old = normalize(before_root/view/f'{n:03d}.png')
                new = normalize(HERE/f'x/scenery-contrast/{view}/contrast-plus/{n:03d}.png')
                for col, (label, source) in enumerate([('TEXTURED WATER', old), ('OLD GREY WATER', new)]):
                    draw.text((col*320+4, row*220+4), view.upper()+' / '+label, fill='black')
                    image.paste(source.resize((320, 200), Image.Resampling.NEAREST), (col*320, row*220+20))
            comparison.append(image)
        gif(OUT/'water-before-after.gif', comparison, [100]*32)
        comparison[8].save(OUT/'water-before-after.png')
    manifest = dict(platform='PC-only real C key replay; no TI measurements.',
                    timing='Uniform 20 FPS entry/exit preview; 10 FPS water comparison; neither measures speed.',
                    frames=136, low_poly_frames_per_direction=17,
                    checks='Unit records, turn, mode, AI and RNG unchanged; completes entry and exit.',
                    sources={f:hashlib.sha256((HERE/f).read_bytes()).hexdigest() for f in
                             ['fft.c', 'fft.h', 'perspective.inc', 'fixed.inc', 'tools/perspective_motion_capture.c',
                              'tools/perspective_motion.py']})
    (OUT/'manifest-pc.json').write_text(json.dumps(manifest, indent=2)+'\n')
    print('136 real C key-replay frames; 17 low-poly entry and exit steps, tactical/RNG conservation. No TI validation.')


if __name__ == '__main__':
    main()
