#!/usr/bin/env python3
"""PC/sanitizer actual Dead renderer against approved pixels; optional one68000 carrier."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

from check_flow import HERE, ROOT, data_links, run
import monster_ko_bank as bank
import monster_ko_review as review
from PIL import Image


def normalize(path):
    tones = {216: 255, 152: 170, 88: 85, 24: 0}
    return Image.open(path).convert('L').point([tones.get(x, x) for x in range(256)]).convert('RGB')


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out', type=Path, default=HERE/'x/monster-ko/integration')
    ap.add_argument('--native', action='store_true')
    args = ap.parse_args()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    disc, files = review.extract.disc_files(str(review.core_audit.DEFAULT_DISC))
    bd = review.battle.Disc(str(review.core_audit.DEFAULT_DISC))
    raw, info = bank.build(disc, files, bd)
    (out/'ffko.bin').write_bytes(raw)
    common = [HERE/'check_monster_ko.c', ROOT/'runtime/core/rt_core.c']
    logs = []
    approved = Image.open(HERE/'x/monster-ko/source-and-four-greys.png').convert('RGB')
    for label, flags in [('pc', []), ('san', ['-fsanitize=address,undefined', '-fno-sanitize=shift', '-fno-pie', '-no-pie'])]:
        binary = out/label
        (out/f'{label}-build.log').write_text(run(['cc', '-std=gnu99', '-O1', '-g', '-w', *flags,
              '-o', binary, *common, ROOT/'runtime/platform-sw/rt_sw.c']))
        with tempfile.TemporaryDirectory(prefix='fft-monster-ko-') as name:
            data = Path(name)
            data_links(data)
            (data/'ffko.bin').write_bytes(raw)
            result = subprocess.run(['env', 'ASAN_OPTIONS=detect_leaks=0', 'UBSAN_OPTIONS=halt_on_error=1', str(binary)],
                                    cwd=data, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            (out/f'{label}.log').write_text(result.stdout)
            if result.returncode:
                print(result.stdout)
                result.check_returncode()
            logs.append(result.stdout)
            if label == 'pc':
                for i in range(6):
                    for direction in range(4):
                        # Crop the already approved image, not a regenerated art oracle.
                        tile = approved.crop((600+100*(direction & 1), 94+158*i,
                                              696+100*(direction & 1), 214+158*i))
                        tile = tile.resize((32, 40), Image.Resampling.NEAREST)
                        if direction & 2:
                            tile = tile.transpose(Image.Transpose.FLIP_LEFT_RIGHT)
                        expected = Image.new('RGB', (160, 100), (170, 170, 170))
                        expected.paste(tile, (64, 40))
                        image = normalize(data/f'pose-{4*i+direction:02}.png')
                        if image.tobytes() != expected.tobytes():
                            image.save(out/'mismatch.png')
                            expected.save(out/'expected.png')
                            raise AssertionError((i, direction, 'approved pixels differ'))
                for p in data.glob('pose-*.png'):
                    (out/p.name).write_bytes(p.read_bytes())
    assert logs[0] == logs[1], 'sanitizer replay differs'
    match = re.search(r'monster-ko (\w+) failures 0 checks (\d+)', logs[0])
    assert match, logs[0]
    record = dict(hash=match[1], checks=int(match[2]), sanitizer_equal=True, approved_screens=24,
                  bank_bytes=len(raw), bank_sha256=hashlib.sha256(raw).hexdigest(), bank=info,
                  native=False, approved_image_sha256=hashlib.sha256(approved.tobytes()).hexdigest())
    print(match[0], flush=True)
    if args.native:
        (out/'native-build.log').write_text(run([ROOT/'tools/bin/ti-cc', '-DRT_CYCLES', '-o', out/'monko',
                *common, ROOT/'lib/unpack68k.s', ROOT/'tools/extgraph/lib/tilemap.a']))
        with tempfile.TemporaryDirectory(prefix='fft-monster-ko-native-') as name:
            data = Path(name)
            (data/'ffko.bin').write_bytes(raw)
            run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth', '-89', 'dat', 'ffko.bin', 'ffko'], data)
            native_files = {p.stem: p for folder in (HERE.parent/'opening/data', HERE/'data') for p in folder.glob('*.89y')}
            native_files['ffko'] = data/'ffko.89y'
            command = [ROOT/'tools/bin/ti-cycles', '--arg', 61, '--frames', 0, '--max', 300]
            for p in native_files.values():
                command += ['--file', p]
            native = run([*command, out/'monko.89z'])
            (out/'native.log').write_text(native)
        values = [int(v, 16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)', native)]
        expected = [int(v, 16) for v in re.findall(r'ko-screen \d+ (\w+)', logs[0])]
        expected += [int(match[1], 16), 0, int(match[2])]
        assert values == [0]+expected, (values, expected)
        record.update(native=True, native_screens=len(expected)-3)
        print('One68000 carrier: same checks and32 screens', flush=True)
    (out/'results.json').write_text(json.dumps(record, indent=2)+'\n')


if __name__ == '__main__':
    main()
