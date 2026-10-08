#!/usr/bin/env python3
"""Move the already validated sprites to an in-place archived data bank.

Metadata is big endian; sprite words are native endian on each target. No
resampling or palette change: every row is checked against the original header.
"""
from pathlib import Path
import re
import struct

GAME = Path(__file__).resolve().parents[1]


def main():
    source = (GAME / 'generated/art.h').read_text()
    planes = {}
    for bits, name, count, body in re.findall(r'static const u(\d+) (art_\d+_\w+)\[(\d+)\] = \{([^}]+)\};', source):
        words = [int(x.replace('UL', ''), 0) for x in body.split(',')]
        assert len(words) == int(count)
        planes[name] = (int(bits), words)
    records = re.findall(r'\{\{(\d+),(\d+),(art_\d+)_light,\3_dark,\3_mask\},(-?\d+),(-?\d+)\}', source)
    assert len(records) == 139
    banks = []
    for endian in ('=', '>'):
        bank = bytearray(b'WJA1' + struct.pack('>HH', len(records), 0))
        bank.extend(bytes(len(records) * 8))
        for i, (w, h, name, x, y) in enumerate(records):
            w, h = int(w), int(h)
            while len(bank) & 3:
                bank.append(0)
            offset = len(bank)
            struct.pack_into('>BBbbHH', bank, 8 + 8 * i, w, h, int(x), int(y), offset, 0)
            for kind in ('light', 'dark', 'mask'):
                bits, words = planes[name + '_' + kind]
                assert bits == w and len(words) == h
                packed = struct.pack(endian + ('H' if w == 16 else 'I') * h, *words)
                assert struct.unpack(endian + ('H' if w == 16 else 'I') * h, packed) == tuple(words)
                bank.extend(packed)
        assert len(bank) < 65500
        struct.pack_into('>H', bank, 6, len(bank))
        banks.append(bank)
    (GAME / 'wjart.bin').write_bytes(banks[0])
    (GAME / 'wjart.be.bin').write_bytes(banks[1])
    start = source.index('static const u8 art_div5')
    (GAME / 'generated/art_bank.h').write_text('/* Sprite words are in wjart, read in place. */\n#define WJ_ART_COUNT 139\n' + source[start:])
    print(f'Archived art: {len(records)} identical sprites, {len(banks[0])} bytes per bank')


if __name__ == '__main__':
    main()
