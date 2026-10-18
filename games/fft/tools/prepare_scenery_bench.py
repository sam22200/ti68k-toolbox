#!/usr/bin/env python3
"""Compare flat/textured scenery, always with actors. PC preparation, never TI tools."""
import hashlib
import json
from pathlib import Path
import subprocess

from texture_review import HERE, VIEWS

OUT = HERE/'x/scenery-benchmark'
BANKS = ['fftmesh', 'fftu', 'fftw', 'ffti', 'fftv0', 'fftv1', 'fftv2', 'fftv3']


def main():
    data = OUT/'data'; data.mkdir(parents=True, exist_ok=True)
    for name in BANKS:
        path = data/(name+'.bin')
        if not path.exists():
            path.symlink_to(HERE/(name+'.bin'))
    for view, name in enumerate(VIEWS):
        source = HERE/f'x/scenery-contrast/{name}/contrast-plus/contrast.bin'
        (data/f'fftb{view}.bin').write_bytes(source.read_bytes())
    binary = OUT/'capture-pc'
    subprocess.run(['cc', '-std=gnu99', '-O1', '-DFFT_EXPLORATION', '-DFFT_FIXED', '-o', str(binary),
                    'tools/fixed_scenery_bench.c', '../../runtime/core/rt_core.c', '../../runtime/platform-sw/rt_sw.c'], cwd=HERE, check=True)
    hashes = {}
    for case in range(8):
        log = subprocess.check_output([str(binary), str(case), '128'], cwd=data, text=True)
        (OUT/f'pc-{case}.log').write_text(log)
        assert len(log.splitlines()) == 128
        if case < 4:
            reference = json.loads((HERE/f'x/scenery-contrast/{VIEWS[case]}/contrast-plus/metadata.json').read_text())['checksums']
            assert [int(line.split()[1], 16) for line in log.splitlines()[:32]] == reference
        else:
            reference = json.loads((HERE/f'x/scenery-textures/{VIEWS[case & 3]}/flat/metadata.json').read_text())['checksums']
            assert [int(line.split()[1], 16) for line in log.splitlines()[:32]] == reference
        hashes[str(case)] = log.splitlines()
    for view in range(4):
        assert hashes[str(view)] != hashes[str(view+4)]
    manifest = dict(platform='PC preparation only. No TI build or timing measurement.',
                    cases={str(n):dict(view=VIEWS[n & 3], scenery='textured' if n < 4 else 'flat', characters=True) for n in range(8)},
                    frames_per_case=128, prepared_pc_frames=1024,
                    background_sha256={str(i):hashlib.sha256((data/f'fftb{i}.bin').read_bytes()).hexdigest() for i in range(4)},
                    binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
                    scope='Cold geometry+mask preparation and cached textured replay; background texture mapping remains offline.',
                    note='Measure cold frame separately; warm at least 32 frames. Flat/textured variants retain all actor draws and differ only in cached background values. No runtime texture mapper is measured.')
    (OUT/'prepared-pc.json').write_text(json.dumps(manifest, indent=2)+'\n')
    print('Eight flat/textured scenery cases, all with characters; 1024 PC frames. No TI build or FPS claim.')


if __name__ == '__main__':
    main()
