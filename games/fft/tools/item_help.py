#!/usr/bin/env python3
"""FFT's own item descriptions, for the campaign's shared detail panel (FID1).

usage: item_help.py DISC.cue OUT.bin

OBSERVED on the disc: **EVENT/ATCHELP.LZW** is not compressed despite its name.
It opens with 32 little-endian u32 section offsets into the text that follows at
0x80, and each section is a run of 0xFE-terminated strings in FFT's ordinary
text encoding. The sections that matter:

    11   40 strings
    12  160 strings   job descriptions
    13  256 strings   **item descriptions**
    15  512 strings   ability descriptions
    19  188 strings

Within an entry the prose comes first, then a format code in 0xE0..0xEF opens
the stat block the original prints under it ("Attack power 3  Evade 5%", the
list of jobs that may equip it). Those numbers are already in `ffmg`'s item
record, so this bank keeps the prose alone: 253 of the 256 items have one,
11,855 bytes in all.

A code in 0xE0..0xEF takes exactly one argument byte. Reading it as two ate the
following letter and produced "~ttack power"; the whole file decodes cleanly
once that is right, which is also what shows that nothing here is compressed.

FID1: magic, u16 items, u16 offset an item (0: no description, offsets from the
bank's start), then the NUL-terminated strings. Read in place, never unpacked.
"""
from pathlib import Path
import struct
import sys

import extract
import management_data as md

SECTION = 13                      # the item descriptions
ITEMS = 256


def prose(b):
    """One entry's description, without the stat block the port derives itself."""
    out, i = [], 0
    while i < len(b):
        c = b[i]; i += 1
        if 0xE0 <= c <= 0xEF:
            break                 # the stat block opens with a format code
        if c < 10: out.append(chr(48 + c))
        elif c < 36: out.append(chr(55 + c))
        elif c < 62: out.append(chr(c - 36 + 97))
        elif c == 0xFA or c == 0xF8: out.append(' ')
        elif 0xD0 <= c <= 0xDF and i < len(b):
            g = (c & 15) * 208 + b[i]; i += 1
            out.append(md.PAGED.get(g, '?'))
        elif c in md.SIGNS: out.append(md.SIGNS[c])
    return ' '.join(''.join(out).split())


def build(cue):
    disc, files = extract.disc_files(cue)
    raw = disc.read(*files['EVENT/ATCHELP.LZW'])
    offs = struct.unpack_from('<32I', raw, 0)
    seg = raw[128 + offs[SECTION]:128 + offs[SECTION + 1]]
    texts, at = [], 0
    while len(texts) < ITEMS:
        j = seg.find(0xFE, at)
        if j < 0:
            break
        texts.append(prose(seg[at:j])); at = j + 1
    if len(texts) != ITEMS:
        raise ValueError('ATCHELP section %d holds %d entries' % (SECTION, len(texts)))
    body, where = bytearray(), {}
    head = 4 + 2 + 2 * ITEMS
    table = bytearray(2 * ITEMS)
    for i, t in enumerate(texts):
        if not t:
            continue
        if t not in where:
            where[t] = head + len(body)
            body += t.encode('ascii') + b'\0'
        struct.pack_into('>H', table, 2 * i, where[t])
    out = b'FID1' + struct.pack('>H', ITEMS) + bytes(table) + bytes(body)
    return out, dict(bytes=len(out), described=sum(1 for t in texts if t), unique=len(where))


def main():
    cue, out = sys.argv[1], Path(sys.argv[2])
    raw, info = build(cue)
    out.write_bytes(raw)
    print('%s: %d bytes, %d items described, %d distinct' %
          (out.name, info['bytes'], info['described'], info['unique']))


if __name__ == '__main__':
    main()
