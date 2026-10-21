#!/usr/bin/env python3
"""Measure native reach creation separately from warmed rendering; no TiEmu."""
from pathlib import Path
import argparse
import json
import re
import statistics
import subprocess

ROOT = Path(__file__).resolve().parents[3]
GAME = ROOT / 'games/fft'
DATA = GAME / 'campaign/data'
OUT = GAME / 'campaign/x/ring-diff'
KEYS = ('0\n20 ENTER\n21\n45 RIGHT\n46\n60 LEFT\n61\n80 MODE\n81\n'
        '110 B\n111\n130 ENTER\n131\n155 B\n156\n170 DOWN\n171\n'
        '172 ENTER\n173\n175 ENTER\n176\n220 MODE\n221\n240 B\n241\n')


def run(args):
    p = subprocess.run(list(map(str, args)), cwd=DATA, text=True,
                       stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if p.returncode:
        print(p.stdout)
        p.check_returncode()
    return p.stdout


def measure(binary, name):
    command = [ROOT/'tools/bin/ti-cycles', '--max', 1000, '--arg', 61,
               '--frames', 260, '--keys', OUT/'range.keys',
               '--png', OUT/(name+'.png')]
    for bank in ('ffgv0', 'ffgv1', 'ffgv2', 'ffgv3', 'fftu', 'fftw', 'ffgi'):
        command += ['--file', DATA/(bank+'.89y')]
    log = run([*command, binary])
    (OUT/(name+'.log')).write_text(log)
    values = [int(v, 16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)', log)]
    assert values[-2] == 0xACAD0001 and values[-3] == 260
    assert len(values[:-3]) == 520
    frames = {m & 65535: c for m, c in zip(values[:-3:2], values[1:-3:2])}
    return dict(binary_bytes=binary.stat().st_size,
                checksum=re.search(r'checksum\s+(\w+)', log)[1],
                state_hash=values[-1],
                event_cycles={f: frames[f] for f in (20, 81, 130, 175, 220)},
                warm_average_cycles=round(statistics.mean(frames[f] for f in range(22, 45))))


def main():
    parser = argparse.ArgumentParser(__doc__)
    parser.add_argument('--baseline', type=Path,
                        help='optional earlier instrumented campaign check binary')
    args = parser.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    (OUT/'range.keys').write_text(KEYS)
    run([ROOT/'tools/bin/ti-cc', '-o', OUT/'ringdiff', '-DRT_CYCLES',
         '-DFFT_MEASURE', '-DCHECK_GAR', GAME/'campaign/check_native.c',
         ROOT/'runtime/core/rt_core.c', ROOT/'lib/unpack68k.s',
         ROOT/'tools/extgraph/lib/tilemap.a'])
    results = dict(current=measure(OUT/'ringdiff.89z', 'diff'))
    if args.baseline:
        results['baseline'] = measure(args.baseline.resolve(), 'baseline')
        for key in ('checksum', 'state_hash'):
            assert results['current'][key] == results['baseline'][key], key
    report = dict(measurement='Actual MC68000 datasheet cycles; nominal 12 MHz. '
                  'Grayscale, frame wait and real AMS overhead excluded. '
                  'No emulator or hardware FPS claim.', scenario=61, results=results)
    (OUT/'profile.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
