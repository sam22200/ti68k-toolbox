#!/usr/bin/env python3
"""Compose Gariland's armed blows offline: WEP.SPR, WEP1/2.SHP and .SEQ.

usage: weapons.py DISC.cue weapons.h [preview.png]
Writes fftw.bin beside the header (both disc-derived, never committed).
The level strike is TYPE1/2.SEQ 130/131: wind-up, then body frames 87/89/90
and 136/138/139. Its weapon scripts 2/3 supply the three blade frames. The
SHP rotation is a screen rotation (despite its name in fft_decomp), performed
here rather than on the calculator. Two-pixel blades and a white outline
are TI readability adaptations. Wind-up hides the weapon by user choice.
"""
import os
import struct
import sys
import numpy as np
from scipy import ndimage
from PIL import Image
import extract
import units

W, H, FOOT = 48, 38, 30              # offline canvas; each stored frame cropped to 32 columns
WEAPONS = [(1, 1), (0x13, 3)]                 # item id, weapon type: Dagger, Broad Sword


def strike_frames(seq, direction):
    """The level swing's weapon frames up to its held impact pose."""
    off = struct.unpack_from('<i', seq, 4 + 4 * (2 + direction))[0]
    p, frames, striking = 0x406 + off, [], False
    while len(frames) < 3:
        op = seq[p]; p += 1
        if op == 0xff:
            op = seq[p]; p += 1
            assert op == 0xf2, 'unexpected weapon strike opcode %02x' % op
            p += 2                           # secondary effect: omitted on the TI
            striking = True
        else:
            if striking: frames.append(op)
            p += 1                          # wait (2 ticks for each blade frame)
    return frames


def weapon(spr, shp, frame, item, battle):
    """Rotated SHP parts at source resolution, sampled into the unit's canvas."""
    graphic, row = battle[0x943e4 - 0x67000 + item * 2:0x943e4 - 0x67000 + item * 2 + 2]
    pal = np.frombuffer(spr[(graphic >> 4) * 32:(graphic >> 4) * 32 + 32], '<u2').astype(int)
    lum = (pal & 31) * .299 + (pal >> 5 & 31) * .587 + (pal >> 10 & 31) * .114
    grey = np.where(lum <= 7, 3, np.where(lum <= 16, 2, np.where(lum <= 22, 1, 0)))
    packed = np.frombuffer(spr[0x200:0x8200], np.uint8).reshape(256, 128)
    sheet = np.empty((256, 256), np.uint8)
    sheet[:, ::2] = packed & 15; sheet[:, 1::2] = packed >> 4
    o = 0x846 + struct.unpack_from('<I', shp, 0x44 + 4 * frame)[0]
    angle = struct.unpack_from('<h', battle, 0x94508 - 0x67000 + 2 * (shp[o] >> 3))[0]
    a = angle * np.pi / 2048
    co, si = np.cos(a), np.sin(a)
    # Inverse map each target pixel's 3x3 source samples through the screen rotation.
    yy, xx = np.mgrid[:H, :W]
    samples = []
    for dy in (-1/3, 0, 1/3):
        for dx in (-1/3, 0, 1/3):
            x = (xx + .5 + dx - W / 2) / units.SCALE
            y = (yy + .5 + dy - FOOT) / units.SCALE
            rx, ry = co * x + si * y, -si * x + co * y
            canvas = np.zeros((H, W), np.uint8)
            for j in reversed(range((shp[o] & 7) + 1)):
                sx, sy, fl = struct.unpack_from('<bbH', shp, o + 2 + 4 * j)
                w, h = units.SIZES[fl >> 10 & 15]
                tx, ty = np.floor(rx - sx).astype(int), np.floor(ry - sy).astype(int)
                valid = (tx >= 0) & (tx < w) & (ty >= 0) & (ty < h)
                if fl & 0x4000: tx = w - 1 - tx
                if fl & 0x8000: ty = h - 1 - ty
                uvx, uvy = (fl & 31) * 8, (fl >> 5 & 31) * 8 + row * 8
                assert uvx + w <= 256 and uvy + h <= 256
                pix = sheet[uvy + ty.clip(0, h - 1), uvx + tx.clip(0, w - 1)]
                canvas[valid & (pix != 0)] = pix[valid & (pix != 0)]
            samples.append(canvas)
    samples = np.stack(samples)
    opaque = (samples != 0).sum(0) >= 3
    hist = np.stack([((samples != 0) & (grey[samples] == g)).sum(0) for g in range(4)])
    g = hist.argmax(0)
    # A one-pixel source blade becomes a readable two-pixel blade at 0.6 scale.
    thick = ndimage.binary_dilation(opaque, np.array([[0, 0, 0], [0, 1, 1], [0, 1, 0]]))
    out = np.full((H, W), -1)
    out[thick] = 1; out[opaque] = g[opaque]
    ring = ndimage.binary_dilation(thick) & ~thick
    out[ring] = 0
    assert not (out[[0, -1]] >= 0).any() and not (out[:, [0, -1]] >= 0).any(), 'blade clipped'
    return out


