#!/usr/bin/env python3
"""Compare native low-poly turns with/without live height contours; no emulator."""
import csv
import hashlib
import json
import re
from pathlib import Path
from statistics import mean

from PIL import Image, ImageDraw
import turn_audit as audit

HERE = audit.HERE
OUT = HERE / 'x/turn-edges'
FLAGS = audit.FLAGS + ['-DFFT_CONFIG', '-DFFT_CONFIG_INTERACTIVE', '-DFFT_TURN_LOWPOLY_NATIVE']
COMMON = ['../../runtime/core/rt_core.c', '../../lib/unpack68k.s', '../../tools/extgraph/lib/tilemap.a']


def run_variant(name, extra):
    folder = OUT / name
    frames = folder / 'frames'
    frames.mkdir(parents=True, exist_ok=True)
    data = HERE / 'x/config/data'
    flags = FLAGS + extra
    audit.run(['cc', '-std=gnu99', '-O1', '-g', *flags, '-o', folder/'oracle',
               'tools/turn_audit_cycles.c', '../../runtime/core/rt_core.c', '../../runtime/platform-sw/rt_sw.c'])
    pc = audit.run([folder/'oracle', frames], data)
    (folder/'pc.csv').write_text(pc)
    audit.run([folder/'oracle', folder/'source.png', '--source'], data)
    audit.run([audit.ROOT/'tools/bin/ti-cc', '-o', folder/'turnedge', *flags,
               '-DRT_CYCLES', 'tools/turn_audit_cycles.c', *COMMON])
    args = [audit.ROOT/'tools/bin/ti-cycles', '--max', 1000000]
    banks = sorted(data.glob('*.89y'))
    assert len(banks) == 9, 'Build current camera-config banks first.'
    for bank in banks:
        args += ['--file', bank]
    log = audit.run([*args, folder/'turnedge.89z'], data)
    (folder/'cycles.log').write_text(log)
    expected = list(csv.reader(pc.splitlines()))
    actual = re.findall(r'shot \d+ checksum (\w+)', log)
    assert [s.lower() for s in actual] == [r[2].lower() for r in expected] and len(actual) == 32
    assert '_main returned' in log
    values = [int(v, 16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)', log)]
    samples, case, ok = [[] for _ in range(8)], None, 0
    for i, value in enumerate(values):
        if value >> 24 == 0x71:
            case = value & 255
        elif value >> 24 == 0x72:
            samples[case].append(values[i+1])
        elif value >> 24 == 0x73:
            assert value & 255
            ok += 1
    assert ok == 8
    audit.run([audit.ROOT/'tools/bin/ti-cc', '-o', folder/'fftcfg', *flags,
               '-DRT_GRAY_COPY_PRESENT', 'tools/camera_tiemu_demo.c',
               '../../runtime/platform-ti68k/rt_ti.c', *COMMON])
    # Same production renderer at corner/street/roof pivots and anchor rounding phases.
    audit.run(['cc', '-std=gnu99', '-O1', '-g', '-fsanitize=address,undefined',
               '-fno-sanitize=shift', *flags, '-o', folder/'broad', 'tools/turn_surface_check.c',
               '../../runtime/core/rt_core.c', '../../runtime/platform-sw/rt_sw.c'])
    broad = audit.run([folder/'broad'], data)
    audit.run([audit.ROOT/'tools/bin/ti-cc', '-o', folder/'broadti', *flags,
               '-DRT_CYCLES', 'tools/turn_surface_check.c', *COMMON])
    broadlog = audit.run([*args, folder/'broadti.89z'], data)
    (folder/'broad-cycles.log').write_text(broadlog)
    assert [s.lower() for s in re.findall(r'shot \d+ checksum (\w+)', broadlog)] == [
        r[1].lower() for r in csv.reader(broad.splitlines())]
    assert len(broad.splitlines()) == 576 and '_main returned' in broadlog
    return dict(work_bytes=values[0], matched_turn_frames=32, matched_broad_frames=576,
                binary_bytes=(folder/'fftcfg.89z').stat().st_size,
                binary_sha256=hashlib.sha256((folder/'fftcfg.89z').read_bytes()).hexdigest(),
                cases=[dict(case=i, cycles=s, total_cycles=sum(s), moving_cycles=sum(s[:3]))
                       for i, s in enumerate(samples)])


def review():
    import numpy as np
    # Endpoints must stay identical; only the three moving pictures may change.
    counts = []
    for case in range(8):
        for frame in range(1, 5):
            a = np.array(Image.open(OUT/f'plain/frames/{case}-{frame}.png').convert('L'))
            b = np.array(Image.open(OUT/f'edges/frames/{case}-{frame}.png').convert('L'))
            changed = a != b
            if frame == 4:
                assert not changed.any()
            else:
                assert changed.any()
                counts.append(int(changed.sum()))
                # SW screenshots simulate LCD tones (24..216), not RGB black/white.
                assert set(np.unique(b[changed])) <= {min(a.min(), b.min()), max(a.max(), b.max())}
    frames, durations = [], []
    # Right from view 0 to 1, then left from view 1 to 0, with identical actors/pivots.
    for case in (0, 3):
        seq = [0, 1, 2, 3, 4] if case == 0 else [1, 2, 3, 4]
        for frame in seq:
            sheet = Image.new('RGB', (640, 224), 'white')
            d = ImageDraw.Draw(sheet)
            for col, (name, label) in enumerate((('plain', 'Before'), ('edges', 'Height edges'))):
                d.text((col*320+5, 4), label + (' / F5' if case == 0 else ' / F1'), fill='black')
                path = OUT/f'{name}/frames/{case}-{frame}.png' if frame else OUT/f'{name}/source.png'
                im = Image.open(path).convert('RGB')
                sheet.paste(im.resize((320, 200), Image.Resampling.NEAREST), (col*320, 24))
            frames.append(sheet)
            durations.append(1100 if frame in (0, 4) else 400)
    frames[2].save(OUT/'comparison.png')
    frames[0].save(OUT/'comparison.gif', save_all=True, append_images=frames[1:],
                   duration=durations, loop=0, disposal=2)
    return dict(changed_pixels_per_moving_frame=counts,
                gif='PC framebuffer comparison; fixed review cadence, not measured FPS.')


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    results = {}
    # GCC4TI shares intermediate paths: all native builds are deliberately sequential.
    for name, flags in [('plain', ['-DFFT_TURN_NO_EDGES']), ('edges', [])]:
        results[name] = run_variant(name, flags)
    a, b = results['plain'], results['edges']
    assert a['work_bytes'] == b['work_bytes']
    result = dict(scope='160x100 native low-poly terrain; three intermediate angles, identical ISO endpoints.',
                  timing='MC68000 datasheet CPU cycles at 12 MHz; no TiEmu, interrupts or presentation waits.',
                  variants=results, review=review(),
                  mean_turn_ms={k: mean(c['total_cycles'] for c in v['cases'])/12000
                                for k, v in results.items()},
                  mean_extra_turn_ms=mean(y['total_cycles']-x['total_cycles']
                                         for x, y in zip(a['cases'], b['cases']))/12000,
                  added_program_bytes=b['binary_bytes']-a['binary_bytes'], added_work_bytes=0,
                  added_asset_bytes=0,
                  source_sha256=hashlib.sha256((HERE/'fft.c').read_bytes()).hexdigest())
    (OUT/'checked.json').write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps({k: v for k, v in result.items() if k != 'variants'}, indent=2))


if __name__ == '__main__':
    main()
