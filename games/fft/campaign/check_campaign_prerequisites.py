#!/usr/bin/env python3
"""PC/sanitizer checks in private data trees; never compile or run TI implicitly."""
import argparse
import json
from pathlib import Path
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]


def run(argv, cwd, log):
    result = subprocess.run(list(map(str, argv)), cwd=cwd, text=True,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    log.write_text(result.stdout)
    if result.returncode:
        print(result.stdout)
        result.check_returncode()
    return result.stdout


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, default=HERE/'x/campaign-prerequisites/tests')
    parser.add_argument('--only', choices=['map', 'save', 'scene', 'raw-management'])
    args = parser.parse_args()
    out = args.out.resolve();out.mkdir(parents=True, exist_ok=True)
    cases = [('map', 'check_scene_controls.c'), ('save', 'test_save.c'), ('scene', 'test_scene.c')]
    if args.only=='raw-management':cases=[('raw-management','test_raw_management.c')]
    results = []
    for name, source in cases:
        if args.only and name != args.only:
            continue
        logs = []
        for mode, flags in [('pc', []), ('san', ['-fsanitize=address,undefined',
                            '-fno-sanitize=shift', '-fno-pie', '-no-pie'])]:
            binary = out/f'{name}-{mode}'
            run(['cc', '-std=gnu99', '-O1', '-g', '-w', *flags, '-o', binary,
                 HERE/source, ROOT/'runtime/core/rt_core.c', ROOT/'runtime/platform-sw/rt_sw.c'],
                ROOT, out/f'{name}-{mode}-build.log')
            with tempfile.TemporaryDirectory(prefix='fft-prerequisites-') as folder:
                base = Path(folder)
                # Preserve the original scene harness's relative directory layout
                # while keeping every save write away from source/proof data.
                for chapter in ['opening', 'campaign']:
                    data = base/chapter/'data';data.mkdir(parents=True)
                    for path in (HERE.parent/chapter/'data').glob('*.bin'):
                        (data/path.name).symlink_to(path)
                # The battle harness loads both arenas from its private cwd.
                data = base/'campaign/data'
                for path in (base/'opening/data').glob('*.bin'):
                    if not (data/path.name).exists():
                        (data/path.name).symlink_to(path.resolve())
                cwd = base/'campaign' if name == 'scene' else data
                log = run(['env', 'ASAN_OPTIONS=detect_leaks=0', 'UBSAN_OPTIONS=halt_on_error=1', binary],
                          cwd, out/f'{name}-{mode}.log')
                logs.append(log)
        if logs[0] != logs[1]:
            raise AssertionError(f'{name}: PC and sanitizer output differ')
        results.append(dict(case=name, pc=True, sanitizers=True, output=logs[0].strip()))
        print(name+': '+logs[0].strip(), flush=True)
    (out/'results.json').write_text(json.dumps(results, indent=2)+'\n')


if __name__ == '__main__':
    main()
