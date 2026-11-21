#!/usr/bin/env python3
"""The source's own shop and Soldier Office rooms (FSK3), four greys.

usage: shop_scene.py DISC.cue OUT.bin

OBSERVED on the disc: the whole game holds **nine** 256x128 16-bit TIM rooms,
and no others -- `MENU/BK_SHOP`, `BK_HONE`, `BK_FITR`, each with a 2 and a 3 for
the other towns' styles. They are pre-rendered, **with their people already in
them**, so there is nothing to place: a room is one picture, the way a battle
map is one view.

Which is which, read off the pictures against the user's own screenshots:

- **BK_SHOP** the store: the shopkeeper behind his counter at x 130..155, his
  customer at x 96..118.
- **BK_HONE** (hone: bone) the **Fur Shop**, hides and a skeleton hanging over
  the room. It is not the Soldier Office, which is what this bank shipped by
  mistake until 2026-11-21.
- **BK_FITR** the **Soldier Office**: a stone room of books and candles, the
  guildmaster seated at x 160..180 and a recruit before him at x 100..122.

The Bar has no picture here at all; neither does anything else.

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

FSK4: magic, u8 rooms, one pad byte, then a u16 offset for each room's speaker
portrait (32x29, 348 bytes) and one for each room's picture (u8 height, then
ZX0-compressed planes). The pad keeps the portraits even: the 68000 reads them
as longs.
"""
from pathlib import Path
import struct
import sys

import numpy as np
from PIL import Image

import extract
import interface
import zx0pack

KEEPER_PAL = 0                    # INTERPRETATION: the sheet's face/palette pairing is unknown
FACE_W, FACE_H = 32, 48
FACE_BYTES = 168 * FACE_W * FACE_H // 2
W, H = 160, 100                   # the whole screen: the dialogue box is drawn over the room
# name, window origin in the source, and the grey mapping. The store is a bright room full of
# dark goods, the office a dim one, so the percentiles that become white and black differ.
# The user chose the store's 15/97 percentiles with a 1.4 gamma on 2026-11-20.
BAYER = np.array([[0, 2], [3, 1]]) / 4.0
# ADAPTED, not OBSERVED: in the source office Ramza stands sixty pixels from the guildmaster's
# desk, and at 160x100 a window cannot hold both once the menu box takes the left half. He is
# cut out of his own floor and set down beside the desk, sixteen pixels right and twenty-four
# up -- one step along the room's isometric floor. Nothing else in the picture moves.
RAMZA_TO_DESK = dict(box=(98, 70, 124, 110), to=(22, -24), floor=-24)
GUILDMASTER = dict(box=(160, 40, 184, 82), sat=5, val=99)   # behind his desk, in the glare
# The Fur Shop's two stand close enough together to need no moving: Ramza at x 105..125,
# y 78..115 and the bald merchant at x 140..165, y 60..95, both against dark hides.
FUR_RAMZA = dict(box=(103, 76, 127, 117), sat=4, val=22)
FUR_KEEPER = dict(box=(138, 58, 167, 97), sat=4, val=22)


def actor_mask(sub, sat=4, val=22):
    """The figure inside a box: the room's stone and its light are grey, a person is not.
    `sat` is how colourful a pixel must be to count; the guildmaster needs more of it than
    Ramza, the papers piled beside him being nearly as warm as his robe."""
    from scipy import ndimage
    mask = (sub.max(2) - sub.min(2) >= sat) | (sub.max(2) >= val)
    mask = ndimage.binary_opening(ndimage.binary_closing(mask, np.ones((3, 3))), np.ones((2, 2)))
    label, n = ndimage.label(mask)
    if n:
        sizes = ndimage.sum(mask, label, range(1, n + 1))
        mask = label == int(np.argmax(sizes)) + 1
    return mask


def draw_outline(rgb, box, mask):
    """A one-pixel black contour around the figure. Both of the office's people stand against
    the window's glare, clear on clear; no global grey mapping separates them, and the port's
    own visibility rule asks for a contour. Here it is black, the background being the light."""
    from scipy import ndimage
    x0, y0, x1, y1 = box
    ring = ndimage.binary_dilation(mask, np.ones((3, 3))) & ~mask
    rgb[y0:y1, x0:x1][ring] = 0


