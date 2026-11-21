#!/usr/bin/env python3
"""The source's own shop and Soldier Office rooms (FSK3), four greys.

usage: shop_scene.py DISC.cue OUT.bin

OBSERVED on the disc: MENU/BK_SHOP.TIM and MENU/BK_HONE.TIM are the rooms the
source draws behind the store and the Soldier Office -- 256x128 16-bit TIM
images, pre-rendered, **with their people already in them** (the shopkeeper
behind his counter at x 130..155, his customer at x 96..118, the office clerk
at his desk and a recruit before him). There is nothing to place: the room is
one picture, the way a battle map is one view. BK_SHOP2/3 and BK_HONE2/3 are
the other towns' rooms, and BK_FITR* the fitting room.

Each record is a 160x100 window of the source taken at 1:1 -- the source's own
pixels read far better in four greys than a scaled-down room -- framed to put
both figures to the right of the menu box, and ZX0-compressed as two planes.
The whole screen is ripped: the dialogue box is drawn over the room, and an
empty band above it looked wrong.

The grey mapping is the one the user chose on 2026-11-20 out of five: the 15th
and 97th luminance percentiles become black and white, through a 1.4 gamma.

OBSERVED: **EVENT/WLDFACE.BIN** is the world-map and menu portrait sheet -- 4bpp,
256 pixels wide, 168 faces of 32x48, eight to a row. These are the faces the
towns' speakers use, not the battle sheets' own. Face 8 is the moustached man
the user's shop screenshot shows; the Soldier Office's clerk is one of the bald
ones beside him and is not pinned down yet (INTERPRETATION: matched by eye
against the user's screenshot, not read from the game's own table).

The sheet's own palettes are the last 2048 bytes of the file: 168 faces of
32x48 take 129,024 bytes, and what follows is **64 CLUTs of 16 BGR555 colours**.
The index is a palette slot, not a brightness -- a face's skin is index 8 and
its hair index 1..3 -- so reading the index as luminance produced a negative.
The greys come from the palette's own luminance, through the same rule as the
rest of the port.

FSK3: magic, u8 rooms, one pad byte, u16 offsets (portrait 32x29/348 bytes, then
each room's u8 height and ZX0-compressed planes), then those records. The pad
keeps the portrait record even: the 68000 reads it as longs.
"""
from pathlib import Path
import struct
import sys

import numpy as np
from PIL import Image

import extract
import interface
import zx0pack

KEEPER = 8                        # EVENT/WLDFACE.BIN face index
KEEPER_PAL = 0                    # INTERPRETATION: the sheet's face/palette pairing is unknown
FACE_W, FACE_H = 32, 48
FACE_BYTES = 168 * FACE_W * FACE_H // 2
W, H = 160, 100                   # the whole screen: the dialogue box is drawn over the room
# name, window origin in the source, and the grey mapping. The store is a bright room full of
# dark goods, the office a dim one, so the percentiles that become white and black differ.
# The user chose the 15/97 percentiles with a 1.4 gamma on 2026-11-20.
VARIANTS = (
    ('BK_SHOP', 8, 18, dict(lo=15, hi=97, gamma=1.4)),
    ('BK_HONE', 8, 23, dict(lo=10, hi=99, gamma=1.4)),
)
BAYER = np.array([[0, 2], [3, 1]]) / 4.0


def face_greys(px, pal):
    """Four greys from the palette's own range. These faces live in a narrow band -- palette 0
    runs 5.3 to 26.2 of 31 -- so the port's fixed thresholds leave almost every pixel mid-grey
    and the portrait unreadable. Stretch the fifteen drawn colours over the full range first."""
    lum = (pal & 31) * .299 + (pal >> 5 & 31) * .587 + (pal >> 10 & 31) * .114
    low, high = lum[1:].min(), lum[1:].max()
    lum = (lum - low) / max(high - low, 1e-6) * 31
    g = np.where(lum <= 7, 3, np.where(lum <= 16, 2, np.where(lum <= 22, 1, 0)))
    return np.where(px == 0, -1, g[px])


