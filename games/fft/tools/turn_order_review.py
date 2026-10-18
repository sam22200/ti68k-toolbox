#!/usr/bin/env python3
"""Compare ordered native captures using the fixed-address plane-copy control."""
from bisect import bisect_right
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont
import turn_compare_gif as comparison

OUT = comparison.HERE/'x/turn-direction'


def read_clip(name, raw):
    folder = comparison.OUT/name
    source = np.asarray(Image.open(folder/'source.png').convert('L'))[:8] < 128
    target = np.asarray(Image.open(folder/'frames/0-4.png').convert('L'))[:8] < 128
    image = Image.open(raw)
    frames, times = [], []
    elapsed, start, first_target, source_seen = 0, None, None, None
    for index in range(image.n_frames):
        image.seek(index)
        frame = image.convert('L').copy()
        header = np.asarray(frame)[:8] < 128
        if source_seen is None and np.array_equal(header, source):
            source_seen = elapsed
        # Retain a settled source rather than the initial grayscale inference.
        # This trims only the deliberate source hold, never a moving picture.
        if start is None and source_seen is not None and elapsed >= source_seen+350 and np.array_equal(header, source):
            start = elapsed
        if start is not None:
            frames.append(frame)
            times.append(elapsed-start)
            if elapsed > start and first_target is None and np.array_equal(header, target):
                first_target = elapsed
            if first_target is not None and elapsed >= first_target+150 and np.array_equal(header, target):
                break
        elapsed += image.info.get('duration', 50)
    assert start is not None and first_target is not None and elapsed >= first_target+150
    order = comparison.check_order(frames, times, folder)
    return dict(frames=frames, times=times, duration=elapsed-start, order=order,
                source_ms=start, end_ms=elapsed,
                recording_sha256=hashlib.sha256(raw.read_bytes()).hexdigest())


def main():
    variants = [('textured80x25', 'copy-textured.gif', 'ffttexcp.89z', 'TEXTURED 80x25'),
                ('lowpoly160', 'copy-labelled.gif', 'fftcopy.89z', 'LOW POLY 160x100')]
    clips = [read_clip(name, OUT/raw) for name, raw, _, _ in variants]
    duration = max(c['duration'] for c in clips)+1200
    duration = ((duration+49)//50)*50
    font = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf', 21)
    small = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf', 17)
    palette = [shade for shade in range(256) for _ in range(3)]
    pictures = []
    for elapsed in range(0, duration, 50):
        canvas = Image.new('L', (1296, 512), 245)
        draw = ImageDraw.Draw(canvas)
        for side, (variant, clip) in enumerate(zip(variants, clips)):
            x = side*656
            draw.text((x+10, 3), variant[3], fill=0, font=font)
            draw.text((x+10, 31), 'F5: A -> 1 -> 2 -> 3 -> B', fill=0, font=small)
            i = max(0, bisect_right(clip['times'], elapsed)-1)
            canvas.paste(clip['frames'][i].resize((640,400), Image.Resampling.NEAREST), (x,58))
        draw.text((10, 463), 'Actual TiEmu | fixed grayscale plane addresses / copy control | original playback speed', fill=0, font=small)
        draw.text((10, 487), 'Both frame orders verified; source holds aligned; each target held after arrival. No new timing claim.', fill=0, font=small)
        indexed = Image.frombytes('P', canvas.size, canvas.tobytes())
        indexed.putpalette(palette)
        pictures.append(indexed)
    path = OUT/'ordered-textured80x25-vs-lowpoly160-tiemu.gif'
    pictures[0].save(path, save_all=True, append_images=pictures[1:], duration=50,
                     loop=0, optimize=False, disposal=1)
    pictures[len(pictures)//2].convert('RGB').save(OUT/'ordered-preview.png')
    metadata = dict(scope='Native TiEmu fixed-address copy presentation control; same rendering angles and geometry as before, numbered frames.',
                    timing='Original capture timestamps; source holds aligned after 350 ms grayscale settling, final pictures held. No moving picture removed. Previous timer measurements do not describe this presenter.',
                    duration_ms=duration, gif_sha256=hashlib.sha256(path.read_bytes()).hexdigest(), variants={})
    for (name, _, binary, _), clip in zip(variants, clips):
        metadata['variants'][name] = {k:v for k,v in clip.items() if k not in ('frames','times')}
        metadata['variants'][name]['binary_sha256'] = hashlib.sha256((OUT/binary).read_bytes()).hexdigest()
    (OUT/'ordered-capture.json').write_text(json.dumps(metadata, indent=2)+'\n')
    print(json.dumps(metadata, indent=2))


if __name__ == '__main__':
    main()
