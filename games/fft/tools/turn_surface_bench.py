#!/usr/bin/env python3
"""Native surface optimization controls, broad camera parity and sequential demo."""
import csv
import hashlib
import json
from pathlib import Path
import re
from statistics import mean

from PIL import Image, ImageDraw
import numpy as np

import camera_cuts as demo
import turn_audit as audit
from turn_texture_bench import stage
from turn_surface_assets import prepare

HERE = Path(__file__).resolve().parents[1]
OUT = HERE / 'x/turn-surface'
BASE = audit.FLAGS.copy() + ['-DFFT_TURN_TEXTURE']
FAST = ['-DFFT_TURN_TEXTURE_SURFACES', '-DFFT_TURN_TEXTURE_MERGE', '-DFFT_TURN_TEXTURE_SPANS']


def camera_check(name, flags):
    folder = OUT / name
    args = ['tools/turn_surface_check.c', '../../runtime/core/rt_core.c']
    audit.run(['cc', '-std=gnu99', '-O1', '-g', *flags, '-o', folder / 'check-pc', *args,
               '../../runtime/platform-sw/rt_sw.c'])
    pc = audit.run([folder / 'check-pc'], folder / 'data')
    (folder / 'camera-check.csv').write_text(pc)
    # The lossless reference also checks visibility/spans against the old rasterizer.
    if name == 'reference':
        return pc
    audit.run([audit.ROOT / 'tools/bin/ti-cc', '-o', folder / 'fftscheck', *flags, '-DRT_CYCLES',
               *args, '../../lib/unpack68k.s', '../../tools/extgraph/lib/tilemap.a'])
    command = [audit.ROOT / 'tools/bin/ti-cycles', '--max', 1000000]
    for path in sorted((folder / 'data').glob('*.89y')):
        command += ['--file', path]
    log = audit.run([*command, folder / 'fftscheck.89z'], folder / 'data')
    (folder / 'camera-check-cycles.log').write_text(log)
    actual = re.findall(r'shot \d+ checksum (\w+)', log)
    expected = [row[1].lower() for row in csv.reader(pc.splitlines())]
    assert len(actual) == len(expected) == 576
    assert [x.lower() for x in actual] == expected
    assert '(0x74000240)' in log and '_main returned' in log
    return pc


def main():
    assets = OUT / 'assets'
    manifest = prepare(assets)
    variants = [
        ('reference', []),
        ('visibility', ['-DFFT_TURN_TEXTURE_SURFACES']),
        ('merged-spans', FAST),
        ('merged-half-y', FAST + ['-DFFT_TURN_TEXTURE_HALF_Y']),
    ]
    reports = {}
    for name, flags in variants:
        audit.OUT, audit.FLAGS = OUT / name, BASE + flags
        stage(audit.OUT / 'data', assets)
        audit.main()
        reports[name] = json.loads((audit.OUT / 'headless.json').read_text())
    reference = (OUT / 'reference/pc.csv').read_text()
    assert reference == json.loads((HERE / 'tools/turn_texture_reference.json').read_text())['pc_csv']
    assert (OUT / 'visibility/pc.csv').read_text() == reference
    settled = [r for r in csv.reader(reference.splitlines()) if r[1] == '4']
    for name, _ in variants:
        assert [r for r in csv.reader((OUT / name / 'pc.csv').read_text().splitlines()) if r[1] == '4'] == settled
    broad = {name: camera_check(name, BASE + flags) for name, flags in variants}
    assert broad['reference'] == broad['visibility'], 'Angle lists changed a camera picture'
    # Record every visual difference, including the worst frame for review.
    changes = {}
    for name in ['merged-spans', 'merged-half-y']:
        diffs = []
        for case in range(8):
            for frame in range(1, 4):
                path = f'{case}-{frame}.png'
                a = np.asarray(Image.open(OUT / 'reference/frames' / path).convert('L'))
                b = np.asarray(Image.open(OUT / name / 'frames' / path).convert('L'))
                diffs.append(dict(case=case, frame=frame, changed_pixels=int(np.count_nonzero(a != b))))
        changes[name] = dict(frames=diffs, max_changed_pixels=max(d['changed_pixels'] for d in diffs),
                             scope='160x100 intermediate pictures including actors; settled views identical')
    baseline = mean(c['cpu_only_ms_at_12mhz'] for c in reports['reference']['cases'])
    first = json.loads((HERE / 'tools/turn_texture_reference.json').read_text())
    initial = mean(c['cpu_only_ms_at_12mhz'] for c in first['cases'])
    result = dict(assets=manifest, scope='Datasheet CPU only. No emulator timing inferred.',
                  broad_camera_cases='6 pivots x 4 anchor phases x 8 directions x 3 intermediate angles = 576',
                  image_changes=changes, variants={})
    for name, report in reports.items():
        value = mean(c['cpu_only_ms_at_12mhz'] for c in report['cases'])
        result['variants'][name] = dict(mean_cpu_ms=value,
            min_cpu_ms=min(c['cpu_only_ms_at_12mhz'] for c in report['cases']),
            max_cpu_ms=max(c['cpu_only_ms_at_12mhz'] for c in report['cases']),
            reduction_from_reference_percent=100 * (1 - value / baseline),
            reduction_from_initial_percent=100 * (1 - value / initial),
            matched_turn_frames=32, matched_camera_frames=576 if name != 'reference' else 0,
            binary_bytes=report['binary_bytes'], binary_sha256=report['binary_sha256'])
    sheet = Image.new('RGB', (1280, 690), '#eeeeee')
    draw = ImageDraw.Draw(sheet)
    for row, (name, label) in enumerate([
        ('reference', 'Reference 80x50'),
        ('merged-spans', 'Merged surfaces + visibility + directed spans - 80x50'),
        ('merged-half-y', 'Optional 80x25 moving scenery - same textures and native actors'),
    ]):
        draw.text((8, row * 230 + 5), label, fill='black')
        for frame in range(1, 5):
            im = Image.open(OUT / name / 'frames' / f'0-{frame}.png').convert('RGB')
            sheet.paste(im.resize((320, 200), Image.Resampling.NEAREST), ((frame-1)*320, row*230+25))
    sheet.save(OUT / 'comparison.png')
    # Continuous native sequence, including cache invalidation and fixed inspection return.
    demo.OUT = OUT / 'demo'
    demo.FLAGS += ['-DFFT_TURN_TEXTURE'] + FAST
    stage(demo.OUT / 'data', assets)
    demo.main()
    result['sequential_pc_ti_frames'] = 512
    result['mapper_source_sha256'] = hashlib.sha256((HERE / 'turn_texture.inc').read_bytes()).hexdigest()
    (OUT / 'comparison.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
