#!/usr/bin/env python3
"""Measure depth-1 sprite parts on the running FFT; outputs stay local.

usage: water.py DISC.cue STATE
The state is oracle.py's reproducible Gariland first-turn door. Each canal's
terrain record is injected beneath a visible unit, then probed with all six
human sheets at its floor height. This is a display probe, not a movement oracle.
"""
from pathlib import Path
import struct
import sys
import numpy as np
from PIL import Image, ImageDraw
import extract
import oracle
import units
from psxgpu import frame_log, parse

MISC, STRIDE = 0x800b7308, 0x440
# Six loaded sheets, in units.CHARS order, and their battle unit ids.
SLOTS = ((6, 16), (4, 0), (0, 1), (5, 5), (3, 4), (8, 18))


def shape(shp, frame, wet):
    table = struct.unpack_from('<I', shp)[0] if wet else 8
    off = table + 0x402 + struct.unpack_from('<I', shp, table + 4 * frame)[0]
    parts = []
    for j in range((shp[off] & 7) + 1):
        x, y, attr = struct.unpack_from('<bbH', shp, off + 2 + 4 * j)
        w, h = units.SIZES[attr >> 10 & 15]
        parts.append((x, y, w, h, (attr & 31) * 8, (attr >> 5 & 31) * 8,
                      (shp[off + 1] & 0x61) | ((attr & 0xc000) >> 13)))
    return parts


def compose(spr, parts):
    packed = np.frombuffer(spr[512:512 + 32768], np.uint8).reshape(256, 128)
    sheet = np.empty((256, 256), np.uint8)
    sheet[:, ::2] = packed & 15; sheet[:, 1::2] = packed >> 4
    canvas = np.zeros((64, 64), np.uint8)
    for x, y, w, h, u, v, flags in reversed(parts):
        tex = sheet[v:v + h, u:u + w]
        if flags & 2: tex = tex[:, ::-1]
        if flags & 4: tex = tex[::-1]
        dst = canvas[48 + y:48 + y + h, 32 + x:32 + x + w]
        dst[tex != 0] = tex[tex != 0]
    return canvas


def gpu_uvs(p, m):
    # Display slots are shared scratch space: another unit can overwrite them
    # later in the same frame. Read the packets actually submitted for this CLUT.
    clut, page = p.read(m + 0x10, 2), p.read(m + 0x0e, 2)
    return sorted(tuple(q['uv']) for q in parse(*frame_log(p))
                  if q['kind'] == 'poly' and q['clut'] == clut and q['tpage'] == page)


def expected_uvs(parts):
    return sorted(((u, v), (u + w, v), (u, v + h), (u + w, v + h))
                  for _, _, w, h, u, v, _ in parts)


def place(p, m, battle_id, x, y, h):
    a = oracle.UNITS + battle_id * oracle.UNIT_SIZE
    p.write(a + 0x47, x); p.write(a + 0x48, y)
    p.write(m + 0x7c, x); p.write(m + 0x7d, y)
    for j, val in enumerate((28 * x + 14, -12 * h, 28 * y + 14)):
        p.write(m + 0x18 + 4 * j, val * 4096, 4)
        p.write(m + 0x40 + 2 * j, val, 2)