def portrait(disc, files, index, palette=KEEPER_PAL):
    """One EVENT/WLDFACE.BIN face as the campaign's 32x29 portrait record (348 bytes)."""
    b = disc.read(*files['EVENT/WLDFACE.BIN'])
    raw = np.frombuffer(b[:FACE_BYTES], np.uint8)
    px = np.empty(raw.size * 2, np.uint8)
    px[0::2] = raw & 15; px[1::2] = raw >> 4
    sheet = px.reshape(-1, 256)
    pal = np.frombuffer(b[FACE_BYTES:], '<u2')[palette * 16:(palette + 1) * 16].astype(int)
    r, c = divmod(index, 8)
    face = sheet[r * FACE_H:(r + 1) * FACE_H, c * FACE_W:(c + 1) * FACE_W]
    small = np.asarray(Image.fromarray(face.astype(np.uint8))
                       .resize((17, 27), Image.Resampling.NEAREST), dtype=int)
    im = np.full((29, 32), -1, dtype=int)
    im[1:28, 1:18] = face_greys(small, pal)           # index 0 is the background
    interface.outline(im)
    return bytes(interface.planes(im, 32))


def room(disc, files, name, x0, y0, lo=2, hi=70, gamma=1.0, dither=False):
    """One MENU/BK_*.TIM window in the runtime's colours (C_WHITE 0 .. C_BLACK 3),
    so a dark source pixel is black."""
    b = disc.read(*files['MENU/%s.TIM' % name])
    _, _, _, w, h = struct.unpack_from('<IHHHH', b, 8)
    px = np.frombuffer(b, '<u2', count=w * h, offset=20).reshape(h, w).astype(int)
    lum = (px & 31) / 31 * .299 + (px >> 5 & 31) / 31 * .587 + (px >> 10 & 31) / 31 * .114
    crop = lum[y0:y0 + H, x0:x0 + W]
    crop = np.pad(crop, ((0, H - crop.shape[0]), (0, W - crop.shape[1])))
    a, b2 = np.percentile(crop, lo), np.percentile(crop, hi)
    n = np.clip((crop - a) / max(b2 - a, 1e-6), 0, 1) ** gamma
    d = BAYER[np.arange(H)[:, None] % 2, np.arange(W)[None, :] % 2] if dither else .5
    return 3 - np.clip(np.floor(n * 3 + d), 0, 3).astype(int)


PBYTES = 30                       # the runtime's plane stride (rt.h RT_PBYTES)


def planes(grey):
    """The two planes the LCD wants, at the runtime's own stride so the room can be
    decompressed straight into an off-screen page: bit 0 of the colour is light, bit 1 dark."""
    out = bytearray()
    for bit in (1, 2):
        for row in grey:
            line = bytearray(PBYTES)
            for x in range(0, W, 8):
                line[x // 8] = int(np.packbits((row[x:x + 8] & bit) != 0)[0])
            out += line
    return bytes(out)


def build(cue):
    disc, files = extract.disc_files(cue)
    records = [portrait(disc, files, KEEPER)]
    for name, x0, y0, how in VARIANTS:
        records.append(bytes((H,)) + zx0pack.pack(planes(room(disc, files, name, x0, y0, **how))))
    # The portrait is drawn straight from the bank by a 32-wide sprite routine, which reads
    # longs: its record must start on an even address, so the header is padded to six bytes.
    at = 6 + 2 * len(records)
    head = b'FSK3' + bytes((len(VARIANTS), 0))
    for record in records:
        head += struct.pack('>H', at)
        at += len(record)
    out = head + b''.join(records)
    assert len(out) == at
    return out, dict(bytes=len(out), keeper=KEEPER, rooms=[len(r) for r in records[1:]])


def main():
    cue, out = sys.argv[1], Path(sys.argv[2])
    raw, info = build(cue)
    out.write_bytes(raw)
    print('%s: %d bytes, rooms %s, shopkeeper %s' %
          (out.name, info['bytes'], info['rooms'], 'WLDFACE %d' % info['keeper']))


if __name__ == '__main__':
    main()
