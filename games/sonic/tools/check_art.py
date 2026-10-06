#!/usr/bin/env python3
"""Check extraction against original video and validate both native art banks."""
import json
from pathlib import Path
import struct

import numpy as np

from art import ROOT, GAME, mapping, sonic_patterns, word
from mdrun import MD


def main():
    rom = (ROOT / 'roms/md/Sonic_1.md').read_bytes()
    md = MD(ROOT / 'roms/md/Sonic_1.md')
    try:
        md.load(ROOT / 'sources/sonic1_md/start.state')
        md.lib.retro_run()
        vram, _, regs = md.vdp()
        assert regs[5] == 0x7c and len(vram) == 65536
        # Core normal intensity is 3-bit CRAM expanded to even 4-bit levels,
        # then MAKE_PIXEL quantizes to RGB565 (vdp_render.c), not linear 0..255.
        palette = []
        for i in range(64):
            value = md.read(0xfffb00 + i * 2, 2)
            r, g, b = [((value >> shift) & 7) * 2 for shift in (1, 5, 9)]
            pixel = r << 12 | (r >> 3) << 11 | g << 7 | (g >> 2) << 5 | b << 1 | b >> 3
            palette.append(((pixel >> 11) * 255 // 31, ((pixel >> 5) & 63) * 255 // 63, (pixel & 31) * 255 // 31))
        palette = np.array(palette, dtype=np.int16)
        frame = md.read(0xffd01a)
        indexed = mapping(rom, sonic_patterns(rom, frame), md.read(0xffd004, 4), frame, 0x780)
        x = md.read(0xffd008, 2) - md.read(0xfff700, 2)
        y = md.read(0xffd00c, 2) - md.read(0xfff704, 2)
        actual = np.array(md.image(), dtype=np.int16)[y - 48:y + 48, x - 48:x + 48]
        mask = indexed != 0
        mask[62:] = False  # foreground priority grass can cover the shoes
        differences = np.max(np.abs(actual - palette[indexed]), axis=2)[mask]
        assert len(differences) > 200 and np.max(differences) <= 1, 'Sonic mapping/DPLC must match original video'
        print('Sonic ROM sprite:', len(differences), 'opaque pixels agree with original video (RGB565 tolerance1)')
    finally:
        md.close()
    host = (GAME / 'sonart.bin').read_bytes()
    ti = (GAME / 'sonart.be.bin').read_bytes()
    assert host[:32] == ti[:32] and len(host) == len(ti) < 65518
    _, _, tiles, count, table, offset = struct.unpack_from('>6H', host, 4)
    assert host[32:3360] == ti[32:3360] and max(host[32:3360]) < tiles
    assert struct.unpack_from('=' + str(tiles * 32) + 'H', host, 3360) == struct.unpack_from('>' + str(tiles * 32) + 'H', ti, 3360)
    labels = json.loads((GAME / 'x/art.json').read_text())['labels']
    shifted = word(host, 16)
    assert shifted % 4 == 0 and shifted + 6400 == len(host)
    assert struct.unpack_from('=1600I', host, shifted) == struct.unpack_from('>1600I', ti, shifted)
    for i in range(count):
        w, h, dx, dy, p, _ = struct.unpack_from('>BBbbHH', host, table + i * 8)
        assert host[table + i * 8:table + (i + 1) * 8] == ti[table + i * 8:table + (i + 1) * 8]
        fmt = {8: 'B', 16: 'H', 32: 'I'}[w]
        assert p >= offset and p % (w >> 3) == 0 and p + h * (w >> 3) * 3 <= len(host)
        planes = struct.unpack_from('=' + str(h * 3) + fmt, host, p)
        assert planes == struct.unpack_from('>' + str(h * 3) + fmt, ti, p)
        # No colour bits may leak into transparent padding. For the main
        # actors every colour-bearing pixel has room for its white outline.
        light, dark, mask = planes[:h], planes[h:h * 2], planes[h * 2:]
        if labels[i].startswith(tuple(f'ring_{n}_' for n in range(4))):
            assert (w, h) == (8, 8)
            assert all(d == (m ^ 255) for d, m in zip(dark, mask)), 'burst compositor requires dark == opacity'
            if labels[i].endswith('_right'):
                frame = int(labels[i].split('_')[1])
                for dx in range(25):
                    values = struct.unpack_from('=16I', host, shifted + frame * 1600 + dx * 64)
                    assert values == tuple(v << (24 - dx) for v in (*light, *dark))
        for y, (l, d, m) in enumerate(zip(light, dark, mask)):
            assert not ((l | d) & m)
            if labels[i].startswith(('sonic_', 'moto_', 'buzz_', 'chop_')):
                shape = l | d
                if shape:
                    assert y > 0 and y < h - 1
                    assert not ((shape << 1 | shape >> 1) & m)
                    assert not (shape & (mask[y - 1] | mask[y + 1]))
    print('Art banks: tiles, offsets, native byte order, transparent padding and actor outlines passed')


if __name__ == '__main__':
    main()
