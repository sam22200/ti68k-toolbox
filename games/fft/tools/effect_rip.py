#!/usr/bin/env python3
"""Decode an EFFECT/Exxx.BIN sprite atlas from the local disc into a PNG.

The original's combat effects live in EFFECT/, 512 files, 401 of them non-empty.
291 are data and the rest are MIPS routines, mixed in with no separation: Cure 4
is code, Cure 1-3 are data, so some abilities have no atlas to take.

A data file opens with the little-endian u32 40 and a ten-entry u32 offset table.
Section 9 holds the pictures: 1028 bytes of palette, then an 8bpp image 128 wide.

  **The height is not fixed.** It is (len(section 9) - 1028) / 128, and it varies:
  Fire is 128x120, Fire 2 is 128x160, Fire 3 and 4 are 128x256. Assuming 256 --
  which the community rule of thumb implies for files of 34..70 KB -- silently
  drops every smaller effect, Fire among them.

Colour is PS1 BGR555 with the top bit as the semi-transparency flag:

  * palette index 0 is transparent, whatever colour it holds (here often 0xffff);
  * a raw value of 0x0000 is transparent;
  * **bit 15 set does not mean black.** It marks a colour the hardware blends,
    and these effects blend additively, so a dark flagged colour contributes
    almost nothing and shows the scenery through. Painting those opaque puts a
    black halo around every sprite, which is what a first pass at this did.

Which ability an effect belongs to is the file's *name*, not a table: there is no
ability-to-effect table in BATTLE.BIN. The number tracks the ability id for the
first 35 or so (Cure 0x001 -> E001, Fire 0x010 -> E016, Ice 0x018 -> E024) and
then drifts, because the catalogue holds effects no ability uses. The 14-byte
ability record carries the element mask, not an effect id. Names:
https://ffhacktics.com/wiki/Effects

Usage:  effect_rip.py 16 17 18 --out DIR     (or --all)
"""
import argparse
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import core_audit  # noqa: E402
import extract  # noqa: E402

PALETTE_BYTES = 1028
WIDTH = 128


def atlas(raw):
    """(width, height, pixels) of an effect file, or None when it is not data."""
    if len(raw) < 40 or struct.unpack_from('<I', raw, 0)[0] != 40:
        return None                      # a MIPS routine, not an atlas
    off = struct.unpack_from('<10I', raw, 0)
    if not off[9] or off[9] >= len(raw):
        return None
    sec = raw[off[9]:]
    if len(sec) < PALETTE_BYTES + WIDTH:
        return None
    height = (len(sec) - PALETTE_BYTES) // WIDTH
    colours = struct.unpack_from('<512H', sec, 4)[:256]
    return WIDTH, height, sec[PALETTE_BYTES:PALETTE_BYTES + WIDTH * height], colours


def rgba(colours, index):
    """One pixel as RGBA. Transparent where the hardware would not draw."""
    c = colours[index]
    if index == 0 or c == 0:
        return (0, 0, 0, 0)
    r, g, b = (c & 31) << 3, ((c >> 5) & 31) << 3, ((c >> 10) & 31) << 3
    # bit 15: blended, not opaque. Additive blending makes a dark one invisible,
    # so its alpha follows its brightness rather than being forced to 255.
    return (r, g, b, min(255, (r + g + b) // 2) if c >> 15 else 255)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('ids', nargs='*', type=int, help='effect numbers, e.g. 16 17 18')
    ap.add_argument('--all', action='store_true', help='every effect that is data')
    ap.add_argument('--out', type=Path, required=True)
    a = ap.parse_args()
    if not a.ids and not a.all:
        ap.error('give effect numbers or --all')
    from PIL import Image
    disc, files = extract.disc_files(str(core_audit.DEFAULT_DISC))
    a.out.mkdir(parents=True, exist_ok=True)
    ids = range(512) if a.all else a.ids
    data = code = empty = 0
    for n in ids:
        key = 'EFFECT/E%03d.BIN' % n
        if key not in files or files[key][1] == 0:
            empty += 1
            continue
        got = atlas(disc.read(*files[key]))
        if not got:
            code += 1
            continue
        w, h, pixels, colours = got
        im = Image.new('RGBA', (w, h))
        im.putdata([rgba(colours, p) for p in pixels])
        im.save(a.out / ('E%03d.png' % n))
        data += 1
        if not a.all:
            print('E%03d  %dx%d' % (n, w, h))
    print('%d atlases, %d routines, %d empty -> %s' % (data, code, empty, a.out))


if __name__ == '__main__':
    main()
