#!/usr/bin/env python3
"""Build the actual TiEmu timer benchmark and cross-check the matching TI cycle binary.

Explicit TI validation command. Unlike the *review tools, this invokes GCC4TI and ti-cycles.
The graphical emulator is launched separately with fftemu.89z and the archived banks.
"""
import csv
import hashlib
import json
from pathlib import Path
import re
import statistics
import subprocess
import sys

from texture_review import HERE, VIEWS
from prepare_scenery_bench import BANKS, OUT

ROOT = HERE.parents[1]


def run(args, cwd=ROOT):
    return subprocess.check_output([str(a) for a in args], cwd=cwd, text=True)


def main():
    run([sys.executable, 'tools/prepare_scenery_bench.py'], cwd=HERE)
    banks = OUT/'banks'; banks.mkdir(exist_ok=True)
    for name in BANKS:
        data = bytearray((HERE/(name+'.89y')).read_bytes())
        data[0x3c+13] = 3 # archive at transfer; the real calculator must retain free RAM
        (banks/(name+'.89y')).write_bytes(data)
    for view in range(4):
        name = f'fftb{view}'
        # ttbin2oth uses the output prefix verbatim as the calculator variable name.
        # A full output path would generate an invalid name like '/home/sa'.
        run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth', '-89', 'dat', OUT/'data'/(name+'.bin'), name], cwd=banks)
        path = banks/(name+'.89y'); data = bytearray(path.read_bytes()); data[0x3c+13] = 3
        assert data[64:72].split(b'\0')[0].decode() == name
        path.write_bytes(data)
    common = ['-DFFT_EXPLORATION', '-DFFT_FIXED']
    runtime = ['runtime/core/rt_core.c', 'lib/unpack68k.s', 'tools/extgraph/lib/tilemap.a']
    run([ROOT/'tools/bin/ti-cc', '-o', OUT/'fftemu', *common,
         'games/fft/tools/scenery_tiemu.c', *runtime])
    run([ROOT/'tools/bin/ti-cc', '-o', OUT/'fftscyc', '-DRT_CYCLES', '-DFFT_CAPTURE', *common,
         'games/fft/tools/fixed_scenery_bench.c', 'runtime/platform-ti68k/rt_ti.c', *runtime])
    summary = []
    for case in range(8):
        args = [ROOT/'tools/bin/ti-cycles', '--arg', case, '--frames', 128, '--max', 20000,
                '--png', OUT/f'cycles-{case}.png']
        for path in sorted(banks.glob('*.89y')):
            args += ['--file', path]
        log = run([*args, OUT/'fftscyc.89z']); (OUT/f'cycles-{case}.log').write_text(log)
        assert '_main returned' in log
        hashes = re.findall(r'shot \d+ checksum (\w+)', log)
        expected = [line.split()[1] for line in (OUT/f'pc-{case}.log').read_text().splitlines()]
        assert hashes[:-1] == expected and len(expected) == 128
        values = [int(v, 16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)', log)]
        costs = [values[i+1] for i, v in enumerate(values[:-1]) if v >> 24 == 0x78]
        assert len(costs) == 128
        warm = costs[32:]; average = statistics.mean(warm)
        summary.append(dict(case=case, textured=case < 4, view=VIEWS[case & 3],
                            cold_cycles=costs[0], warm_avg=average, warm_min=min(warm), warm_max=max(warm),
                            estimated_capped_fps=min(32, 10920000/(average+600)),
                            estimated_raw_fps=10920000/(average+600),
                            estimated_cold_ms=(costs[0]+600)*1000/10920000, source_frames_match=128))
        print(case, VIEWS[case & 3], 'texture' if case < 4 else 'flat', average, 'cycles/frame', flush=True)
    for view in range(4):
        assert summary[view]['warm_avg'] == summary[view+4]['warm_avg']
        assert summary[view]['cold_cycles']-summary[view+4]['cold_cycles'] == 36268
    (OUT/'cycles-summary.json').write_text(json.dumps(summary, indent=2)+'\n')
    with (OUT/'cycles-summary.csv').open('w') as file:
        writer = csv.DictWriter(file, fieldnames=list(summary[0])); writer.writeheader(); writer.writerows(summary)
    manifest = dict(timing='MC68000 datasheet counts, estimated AMS calls. FPS estimate: 12 MHz, 9% grayscale reserve, 600 cycles input/update allowance, 32 FPS cap.',
                    scope='Cached flat/textured scenery, actors in both. Texture mapping is offline.',
                    matched_frames=1024, binary_sha256={name:hashlib.sha256((OUT/(name+'.89z')).read_bytes()).hexdigest()
                                                       for name in ['fftemu', 'fftscyc']},
                    source_sha256={name:hashlib.sha256((HERE/name).read_bytes()).hexdigest() for name in
                                   ['fft.c', 'fft.h', 'perspective.inc', 'fixed.inc', 'tools/fixed_scenery_bench.c',
                                    'tools/scenery_tiemu.c', 'tools/scenery_ti_benchmark.py']},
                    bank_sha256={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(banks.glob('*.89y'))})
    (OUT/'validated-ti.json').write_text(json.dumps(manifest, indent=2)+'\n')
    print('1024 PC/TI frames match; warmed costs identical. fftemu.89z ready for the timer run in TiEmu.')


if __name__ == '__main__':
    main()
