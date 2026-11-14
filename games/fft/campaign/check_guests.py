#!/usr/bin/env python3
"""Guest/save boundaries on PC/sanitizers; opt-in actual native SAVE/restore."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import tempfile
from check_flow import HERE, ROOT, data_links, fixture, run
import guest_bank


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out', type=Path, default=HERE/'x/guests/runtime')
    ap.add_argument('--data-dir', type=Path)
    ap.add_argument('--native', action='store_true')
    args = ap.parse_args(); out = args.out.resolve(); out.mkdir(parents=True, exist_ok=True)
    source = args.data_dir.resolve() if args.data_dir else None
    bank = guest_bank.build([dict(route='g', slot=0, id=4)])
    initial = bytearray(b'FSV5'+fixture()[4:]+bytes(66)); initial[36] = 16
    for i in range(7, 16): initial[294+176*i:294+176*(i+1)] = initial[470:646]
    logs = []
    for mode, flags in [('pc', []), ('san', ['-fsanitize=address,undefined', '-fno-sanitize=shift', '-fno-pie', '-no-pie'])]:
        binary = out/mode
        (out/f'{mode}-build.log').write_text(run(['cc', '-std=gnu99', '-O1', '-g', '-w', *flags, '-o', binary,
             HERE/'check_guests.c', ROOT/'runtime/core/rt_core.c', ROOT/'runtime/platform-sw/rt_sw.c']))
        with tempfile.TemporaryDirectory(prefix='fft-guests-') as name:
            data = Path(name); data_links(data, source)
            (data/'ffgu.bin').unlink(missing_ok=True); (data/'ffgu.bin').write_bytes(bank)
            (data/'ffsave.sav').write_bytes(initial)
            log = run(['env', 'ASAN_OPTIONS=detect_leaks=0', 'UBSAN_OPTIONS=halt_on_error=1', binary], data)
            (out/f'{mode}.log').write_text(log); logs.append(log)
    assert logs[0] == logs[1]
    report = dict(runtime=logs[0].strip(), sanitizer_equal=True, native=False)
    if args.native:
        if not source: raise ValueError('--native requires matching installed modules in --data-dir')
        (out/'native-build.log').write_text(run([ROOT/'tools/bin/ti-cc', '-DRT_CYCLES', '-o', out/'guestck',
             HERE/'check_guests.c', ROOT/'runtime/core/rt_core.c', ROOT/'lib/unpack68k.s', ROOT/'tools/extgraph/lib/tilemap.a']))
        with tempfile.TemporaryDirectory(prefix='fft-guests-native-') as name:
            data = Path(name); (data/'ffgu.bin').write_bytes(bank); (data/'ffsave.sav').write_bytes(initial)
            for ext, kind, var in [('bin', 'dat', 'ffgu'), ('sav', 'sav', 'ffsave')]:
                run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth', '-89', kind, f'{var}.{ext}', var], data)
            cmd = [ROOT/'tools/bin/ti-cycles', '--frames', 0, '--arg', 261, '--max', 20000]
            for p in source.glob('*.89y'): cmd += ['--file', p]
            log = run([*cmd, '--file', data/'ffgu.89y', '--file', data/'ffsave.89y', out/'guestck.89z'])
            (out/'native.log').write_text(log)
        expected = re.search(r'guest-runtime (\w+) failures 0 checks (\d+)', logs[0]); assert expected
        values = [int(v, 16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)', log)]
        assert values[-3:] == [int(expected[1], 16), 0, int(expected[2])], values
        report['native'] = True
    report['source_sha256'] = {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
        for p in [HERE/'save_types.h', HERE/'save.h', HERE/'guest_roster.h', HERE.parent/'save.inc',
                  HERE.parent/'guest_load.inc', HERE.parent/'guest_bank.h', HERE/'check_guests.c']}
    (out/'results.json').write_text(json.dumps(report, indent=2)+'\n'); print(json.dumps(report, indent=2))


if __name__ == '__main__': main()