def main(cue, state):
    disc, files = extract.disc_files(cue)
    read = lambda name: disc.read(*files[name])
    p = oracle.PSX(cue); p.load(state)
    water = [(c % 10, c // 10) for c in range(150)
             if p.read(oracle.TILES + 8 * c + 3) >> 5]
    assert len(water) == 38
    assert all(p.read(oracle.TILES + 8 * (y * 10 + x)) & 63 == 14
               and p.read(oracle.TILES + 8 * (y * 10 + x) + 2) == 0
               and p.read(oracle.TILES + 8 * (y * 10 + x) + 3) == 32 for x, y in water)
    records = {(x, y): bytes(p.ram[p.addr(oracle.TILES + 8 * (y * 10 + x)):
                                 p.addr(oracle.TILES + 8 * (y * 10 + x)) + 8]) for x, y in water}
    checks, rows = 0, []
    for char, (slot, bid) in zip(units.CHARS, SLOTS):
        _, name, typ = char
        shp, spr = read('BATTLE/' + typ + '.SHP'), read('BATTLE/' + name + '.SPR')
        uv_sets = {wet: [expected_uvs(shape(shp, f, wet)) for f in range(9, 19)] for wet in (False, True)}
        # Compare normal/submerged source pixels: poses retain the head position.
        drops, bottoms = [], []
        for frame in range(9, 19):
            dry, wet = (compose(spr, shape(shp, frame, w)) for w in (False, True))
            dr, wr = (np.nonzero(a.any(1))[0] for a in (dry, wet))
            assert dr.min() == wr.min()
            drops.append(int(dr.max() - wr.max())); bottoms.append(int(wr.max() - 48))
        rows.append('%s: 10 idle/walk poses, lower extent removed %d..%d source rows; wet bottom %d..%d'
                    % (name, min(drops), max(drops), min(bottoms), max(bottoms)))
        # Fresh state per cell. Keep the sprite on screen: the original culls
        # off-camera units, so copy this canal record to the visible test tile.
        for x, y in water:
            for wet in (True, False):
                p.load(state); m = MISC + slot * STRIDE
                # The enemy Chemist starts in Critical, whose held frame never
                # advances. Use healthy idle poses for this paired display probe.
                a = oracle.UNITS + bid * oracle.UNIT_SIZE
                p.write(a + 0x28, p.read(a + 0x2a, 2), 2)
                p.write(m + 0x140, p.read(m + 0x140, 4) & ~0x400, 4)
                tile = oracle.TILES + 8 * (11 * 10 + 4)
                for j, value in enumerate(records[x, y]): p.write(tile + j, value)
                place(p, m, bid, 4, 11, 0)
                p.write(m + 0x0c, 7, 2)       # request ordinary idle (animation 6 + 1)
                p.write(tile + 3, 32 if wet else 0)
                # Teleports do not invalidate a pose already expanded into the
                # shared display slot; wait through an ordinary animation change.
                oracle.run(p, 24)
                before = checks
                for f in range(26):
                    frame = p.read(m + 0x1e0, 2)
                    p.lib.retro_tiport_gplog_reset()
                    p.pressed = 0; p.run()
                    assert p.read(m + 0x42, 2, True) == 0
                    actual = gpu_uvs(p, m)
                    if f < 2 or not actual: continue
                    # Submitted packets lag the current animation record. Match
                    # the complete UV set to a source idle/walk pose, not that
                    # record's newer frame number.
                    assert actual in uv_sets[wet], (name, x, y, wet, frame, actual)
                    checks += 1
                assert checks > before, (name, x, y, wet, 'no submitted sprite')
    # One real pad-driven move into the adjacent canal: no coordinate pokes.
    p.load(state)
    sequence = oracle.taps('CIRCLE:60 CIRCLE:60 UP:30 CIRCLE:100')
    trace, immersed = [], False
    m = MISC + 6 * STRIDE
    shp = read('BATTLE/TYPE1.SHP')
    for f, key in enumerate(sequence):
        frame = p.read(m + 0x1e0, 2)
        p.lib.retro_tiport_gplog_reset()
        p.pressed = key; p.run()
        xyz = tuple(p.read(m + 0x40 + 2 * j, 2, True) for j in range(3))
        wet = gpu_uvs(p, m) in [expected_uvs(shape(shp, k, True)) for k in range(9, 19)]
        immersed |= wet
        trace.append('%d %d %d %d %d %d %d' % (f, *xyz, p.read(m + 0x7f), frame, wet))
    assert immersed and (p.read(m + 0x7c), p.read(m + 0x7d)) == (5, 11)
    out = Path('x'); out.mkdir(exist_ok=True)
    (out / 'm27_original_walk.txt').write_text('frame x height z step_phase pose submerged\n' + '\n'.join(trace) + '\n')
    p.image().save(out / 'm27_original_water.png')
    # Source-resolution sheet, dry/wet side by side; no target-art generation.
    sheet = Image.new('RGB', (512, 6 * 160), 'white'); draw = ImageDraw.Draw(sheet)
    for i, (_, name, typ) in enumerate(units.CHARS):
        spr, shp = read('BATTLE/' + name + '.SPR'), read('BATTLE/' + typ + '.SHP')
        pal = np.frombuffer(spr[:32], '<u2').astype(int)
        rgb = np.stack((pal & 31, pal >> 5 & 31, pal >> 10 & 31), 1) * 255 // 31
        for wet in (0, 1):
            cv = compose(spr, shape(shp, 11, wet)); image = rgb[cv].astype(np.uint8)
            image[cv == 0] = (180, 200, 210)
            sheet.paste(Image.fromarray(image).resize((128, 128), Image.Resampling.NEAREST), (wet * 256, i * 160 + 20))
        draw.text((4, i * 160 + 4), name + ' dry / depth 1', fill='black')
    sheet.save(out / 'm27_original_poses.png')
    (out / 'm27_original.txt').write_text('\n'.join(rows) + '\n%d live part checks; natural canal entry passed\n' % checks)
    print('\n'.join(rows))
    print('%d live part checks: 38 canals x 6 sheets x wet/dry, 26-frame windows; natural canal entry passed' % checks)


if __name__ == '__main__': main(*sys.argv[1:])
