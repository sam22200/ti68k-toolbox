#!/usr/bin/env python3
"""Build and cross-check direct inspection and real native F1/F5 view switches."""
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess

HERE = Path(__file__).resolve().parents[1]
ROOT = HERE.parents[1]
OUT = HERE/'x/camera-cuts-poly'
FLAGS = ['-DFFT_EXPLORATION', '-DFFT_FIXED', '-DFFT_CINEMA', '-DFFT_CAMERA_CUT_DEMO']


def run(args, cwd=HERE):
    return subprocess.check_output([str(a) for a in args], cwd=cwd, text=True)


def main():
    data = OUT/'data'; data.mkdir(parents=True, exist_ok=True)
    source = HERE/'x/tiemu-camera/data'
    assert (source/'fftlook.bin').exists(), 'Prepare make tiemu-camera-demo first.'
    for path in source.iterdir():
        if path.suffix in ('.bin', '.89y'): shutil.copy2(path, data/path.name)
    run(['cc', '-std=gnu99', '-O1', '-g', *FLAGS, '-o', OUT/'oracle',
         'tools/camera_tiemu_demo.c', '../../runtime/core/rt_core.c',
         '../../runtime/platform-sw/rt_sw.c'])
    for name, extra in [('fftcut', []), ('fftcutc', ['-DRT_CYCLES'])]:
        run([ROOT/'tools/bin/ti-cc', '-o', OUT/name, *FLAGS, *extra,
             'tools/camera_tiemu_demo.c', '../../runtime/core/rt_core.c',
             '../../runtime/platform-ti68k/rt_ti.c', '../../lib/unpack68k.s',
             '../../tools/extgraph/lib/tilemap.a'])
    count = 512
    pc = run([OUT/'oracle', count], data); (OUT/'pc.log').write_text(pc)
    args = [ROOT/'tools/bin/ti-cycles', '--arg', 65, '--frames', count,
            '--max', 100000, '--png', OUT/'headless-ti.png']
    for path in sorted(data.glob('*.89y')): args += ['--file', path]
    log = run([*args, OUT/'fftcutc.89z'], data); (OUT/'cycles.log').write_text(log)
    actual = re.findall(r'shot \d+ checksum (\w+)', log)
    expected = [line.split()[1].lower() for line in pc.splitlines()]
    assert '_main returned' in log and len(actual) == count+1
    assert [s.lower() for s in actual[:-1]] == expected
    report = dict(scope='Bounded native camera demonstration, compact ISO UI; no full tactical release.',
        script='F5 twice, F1 twice, direct wide, direct close, direct ISO; paused actors animate.',
        switches=('Three affine-textured native intermediate frames, then the prepared destination.' if '-DFFT_TURN_TEXTURE' in FLAGS else
                  'Three native low-poly intermediate frames, then the textured destination. No moving texture mapper.'),
        matched_frames=count, perspective_motion=False, animated_isometric_turns=True, cinematic_enabled=False,
        binary_bytes=(OUT/'fftcut.89z').stat().st_size,
        binary_sha256=hashlib.sha256((OUT/'fftcut.89z').read_bytes()).hexdigest(),
        source_sha256={n:hashlib.sha256((HERE/n).read_bytes()).hexdigest() for n in
            ['fft.c','perspective.inc','fixed.inc','cinema.inc','tools/camera_tiemu_demo.c']})
    (OUT/'checked.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__': main()
