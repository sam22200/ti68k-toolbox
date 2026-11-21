#!/usr/bin/env python3
"""The source's own shop and Soldier Office rooms (FSK2), four greys.

usage: shop_scene.py DISC.cue OUT.bin

OBSERVED on the disc: MENU/BK_SHOP.TIM and MENU/BK_HONE.TIM are the rooms the
source draws behind the store and the Soldier Office -- 256x128 16-bit TIM
images, pre-rendered, **with their people already in them** (the shopkeeper
leaning on his counter, a customer in front of it). There is nothing to place:
the room is one picture, the way a battle map is one view. BK_SHOP2/3 and
BK_HONE2/3 are the other towns' rooms, and BK_FITR* the fitting room; this bank
ships the first of each.

Each room is cropped to its lit area, scaled to the whole 160x100 screen,
mapped to four greys through the same luminance rule the rest of the port uses,
and ZX0-compressed as two planes.

The speaker's portrait is provisional: the shopkeeper's own event portrait has
not been identified yet (EVENT/EVTFACE.BIN holds 32 of them, 32x48, 4bpp). The
bank carries the 40M townsperson's battle portrait meanwhile, read exactly as
common_portraits.py reads a speaker's.

FSK2: magic, u16 offsets (portrait 32x29/348 bytes, then each room's
ZX0-compressed planes), then those records.
"""
from pathlib import Path
import struct
import sys

import numpy as np
import extract
import common_portraits
import zx0pack

KEEPER = '40M'
# Each room, and where its 160x100 window sits in the 256x128 source. The window is taken at
# 1:1: the source's own pixels read far better in four greys than a scaled-down room, and the
# framing puts the shopkeeper and his counter to the right of the menu box, as the source does.
# name, window origin in the source, and the luminance percentiles that become black and
# white: the store is a bright room with dark goods, the office a dim one.
ROOMS = (('BK_SHOP', 40, 14, 2, 70), ('BK_HONE', 40, 22, 6, 97))
W, H = 160, 100


def room(disc, files, name, x0, y0, lo_pct, hi_pct):
    """One MENU/BK_*.TIM window as a 160x100 picture in the runtime's colours
    (C_WHITE 0, C_LGRAY 1, C_DGRAY 2, C_BLACK 3), so a dark source pixel is black."""
    b = disc.read(*files['MENU/%s.TIM' % name])
    _, _, _, w, h = struct.unpack_from('<IHHHH', b, 8)
    px = np.frombuffer(b, '<u2', count=w * h, offset=20).reshape(h, w).astype(int)
    lum = (px & 31) / 31 * .299 + (px >> 5 & 31) / 31 * .587 + (px >> 10 & 31) / 31 * .114
    crop = lum[y0:y0 + H, x0:x0 + W]
    crop = np.pad(crop, ((0, H - crop.shape[0]), (0, W - crop.shape[1])))
    lo, hi = np.percentile(crop, lo_pct), np.percentile(crop, hi_pct)
    n = np.clip((crop - lo) / max(hi - lo, 1e-6), 0, 1)
    return 3 - np.clip(np.floor(n * 3 + .5), 0, 3).astype(int)


def planes(grey):
    """The two planes the LCD wants: bit 0 of the colour is light, bit 1 is dark."""
    out = bytearray()
    for bit in (1, 2):                                   # C_LGRAY = 1, C_DGRAY = 2, white = 3
        for row in grey:
            for x in range(0, W, 8):
                out.append(int(np.packbits((row[x:x + 8] & bit) != 0)[0]))
    return bytes(out)


def build(cue):
    disc, files = extract.disc_files(cue)
    portrait = common_portraits.portrait(disc, files, KEEPER)
    records = [portrait] + [zx0pack.pack(planes(room(disc, files, *r))) for r in ROOMS]
    at = 4 + 2 * len(records)
    head = b'FSK2'
    for record in records:
        head += struct.pack('>H', at)
        at += len(record)
    out = head + b''.join(records)
    assert len(out) == at
    return out, dict(bytes=len(out), keeper=KEEPER,
                     rooms=[len(r) for r in records[1:]])


def main():
    cue, out = sys.argv[1], Path(sys.argv[2])
    raw, info = build(cue)
    out.write_bytes(raw)
    print('%s: %d bytes, rooms %s, shopkeeper %s' %
          (out.name, info['bytes'], info['rooms'], info['keeper']))


if __name__ == '__main__':
    main()