def build(cue, header, preview=None):
    disc, files = extract.disc_files(cue)
    read = lambda name: disc.read(*files[name])
    battle, spr = read('BATTLE.BIN'), read('BATTLE/WEP.SPR')
    out, frames, offsets = bytearray(), [], []
    def store(s, inside=False):
        bits = 1 << np.arange(s.shape[1] - 1, -1, -1, dtype=np.uint32)
        planes = [((s >= 0) & ((s & 1) != 0)), ((s >= 0) & ((s & 2) != 0)), s < 0]
        if inside:
            e = ndimage.binary_erosion(s >= 0, iterations=2)
            from art import SHADOW
            for r, row in enumerate(SHADOW.strip('\n').split('\n')):
                for x, c in enumerate(row):
                    y = units.FOOT - 1 + r
                    if c != '.' and s[y, x] < 0: s[y, x] = 2; e[y, x] = True
            planes = [((s >= 0) & ((s & 1) != 0)), ((s >= 0) & ((s & 2) != 0)), s < 0, e]
        rows = [int((p[y] * bits).sum()) for p in planes for y in range(len(s))]
        out.extend(struct.pack('>%d%s' % (len(rows), 'H' if s.shape[1] == 16 else 'I'), *rows))
        frames.append(s.copy())
    # Weapon-specific hands/body: gfx, mirror, front/back, wind + three blade phases.
    for _, name, typ in units.CHARS:
        body, shapes = read('BATTLE/%s.SPR' % name), units.shp_frames(read('BATTLE/%s.SHP' % typ))
        poses = [units.sprite(body, shapes[k]) for k in (86, 87, 89, 90, 135, 136, 138, 139)]
        for s in poses + [s[:, ::-1] for s in poses]: store(s.copy(), inside=True)
    body_bytes = len(out)
    # Blade-only sprites: TYPE1/2, knife/sword, mirror, front/back, three blade phases.
    for wep in ('WEP1', 'WEP2'):
        shp, seq = read('BATTLE/%s.SHP' % wep), read('BATTLE/%s.SEQ' % wep)
        for item, wtype in WEAPONS:
            first = struct.unpack_from('<H', shp, 4 + 2 * wtype)[0]
            poses = [weapon(spr, shp, first + k, item, battle) for back in range(2) for k in strike_frames(seq, back)]
            for mirror in range(2):
                for s in poses:
                    s = s[:, ::-1] if mirror else s
                    left = min(int(np.nonzero((s >= 0).any(0))[0][0]), W - 32)
                    offsets.append(left - W // 2)
                    # Same anchor as the body, two spare rows above and below.
                    store(s[2:, left:left + 32])
    # Exact blade records share storage; keep original per-pose horizontal anchors.
    frame_bytes = 12 * (H - 2)
    blades = [bytes(out[i:i+frame_bytes]) for i in range(body_bytes, len(out), frame_bytes)]
    unique = list(dict.fromkeys(blades)); lookup = {frame:i for i,frame in enumerate(unique)}
    ids = bytes(lookup[frame] for frame in blades)
    assert len(unique) < 256 and all(unique[ids[i]] == frame for i,frame in enumerate(blades))
    packed = [extract.zx0pack.pack(frame) for frame in unique]
    starts = [0]
    for frame in packed: starts.append(starts[-1] + len(frame))
    directory = struct.pack('>%dH' % len(starts), *starts)
    index_off = body_bytes + len(ids)
    blade_off = index_off + len(directory)
    out = out[:body_bytes] + ids + directory + b''.join(packed)
    assert len(out) < 65520
    with open(header, 'w') as f:
        f.write('// Disc-derived armed poses, tools/weapons.py; never committed.\n'
                '#define WEAPON_SH %d\n#define WEAPON_FOOT %d\n#define WEAPON_FB %d\n'
                '#define WEAPON_INDEX %dUL\n#define WEAPON_ZOFF %dUL\n#define WEAPON_OFF %dUL\n'
                '#define WEAPON_UNIQUE %d\n'
                % (H - 2, FOOT - 2, frame_bytes, body_bytes, index_off, blade_off, len(unique)))
        f.write('static const s8 weapon_x[48] = { %s };\n' % ', '.join(map(str, offsets)))
    open(os.path.join(os.path.dirname(header), 'fftw.bin'), 'wb').write(out)
    if preview:
        lcd = np.array([[208, 214, 190], [150, 158, 135], [90, 96, 80], [30, 34, 28]], np.uint8)
        sheet = Image.new('RGB', (16 * 32 * 4, 9 * 36 * 4), 'white')
        for i, s in enumerate(frames):
            im = np.tile(lcd[1], (*s.shape, 1)); im[s >= 0] = lcd[s[s >= 0]]
            tile = Image.fromarray(im).resize((s.shape[1] * 4, s.shape[0] * 4), Image.Resampling.NEAREST)
            sheet.paste(tile, (i % 16 * 32 * 4, i // 16 * 36 * 4))
        sheet.save(preview)
    print('armed poses: %d frames, %d bytes' % (len(frames), len(out)))


def check_original(cue, state):
    """Check the three back-facing level-strike records on the running original.

    The battle state starts with Broad Sword; explicitly change its initialized
    display weapon too for the Dagger trial (changing stats alone leaves it stale).
    """
    import oracle
    disc, files = extract.disc_files(cue)
    read = lambda name: disc.read(*files[name])
    battle, shp, seq = read('BATTLE.BIN'), read('BATTLE/WEP1.SHP'), read('BATTLE/WEP1.SEQ')
    psx = oracle.PSX(cue)
    misc = 0x800b7308 + 6 * 0x440
    for item, typ in WEAPONS:
        psx.load(state)
        psx.write(oracle.UNITS + oracle.UNIT_SIZE * oracle.ATTACKER + 0x1d, item)
        psx.write(misc + 0x13a, item); psx.write(misc + 0x13b, typ)
        oracle.to_target(psx)
        expected = [struct.unpack_from('<H', shp, 4 + 2 * typ)[0] + k for k in strike_frames(seq, 1)]
        seen = set()
        inputs = oracle.taps('CIRCLE:60 CIRCLE:60 CIRCLE:250')
        for key in inputs:
            psx.pressed = key; psx.run()
            frame = psx.read(misc + 0x210, 2)
            if not psx.read(misc + 0x208, 2) or frame not in expected or frame in seen: continue
            phase = expected.index(frame)
            assert psx.read(misc + 0x1e0, 2) == (136, 138, 139)[phase]
            assert psx.read(misc + 0x20c, 2) == 3
            display = psx.read(misc + 0x22c, 4)
            off = 0x846 + struct.unpack_from('<I', shp, 0x44 + 4 * frame)[0]
            angle = struct.unpack_from('<H', battle, 0x94508 - 0x67000 + 2 * (shp[off] >> 3))[0]
            assert psx.read(display + 0x0c, 2) == angle
            sx, sy, attr = struct.unpack_from('<bbH', shp, off + 2)
            w, h = units.SIZES[attr >> 10 & 15]
            row = battle[0x943e4 - 0x67000 + 2 * item + 1] * 8
            actual = tuple(psx.read(display + 0x0e + o, 1, signed=o < 2) for o in range(6))
            assert actual == (sx, sy, w, h, (attr & 31) * 8, (attr >> 5 & 31) * 8 + row), actual
            seen.add(frame)
        assert seen == set(expected), (item, expected, seen)
        print('original item %02x: body, weapon SEQ/SHP, rotation and UV match frames %s' % (item, expected))


if __name__ == '__main__':
    if sys.argv[1] == '--check': check_original(*sys.argv[2:])
    else: build(*sys.argv[1:])
