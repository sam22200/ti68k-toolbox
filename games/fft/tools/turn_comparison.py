#!/usr/bin/env python3
"""True native 160x100 low-poly versus 80x25 textured rotation controls."""
import csv
import hashlib
import json
from pathlib import Path
from statistics import mean

from PIL import Image, ImageDraw

import camera_cuts as demo
import turn_audit as audit
import turn_surface_bench as surface
from turn_surface_assets import prepare
from turn_texture_bench import stage

HERE = Path(__file__).resolve().parents[1]
OUT = HERE / 'x/turn-comparison'
BASE = audit.FLAGS.copy()
VARIANTS = [
    ('lowpoly160', ['-DFFT_TURN_LOWPOLY_NATIVE']),
    ('textured80x25', ['-DFFT_TURN_TEXTURE', '-DFFT_TURN_TEXTURE_HALF_Y'] + surface.FAST),
]


def main():
    assets = OUT / 'assets'
    prepare(assets)
    reports = {}
    for name, flags in VARIANTS:
        audit.OUT, audit.FLAGS = OUT / name, BASE + flags
        if name == 'textured80x25':
            stage(audit.OUT / 'data', assets)
        audit.main()
        audit.run([audit.OUT/'oracle', audit.OUT/'source.png', '--source'], audit.OUT/'data')
        reports[name] = json.loads((audit.OUT / 'headless.json').read_text())
    a = list(csv.reader((OUT / 'lowpoly160/pc.csv').read_text().splitlines()))
    b = list(csv.reader((OUT / 'textured80x25/pc.csv').read_text().splitlines()))
    assert [r for r in a if r[1] == '4'] == [r for r in b if r[1] == '4']
    assert reports['lowpoly160']['work_bytes'] == reports['textured80x25']['work_bytes'] + 200
    surface.OUT = OUT
    for name, flags in VARIANTS:
        surface.camera_check(name, BASE + flags)
    demo_base = demo.FLAGS.copy()
    for name, flags in VARIANTS:
        demo.OUT, demo.FLAGS = OUT / (name + '-demo'), demo_base + flags
        if name == 'textured80x25':
            stage(demo.OUT / 'data', assets)
        demo.main()
    sheet = Image.new('RGB', (1280, 460), '#eeeeee')
    draw = ImageDraw.Draw(sheet)
    for row, (name, label) in enumerate([
        ('textured80x25', 'Textured 80x25 - detailed original mesh, native actors and depth cover'),
        ('lowpoly160', 'Low poly 160x100 - simplified terrain, native actors without terrain cover'),
    ]):
        draw.text((8, row*230+5), label, fill='black')
        for frame in range(1, 5):
            im = Image.open(OUT / name / 'frames' / f'0-{frame}.png').convert('RGB')
            sheet.paste(im.resize((320, 200), Image.Resampling.NEAREST), ((frame-1)*320, row*230+25))
    sheet.save(OUT / 'comparison.png')
    result = dict(scope='Same logical map, camera pivot, source/target and 3 angles; different scenery representations.',
        geometry='Low poly uses tile tops/sides and painter order; texture variant uses the original detailed merged mesh and depth. Not an identical-mesh texture-off control.',
        actors='Native TI sprites in both; terrain cover only in the textured renderer during motion.',
        checks='32 PC/TI turn pictures + 576 broad camera pictures + 512 sequential pictures per variant; same settled views and unchanged battle state/RNG.',
        timing='Datasheet CPU only; actual emulator timing belongs to each tiemu.json.',
        source_sha256={p: hashlib.sha256((HERE/p).read_bytes()).hexdigest() for p in ['fft.c', 'turn_texture.inc', 'tools/turn_surface_check.c']},
        variants={})
    for name, report in reports.items():
        result['variants'][name] = dict(mean_cpu_ms=mean(c['cpu_only_ms_at_12mhz'] for c in report['cases']),
            min_cpu_ms=min(c['cpu_only_ms_at_12mhz'] for c in report['cases']),
            max_cpu_ms=max(c['cpu_only_ms_at_12mhz'] for c in report['cases']),
            work_bytes=report['work_bytes'], binary_bytes=report['binary_bytes'], binary_sha256=report['binary_sha256'])
    (OUT/'comparison.json').write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
