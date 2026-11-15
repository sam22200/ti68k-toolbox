#!/usr/bin/env python3
"""Wide monster body renderer (FMB1) with two synthetic creatures drawn here.

The creatures are original test shapes (a round one, a wide one with claws), not
game art. The script writes their PNGs and manifest, packs them with
tools/monster_body_bank.py, runs check_monster_body.c on PC and ASan/UBSan, and
compares the idle screens with the PNGs composed independently (mirrors
included), the water crop and the enemy marker. --native adds one 68000 run.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

import numpy as np
from PIL import Image
from scipy import ndimage

from check_flow import HERE, ROOT, data_links, run
sys.path.insert(0, str(HERE.parent/'tools'))
import monster_body_bank as bank  # noqa: E402

FOOT = (80, 68)
TONE = (255, 170, 85, 0)


def ellipse(shape, cx, cy, rx, ry):
    yy, xx = np.mgrid[:shape[0], :shape[1]]
    return ((xx-cx)/rx)**2+((yy-cy)/ry)**2 <= 1


def finish(t):
    """Black edge on the body, as artwork has; -1 stays transparent."""
    body = t >= 0
    edge = body & ~ndimage.binary_erosion(body, structure=bank.CROSS)
    t = t.copy()
    t[edge] = 3
    return t


def blob(front, dy=0, legs=0, spike=0, squash=0, dead=False):
    """Family A: a round creature, one big eye and a tail (asymmetric for mirrors)."""
    t = np.full((40, 44), -1)
    if dead:
        t[ellipse(t.shape, 20, 33, 13, 3)] = 2
        return finish(t)
    t[ellipse(t.shape, 20, 20+dy+squash, 12, 11-squash)] = 1
    for x, off in ((14, legs), (24, -legs)):
        t[29+dy:35+min(off, 0), x:x+3] = 2
    t[24+dy:26+dy, 31:37] = 2                       # tail to the right
    if spike:
        t[19+dy:22+dy, 8-spike:9] = 3                 # a horn thrust to the left
    if front:
        t[16+dy:20+dy, 13:17] = 3                     # big eye
        t[17+dy:19+dy, 23:25] = 3                     # small eye
    return finish(t)


def crab(front, dy=0, legs=0, claw=0, squash=0, dead=False):
    """Family B: a wide creature with two claws and antennae, 44 pixels across."""
    t = np.full((46, 48), -1)
    if dead:
        t[33:38, 6:42] = 2
        return finish(t)
    t[16+dy+squash:34+dy, 9:39] = 1
    t[18+dy:26+dy, 2:9] = 2                          # left claw
    t[20+dy-claw:28+dy-claw, 39:46] = 2              # right claw (raised to attack)
    for x in (12, 20, 28, 34):
        t[34+dy:39+(legs if x % 16 else -legs), x:x+3] = 2
    t[5+dy:16+dy, 15:17] = 3                         # antennae
    t[8+dy:16+dy, 30:32] = 3
    if front:
        t[21+dy:24+dy, 16:20] = 3
        t[21+dy:24+dy, 27:31] = 0
    return finish(t)


FAMILIES = [
    (94, blob, (20, 35), dict(spike='spike')),
    (133, crab, (24, 39), dict(spike='claw')),
]


def fixtures(out):
    manifest = {'families': []}
    for job, draw, foot, names in FAMILIES:
        key = names['spike']
        poses = {
            'idle': ([{}, {'dy': -1}], 6),
            'walk': ([{'legs': 1}, {'legs': -1}], 4),
            'attack': ([{key: 2}, {key: 5}, {key: 7}], 3),
            'hurt': ([{'squash': 2}], 8),
            'dead': ([{'dead': True}], 8),
        }
        anims = {}
        for pose, (frames, wait) in poses.items():
            entry = {'wait': wait, 'foot': list(foot)}
            for facing in ('front', 'back'):
                names_ = []
                for k, args in enumerate(frames):
                    t = draw(facing == 'front', **args)
                    rgba = np.zeros(t.shape+(4,), np.uint8)
                    rgba[..., :3] = np.array(TONE, np.uint8)[np.clip(t, 0, 3)][..., None]
                    rgba[..., 3] = np.where(t >= 0, 255, 0)
                    name = f'{job}-{pose}-{facing}-{k}.png'
                    Image.fromarray(rgba).save(out/name)
                    names_.append(name)
                entry[facing] = names_
            anims[pose] = entry
        manifest['families'].append({'jobs': [job], 'anims': anims})
    path = out/'manifest.json'
    path.write_text(json.dumps(manifest, indent=1))
    return path


def expected_idle(out, job, foot, mirror):
    """The idle frame composed from the PNG directly: white ring, light-grey background."""
    rgba = np.asarray(Image.open(out/f'{job}-idle-front-0.png'))
    body = rgba[..., 3] >= 128
    ring = ndimage.binary_dilation(body, structure=bank.CROSS) & ~body
    screen = np.full((100, 160), 170, np.uint8)
    fx, fy = foot
    for y, x in zip(*np.nonzero(body | ring)):
        sx, sy = FOOT[0]+x-fx, FOOT[1]+y-fy
        if mirror:
            sx = 2*FOOT[0]-1-sx
        if 0 <= sx < 160 and 0 <= sy < 100:
            screen[sy, sx] = rgba[y, x, 0] if body[y, x] else 255
    return screen


def normalize(path):
    tones = {216: 255, 152: 170, 88: 85, 24: 0}
    return np.asarray(Image.open(path).convert('L').point([tones.get(x, x) for x in range(256)]))


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--out', type=Path, default=HERE/'x/monster-body/check')
    ap.add_argument('--native', action='store_true')
    args = ap.parse_args()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    raw, info = bank.pack(bank.load(fixtures(out)))
    (out/'ffmb.bin').write_bytes(raw)
    bank.preview(raw, out/'sheet.png')
    common = [HERE/'check_monster_body.c', ROOT/'runtime/core/rt_core.c']
    logs = []
    for label, flags in [('pc', []), ('san', ['-fsanitize=address,undefined', '-fno-sanitize=shift', '-fno-pie', '-no-pie'])]:
        binary = out/label
        (out/f'{label}-build.log').write_text(run(['cc', '-std=gnu99', '-O1', '-g', '-w', *flags,
              '-o', binary, *common, ROOT/'runtime/platform-sw/rt_sw.c']))
        with tempfile.TemporaryDirectory(prefix='fft-monster-body-') as name:
            data = Path(name)
            data_links(data)
            (data/'ffmb.bin').write_bytes(raw)
            result = subprocess.run(['env', 'ASAN_OPTIONS=detect_leaks=0', 'UBSAN_OPTIONS=halt_on_error=1', str(binary)],
                                    cwd=data, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            (out/f'{label}.log').write_text(result.stdout)
            if result.returncode:
                print(result.stdout)
                result.check_returncode()
            logs.append(result.stdout)
            if label == 'pc':
                for p in data.glob('body-*.png'):
                    (out/p.name).write_bytes(p.read_bytes())
    assert logs[0] == logs[1], 'sanitizer replay differs'
    match = re.search(r'monster-body (\w+) failures 0 checks (\d+)', logs[0])
    assert match, logs[0]
    # Idle screens 0..7: family, direction (front, back, mirrored front, mirrored back).
    for i, (job, _, foot, _) in enumerate(FAMILIES):
        for direction in (0, 2):
            got = normalize(out/f'body-{4*i+direction:02}.png')
            want = expected_idle(out, job, foot, direction == 2)
            if not np.array_equal(got, want):
                Image.fromarray(want).save(out/'expected.png')
                raise AssertionError((job, direction, 'idle pixels differ'))
        back, mirrored = (normalize(out/f'body-{4*i+d:02}.png') for d in (1, 3))
        # The feet at x=80 mirror x to 159-x: a whole-screen flip of the back view.
        assert np.array_equal(back[:, ::-1], mirrored), (job, 'mirrored back view')
    # Water: nothing drawn below the crop line (feet - UNIT_FOOT + WATER_ROWS).
    water = normalize(out/'body-18.png')
    rows = np.nonzero((water != 170).any(1))[0]
    assert rows.size and rows.max() < FOOT[1], 'water crop'
    # The enemy diamond sits above the taller body, not inside it.
    plain, enemy = normalize(out/'body-04.png'), normalize(out/'body-13.png')
    diff = np.nonzero((plain != enemy).any(1))[0]
    top = np.nonzero((plain != 170).any(1))[0].min()
    assert diff.size and diff.max() < top, 'enemy marker overlaps the body'
    record = dict(hash=match[1], checks=int(match[2]), sanitizer_equal=True, screens=19,
                  bank=info, bank_sha256=hashlib.sha256(raw).hexdigest(), native=False)
    print(match[0], flush=True)
    if args.native:
        (out/'native-build.log').write_text(run([ROOT/'tools/bin/ti-cc', '-DRT_CYCLES', '-o', out/'monbody',
                *common, ROOT/'lib/unpack68k.s', ROOT/'tools/extgraph/lib/tilemap.a']))
        with tempfile.TemporaryDirectory(prefix='fft-monster-body-native-') as name:
            data = Path(name)
            (data/'ffmb.bin').write_bytes(raw)
            run([ROOT/'tools/gcc4ti-bin/bin/ttbin2oth', '-89', 'dat', 'ffmb.bin', 'ffmb'], data)
            files = {p.stem: p for folder in (HERE.parent/'opening/data', HERE/'data') for p in folder.glob('*.89y')}
            files['ffmb'] = data/'ffmb.89y'
            command = [ROOT/'tools/bin/ti-cycles', '--arg', 61, '--frames', 0, '--max', 300]
            for p in files.values():
                command += ['--file', p]
            native = run([*command, out/'monbody.89z'])
            (out/'native.log').write_text(native)
        values = [int(v, 16) for v in re.findall(r'value: .*?\(0x([0-9a-f]+)\)', native)]
        expected = [int(v, 16) for v in re.findall(r'body-screen \d+ (\w+)', logs[0])]
        expected += [int(match[1], 16), 0, int(match[2])]
        assert values == [0]+expected, (values, expected)
        record.update(native=True)
        print('One 68000 carrier: same checks and screens', flush=True)
    (out/'results.json').write_text(json.dumps(record, indent=2)+'\n')


if __name__ == '__main__':
    main()
