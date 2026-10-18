#!/usr/bin/env python3
"""PC review of intermediate water, close/wide entry/exit and animated zoom."""
import csv
import hashlib
import io
import json
import subprocess
import sys

import numpy as np
from PIL import Image, ImageDraw
from perspective_demo import gif
from texture_review import HERE, VIEWS, normalize

OUT = HERE/'x/perspective-modes'


def main():
    subprocess.run([sys.executable, 'tools/texture_contrast_review.py', *sys.argv[1:]], cwd=HERE, check=True)
    OUT.mkdir(parents=True, exist_ok=True)
    binary = OUT/'capture-pc'
    subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-DFFT_EXPLORATION', '-DFFT_FIXED', '-DFFT_CAMERA_MOTION',
                    '-o', str(binary), 'tools/perspective_modes_capture.c',
                    '../../runtime/core/rt_core.c', '../../runtime/platform-sw/rt_sw.c'], cwd=HERE, check=True)
    backgrounds = [HERE/f'x/scenery-contrast/{view}/contrast-plus/contrast.bin' for view in ['close-front', 'wide-front']]
    cases = []
    for wide, name in enumerate(['close-entry', 'wide-entry']):
        folder = OUT/name; folder.mkdir(exist_ok=True)
        log = subprocess.check_output([str(binary), str(folder), *map(str, backgrounds), str(wide)], cwd=HERE, text=True)
        (folder/'frames.csv').write_text(log)
        rows = list(csv.DictReader(io.StringIO(log)))
        assert len(rows) == 272
        for mode in ['1', '2', '3']:
            assert sum(r['transition'] == mode for r in rows) == 34
        frames = [normalize(folder/f'{n:03d}.png') for n in range(272)]
        cases.append(frames)
        gif(OUT/(name+'.gif'), [im.resize((480, 300), Image.Resampling.NEAREST) for im in frames], [50]*272)
    montage = []
    for n in range(272):
        image = Image.new('RGB', (960, 320), 'white'); draw = ImageDraw.Draw(image)
        for col, frames in enumerate(cases):
            draw.text((col*480+4, 4), ['DIRECT CLOSE ENTRY', 'DIRECT WIDE ENTRY'][col], fill='black')
            image.paste(frames[n].resize((480, 300), Image.Resampling.NEAREST), (col*480, 20))
        montage.append(image)
    gif(OUT/'all-transitions.gif', montage, [50]*272)
    montage[96].save(OUT/'zoom-endpoints.png')
    source_manifest = json.loads((HERE/'x/scenery-textures/manifest-pc.json').read_text())
    labels_by_view = {view:np.frombuffer((HERE/f'x/scenery-textures/{view}/flat/faces.bin').read_bytes(), '>u2').reshape(100, 160)
                      for view in VIEWS}
    comparison = []
    has_before = all((OUT/f'water-grey-before/{v}/008.png').exists() for v in VIEWS)
    if has_before:
        for n in range(32):
            image = Image.new('RGB', (960, 880), 'white'); draw = ImageDraw.Draw(image)
            for row, view in enumerate(VIEWS):
                original = normalize(HERE/f'x/perspective-motion/before-water/{view}/{n:03d}.png')
                flat = normalize(OUT/f'water-grey-before/{view}/{n:03d}.png')
                middle = normalize(HERE/f'x/scenery-contrast/{view}/contrast-plus/{n:03d}.png')
                water = np.isin(labels_by_view[view], source_manifest['water_face_ids'])
                assert np.array_equal(np.asarray(flat)[~water], np.asarray(middle)[~water])
                for col, (label, frame) in enumerate([('ORIGINAL TEXTURE', original), ('FLAT GREY', flat), ('MIDDLE', middle)]):
                    draw.text((col*320+4, row*220+4), view.upper()+' / '+label, fill='black')
                    image.paste(frame.resize((320, 200), Image.Resampling.NEAREST), (col*320, row*220+20))
            comparison.append(image)
        gif(OUT/'water-three-versions.gif', comparison, [100]*32)
        comparison[8].save(OUT/'water-three-versions.png')
    manifest = dict(platform='PC-only actual C key replay; no TI run or FPS measurement.',
                    frames=544, steps_per_motion=17, each_case='two entries, two exits, two zoom changes',
                    timing='20 FPS transition review; 10 FPS water review; illustrative playback only.',
                    checks='Correct direct/remembered endpoints; tactical units/turn/AI/RNG unchanged; water changes confined to canal.',
                    sources={f:hashlib.sha256((HERE/f).read_bytes()).hexdigest() for f in
                             ['fft.c', 'fft.h', 'perspective.inc', 'fixed.inc', 'tools/perspective_modes.py',
                              'tools/perspective_modes_capture.c']})
    (OUT/'manifest-pc.json').write_text(json.dumps(manifest, indent=2)+'\n')
    print('544 C replay frames: direct close/wide, remembered re-entry, smooth zoom both ways, intermediate canal. No TI validation.')


if __name__ == '__main__':
    main()
