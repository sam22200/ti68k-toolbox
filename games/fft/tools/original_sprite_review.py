#!/usr/bin/env python3
"""Local-disc source sprites in actual PC perspective frames; no TI build or validation.

The Python compositor is a visual experiment, not a measured calculator renderer.
It keeps the C renderer's actor anchors, ordering, original idle poses and opaque depth.
"""
import hashlib
import json
from pathlib import Path
import subprocess
import sys

import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage

import extract
import units
from perspective_demo import gif

HERE = Path(__file__).resolve().parents[1]
OUT = HERE / 'x/original-sprites'
VIEWS = ['close-front', 'close-back', 'wide-front', 'wide-back']
TONES = [255, 170, 85, 0]


def compose(spr, tiles):
    """Compose raw palette indices at the original feet anchor; no 0.6 reduction."""
    packed = np.frombuffer(spr[512:512+128*256], np.uint8).reshape(256, 128)
    sheet = np.empty((256, 256), np.uint8)
    sheet[:, ::2] = packed & 15
    sheet[:, 1::2] = packed >> 4
    canvas = np.zeros((128, 128), np.uint8)
    for sx, sy, x, y, w, h, fx, fy in reversed(tiles):
        tile = sheet[y:y+h, x:x+w]
        if fx:
            tile = tile[:, ::-1]
        if fy:
            tile = tile[::-1]
        region = canvas[96+sy:96+sy+h, 64+sx:64+sx+w]
        region[tile > 0] = tile[tile > 0]
    assert np.any(canvas)
    return canvas


