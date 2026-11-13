#!/usr/bin/env python3
"""Build one chapter-wide FGO1 directory of fixed named encounter objectives.

Usage: objective_bank.py objectives.json ffgo.bin
Input: [{"route":"g", "target":{"slot":1,"name":"Keane"},
         "protect":{"slot":0,"name":"Delita"}}, ...]
Names must match the final encounter identities; runtime rejects mismatches.
Randomly named targets require an authored fixed name before using this format.
"""
import json
from pathlib import Path
import struct
import sys

import banner
from chapter_resources import BlobPool


def validate(raw, routes=None):
    """Same bounded spans as the cold reader; names/pictures stay archive-owned."""
    if len(raw) < 8 or raw[:4] != b'FGO1': raise ValueError('Invalid FGO1 header')
    size, count = struct.unpack_from('>HH', raw, 4)
    head = 8+16*count
    if size != len(raw) or not 1 <= count <= 254 or head > size:
        raise ValueError('Invalid FGO1 directory')
    seen = set()
    for i in range(count):
        at = 8+16*i
        route, zero, target, protect, first, second, w0, w1, tn, pn = struct.unpack_from('>BB4H2B2H', raw, at)
        route = chr(route)
        if route not in 'abdefghijklmnopqrstuvwxyz0123456789' or route in seen or zero or (routes is not None and route not in routes):
            raise ValueError('Invalid FGO1 route')
        seen.add(route)
        if not target | protect or (target | protect) >= 8192 or target & protect or target & (target-1) or protect & (protect-1):
            raise ValueError('Invalid FGO1 objective masks')
        for j, (off, needed) in enumerate(zip((first, second, tn, pn), (True, bool(protect), bool(target), bool(protect)))):
            if not needed:
                if off or (j == 1 and w1): raise ValueError('Unexpected FGO1 field')
                continue
            p = at+off
            if not off or off & 1 or not head <= p < size: raise ValueError('Invalid FGO1 offset')
            if j < 2:
                if p+4 > size: raise ValueError('Truncated FGO1 strip')
                x, top, rows, stride = raw[p:p+4]
                if not 1 <= (w0, w1)[j] <= 152 or x & 15 or top != (3, 25)[j] or not 1 <= rows <= 18 or not 2 <= stride <= 20 or stride & 1 or x+8*stride > 160 or p+4+3*rows*stride > size:
                    raise ValueError('Invalid FGO1 strip')
            else:
                end = raw.find(b'\0', p)
                if not 1 <= end-p <= 31 or any(not 32 <= c <= 126 for c in raw[p:end]):
                    raise ValueError('Invalid FGO1 name')
    return seen


def build(entries):
    if not 1 <= len(entries) <= 254:
        raise ValueError('Expected1..254 objective routes')
    routes = set()
    normalized = []
    for entry in entries:
        route = entry['route']
        if len(route) != 1 or route not in 'abdefghijklmnopqrstuvwxyz0123456789' or route in routes:
            raise ValueError('Invalid/duplicate objective route')
        routes.add(route)
        pair = []
        for key in ('target', 'protect'):
            item = entry.get(key)
            if item is None:
                pair.append((0, '')); continue
            slot, name = item['slot'], item['name'].upper()
            if type(slot) is not int or not 0 <= slot < 13 or not 1 <= len(name) <= 31 or any(not 32 <= ord(c) <= 126 for c in name):
                raise ValueError('Invalid objective identity')
            pair.append((1 << slot, name))
        (target, tn), (protect, pn) = pair
        if not target | protect or target & protect:
            raise ValueError('Empty/overlapping objectives')
        texts = ([f'DEFEAT {tn}!', f'PROTECT {pn}!' if protect else ''] if target else
                 [f'PROTECT {pn}!', 'DEFEAT ALL FOES!'])
        normalized.append((route, target, protect, tn, pn, texts))
    chars = ''.join(sorted(set(banner.CHARS+''.join(''.join(e[-1]) for e in normalized)) - {' '}))
    glyphs = banner._font_outlines(banner.TTF, banner.FONT, chars, banner.CAP, banner.Y0)
    pool = BlobPool(bytes(8+16*len(entries)))
    records = []
    for index, (route, target, protect, tn, pn, texts) in enumerate(normalized):
        lines = []
        for text, top in zip(texts, (4, 26)):
            for cap in (12, 11, 10):
                width = sum((glyphs[c][0] if c != ' ' else banner.SPACE)*cap//banner.CAP+1 for c in text)
                if width <= 152: break
            else: raise ValueError('Goal name requires an explicit large-text layout')
            lines.append((text, cap, top))
        old = banner.LINES
        try:
            banner.LINES = lines + [('VICTORY!', 18, 4), ('DEFEAT', 18, 4)]
            # Only the two new strips are stored; result strips remain once in core.
            packet, widths = banner.strip_bytes(glyphs)
        finally:
            banner.LINES = old
        at = 8+16*index
        offsets = []
        for line in range(2):
            if not texts[line]: offsets.append(0); continue
            p = struct.unpack_from('>H', packet, 2*line)[0]
            _, _, rows, stride = packet[p:p+4]
            offsets.append(pool.add(packet[p:p+4+3*rows*stride])-at)
        names = [pool.add(name.encode()+b'\0')-at if name else 0 for name in (tn, pn)]
        records.append(struct.pack('>BB4H2B2H', ord(route), 0, target, protect, *offsets,
                                   widths[0], widths[1] if texts[1] else 0, *names))
    if len(pool.data) & 1: pool.data.append(0)
    if len(pool.data) > 65512: raise ValueError('Objective bank exceeds one archive variable')
    pool.data[:8] = b'FGO1'+struct.pack('>HH', len(pool.data), len(entries))
    for i, record in enumerate(records): pool.data[8+16*i:24+16*i] = record
    result = bytes(pool.data)
    validate(result)
    return result


if __name__ == '__main__':
    Path(sys.argv[2]).write_bytes(build(json.loads(Path(sys.argv[1]).read_text())))
