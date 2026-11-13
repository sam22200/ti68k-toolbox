#!/usr/bin/env python3
"""Bounded source-pose study; writes private previews, never shipping actor banks.

Resolve each species through retail sheet metadata and execute the original MIPS
SEQ unpacker/player. Only idle/Dead's first displayed frame in both directions
is studied. The four-grey image is a proposal at 0.6 scale, not a runtime build.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

import numpy as np
from PIL import Image, ImageDraw
from scipy import ndimage

import battle
import core_audit
import extract
import units

HERE = Path(__file__).resolve().parents[1]
SAMPLES = [('Behemoth', 147), ('Bomb', 136), ('Goblin', 135),
           ('Skeleton', 139), ('Squid', 138), ('Chocobo', 134)]
SHP = ('TYPE1', 'TYPE2', 'CYOKO', 'MON')
SEQ = ('TYPE1', 'TYPE3', 'CYOKO', 'MON')
ANIMATIONS = (6, 7, 52, 53)  # idle*2 and Dead*2, each front/back


def shape(data, frame):
    """Read one normal SHP frame, retaining all parts, flips and anchor shifts."""
    if len(data) < 0x40a or not 0 <= frame < 208:
        raise ValueError('truncated SHP or invalid primary frame')
    offset = struct.unpack_from('<I', data, 8 + 4 * frame)[0]
    size = struct.unpack_from('<H', data, 0x408)[0]
    start = 0x40a + offset
    end = min(len(data), 0x40a + size)
    if offset == 0xffffffff or start + 2 > end:
        raise ValueError('missing SHP frame')
    count = (data[start] & 7) + 1
    if start + 2 + 4 * count > end:
        raise ValueError('truncated SHP parts')
    parts = []
    for i in range(count):
        x, y, attr = struct.unpack_from('<bbH', data, start + 2 + 4*i)
        w, h = units.SIZES[(attr >> 10) & 15]
        parts.append((x, y, (attr & 31)*8, ((attr >> 5) & 31)*8,
                      w, h, bool(attr & 0x4000), bool(attr & 0x8000)))
    return parts


def compose(spr, parts):
    """Raw indexed pixels, feet at (64,96); clip only transparent dummy parts."""
    if len(spr) < 512 + 128*256:
        raise ValueError('truncated ordinary SPR sheet')
    packed = np.frombuffer(spr, np.uint8, 128*256, 512).reshape(256, 128)
    sheet = np.empty((256, 256), np.uint8)
    sheet[:, ::2], sheet[:, 1::2] = packed & 15, packed >> 4
    canvas = np.zeros((128, 128), np.uint8)
    for x, y, u, v, w, h, fx, fy in reversed(parts):
        if not (0 <= u <= 256-w and 0 <= v <= 256-h):
            raise ValueError('SHP part exceeds ordinary sheet')
        tile = sheet[v:v+h, u:u+w]
        if fx:
            tile = tile[:, ::-1]
        if fy:
            tile = tile[::-1]
        rows, cols = np.nonzero(tile)
        xs, ys = cols + 64 + x, rows + 96 + y
        if np.any((xs < 0) | (xs >= 128) | (ys < 0) | (ys >= 128)):
            raise ValueError('opaque SHP part exceeds study canvas')
        canvas[ys, xs] = tile[rows, cols]
    if not np.any(canvas):
        raise ValueError('empty source pose')
    return canvas


def first_frame(seq, animation):
    """Strict direct-pair reader; fail rather than guess through SEQ opcodes."""
    if len(seq) < 0x406 or not 0 <= animation < 256:
        raise ValueError('truncated SEQ or invalid animation')
    offset = struct.unpack_from('<I', seq, 4 + 4*animation)[0]
    size = struct.unpack_from('<H', seq, 0x404)[0]
    start = 0x406 + offset
    if offset == 0xffffffff or start + 2 > min(len(seq), 0x406 + size):
        raise ValueError('missing SEQ animation')
    frame, wait = seq[start:start+2]
    if frame == 255:
        raise ValueError('first frame requires SEQ control execution')
    return frame, wait


class Original:
    """Execute retail instructions; isolate only the downstream GPU part loader."""
    def __init__(self, disc):
        import unicorn
        import surface_data as s
        from status_state_oracle import bind_function
        self.s = s
        names = {'unpack_seq': 'battle_gfx_unpack_unit_seq_data',
                 'run_seq': 'battle_gfx_run_unit_seq_script',
                 'load_parts': 'battle_gfx_load_trap_and_unit_frame_parts'}
        self.hashes = {key: bind_function(key, name, 'battle', disc)
                       for key, name in names.items()}
        self.o = s.Original(disc)
        self.o.instruction_limit = 100000
        self.frames = []

        def load(uc, address, size, data):
            self.frames.append((uc.reg_read(self.o.reg.UC_MIPS_REG_A2),
                                uc.reg_read(self.o.reg.UC_MIPS_REG_A3)))
            uc.reg_write(self.o.reg.UC_MIPS_REG_PC,
                         uc.reg_read(self.o.reg.UC_MIPS_REG_RA))

        addr = s.FUNCTIONS['load_parts'][0]
        self.o.uc.hook_add(unicorn.UC_HOOK_CODE, load, begin=addr, end=addr)

    def query(self, seq, animation):
        o = self.o
        source, table, payload, unit, state = (0x801c0000, 0x801c4000,
                                               0x801c5000, 0x801d0000, 0x801d1000)
        o.uc.mem_write(source & 0x1fffff, seq)
        o.write(0x800a1c58, payload)  # g_battle_gfx_seq_data_cursor
        o.call('unpack_seq', table, source)
        o.uc.mem_write(unit & 0x1fffff, bytes(0x400))
        record = bytearray(0x30)
        struct.pack_into('<H', record, 4, animation)
        struct.pack_into('<I', record, 0x20, table)
        o.uc.mem_write(state & 0x1fffff, bytes(record))
        self.frames.clear()
        o.call('run_seq', unit, state, 1)
        result = bytes(o.uc.mem_read((state & 0x1fffff)+6, 6))
        pos, frame, wait = struct.unpack('<3H', result)
        if self.frames != [(frame, animation)] or pos != 2:
            raise ValueError('unexpected retail SEQ execution path')
        return frame, wait


def preview(canvas, spr, grey, with_masks=False):
    """Same 0.6 scale/anchor in both columns; 32x40 study envelope, no crop.

    This envelope deliberately does not claim compatibility with the current
    16x26/208-byte runtime cache. Never apply human skin/eye corrections here.
    """
    pal = np.frombuffer(spr, '<u2', 16).astype(int)
    rgb = np.stack((pal & 31, (pal >> 5) & 31, (pal >> 10) & 31), 1)
    rgb = ((rgb*255 + 15)//31).astype(np.uint8)
    lum = rgb @ np.array([.299, .587, .114]) * 31/255
    tones = np.where(lum <= 7, 3, np.where(lum <= 16, 2, np.where(lum <= 22, 1, 0)))
    tones[1] = 3
    yy, xx = np.mgrid[:128, :128]
    x = np.floor((xx+.5-64)*units.SCALE).astype(int)+16
    y = np.floor((yy+.5-96)*units.SCALE).astype(int)+28
    opaque = canvas > 0
    if np.any(opaque & ((x < 1) | (x >= 31) | (y < 1) | (y >= 39))):
        raise ValueError('source pose does not fit study envelope')
    valid = (x >= 0) & (x < 32) & (y >= 0) & (y < 40)
    cell = (y*32+x)[valid]

    def count(mask):
        return np.bincount(cell, weights=mask[valid], minlength=1280).reshape(40, 32)

    total, body = count(np.ones_like(opaque)), count(opaque)
    mask = (body*2 >= total) & (total > 0)
    pixels = np.full((40, 32, 3), 170, np.uint8)
    if grey:
        hist = np.stack([count(opaque & (canvas != 1) & (tones[canvas] == g))
                         for g in range(4)], 2)
        color = np.where(hist.sum(2) > 0, hist.argmax(2), 3)
        pixels[mask] = np.repeat((255-color[mask]*85)[:, None], 3, 1)
        cross = np.array([[0, 1, 0], [1, 1, 1], [0, 1, 0]])
        edge = mask & ~ndimage.binary_erosion(mask, structure=cross)
        ring = ndimage.binary_dilation(mask, structure=cross) & ~mask
        pixels[edge], pixels[ring] = 0, 255
    else:
        # Identical sample footprint; average only source opaque RGB pixels.
        for c in range(3):
            channel = count(rgb[canvas, c]*opaque)/body.clip(1)
            pixels[:, :, c][mask] = channel[mask].round().astype(np.uint8)
    image = Image.fromarray(pixels)
    if with_masks:
        if not grey:
            raise ValueError('only four-grey previews have native masks')
        return image, mask | ring, mask
    return image


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--disc', type=Path, default=core_audit.DEFAULT_DISC)
    ap.add_argument('--out', type=Path, default=HERE/'campaign/x/monster-ko')
    args = ap.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    disc, files = extract.disc_files(str(args.disc))
    bd = battle.Disc(str(args.disc))
    original = Original(bd)
    image = Image.new('RGB', (800, 72 + len(SAMPLES)*158), 'white')
    draw = ImageDraw.Draw(image)
    draw.text((8, 6), 'SOURCE POSES / 0.6 SCALE STUDY - NOT INTEGRATED', fill='black')
    draw.text((8, 24), 'Each pair: front / back. Same scale and feet anchor; 3x nearest display.', fill='black')
    for col, label in enumerate(('Standing / source RGB', 'KO / source RGB',
                                 'Standing / four greys', 'KO / four greys')):
        draw.text((col*200+8, 47), label, fill='black')
    rows, hashes, oracle = [], {}, {}
    for row, (name, sheet_id) in enumerate(SAMPLES):
        at = 0x80094748-battle.BAT+4*sheet_id
        shp, seq_id, _, _ = bd.bat[at:at+4]
        lba, size = struct.unpack_from('<II', bd.bat, 0x80094cd4-battle.BAT+8*sheet_id)
        path = next(k for k, extent in files.items() if extent[0] == lba)
        if ((files[path][1]+2047)//2048)*2048 != size:
            raise ValueError('sheet extent disagrees with retail table')
        paths = (path, f'BATTLE/{SHP[shp]}.SHP', f'BATTLE/{SEQ[seq_id]}.SEQ')
        spr, shapes, seq = [disc.read(*files[p]) for p in paths]
        for p, data in zip(paths, (spr, shapes, seq)):
            hashes[p] = hashlib.sha256(data).hexdigest()
        samples = []
        for animation in ANIMATIONS:
            expected = first_frame(seq, animation)
            key = (seq_id, animation)
            if key not in oracle:
                oracle[key] = original.query(seq, animation)
            if expected != oracle[key]:
                raise ValueError('source parser differs from original MIPS')
            frame, wait = expected
            canvas = compose(spr, shape(shapes, frame))
            yy, xx = np.nonzero(canvas)
            samples.append(dict(animation=animation, frame=frame, wait=wait,
                                bounds=[int(xx.min()-64), int(yy.min()-96),
                                        int(xx.max()-64), int(yy.max()-96)],
                                opaque_pixels=int(len(xx))))
            pose = int(animation >= 52)
            direction = animation & 1
            for grey in (False, True):
                col = pose + 2*grey
                tile = preview(canvas, spr, grey)
                image.paste(tile.resize((96, 120), Image.Resampling.NEAREST),
                            (200*col + direction*100, 94 + row*158))
        draw.text((8, 76 + row*158), name, fill='black')
        rows.append(dict(name=name, sheet_id=sheet_id, files=paths, poses=samples))
    image.save(args.out/'source-and-four-greys.png')
    report = dict(scope='Source first idle/Dead frames, six sheets/four SEQ families; '
                       'GPU part loading isolated. Preview only; no runtime, bank, ABI or TI change.',
                  mips_cases=len(oracle), source_sha256=hashes,
                  battle_sha256=hashlib.sha256(bd.bat).hexdigest(),
                  functions_sha256=original.hashes, rows=rows,
                  approval='pending', study_envelope=[32, 40], runtime_envelope=[16, 26])
    (args.out/'results.json').write_text(json.dumps(report, indent=2)+'\n')
    print(f'{len(oracle)} original-MIPS frame/wait pairs; {len(rows)} sheets; preview pending review')


if __name__ == '__main__':
    main()
