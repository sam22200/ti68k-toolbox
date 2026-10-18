#!/usr/bin/env python3
"""Measure lossless pointer access and opt-in half-height textured turn scenery."""
import csv
import json
from pathlib import Path
from statistics import mean

import turn_audit as audit
from turn_texture_assets import prepare
from turn_texture_bench import stage

HERE = Path(__file__).resolve().parents[1]
OUT = HERE / 'x/turn-opt'


def main():
    reference = json.loads((HERE / 'tools/turn_texture_reference.json').read_text())
    assets = OUT / 'assets'
    prepare(assets)
    flags = audit.FLAGS.copy() + ['-DFFT_TURN_TEXTURE']
    reports = {}
    for name, extra in [('pointers', []), ('half-y', ['-DFFT_TURN_TEXTURE_HALF_Y'])]:
        audit.OUT = OUT / name
        audit.FLAGS = flags + extra
        stage(audit.OUT / 'data', assets)
        audit.main()
        reports[name] = json.loads((audit.OUT / 'headless.json').read_text())
    full = (OUT / 'pointers/pc.csv').read_text()
    reduced = (OUT / 'half-y/pc.csv').read_text()
    assert full == reference['pc_csv'], 'Lossless optimization changed the baseline pictures'
    a, b = list(csv.reader(full.splitlines())), list(csv.reader(reduced.splitlines()))
    assert len(a) == len(b) == 32
    assert all(x == y for x, y in zip(a, b) if x[1] == '4'), 'Settled views must remain identical'
    baseline = mean(c['cpu_only_ms_at_12mhz'] for c in reference['cases'])
    comparison = dict(
        scope='Eight full F1/F5 turns, three intermediate frames and final decode/render. Datasheet CPU only; no new TiEmu timing.',
        historical_binary_sha256=reference['binary_sha256'],
        baseline_mean_cpu_ms=baseline,
        variants={name: dict(
            mean_cpu_ms=mean(c['cpu_only_ms_at_12mhz'] for c in report['cases']),
            min_cpu_ms=min(c['cpu_only_ms_at_12mhz'] for c in report['cases']),
            max_cpu_ms=max(c['cpu_only_ms_at_12mhz'] for c in report['cases']),
            reduction_percent=100 * (1 - mean(c['cpu_only_ms_at_12mhz'] for c in report['cases']) / baseline),
            matched_pc_ti_frames=report['matched_pc_ti_frames'],
            binary_bytes=report['binary_bytes'], binary_sha256=report['binary_sha256'],
        ) for name, report in reports.items()},
    )
    (OUT / 'comparison.json').write_text(json.dumps(comparison, indent=2) + '\n')
    # Art review only: these PC pictures match each variant's native TI checksums.
    from PIL import Image, ImageDraw
    sheet = Image.new('RGB', (1280, 460), '#eeeeee')
    draw = ImageDraw.Draw(sheet)
    for row, (name, title) in enumerate([
        ('pointers', '80x50 - unchanged image'),
        ('half-y', '80x25 during turns - native-resolution actors; same settled view'),
    ]):
        draw.text((8, row * 230 + 5), title, fill='black')
        for frame in range(1, 5):
            shot = Image.open(OUT / name / 'frames' / f'0-{frame}.png').convert('RGB')
            sheet.paste(shot.resize((320, 200), Image.Resampling.NEAREST), ((frame - 1) * 320, row * 230 + 25))
    sheet.save(OUT / 'resolution-comparison.png')
    print(json.dumps(comparison, indent=2))


if __name__ == '__main__':
    main()
