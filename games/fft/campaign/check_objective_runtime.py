#!/usr/bin/env python3
"""Focused real PC actions/save receipts and approved objective pixels."""
import argparse
import json
import re
from pathlib import Path
import subprocess
import tempfile

from PIL import Image
from check_flow import HERE, ROOT, data_links, fixture, run
import objective_bank


def render_objectives(out, data_source, banks):
    binary = out/'visual'
    (out/'visual-build.log').write_text(run(['cc', '-std=gnu99', '-O1', '-g', '-w', '-o', binary,
           HERE/'review_objectives_runtime.c', ROOT/'runtime/core/rt_core.c', ROOT/'runtime/platform-sw/rt_sw.c']))
    screens = []
    with tempfile.TemporaryDirectory(prefix='fft-objective-pixels-') as name:
        data = Path(name); data_links(data, data_source)
        for i, bank in enumerate([None]+banks):
            (data/'ffgo.bin').unlink(missing_ok=True)
            if bank: (data/'ffgo.bin').write_bytes(bank)
            shot = out/f'objective-{i}.png'
            screens.append(run([binary, shot], data).strip())
            approved = HERE/f'x/objectives/review/objective-{i}.png'
            assert Image.open(shot).tobytes() == Image.open(approved).tobytes(), (i, 'approved pixels differ')
    return screens


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out', type=Path, default=HERE/'x/objectives/integration')
    ap.add_argument('--data-dir', type=Path)
    ap.add_argument('--native', action='store_true')
    args = ap.parse_args(); out = args.out.resolve(); out.mkdir(parents=True, exist_ok=True)
    data_source = args.data_dir.resolve() if args.data_dir else None
    target = dict(slot=1, name='Keane'); protect = dict(slot=0, name='Delita')
    pairs = [dict(target=target), dict(protect=protect), dict(target=target, protect=protect)]
    banks = [objective_bank.build([dict(route='g', **p)]) for p in pairs]
    (out/'ffgo.bin').write_bytes(banks[2])
    # Exact reuse across routes: equal titles/names share physical spans.
    pooled = objective_bank.build([dict(route=r, **pairs[2]) for r in ('g', 'h')])
    assert len(pooled) == len(banks[2])+16
    logs = []
    for label, flags in [('pc', []), ('san', ['-fsanitize=address,undefined', '-fno-sanitize=shift', '-fno-pie', '-no-pie'])]:
        binary = out/label
        (out/f'{label}-build.log').write_text(run(['cc', '-std=gnu99', '-O1', '-g', '-w', *flags, '-o', binary,
             HERE/'check_objective_runtime.c', ROOT/'runtime/core/rt_core.c', ROOT/'runtime/platform-sw/rt_sw.c']))
        with tempfile.TemporaryDirectory(prefix='fft-objective-run-') as name:
            data = Path(name); data_links(data, data_source)
            (data/'ffgo.bin').unlink(missing_ok=True); (data/'ffgo.bin').write_bytes(banks[2])
            (data/'ffsave.sav').write_bytes(b'FSV5'+fixture()[4:]+bytes(66))
            p = subprocess.run(['env', 'ASAN_OPTIONS=detect_leaks=0', 'UBSAN_OPTIONS=halt_on_error=1', str(binary)], cwd=data,
                               text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            (out/f'{label}.log').write_text(p.stdout)
            if p.returncode: print(p.stdout); p.check_returncode()
            logs.append(p.stdout)
    assert logs[0] == logs[1]
    screens = render_objectives(out, data_source, banks)
    report = dict(runtime=logs[0].strip(), sanitizer_equal=True, approved_screens=screens,
                  bank_bytes=len(banks[2]), pooled_two_route_bytes=len(pooled), native=False)
    if args.native:
        if not data_source: raise ValueError('--native requires --data-dir with matching installed modules')
        (out/'native-build.log').write_text(run([ROOT/'tools/bin/ti-cc', '-DRT_CYCLES', '-o', out/'goalck',
             HERE/'check_objective_runtime.c', ROOT/'runtime/core/rt_core.c', ROOT/'lib/unpack68k.s', ROOT/'tools/extgraph/lib/tilemap.a']))
        with tempfile.TemporaryDirectory(prefix='fft-objective-native-') as name:
            data = Path(name)
            (data/'ffgo.bin').write_bytes(banks[2]); (data/'ffsave.sav').write_bytes(b'FSV5'+fixture()[4:]+bytes(66))
            for ext, kind, bankname in [('bin', 'dat', 'ffgo'), ('sav', 'sav', 'ffsave')]:
                run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth', '-89', kind, f'{bankname}.{ext}', bankname], data)
            command = [ROOT/'tools/bin/ti-cycles', '--frames', 0, '--arg', 261, '--max', 10000]
            for p in data_source.glob('*.89y'): command += ['--file', p]
            command += ['--file', data/'ffgo.89y', '--file', data/'ffsave.89y', out/'goalck.89z']
            native = run(command); (out/'native.log').write_text(native)
        match = re.search(r'objective-runtime (\w+) failures 0 checks (\d+)', logs[0]); assert match
        values = [int(v, 16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)', native)]
        assert values[-3:] == [int(match[1], 16), 0, int(match[2])], values
        report['native'] = True
        (out/'native-visual-build.log').write_text(run([ROOT/'tools/bin/ti-cc', '-DRT_CYCLES', '-o', out/'goalvis',
             HERE/'review_objectives_runtime.c', ROOT/'runtime/core/rt_core.c', ROOT/'lib/unpack68k.s', ROOT/'tools/extgraph/lib/tilemap.a']))
        with tempfile.TemporaryDirectory(prefix='fft-objective-native-picture-') as name:
            data = Path(name); (data/'ffgo.bin').write_bytes(banks[2])
            run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth', '-89', 'dat', 'ffgo.bin', 'ffgo'], data)
            command = [ROOT/'tools/bin/ti-cycles', '--frames', 0, '--arg', 61, '--max', 1000]
            for p in data_source.glob('*.89y'): command += ['--file', p]
            native = run([*command, '--file', data/'ffgo.89y', out/'goalvis.89z'])
            (out/'native-visual.log').write_text(native)
        checksum = re.findall(r'checksum (\w+)', native)[-1].lower()
        costs = [int(v, 16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)', native)]
        assert checksum == '168f' and costs[-1] < 360000, (checksum, costs)
        report['native_banner'] = dict(checksum=checksum, cycles=costs[-1])
    (out/'results.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__': main()
