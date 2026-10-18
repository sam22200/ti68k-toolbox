#!/usr/bin/env python3
"""Compare 40x100 moving scenery with 80x50, keeping native-resolution actors."""
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
OUT = HERE / 'x/turn-horizontal'
BASE = audit.FLAGS.copy() + ['-DFFT_TURN_TEXTURE'] + surface.FAST
X40 = ['-DFFT_TURN_TEXTURE_QUARTER_X']


def main():
    normal = OUT / 'assets-80x50'
    horizontal = OUT / 'assets'
    old_assets = prepare(normal)
    assets = prepare(horizontal, horizontal=True)
    assert old_assets['bytes'] == 31270 and assets['bytes'] == 21306
    reports = {}
    for name, extra, data in [('80x50', [], normal), ('40x100', X40, horizontal)]:
        audit.OUT, audit.FLAGS = OUT / name, BASE + extra
        stage(audit.OUT / 'data', data)
        audit.main()
        reports[name] = json.loads((audit.OUT / 'headless.json').read_text())
    a = list(csv.reader((OUT / '80x50/pc.csv').read_text().splitlines()))
    b = list(csv.reader((OUT / '40x100/pc.csv').read_text().splitlines()))
    assert [row for row in a if row[1] == '4'] == [row for row in b if row[1] == '4']
    # The complete 80x50 pictures must also remain exactly as before the new layout.
    previous = HERE / 'x/turn-surface/merged-spans/pc.csv'
    if previous.exists():
        assert (OUT / '80x50/pc.csv').read_text() == previous.read_text()
    surface.OUT = OUT
    stage(OUT / 'reference/data', horizontal)
    full = surface.camera_check('reference', BASE + X40 + ['-DFFT_TURN_TEXTURE_ALL_FACES'])
    clipped = surface.camera_check('40x100', BASE + X40)
    assert full == clipped, 'New angle lists dropped visible geometry'
    demo.OUT = OUT / 'demo'
    demo.FLAGS += ['-DFFT_TURN_TEXTURE'] + surface.FAST + X40
    stage(demo.OUT / 'data', horizontal)
    demo.main()
    sheet = Image.new('RGB', (1280, 460), '#eeeeee')
    draw = ImageDraw.Draw(sheet)
    for row, (name, title) in enumerate([
        ('80x50', '80x50 - optimized reference, 2x2 scenery pixels'),
        ('40x100', '40x100 - 4x1 scenery pixels, native-resolution actors'),
    ]):
        draw.text((8, row*230+5), title, fill='black')
        for frame in range(1, 5):
            im = Image.open(OUT / name / 'frames' / f'0-{frame}.png').convert('RGB')
            sheet.paste(im.resize((320, 200), Image.Resampling.NEAREST), ((frame-1)*320, row*230+25))
    sheet.save(OUT / 'comparison.png')
    result = dict(scope='Same logical mesh, textures, camera positions, angles and actors. Datasheet CPU only; no inferred emulator duration.',
                  assets=assets, settled_views_identical=True, matched_camera_frames=576,
                  horizontal_expander='32,000 individual output-plane bits checked on PC and native TI before camera checks.',
                  matched_sequential_frames=512,
                  mapper_source_sha256=hashlib.sha256((HERE / 'turn_texture.inc').read_bytes()).hexdigest(),
                  variants={})
    for name, report in reports.items():
        result['variants'][name] = dict(mean_cpu_ms=mean(c['cpu_only_ms_at_12mhz'] for c in report['cases']),
            min_cpu_ms=min(c['cpu_only_ms_at_12mhz'] for c in report['cases']),
            max_cpu_ms=max(c['cpu_only_ms_at_12mhz'] for c in report['cases']),
            matched_turn_frames=32, work_bytes=report['work_bytes'],
            binary_bytes=report['binary_bytes'], binary_sha256=report['binary_sha256'])
    result['cpu_change_percent'] = 100 * (result['variants']['40x100']['mean_cpu_ms'] / result['variants']['80x50']['mean_cpu_ms'] - 1)
    (OUT / 'comparison.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
