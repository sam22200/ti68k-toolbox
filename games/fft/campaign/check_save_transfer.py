#!/usr/bin/env python3
"""Real PC/68000 file interchange, in private directories, with no screenshots."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
sys.path.insert(0, str(HERE.parent/'tools'))
import save_transfer as transfer


def run(command, cwd, logfile):
    process = subprocess.run(list(map(str, command)), cwd=cwd, text=True,
                             stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                             env={**os.environ, 'ASAN_OPTIONS':'detect_leaks=0', 'UBSAN_OPTIONS':'halt_on_error=1'})
    logfile.write_text(process.stdout)
    if process.returncode:
        print(process.stdout)
        process.check_returncode()
    return process.stdout


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seed', type=Path, default=HERE/'x/pilot-e2e/final/pc/new-game-to-world-save.sav')
    parser.add_argument('--out', type=Path, default=HERE/'x/save-transfer/latest')
    parser.add_argument('--native', action='store_true', help='One small real TI-runtime I/O check under ti-cycles')
    args = parser.parse_args()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    _, original = transfer.read_save(args.seed)
    if original[:4] != b'FSV9':
        parser.error('Use a current FSV9 seed from an actual pilot run')
    report = dict(save_bytes=len(original), sha256=hashlib.sha256(original).hexdigest(), modes=[], screenshots=0,
                  no_live_emulator=True, native_hardware_ram_measured=False)
    with tempfile.TemporaryDirectory(prefix='exchange-', dir=out) as temporary:
        work = Path(temporary)
        pc = work/'ffsave1.sav'
        pc.write_bytes(original)
        legacy = work/'ffsave3.sav'
        legacy.write_bytes(b'FSV8' + original[4:4024])
        # Preserve the real pilot as slot1 and an FSV8 migration control as slot3.
        for source in (pc, legacy):
            transfer.to_ti(source, work/(source.stem+'.89y'))
        expected = None
        for mode in ('pc', 'san'):
            binary = out/('read-'+mode)
            flags = ['-fsanitize=address,undefined', '-fno-sanitize=shift', '-fno-omit-frame-pointer', '-no-pie'] if mode == 'san' else []
            run(['cc', '-std=gnu99', '-O1', '-g', *flags, '-o', binary, HERE/'check_save_transfer.c',
                 ROOT/'runtime/core/rt_core.c', ROOT/'runtime/platform-sw/rt_sw.c'], ROOT, out/(mode+'-build.log'))
            log = run([binary], work, out/(mode+'.log'))
            match = re.search(r'save-transfer 0 4684 ([0-9A-F]{8}) ([0-9A-F]{8})', log)
            assert match, log
            values = [0, 4684, *[int(x,16) for x in match.groups()]]
            assert (work/'ffsave2.sav').read_bytes() == original
            if expected is None:
                expected = values
            else:
                assert values == expected
            report['modes'].append(mode)
        if args.native:
            native = out/'native'
            native.mkdir(exist_ok=True)
            wrapper = native/'bridge.c'
            wrapper.write_text('#include "'+str(HERE/'check_save_transfer.c')+'"\n')
            run([ROOT/'tools/bin/ti-cc', '-DRT_CYCLES', '-o', 'ffxsave', wrapper,
                 ROOT/'runtime/core/rt_core.c', ROOT/'tools/extgraph/lib/tilemap.a'], native, native/'build.log')
            # Import an existing slot2 too, so native output must replace its VAT handle.
            replacement = work/'ffsave2.sav'
            replacement.write_bytes(legacy.read_bytes())
            transfer.to_ti(replacement, work/'ffsave2.89y')
            saved = work/'native-saved'
            saved.mkdir()
            log = run([ROOT/'tools/bin/ti-cycles', '--max', '100', '--save-dir', saved,
                       *[x for n in (1,2,3) for x in ('--file',work/f'ffsave{n}.89y')], native/'ffxsave.89z'],
                      native, native/'replay.log')
            values = [int(x,16) for x in re.findall(r'value: .*?\(0x([0-9a-f]+)\)', log)]
            assert values == expected, (values, expected)
            assert '_main returned' in log
            assert [p.name for p in saved.iterdir()] == ['ffsave2.89y']
            # Decode the file actually emitted by native AMS I/O, not a prior artifact.
            received = work/'received'
            _, actual, _ = transfer.to_pc(saved/'ffsave2.89y', received)
            assert actual == original
            transfer.to_pc(work/'ffsave3.89y', received)
            returned = run([out/'read-san', 'ffsave2'], received, out/'returned-san.log')
            assert returned == (out/'san.log').read_text()
            assert (received/'ffsave2.sav').read_bytes() == original
            (out/'roundtrip.sav').write_bytes(actual)
            (native/'ffsave2.89y').write_bytes((saved/'ffsave2.89y').read_bytes())
            report.update(native_values=values, pc_ti_pc_identical=True, native_atomic_replacement=True,
                          legacy_migration_equal=True, returned_pc_sanitizers_pass=True)
    report['sources'] = {str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in
                         (HERE/'check_save_transfer.c', Path(__file__), HERE.parent/'tools/save_transfer.py',
                          HERE/'save.h', HERE/'save_types.h')}
    (out/'results.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