def move_actor(rgb, box, to, floor):
    """Lift the figure inside `box` off its floor, set it down at `to` (dx, dy), outline it."""
    x0, y0, x1, y1 = box
    sub = rgb[y0:y1, x0:x1]
    mask = actor_mask(sub)
    actor = sub.copy()
    rgb[y0:y1, x0:x1][mask] = rgb[y0:y1, x0 + floor:x1 + floor][mask]   # the floor he stood on
    dx, dy = to
    there = (x0 + dx, y0 + dy, x1 + dx, y1 + dy)
    rgb[there[1]:there[3], there[0]:there[2]][mask] = actor[mask]
    draw_outline(rgb, there, mask)
    return rgb


def outline_actor(rgb, box, sat=4, val=22):
    draw_outline(rgb, box, actor_mask(rgb[box[1]:box[3], box[0]:box[2]], sat, val))
    return rgb


# name, window origin, the speaker's EVENT/WLDFACE.BIN face, and the grey mapping.
VARIANTS = (
    ('BK_SHOP', 8, 18, 8, dict(lo=15, hi=97, gamma=1.4)),
    # The office's stone and its near-white candles are too far apart for four flat greys: the
    # user chose the dithered mapping on 2026-11-21, framed so Ramza clears the menu box.
    ('BK_FITR', 26, 8, 8, dict(lo=25, hi=85, dither=0.62, move=RAMZA_TO_DESK,
                               outline=(GUILDMASTER,))),
    ('BK_HONE', 8, 26, 9, dict(lo=45, hi=95, dither=0.72,
                               outline=(FUR_RAMZA, FUR_KEEPER))),
)


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



# name, window origin, the speaker's EVENT/WLDFACE.BIN face, and the grey mapping.
def room(disc, files, name, x0, y0, lo=2, hi=70, gamma=1.0, dither=0, move=None, outline=()):
    """One MENU/BK_*.TIM window in the runtime's colours (C_WHITE 0 .. C_BLACK 3),
    so a dark source pixel is black."""
    b = disc.read(*files['MENU/%s.TIM' % name])
    _, _, _, w, h = struct.unpack_from('<IHHHH', b, 8)
    px = np.frombuffer(b, '<u2', count=w * h, offset=20).reshape(h, w).astype(int)
    rgb = np.dstack([px & 31, px >> 5 & 31, px >> 10 & 31]).astype(float)
    if move:
        rgb = move_actor(rgb, **move)
    for one in outline:
        rgb = outline_actor(rgb, **one)
    lum = rgb[:, :, 0] / 31 * .299 + rgb[:, :, 1] / 31 * .587 + rgb[:, :, 2] / 31 * .114
    crop = lum[y0:y0 + H, x0:x0 + W]
    crop = np.pad(crop, ((0, H - crop.shape[0]), (0, W - crop.shape[1])))
    a, b2 = np.percentile(crop, lo), np.percentile(crop, hi)
    n = np.clip((crop - a) / max(b2 - a, 1e-6), 0, 1) ** gamma
    # The dither is confined to the highlights (`dither` is where it starts): a candle or a
    # stained window keeps its texture, while the stone and the people stay on flat greys and
    # read. Dithering the whole picture turned both figures into noise.
    d = np.full(n.shape, .5)
    if dither:
        bayer = BAYER[np.arange(H)[:, None] % 2, np.arange(W)[None, :] % 2]
        d = np.where(n >= dither, bayer, d)
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
    records = [portrait(disc, files, face) for _, _, _, face, _ in VARIANTS]
    for name, x0, y0, _, how in VARIANTS:
        records.append(bytes((H,)) + zx0pack.pack(planes(room(disc, files, name, x0, y0, **how))))
    # The portrait is drawn straight from the bank by a 32-wide sprite routine, which reads
    # longs: its record must start on an even address, so the header is padded to six bytes.
    at = 6 + 2 * len(records)
    head = b'FSK4' + bytes((len(VARIANTS), 0))
    for record in records:
        head += struct.pack('>H', at)
        at += len(record)
    out = head + b''.join(records)
    assert len(out) == at
    n = len(VARIANTS)
    return out, dict(bytes=len(out), keeper=[v[3] for v in VARIANTS],
                     rooms=[len(r) for r in records[n:]])


def main():
    cue, out = sys.argv[1], Path(sys.argv[2])
    raw, info = build(cue)
    out.write_bytes(raw)
    print('%s: %d bytes, rooms %s, shopkeeper %s' %
          (out.name, info['bytes'], info['rooms'], 'WLDFACE %s' % info['keeper']))


if __name__ == '__main__':
    main()
