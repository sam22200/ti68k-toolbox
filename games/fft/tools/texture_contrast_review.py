#!/usr/bin/env python3
"""Offline contrast variants, replaying equal-size backgrounds through the same PC renderer."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

import numpy as np
from PIL import Image, ImageDraw
from perspective_demo import gif
from texture_review import HERE, VIEWS, TONES, normalize, pack, unpack

SOURCE = HERE/'x/scenery-textures'
OUT = HERE/'x/scenery-contrast'
PROFILES = [('current', 1.), ('contrast-plus', .65), ('contrast-strong', .35)]


def main():
    # Regenerate the source luminance with the actual geometry and filtering, never infer
    # missing intermediate luminance from the already-quantized two-bit images.
    subprocess.run([sys.executable, 'tools/texture_review.py', *sys.argv[1:]], cwd=HERE, check=True)
    OUT.mkdir(parents=True, exist_ok=True)
    source_manifest = json.loads((SOURCE/'manifest-pc.json').read_text())
    original = np.array(source_manifest['thresholds'])
    binary = SOURCE/'capture-pc'
    images, stats, profiles = {}, {}, {}
    for style, width in PROFILES:
        thresholds = original[1]+(original-original[1])*width
        assert np.all(np.diff(thresholds) > 0)
        profiles[style] = thresholds.tolist()
    for view, name in enumerate(VIEWS):
        folder = SOURCE/name/'textures'
        source_background = unpack((folder/'textured.bin').read_bytes())
        samples = np.load(folder/'luminance.npy', allow_pickle=False)
        edges = np.frombuffer((folder/'edges.bin').read_bytes(), np.uint8).reshape(100, 160)
        valid = np.isfinite(samples) & (edges == 0)
        base_extremes = np.count_nonzero((source_background[valid] == 0) | (source_background[valid] == 3))
        assert valid.any()
        for style, width in PROFILES:
            thresholds = np.array(profiles[style])
            grey = source_background.copy()
            lum = samples[valid]
            grey[valid] = np.where(lum > thresholds[2], 0, np.where(lum > thresholds[1], 1,
                                   np.where(lum > thresholds[0], 2, 3)))
            assert np.array_equal(grey[~valid], source_background[~valid])
            assert np.array_equal(unpack(pack(grey)), grey)
            extremes = int(np.count_nonzero((grey[valid] == 0) | (grey[valid] == 3)))
            assert extremes >= base_extremes
            output = OUT/name/style
            output.mkdir(parents=True, exist_ok=True)
            planes = output/'contrast.bin'
            planes.write_bytes(pack(grey))
            assert planes.stat().st_size == (folder/'textured.bin').stat().st_size == 6000
            Image.fromarray(TONES[grey]).save(output/'background.png')
            if style == 'current':
                assert planes.read_bytes() == (folder/'textured.bin').read_bytes()
                images[name, style] = [normalize(folder/f'{n:03d}.png') for n in range(32)]
            else:
                log = subprocess.check_output([str(binary), str(view), str(output), str(planes)], cwd=HERE, text=True)
                (output/'metadata.json').write_text(log)
                metadata = json.loads(log)
                assert metadata['vertices'] == json.loads((folder/'metadata.json').read_text())['vertices']
                images[name, style] = [normalize(output/f'{n:03d}.png') for n in range(32)]
            stats[name+'-'+style] = dict(texture_pixels=int(valid.sum()),
                                         grey_histogram=np.bincount(grey[valid], minlength=4).tolist(),
                                         black_white_pixels=extremes, background_bytes=6000)
            gif(OUT/f'{name}-{style}.gif', images[name, style], [100]*32)
    comparisons = [('three-contrasts', [p[0] for p in PROFILES]),
                   ('before-after', ['current', 'contrast-plus'])]
    for file, styles in comparisons:
        frames = []
        for n in range(32):
            image = Image.new('RGB', (len(styles)*320, 880), 'white')
            draw = ImageDraw.Draw(image)
            for row, name in enumerate(VIEWS):
                for col, style in enumerate(styles):
                    x, y = col*320, row*220
                    draw.text((x+4, y+4), name.upper()+' / '+style.upper(), fill='black')
                    image.paste(images[name, style][n].resize((320, 200), Image.Resampling.NEAREST), (x, y+20))
            frames.append(image)
        gif(OUT/(file+'.gif'), frames, [100]*32)
        frames[8].save(OUT/(file+'.png'))
    manifest = dict(platform='PC-only offline texture contrast conversion; same C replay binary, no TI validation.',
                    timing='10 FPS visual playback, not a performance measurement.',
                    profiles=profiles, stats=stats, new_replay_frames=256,
                    source_manifest_sha256=hashlib.sha256((SOURCE/'manifest-pc.json').read_bytes()).hexdigest(),
                    replay_binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
                    script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                    checks='Exact baseline; same 6000-byte backgrounds; more extreme tones; non-textured pixels and edges preserved; identical camera transforms.')
    (OUT/'manifest-pc.json').write_text(json.dumps(manifest, indent=2)+'\n')
    print('Three contrast profiles, four views; equal 6000-byte backgrounds, same C renderer; 256 new PC replay frames. No TI validation.')


if __name__ == '__main__':
    main()
