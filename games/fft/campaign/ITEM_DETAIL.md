# The shared item detail

2026-11-21. One panel describes an item wherever the cursor can rest on one,
and one bank carries FFT's own words for it.

## FFT's descriptions are on the disc, uncompressed

OBSERVED: **`EVENT/ATCHELP.LZW` is not compressed despite its name.** It opens
with 32 little-endian u32 section offsets into the text that starts at 0x80,
and each section is a run of 0xFE-terminated strings in FFT's ordinary text
encoding:

| section | strings | what |
| --- | --- | --- |
| 11 | 40 | — |
| 12 | 160 | job descriptions |
| **13** | **256** | **item descriptions** |
| 15 | 512 | ability descriptions |
| 19 | 188 | — |

A format code in 0xE0..0xEF takes **exactly one** argument byte. Reading it as
two swallowed the following letter and produced "~ttack power"; with one, the
whole file decodes cleanly, which is also what shows that nothing in it is
compressed.

Inside an entry the prose comes first, then a format code opens the stat block
the original prints under it ("Attack power 3  Evade 5%", the list of jobs that
may equip it). Those numbers are already in `ffmg`'s item record, so
`tools/item_help.py` keeps the prose alone: **253 of the 256 items have one,
11,855 bytes**, written to `ffid` (FID1, 12,373 bytes with its table) and read
in place from the archive -- no RAM, no decompression.

`EVENT/HELP.LZW` and `EVENT/WLDHELP.LZW` are the menu and tutorial help, and
those two *are* compressed: their literals are the same text encoding but
3-byte codes in 0xF0..0xF7 stand for strings emitted earlier ("Maximum~ of
allies is 16." against ATCHELP's "Maximum number of allies is 16."). Nothing
needs them, so that encoding is not cracked.

## The panel

`games/fft/item_detail.inc` draws it: the item's icon, its name, its family,
a weapon's power, FFT's description wrapped over three lines on the caller's
own font, and every non-zero bonus. The caller fills an `ItemDetail` from its
own banks and supplies four macros (`ID_TEXT`, `ID_NUM`, `ID_ICON`, `ID_ADV`),
so the panel itself needs no bank, no allocation and no state of its own.

**[MODE] opens it, [MODE] or SHIFT closes it**, and it is modal: while it is up
its owner draws no live cursor over it.

## Who calls it

- **The shop** (`worldmap.c`), on the Buy and Sell lists.
- **Management** (`manage.c`), on the Equip screen's slot and on the item
  picker's rows.
- **Deployment** (`deploy.c`) has no item or equipment view -- it picks party
  slots and positions -- so there is nothing to attach it to.
- **The battle** (`fft.c`) likewise: its unit card is portrait, name, job, HP,
  MP, level, EXP, Brave and Faith, with no equipment. Showing a unit's or an
  enemy's equipment in battle is a new screen in the resident, not a call, and
  is left open.
