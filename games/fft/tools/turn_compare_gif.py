#!/usr/bin/env python3
"""Compose real LCD recordings at recorded speed, aligning the first F5 source hold."""
from bisect import bisect_right
import hashlib
import json
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw, ImageFont

HERE = Path(__file__).resolve().parents[1]
OUT = HERE / 'x/turn-comparison'


def check_order(frames, times, folder):
    """Reject native presentation regressions, even when the calculated angles pass."""
    references = [np.asarray(Image.open(folder/'frames'/f'0-{i}.png').convert('L'))[8:93] < 128
                  for i in range(1, 4)]
    sequence, evidence = [], []
    for frame, elapsed in zip(frames, times):
        pixels = np.asarray(frame)[8:93] < 128
        errors = [int(np.count_nonzero(pixels != reference)) for reference in references]
        best = int(np.argmin(errors))
        # Native skin capture loses one horizontal pixel; tolerate that resampling
        # and actor pose changes, but require a clearly separated geometry match.
        if errors[best] < 1500 and sorted(errors)[1] - errors[best] > 1000:
            if not sequence or sequence[-1] != best+1:
                sequence.append(best+1)
                evidence.append(dict(ms=elapsed, frame=best+1, mismatch_pixels=errors[best]))
    assert sequence == [1, 2, 3], f'Invalid native presentation order {sequence}: {folder}'
    return evidence


def clip(folder):
    source = np.asarray(Image.open(folder/'source.png').convert('L'))[:8] < 128
    target = np.asarray(Image.open(folder/'frames/0-4.png').convert('L'))[:8] < 128
    raw = folder/'full-run-tiemu.gif'
    image = Image.open(raw)
    frames, times = [], []
    elapsed, start, end, first_target = 0, None, None, None
    for n in range(image.n_frames):
        image.seek(n)
        frame = image.convert('L').copy()
        header = np.asarray(frame)[:8] < 128
        if start is None and np.array_equal(header, source):
            start = elapsed
        if start is not None:
            times.append(elapsed-start)
            frames.append(frame)
            if elapsed > start and np.array_equal(header, target) and first_target is None:
                first_target = elapsed
            # A new header can precede a completed grayscale plane transfer. Keep all
            # those recorded frames, then freeze a settled destination rather than tearing.
            if first_target is not None and elapsed >= first_target+150 and np.array_equal(header, target):
                end = elapsed
                break
        elapsed += image.info.get('duration', 50)
    assert start is not None and end is not None and end > start, folder
    assert all(f.size == (160, 100) for f in frames)
    check_order(frames, times, folder)
    timer = json.loads((folder/'tiemu.json').read_text())
    headless = json.loads((folder/'headless.json').read_text())
    assert timer['binary_sha256'] == headless['binary_sha256']
    return dict(frames=frames, times=times, duration=end-start,
                start_ms=start, end_ms=end, first_target_ms=first_target, timer=timer,
                recording_sha256=hashlib.sha256(raw.read_bytes()).hexdigest())


def main():
    names = ['textured80x25', 'lowpoly160']
    clips = [clip(OUT/name) for name in names]
    scale, panel, gap, top, bottom = 4, 640, 16, 68, 52
    size = (panel*2+gap, top+100*scale+bottom)
    font_path = '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
    title_font = ImageFont.truetype(font_path, 23)
    small_font = ImageFont.truetype(font_path, 17)
    duration = max(c['duration'] for c in clips) + 1200
    duration = ((duration+49)//50)*50
    palette = [shade for shade in range(256) for _ in range(3)]
    pictures = []
    for elapsed in range(0, duration, 50):
        canvas = Image.new('L', size, 245)
        draw = ImageDraw.Draw(canvas)
        for side, (name, c) in enumerate(zip(names, clips)):
            x = side*(panel+gap)
            title = 'TEXTURED 80x25' if side == 0 else 'LOW POLY 160x100'
            desc = 'Detailed mesh + depth cover' if side == 0 else 'Simplified terrain, native pixels'
            measured = c['timer']['results'][0]['total_ms']/1000
            draw.text((x+12, 4), title, font=title_font, fill=0)
            draw.text((x+12, 35), f'{desc} | timer {measured:.3f} s', font=small_font, fill=0)
            index = max(0, bisect_right(c['times'], elapsed)-1)
            frame = c['frames'][index]
            canvas.paste(frame.resize((panel, 100*scale), Image.Resampling.NEAREST), (x, top))
            if elapsed >= c['duration']:
                draw.rectangle((x+panel-158, top+100*scale-28, x+panel, top+100*scale), fill=255)
                draw.text((x+panel-150, top+100*scale-26), 'TARGET HELD', font=small_font, fill=0)
        draw.text((12, size[1]-48), 'Actual TiEmu recordings | same first F5 turn | source holds aligned', font=small_font, fill=0)
        draw.text((12, size[1]-24), 'Original playback speed; each target is held after arrival until both finish.', font=small_font, fill=0)
        # One fixed grayscale palette preserves every captured LCD shade exactly.
        indexed = Image.frombytes('P', size, canvas.tobytes())
        indexed.putpalette(palette)
        pictures.append(indexed)
    path = OUT/'textured80x25-vs-lowpoly160-tiemu.gif'
    pictures[0].save(path, save_all=True, append_images=pictures[1:], duration=50,
                     loop=0, optimize=False, disposal=1)
    metadata = dict(
        scope='First independent F5 case from real native TiEmu timer captures. Not a continuous gameplay sequence.',
        synchronization='First F5 source-header appearance aligned to t=0; original frame timestamps retained.',
        presentation='No speed change or omitted moving frame. Each final image freezes after its recorded arrival; common 1200 ms tail.',
        timer='Printed values use the internal 256 Hz timer, separate from recording wall-clock timestamps.',
        target_settling='Retain at least 150 ms after first target header before freezing, to avoid capturing a mixed grayscale transfer.',
        capture_sampling_ms=50, duration_ms=duration, gif_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
        variants={name: dict(source_start_ms=c['start_ms'], target_first_ms=c['first_target_ms'], target_settled_ms=c['end_ms'],
                            recording_source_to_target_ms=c['duration'],
                            recording_sha256=c['recording_sha256'],
                            binary_sha256=c['timer']['binary_sha256']) for name, c in zip(names, clips)},
    )
    (OUT/'capture.json').write_text(json.dumps(metadata, indent=2)+'\n')
    pictures[len(pictures)//2].convert('RGB').save(OUT/'comparison-preview.png')
    print(json.dumps(metadata, indent=2))


if __name__ == '__main__':
    main()
