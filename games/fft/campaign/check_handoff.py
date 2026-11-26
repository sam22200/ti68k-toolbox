#!/usr/bin/env python3
"""Chapter handover: the real fp_story against the installed Chapter I and II packs.

Stages each pack's variables the way the calculator holds them, then runs
check_handoff.c on both. No emulator, no window, no save file.
"""
import argparse
import json
from pathlib import Path
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent; ROOT = HERE.parents[2]
sys.path.insert(0, str(HERE.parent / 'tools'))
import chapter_packages as packages


def stage(folder, index, into):
    manifest = json.loads((folder / 'packages.json').read_text()); source = {}
    for row in [manifest['core'], manifest['chapters'][index]]:
        source.update(packages.read_group(folder / row['file']))
    for v in source.values():
        (into / (v.name + '.payload')).write_bytes(v.payload)
    return manifest


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--packages', type=Path, default=HERE / 'x/build-packages2')
    ap.add_argument('--out', type=Path, default=HERE / 'x/handoff')
    args = ap.parse_args(); out = args.out.resolve(); out.mkdir(parents=True, exist_ok=True)
    exe = out / 'check-handoff'
    subprocess.run(['cc', '-std=gnu99', '-O1', '-g', '-Wall', '-Wextra', '-fsanitize=address,undefined',
                    '-fno-omit-frame-pointer', '-no-pie', '-o', str(exe), str(HERE / 'check_handoff.c')],
                   cwd=ROOT, check=True)
    with tempfile.TemporaryDirectory(prefix='fft-handoff-') as temp:
        one, two = Path(temp) / 'one', Path(temp) / 'two'
        one.mkdir(); two.mkdir()
        folder = args.packages.resolve()
        manifest = stage(folder, 0, one); stage(folder, 1, two)
        chapters = [c['id'] for c in manifest['chapters']]
        if chapters != [1, 2]:
            raise SystemExit(f'expected the Chapter I and II packs, found {chapters}')
        p = subprocess.run([str(exe), str(one), str(two)], text=True,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        (out / 'handoff.log').write_text(p.stdout); print(p.stdout.strip())
        p.check_returncode()
    (out / 'results.json').write_text(json.dumps(dict(
        core_key=manifest['core_key'], chapters=chapters,
        checked='prerequisite refusal, handover announced once, an uninstalled destination '
                'returns to the map, a completed chapter keeps its marker'), indent=2) + '\n')


if __name__ == '__main__':
    main()
