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


def animation(raw):
    """The frames of an effect: section 0 is the animation.

    An offset table of u16, then one record a frame. A 28-byte record carries the
    atlas cell at bytes 12..15 (u, v, width, height) and six signed 16-bit quad
    corners at 16..27 -- where to draw that cell. **The motion is in the corners**,
    written out frame by frame rather than stored as a curve: Ice walks its top
    edge 62, 58, 53, 49, 45, 40, 36, 32 while the bottom goes the other way, and
    the crystal grows.

    Styles differ. Cure centres its quads, so the corners are exactly half the
    cell. Fire anchors its quads near the base and steps its uv through a grid of
    40x40 cells, a sprite loop rather than a deformation. Ice does both.

    Records are not all 28 bytes -- Fire's table steps by 28, 52, 76 and 100 --
    so longer variants exist and are skipped here rather than guessed at.

    What this does NOT give: how long a frame holds. The timing lives in one of
    sections 1, 3, 6 or 7; section 4 is fifteen ramps of 160 rising values, which
    looks like interpolation but has not been shown to be. The first eight of the
    record's fourteen 16-bit fields are also unread.
    """
    if len(raw) < 40 or struct.unpack_from('<I', raw, 0)[0] != 40:
        return []
    off = struct.unpack_from('<10I', raw, 0)
    if not off[0] or not off[1] or off[1] <= off[0]:
        return []
    sec = raw[off[0]:off[1]]
    words = struct.unpack_from('<%dH' % (len(sec) // 2), sec, 0)
    starts = []
    for v in words[2:]:                  # words[0..1] head, then the offsets
        if v == 0 or v >= len(sec):
            break
        starts.append(v)
    out = []
    for i, a in enumerate(starts):
        b = starts[i + 1] if i + 1 < len(starts) else len(sec)
        if b - a != 28:
            continue                     # a longer, undecoded variant
        rec = sec[a:b]
        out.append(dict(u=rec[12], v=rec[13], w=rec[14], h=rec[15],
                        corners=struct.unpack_from('<6h', rec, 16)))
    return out


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
    ap.add_argument('--frames', action='store_true', help='print the frame list instead of writing PNGs')
    ap.add_argument('--out', type=Path, help='where the PNGs go; not needed with --frames')
    a = ap.parse_args()
    if not a.ids and not a.all:
        ap.error('give effect numbers or --all')
    if not a.frames and not a.out:
        ap.error('--out is required when writing PNGs')
    disc, files = extract.disc_files(str(core_audit.DEFAULT_DISC))
    if not a.frames:
        from PIL import Image
        a.out.mkdir(parents=True, exist_ok=True)
    ids = range(512) if a.all else a.ids
    data = code = empty = 0
    for n in ids:
        key = 'EFFECT/E%03d.BIN' % n
        if key not in files or files[key][1] == 0:
            empty += 1
            continue
        raw = disc.read(*files[key])
        if a.frames:
            fr = animation(raw)
            if not fr:
                code += 1          # a routine, or no decodable 28-byte record
                continue
            print('E%03d  %d frames' % (n, len(fr)))
            for i, f in enumerate(fr[:6]):
                print('   %2d  cell (%3d,%3d) %dx%d  corners %s'
                      % (i, f['u'], f['v'], f['w'], f['h'], f['corners']))
            data += 1
            continue
        got = atlas(raw)
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
    print('%d %s, %d routines, %d empty%s'
          % (data, 'frame lists' if a.frames else 'atlases', code, empty,
             '' if a.frames else ' -> %s' % a.out))


if __name__ == '__main__':
    main()
