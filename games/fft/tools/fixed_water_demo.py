#!/usr/bin/env python3
"""Capture near/wide water animation from the TI binary; check every frame on PC."""
import csv
import hashlib
import json
import re
from PIL import Image, ImageDraw
from fixed_demo import HERE, ROOT, BANKS, run
from perspective_demo import gif, delays

OUT = HERE/'x/fixed-water'
FRAMES = 512


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    keys = OUT/'camera.txt'
    keys.write_text('0\n64 F5\n65\n128 MODE\n129\n192 F5\n193\n256 F5\n257\n320 MODE\n321\n384 F5\n385\n448 MODE\n449\n512\n')
    oracle = OUT/'pc-oracle'
    run(['cc', '-std=gnu99', '-O1', '-DFFT_EXPLORATION', '-DFFT_FIXED',
         '-o', oracle, 'fft.c', 'tools/perspective_capture.c',
         '../../runtime/core/rt_core.c', '../../runtime/platform-sw/rt_sw.c'])
    binary = HERE/'fftfcapc.89z'
    log = run([ROOT/'tools/bin/ti-cycles', '--arg', 62, '--frames', FRAMES,
               '--max', 20000, '--keys', keys, '--png', OUT/'%03d.png',
               *[v for bank in BANKS for v in ['--file', HERE/(bank+'.89y')]], binary])
    (OUT/'cycles.log').write_text(log)
    assert '_main returned' in log
    pc = run([oracle, 62, keys, FRAMES]); (OUT/'pc.log').write_text(pc)
    hashes = re.findall(r'shot \d+ checksum (\w+)', log)
    expected = re.findall(r'^\d+ (\w+)$', pc, re.M)
    assert hashes[:-1] == expected and len(expected) == FRAMES
    values = [int(v, 16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)', log)]
    metrics = [values[i+1:i+5] for i, v in enumerate(values) if v >> 24 == 0x72]
    assert len(metrics) == FRAMES
    images, seconds, rows = [], [], []
    for n, (geometry, sprites, present, total) in enumerate(metrics):
        images.append(Image.open(OUT/f'{n:03d}.png').convert('RGB').crop((0, 0, 160, 100)))
        dt = max(1/32, (total+600)/(12000000*.91)); seconds.append(dt)
        rows.append(dict(frame=n, geometry=geometry, sprites_water_copy=sprites,
                         present=present, render=total, seconds=dt, checksum=hashes[n]))
    gif(OUT/'perspective.gif', [im.resize((640, 400), Image.Resampling.NEAREST) for im in images], delays(seconds))
    # Four presentations in parallel, warmed source frames, uniform cadence for visual review.
    # Displayed wide-front water is nearest the camera and easiest to inspect.
    comparison = []
    starts = [32, 96, 224, 160]
    labels = ['CLOSE / FRONT', 'CLOSE / REVERSE', 'WIDE / FRONT', 'WIDE / REVERSE']
    for n in range(32):
        im = Image.new('RGB', (640, 440), 'white'); draw = ImageDraw.Draw(im)
        for k, start in enumerate(starts):
            x, y = k % 2 * 320, k // 2 * 220
            im.paste(images[start+n].resize((320, 200), Image.Resampling.NEAREST), (x, y+20))
            draw.text((x+4, y+4), labels[k], fill='black')
        comparison.append(im)
    gif(OUT/'four-views.gif', comparison, delays([1/32]*32))
    comparison[8].save(OUT/'four-views.png')
    with (OUT/'frames.csv').open('w') as f:
        writer = csv.DictWriter(f, fieldnames=list(rows[0])); writer.writeheader(); writer.writerows(rows)
    report = dict(binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
                  mesh_sha256=hashlib.sha256((HERE/'fftmesh.bin').read_bytes()).hexdigest(),
                  frames=FRAMES, matching_pc_frames=len(expected), clock_hz=12000000,
                  grayscale_reserve=.09, frame_cap=32,
                  timing='MC68000 datasheet cycle estimates, estimated AMS calls; not physical hardware FPS. Startup excluded.',
                  comparison='Four warmed 32-frame excerpts, synchronized at 32 FPS for visual review only.',
                  demo_fps=FRAMES/sum(seconds),
                  warmed={name: dict(fps=32/sum(seconds[start:start+32]),
                                    render_cycles=sum(r['render'] for r in rows[start:start+32])/32)
                          for name, start in zip(labels, starts)},
                  first_or_switch_ms={str(n): seconds[n]*1000 for n in [0, 64, 128, 192, 256, 320, 384, 448]},
                  pose_pool_peak=max(v & 0xffffff for v in values if v >> 24 == 0x74),
                  mask_pool_peak=max(v & 0xffffff for v in values if v >> 24 == 0x75),
                  pose_misses_peak_per_zoom=max(v & 0xffffff for v in values if v >> 24 == 0x76),
                  pose_pool_resets_peak_per_zoom=max(v & 0xffffff for v in values if v >> 24 == 0x77))
    (OUT/'manifest.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