def palettes(spr):
    pal = np.frombuffer(spr[:32], '<u2').astype(int)
    rgb = np.stack([pal & 31, pal >> 5 & 31, pal >> 10 & 31], axis=1)
    rgb = ((rgb*255+15)//31).astype(np.uint8)
    lum = (pal & 31)*.299+(pal >> 5 & 31)*.587+(pal >> 10 & 31)*.114
    grey = np.where(lum <= 7, 3, np.where(lum <= 16, 2, np.where(lum <= 22, 1, 0)))
    grey[1] = 3
    grey_rgb = np.repeat(np.array(TONES, np.uint8)[grey, None], 3, axis=1)
    return grey_rgb, rgb


def draw_original(background, depth, actors, sources, color):
    out = np.asarray(background).copy()
    total = 0
    for actor in actors: # far to near, same order as the C renderer
        if not actor['visible']:
            continue
        pose = actor['pose']
        assert actor['gfx'] < len(units.CHARS) and pose % len(units.FRAMES) < 10
        canvas, grey, rgb = sources[actor['gfx'], pose % len(units.FRAMES)]
        # Keep the same physical size: the old bank used 0.6 source px per TI px.
        # Direct inverse mapping avoids enlarging an already-reduced 16x26 image.
        scale = actor['h']*units.SCALE/units.H
        x0 = max(0, int(np.floor(actor['x']-64*scale))-1)
        x1 = min(160, int(np.ceil(actor['x']+64*scale))+1)
        y0 = max(8, int(np.floor(actor['y']-96*scale))-1)
        y1 = min(93, int(np.ceil(actor['y']+32*scale))+1)
        if x0 >= x1 or y0 >= y1:
            continue
        xs = np.floor((np.arange(x0, x1)+.5-actor['x'])/scale+64).astype(int)
        ys = np.floor((np.arange(y0, y1)+.5-actor['y'])/scale+96).astype(int)
        if pose >= len(units.FRAMES):
            xs = 127-xs # same anchor convention as the mirrored TI source bank
        indices = canvas[np.clip(ys, 0, 127)[:, None], np.clip(xs, 0, 127)[None, :]]
        opaque = (indices != 0) & (ys[:, None] >= 0) & (ys[:, None] < 128)
        opaque &= (xs[None, :] >= 0) & (xs[None, :] < 128)
        ring = ndimage.binary_dilation(opaque, structure=np.array([[0, 1, 0], [1, 1, 1], [0, 1, 0]])) & ~opaque
        visible = actor['q']+1 >= depth[y0:y1, x0:x1]
        body, edge = opaque & visible, ring & visible
        region = out[y0:y1, x0:x1]
        region[edge] = 255
        region[body] = (rgb if color else grey)[indices[body]]
        total += np.count_nonzero(body)
    # Four-grey preview must remain exactly four-grey, including the white outline.
    if not color:
        assert set(np.unique(out)).issubset(TONES)
    return Image.fromarray(out), int(total)


def normalize(path):
    tones = {216: 255, 152: 170, 88: 85, 24: 0}
    return Image.open(path).convert('L').point([tones.get(x, x) for x in range(256)]).convert('RGB')


def montage(images, styles, n):
    im = Image.new('RGB', (320*len(styles), 220*len(VIEWS)), 'white')
    draw = ImageDraw.Draw(im)
    for row, view in enumerate(VIEWS):
        for col, style in enumerate(styles):
            x, y = col*320, row*220
            draw.text((x+4, y+4), view.upper()+' / '+style.upper(), fill='black')
            im.paste(images[view, style][n].resize((320, 200), Image.Resampling.NEAREST), (x, y+20))
    return im


def main():
    disc_path = Path(sys.argv[1]) if len(sys.argv) > 1 else HERE/'../../roms/ps1/Final Fantasy Tactics (USA)/Final Fantasy Tactics (USA).cue'
    OUT.mkdir(parents=True, exist_ok=True)
    disc, files = extract.disc_files(str(disc_path))
    shapes, sources, source_hashes = {}, {}, {}
    for kind in ['TYPE1', 'TYPE2']:
        data = disc.read(*files[f'BATTLE/{kind}.SHP'])
        shapes[kind] = units.shp_frames(data)
        source_hashes[kind+'.SHP'] = hashlib.sha256(data).hexdigest()
    for gfx, (_, sheet, kind) in enumerate(units.CHARS):
        spr = disc.read(*files[f'BATTLE/{sheet}.SPR'])
        source_hashes[sheet+'.SPR'] = hashlib.sha256(spr).hexdigest()
        grey, rgb = palettes(spr)
        for k, frame in enumerate(units.FRAMES[:10]):
            sources[gfx, k] = compose(spr, shapes[kind][frame]), grey, rgb

    binary = OUT/'capture-pc'
    subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-DFFT_EXPLORATION', '-DFFT_FIXED',
                    '-o', str(binary), 'tools/original_sprite_capture.c',
                    '../../runtime/core/rt_core.c', '../../runtime/platform-sw/rt_sw.c'], cwd=HERE, check=True)
    images, counts, frames = {}, {}, {}
    reference_checks = 0
    for view, name in enumerate(VIEWS):
        folder = OUT/name
        folder.mkdir(exist_ok=True)
        log = subprocess.check_output([str(binary), str(view), str(folder)], cwd=HERE, text=True)
        actors = json.loads(log)
        (folder/'frames.json').write_text(log)
        assert len(actors) == 32
        depth = np.frombuffer((folder/'depth.bin').read_bytes(), np.uint8).reshape(100, 160)
        for style in ['current', 'original-grey', 'original-color']:
            images[name, style] = []
        counts[name], frames[name] = [], actors
        for n, frame in enumerate(actors):
            current = normalize(folder/f'current-{n:03d}.png')
            background = normalize(folder/f'background-{n:03d}.png')
            reference = HERE/f'x/fixed-review/{name}-auto/{n:03d}.png'
            if reference.exists():
                assert current.tobytes() == normalize(reference).tobytes(), reference
                reference_checks += 1
            if not n:
                # Actual source poses with completely opaque nearer scenery must draw nothing.
                assert all(a['q'] < 254 for a in frame['actors'])
                hidden, pixels = draw_original(background, np.full_like(depth, 255), frame['actors'], sources, False)
                assert pixels == 0 and hidden.tobytes() == background.tobytes()
            images[name, 'current'].append(current)
            for style, color in [('original-grey', False), ('original-color', True)]:
                image, pixels = draw_original(background, depth, frame['actors'], sources, color)
                assert pixels > 0
                assert image.crop((0, 0, 160, 8)).tobytes() == background.crop((0, 0, 160, 8)).tobytes()
                assert image.crop((0, 93, 160, 100)).tobytes() == background.crop((0, 93, 160, 100)).tobytes()
                image.save(folder/f'{style}-{n:03d}.png')
                images[name, style].append(image)
                if not color:
                    counts[name].append(pixels)
        assert images[name, 'original-grey'][0].tobytes() != images[name, 'current'][0].tobytes()
    for file, styles in [('current-original', ['current', 'original-grey']),
                         ('grey-color', ['original-grey', 'original-color'])]:
        comparison = [montage(images, styles, n) for n in range(32)]
        gif(OUT/(file+'.gif'), comparison, [100]*32)
        comparison[8].save(OUT/(file+'.png'))
    for view in VIEWS:
        gif(OUT/(view+'.gif'), images[view, 'original-grey'], [100]*32)
    manifest = dict(platform='PC visual compositor only; no TI build, validation or performance claim.',
                    timing='10 FPS review playback; not measured execution speed.',
                    source_frames=128, rendered_variants=384, source_disc_files_sha256=source_hashes,
                    unchanged_reference_frames=reference_checks,
                    binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
                    capture_sources={f:hashlib.sha256((HERE/f).read_bytes()).hexdigest() for f in
                                     ['fft.c', 'fft.h', 'perspective.inc', 'fixed.inc', 'tools/original_sprite_capture.c',
                                      'tools/original_sprite_review.py', 'tools/units.py']},
                    body_pixels=counts, frame_metadata=frames)
    (OUT/'manifest-pc.json').write_text(json.dumps(manifest, indent=2)+'\n')
    print(f'128 PC source frames, 384 variants; {reference_checks} unchanged reference frames; depth rejection passes; no TI validation.')


if __name__ == '__main__':
    main()
