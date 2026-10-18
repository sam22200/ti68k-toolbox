#!/usr/bin/env python3
"""Build/check the bounded real-calculator camera demonstration, without opening TiEmu."""
import hashlib
import json
from pathlib import Path
import re
import subprocess

HERE = Path(__file__).resolve().parents[1]
ROOT = HERE.parents[1]
OUT = HERE/'x/tiemu-camera'
DATA = OUT/'data'
FLAGS = ['-DFFT_EXPLORATION', '-DFFT_FIXED', '-DFFT_CINEMA', '-DFFT_CAMERA_MOTION', '-DFFT_TURN_HOLD']
BANKS = ['fftu', 'fftw', 'ffti', 'fftmesh', 'fftshot', 'fftlook',
         'fftv0', 'fftv1', 'fftv2', 'fftv3']


def run(args, cwd=HERE):
    return subprocess.check_output([str(a) for a in args], cwd=cwd, text=True)


def main():
    DATA.mkdir(parents=True, exist_ok=True)
    shared = ['../../runtime/core/rt_core.c', '../../runtime/platform-sw/rt_sw.c']
    run(['cc', '-std=gnu99', '-O1', '-g', *FLAGS, '-DFFT_TEXTURE_REVIEW',
         '-DFFT_TEXTURE_PREVIEW', '-o', OUT/'prepare', 'tools/camera_demo_assets.c', *shared])
    run([OUT/'prepare', OUT/'fftlook.bin'])
    for name in BANKS:
        if name == 'fftlook':
            source = OUT/'fftlook.bin'
        elif name.startswith('fftv'):
            source = HERE/'x/isometric-storage/edges'/(name+'.bin')
        else:
            source = HERE/(name+'.bin')
        (DATA/(name+'.bin')).write_bytes(source.read_bytes())
        run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth', '-89', 'dat', name+'.bin', name], DATA)
        variable = DATA/(name+'.89y')
        raw = bytearray(variable.read_bytes()); raw[0x3c+13] = 3
        variable.write_bytes(raw)  # archive directly during the boot transfer
    run(['cc', '-std=gnu99', '-O1', '-g', *FLAGS, '-o', OUT/'demo-pc',
         'tools/camera_tiemu_demo.c', *shared])
    for suffix, extra in [('fftshow', []), ('fftshowc', ['-DRT_CYCLES'])]:
        run([ROOT/'tools/bin/ti-cc', '-o', OUT/suffix, *extra, *FLAGS,
             'tools/camera_tiemu_demo.c', '../../runtime/core/rt_core.c',
             '../../runtime/platform-ti68k/rt_ti.c', '../../lib/unpack68k.s',
             '../../tools/extgraph/lib/tilemap.a'])
    count = 470
    oracle = run([OUT/'demo-pc', count], DATA)
    (OUT/'pc.log').write_text(oracle)
    expected = [line.split()[1].lower() for line in oracle.splitlines()]
    args = [ROOT/'tools/bin/ti-cycles', '--arg', 65, '--frames', count, '--max', 100000,
            '--png', OUT/'headless-ti.png']
    for name in BANKS:
        args += ['--file', DATA/(name+'.89y')]
    log = run([*args, OUT/'fftshowc.89z'], DATA)
    (OUT/'cycles.log').write_text(log)
    actual = re.findall(r'shot \d+ checksum (\w+)', log)
    assert '_main returned' in log
    assert len(actual) == count+1
    assert [v.lower() for v in actual[:-1]] == expected
    final = actual[-1].lower()
    report = dict(scope='Real TI C renderer/combat with compact ISO UI, fixed offline textured inspection endpoints; flat moving geometry.',
                  script='ISO -> wide -> close -> ISO -> validated melee -> cinematic -> ISO',
                  matched_pc_ti_frames=count, final_pc_ti_checksum=final,
                  executable_bytes=(OUT/'fftshow.89z').stat().st_size,
                  assets={n:hashlib.sha256((DATA/(n+'.bin')).read_bytes()).hexdigest() for n in BANKS})
    (OUT/'checked.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
