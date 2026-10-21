#!/usr/bin/env python3
"""Scene sets for the scene player (campaign/scene.c, campaign/ENGINE.md M2).

usage: scenes.py
ffs0 (Orbonne's opening): the title strip, then the chapel with its event-2 beats, all
taken from opening/data/ffost.bin (tools/opening_assets.py). Layout, big-endian:
  "FSC1", the pose table's variable name (6 bytes, NUL padded: logical -> bank frame),
  u16 step count, steps of 8 bytes: kind, arg, u16 a, b, c
    TITLE  arg 0:  a = title strip (18 columns x 42 bytes: light, dark, ink rows)
    STORY  arg = first camera pan: a = picture (ZX0, 6 planes of b bytes per row x 100 rows:
           colour, colour, depth/mask, depth/mask), c = its beats (u16 count, u16 offsets
           from there; each beat as in ffost: event, actors, hold, pan, actors, speaker, text)
    CARD   a = lines (u8 count; per line x, y, colour, text NUL)
Offsets count from the start of the variable.
"""
from pathlib import Path
import re
import struct

HERE = Path(__file__).resolve().parents[1]
TITLE, STORY, CARD = 1, 2, 3


def beats(story):
    """ffost's beats: (event, raw bytes) in order."""
    count = struct.unpack_from('>H', story, 0)[0]
    offs = struct.unpack_from('>%dH' % count, story, 2) + (None,)
    out = []
    for i in range(count):
        a = offs[i]
        n = story[a + 1]
        end = a + 4 + 8 * n
        end = story.index(b'\0', end) + 1
        end = story.index(b'\0', end) + 1
        out.append((story[a], story[a:end]))
    return out


def beat_block(items):
    data = bytearray(2 + 2 * len(items))
    struct.pack_into('>H', data, 0, len(items))
    for i, raw in enumerate(items):
        if len(data) & 1:
            data.append(0)
        struct.pack_into('>H', data, 2 + 2 * i, len(data))
        data += raw
    return bytes(data)


def scene_set(mapping, steps):
    """steps: (kind, arg, [blobs or ints]): blobs are appended, ints kept."""
    head = bytearray(b'FSC1' + mapping.encode('ascii').ljust(6, b'\0'))
    head += struct.pack('>H', len(steps))
    table = len(head)
    head += bytes(8 * len(steps))
    body = bytearray(head)
    for i, (kind, arg, fields) in enumerate(steps):
        values = []
        for f in fields:
            if isinstance(f, int):
                values.append(f)
            else:
                if len(body) & 1:
                    body.append(0)
                values.append(len(body)); body += f
        values += [0] * (3 - len(values))
        struct.pack_into('>BB3H', body, table + 8 * i, kind, arg, *values)
    return bytes(body)


def opening():
    data = HERE / 'opening/data'
    raw = (data / 'ffost.bin').read_bytes()
    header = (data / 'story.h').read_text()
    chapel_bytes = int(re.search(r'#define OPENING_CHAPEL_BYTES (\d+)', header)[1])
    stride = int(re.search(r'#define CHAPEL_STRIDE (\d+)', header)[1])
    chapel_at = struct.unpack_from('>H', raw, 0)[0]
    story = raw[2:chapel_at]
    chapel = raw[chapel_at:chapel_at + chapel_bytes]
    title = raw[chapel_at + chapel_bytes:chapel_at + chapel_bytes + 18 * 42]
    assert len(title) == 18 * 42
    chapel_beats = [b for e, b in beats(story) if e == 2]
    out = scene_set('ffoa', [(TITLE, 0, [title]),
                             (STORY, 48, [chapel, stride, beat_block(chapel_beats)])])
    (data / 'ffs0.bin').write_bytes(out)
    print('ffs0', len(out), 'bytes,', len(chapel_beats), 'chapel beats')


if __name__ == '__main__':
    opening()
