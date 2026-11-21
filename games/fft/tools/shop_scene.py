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

Only the rows the screen actually shows are ripped: the dialogue box covers the
top 34, so each record is a 160x66 window taken at 1:1 (the source's own pixels
read far better in four greys than a scaled-down room), framed to put both
figures to the right of the menu box, and ZX0-compressed as two planes.

Five grey mappings of the store are shipped for the user's choice; the office
follows whichever wins.

The speaker's portrait is provisional: the shopkeeper's own event portrait has
not been identified yet (EVENT/EVTFACE.BIN holds 32 of them, 32x48, 4bpp). The
bank carries the 40M townsperson's battle portrait meanwhile, read exactly as
common_portraits.py reads a speaker's.

FSK3: magic, u8 rooms, u16 offsets (portrait 32x29/348 bytes, then each room's
u8 height and ZX0-compressed planes), then those records.
"""
from pathlib import Path
import struct
import sys

import numpy as np

import extract
import common_portraits
import zx0pack

KEEPER = '40M'
W, H = 160, 66                    # the window the dialogue box does not cover
# name, window origin in the source, and the grey mapping. The store is a bright room full of
# dark goods, the office a dim one, so the percentiles that become white and black differ.
VARIANTS = (
    ('BK_SHOP', 8, 52, dict(lo=10, hi=95)),
    ('BK_SHOP', 8, 52, dict(lo=20, hi=99)),
    ('BK_SHOP', 8, 52, dict(lo=30, hi=99.5)),
    ('BK_SHOP', 8, 52, dict(lo=20, hi=99, dither=True)),
    ('BK_SHOP', 8, 52, dict(lo=15, hi=97, gamma=1.4)),
    ('BK_HONE', 8, 46, dict(lo=20, hi=99)),
)
BAYER = np.array([[0, 2], [3, 1]]) / 4.0


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


def planes(grey):
    """The two planes the LCD wants: bit 0 of the colour is light, bit 1 is dark."""
    out = bytearray()
    for bit in (1, 2):
        for row in grey:
            for x in range(0, W, 8):
                out.append(int(np.packbits((row[x:x + 8] & bit) != 0)[0]))
    return bytes(out)


def build(cue):
    disc, files = extract.disc_files(cue)
    records = [common_portraits.portrait(disc, files, KEEPER)]
    for name, x0, y0, how in VARIANTS:
        records.append(bytes((H,)) + zx0pack.pack(planes(room(disc, files, name, x0, y0, **how))))
    at = 5 + 2 * len(records)
    head = b'FSK3' + bytes((len(VARIANTS),))
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
          (out.name, info['bytes'], info['rooms'], info['keeper']))


if __name__ == '__main__':
    main()
