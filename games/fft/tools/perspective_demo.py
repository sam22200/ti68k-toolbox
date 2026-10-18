#!/usr/bin/env python3
"""Three actual 68000 framebuffer GIFs, with an independent PC checksum per frame."""
import csv
import hashlib
import json
from pathlib import Path
import re
import subprocess
from PIL import Image, ImageDraw

HERE = Path(__file__).resolve().parents[1]
ROOT = HERE.parents[1]
OUT = HERE / 'x/perspective'
NAMES = ['fixed', 'lateral', 'orbit']
SEQUENCE = [(16, ''), (24, 'LEFT'), (48, 'RIGHT'), (24, 'LEFT'),
            (16, 'D LEFT'), (16, 'D RIGHT'), (12, 'UP'), (12, 'DOWN'),
            (128, 'RIGHT'), (16, '')]
BANKS = ['fftmesh', 'fftu', 'fftw', 'ffti', 'fftv0', 'fftv1', 'fftv2', 'fftv3']


def run(args):
    return subprocess.check_output([str(a) for a in args], cwd=HERE, text=True)


def delays(seconds):
    total = previous = 0
    out = []
    for dt in seconds:
        total += dt * 100
        current = round(total)
        out.append(max(10, (current-previous)*10))
        previous = current
    return out


def gif(path, frames, times):
    palette = Image.new('P', (1, 1))
    palette.putpalette([v for shade in [255, 170, 85, 0] for v in [shade]*3] + [0]*756)
    encoded = [f.convert('RGB').quantize(palette=palette, dither=Image.Dither.NONE) for f in frames]
    encoded[0].save(path, save_all=True, append_images=encoded[1:], duration=times,
                    loop=0, optimize=True, disposal=1)
    duration = 0
    with Image.open(path) as im:
        for n in range(im.n_frames):
            im.seek(n)
            duration += im.info['duration']
        assert im.convert('RGB').tobytes() == encoded[-1].convert('RGB').tobytes()
    assert duration == sum(times)


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    frame, lines = 0, []
    for count, keys in SEQUENCE:
        lines.append(f'{frame} {keys}'.rstrip())
        frame += count
    lines.append(str(frame))
    keys = OUT / 'camera.txt'
    keys.write_text('\n'.join(lines)+'\n')
    run(['cc', '-std=gnu99', '-O1', '-DFFT_EXPLORATION', '-o', OUT/'pc-oracle',
         'fft.c', 'tools/perspective_capture.c', '../../runtime/core/rt_core.c',
         '../../runtime/platform-sw/rt_sw.c'])
    binary = HERE/'fftdumpc.89z'
    manifest = {'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(),
                'frames': frame, 'internal_resolution': '80x50',
                'clock_hz': 12000000, 'grayscale_cpu_reserve': .09,
                'timing': 'Datasheet MC68000 emulator; dt=max(1/32, render_cycles/(12000000*.91)); '
                          'excludes startup, assumes 9% grayscale overhead, not physical hardware or TiEmu.',
                'comparison_timing': 'Uniform 8 FPS, visual comparison only', 'cameras': {}}
    all_frames, rows = [], []
    for kind, name in enumerate(NAMES, 62):
        folder = OUT/name
        folder.mkdir(exist_ok=True)
        log = run([ROOT/'tools/bin/ti-cycles', '--arg', kind, '--frames', frame,
                   '--max', 20000, '--keys', keys, '--png', folder/'%03d.png',
                   *[arg for bank in BANKS for arg in ['--file', HERE/(bank+'.89y')]], binary])
        (folder/'cycles.log').write_text(log)
        assert '_main returned' in log
        pc = run([OUT/'pc-oracle', kind, keys, frame])
        (folder/'pc.log').write_text(pc)
        checksums = re.findall(r'shot \d+ checksum (\w+)', log)
        expected = re.findall(r'^\d+ (\w+)$', pc, re.M)
        assert len(checksums) == frame+1 and checksums[:-1] == expected
        values = [int(v, 16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)', log)]
        metrics = []
        for i, v in enumerate(values):
            if v >> 24 == 0x72:
                assert (v & 0xffffff) == len(metrics)
                metrics.append(values[i+1:i+5])
        assert len(metrics) == frame
        images, times = [], []
        for n, (geometry, sprites, present, total) in enumerate(metrics):
            im = Image.open(folder/f'{n:03d}.png').convert('RGB').crop((0, 0, 160, 100))
            images.append(im.resize((640, 400), Image.Resampling.NEAREST))
            times.append(max(1/32, (total+600)/(12000000*.91)))
            rows.append(dict(camera=name, frame=n, geometry=geometry, sprites=sprites,
                             present=present, render=total, seconds=times[-1], checksum=expected[n]))
        gif(OUT/f'{name}.gif', images, delays(times))
        all_frames.append(images)
        steady = times[1:]
        moving = [t for t, m in zip(times[1:], metrics[1:]) if m[0] > 100000]
        cached = [t for t, m in zip(times[1:], metrics[1:]) if m[0] <= 100000]
        manifest['cameras'][name] = dict(checksums_verified=frame,
            average_fps=len(steady)/sum(steady), worst_ms=max(steady)*1000,
            minimum_fps=1/max(steady), cached_fps=len(cached)/sum(cached) if cached else None,
            moving_fps=len(moving)/sum(moving) if moving else None,
            first_frame_ms=times[0]*1000, duration_seconds=sum(delays(times))/1000)
        print(name, json.dumps(manifest['cameras'][name]), flush=True)
    comparison = []
    for n in range(frame):
        im = Image.new('RGB', (960, 224), 'white')
        draw = ImageDraw.Draw(im)
        for k, name in enumerate(NAMES):
            im.paste(all_frames[k][n].resize((320, 200), Image.Resampling.NEAREST), (k*320, 24))
            draw.text((k*320+5, 5), name.upper()+' - visual timing only', fill='black')
        comparison.append(im)
    gif(OUT/'comparison.gif', comparison, [120 if n & 1 else 130 for n in range(frame)])
    with (OUT/'frames.csv').open('w') as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0])); writer.writeheader(); writer.writerows(rows)
    (OUT/'manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')
    # One sheet for visual review, including the far side and pitch changes.
    sheet = Image.new('RGB', (960, 1200), 'white')
    for k in range(3):
        for j, n in enumerate([0, 38, 86, 138, 158, 240]):
            sheet.paste(all_frames[k][n].resize((320, 200), Image.Resampling.NEAREST), (k*320, j*200))
    sheet.save(OUT/'review.png')


if __name__ == '__main__':
    main()
