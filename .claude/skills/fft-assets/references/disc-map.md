# The FFT disc, file by file

Read from the local retail US disc with `extract.disc_files(cue)`, which returns
the ISO walker's `{path: (lba, size)}`. Everything here was verified on that disc;
anything marked *unverified* is a lead, not a fact.

## Access

```python
import sys; sys.path.insert(0, 'games/fft/tools')
import core_audit, extract
disc, files = extract.disc_files(str(core_audit.DEFAULT_DISC))
raw = disc.read(*files['BATTLE/OTHER.SPR'])
```

`core_audit.DEFAULT_DISC` is the local path. 496 files, of which 185 under
`BATTLE/`.

## Unit sprites: `BATTLE/*.SPR`

138 sheets. Layout, documented in full at the top of `games/fft/tools/units.py`:

- bytes 0..511: **16 palettes of 16 BGR555 colours**, colour 0 transparent.
- then the sheet: 256 pixels wide, 4 bits per pixel, low nibble = left pixel.
  `spr[512:512 + 128*256]` reshaped (256, 128) gives the packed rows.

**`OTHER.SPR` is the only exception: its pixels start at 0x400.** It is the one sheet
with 512 colours, thirty-two palettes of sixteen, five each for the chicken, the
frog, the chest and the crystal. Read it at 0x200 like every other sheet and the
whole image slides up four rows: the creatures lose their feet and the palette
bytes come back as two rows of noise above them. The giveaway is geometric - with
the right base every object sits flush on the bottom row of its 24-row cell.

Checked across the whole disc, because a base this quiet is worth not guessing
twice. `BATTLE.BIN` pairs every sheet with its shape table (shape id 0 `TYPE1`,
1 `TYPE2`, 2 `CYOKO`, 3 `MON`, 6 `ARUTE`, 7 `KANZEN`); one sheet of each family
rendered at both bases settles it by eye, and at 0x400 every family breaks, heads
sliced and feet adrift. **137 sheets of 138 are at 0x200; `OTHER.SPR` alone is at
0x400.** `WEP.SPR` is at 0x200 as well, confirmed by drawing `WEP1.SHP`'s own
rectangles over it: at 0x400 the blades stop sitting in their boxes. Measuring how
much art falls outside the rectangles is *not* a usable test on a unit sheet - it
carries a portrait and unused poses that no frame ever names, so both bases leak
about the same and the difference is noise.

`units.py`'s `CRYSTAL` and `CHEST` boxes read `OTHER.SPR` from 0x200 and are
measured on that reading, so the two errors cancel and the chest and crystal come
out whole; the constants carry a note saying so. `EVENT/FRAME.BIN` has no header:
read from 0x40 instead of 0, every row splits across its middle.

Luminance for the TI, as `units.py` fixes it: over a colour's 0..31 components,
`<= 7` black, `<= 16` dark, `<= 22` light, else white; **colour 1 is FFT's own
line colour and is always black**.

Sizes are a weak family hint: 37377 for the simplest sheets (generic chocobos,
`FURAIA`), 43000-49000 for humans and named actors, 85504 for `WEP.SPR`,
33792 for `OTHER.SPR`.

Named sheets worth knowing: `RAMUZA`/`RAMUZA2`/`RAMUZA3` (Ramza's three
chapters), `DILY`/`DILY2`/`DILY3` (Delita), `MINA_M`/`MINA_W` (Squire),
`ITEM_M`/`ITEM_W` (Chemist), `CLOUD`, `ORAN`, `SIMON`, `WIGU` (Wiegraf),
`ZARUE` (Gafgarion), `HIME` (Ovelia), `AGURI` (Agrias).

## Shape tables: `BATTLE/*.SHP`

`units.shp_frames(data)` parses one. Format:

- u32 pointers at `8 + 4*i`, relative to `0x40A`. **The table ends at the first
  zero after index 0.**
- a frame: byte 0 bits 0-2 = tiles - 1, byte 1, then per tile `s8 x, s8 y` from
  the unit's anchor near the feet, and a `u16`: sheet x/8 (bits 0-4), y/8 (5-9),
  size index (10-13), flip x (14), flip y (15). **Drawn last to first.**

`TYPE1.SHP` and `TYPE2.SHP` cover the human bodies; which one a unit uses comes
from `BATTLE.BIN` (below). `OTHER.SHP` has 17 frames only.

## Animation: `BATTLE/*.SEQ`

`TYPE1..4`, `MON`, `OTHER`, `WEP1/2`, `ARUTE`, `CYOKO`, `KANZEN`, `RUKA`,
`EFF1/2`. No parser in the project yet, but the layout is known from `OTHER.SEQ`:
a four-byte header, then **128 little-endian u32 pointers** starting at offset 4,
then the sequence data. `TYPE1.SEQ` and `MON.SEQ` have the same shape. Those pointers are relative to the data, which in `OTHER.SEQ` starts at
**0x408** and not right after the table: 0x220 to 0x400 is `FF` padding, so find
the data by scanning for the first filled region rather than trusting
`4 + 4*count`. A sequence is a run of `(frame, duration)` byte pairs with `FF xx`
opcodes between them; the opcodes take arguments of differing length, so a fixed
two-byte stride drifts and invents frame ids. Reading only the pair distribution
is enough to tell which frames an animation uses.

The pointer table alone says a lot without decoding anything. In `OTHER.SEQ`,
slots 6-23 and 24-41 have **identical pointer deltas** - eighteen sequences of the
same byte lengths, twice over: the chicken's animations and the frog's, built from
the same fourteen frames with different timings. Further on, a run of two-byte
sequences holds the static poses, among them `0f 00` and `10 00`, the treasure
chest's two sides.

The human sequences were read by hand and are recorded in `units.py`:

- battle idle, facing the camera: frames 11 10 9 10 11 12 13 12, at 6 8 10 8
  ticks of 60 Hz; facing away: 16 15 14 15 16 17 18 17.
- the walk uses the same frames at 2 4 6 4 ticks.
- frames 9-13 front, 14-18 back; then each pose front then back: hit 19/20,
  swing wind-up 27/29 and blow 31/33, kneeling 21/22, lying KO 23/24,
  throw 28/30.

FFT draws two directions and mirrors for the other two.

## Effects: the `EFFECT/` directory

512 files, `E000.BIN` to `E511.BIN`, 401 of them non-empty and 20,127,292 bytes
in all -- the original's combat effects. **291 are data**, opening with the
little-endian u32 `40` and a ten-entry u32 offset table; **110 are MIPS code**,
opening with `0x27bd....` (`addiu sp, sp, -N`), so those effects are routines and
not assets. Section 2 of a data file is 64 bytes with only five distinct contents
across all 291, which reads as a shared 32-colour palette; section 9 is the
largest and the most variable. Sections 4, 5, 6 and 7 have one size each (2,404,
600, 24, 6,280) but differ file to file.

Nothing yet links an `E` number to an ability, and no section is decoded. The
survey, with what is observed against what is guessed:
`games/fft/RE_NOTES.md`, "The `EFFECT/` directory".

## `BATTLE.BIN`

Per-unit identity tables, addressed from the PS1 load base `0x80067000`:

| What | Address | Stride |
|---|---|---|
| shape id, 0 = TYPE1, 1 = TYPE2 | `0x80094748 + 4*identity` | 4 |
| sprite file LBA and size | `0x80094cd4 + 8*identity` | 8 |

Identities 1..52 are the named humans, 96..133 the generic job bodies
(`actor_storage_study.py` walks exactly that range).

## `BATTLE/OTHER.SPR` and `OTHER.SHP`

What a transformed or dead unit wears, and what it leaves behind. Pixels start at
**0x400** here, not 0x200 (above).

`OTHER.SHP` has **17 frames**: 0 a small marker, 1-14 a creature, 15-16 the
treasure chest. Every creature frame is a single 16x24 tile at sheet
x = 0, 16, 32 ... 208 and **sheet y = 0**; the chest's two frames are 24x24 at
x = 224, y = 0 and y = 48.

**The sheet is a grid of object slot by drawing mode, and the shape table names
neither.** The slot is a sheet-Y offset the engine adds: **0 chicken, 48 frog,
96 crystal**. Frames 15 and 16 spend the first two slots on the chest's two sides
instead, and `OTHER.SEQ` holds each as a static two-byte sequence, `0f 00` and
`10 00`. The crystal has no frame of its own at all: it is frames 1-8 at +96.

**The mode is a further +24, and it is the second camera pitch.** Nothing in the
SHP or the SEQ ever names y = 24 or y = 72, so the engine applies it, and it
applies to the treasure chest exactly as to the creatures - which rules out any
per-unit state, since a chest has none. What the +24 drawing changes is only the
vertical extent: the chest keeps its lid and its base outline and loses the eight
rows of wall between them, the chicken and the frog lose the legs that reach down,
and every drawing is pixel-identical to its twin for the ten rows above the cut.
The game has two battle camera pitches, about 26.5 degrees and an upper view, and
the PS1 cannot foreshorten a sprite to match: a sprite quad takes the camera's X
and Z zoom but not its Y, so the second pitch has to be drawn by hand. The
crystal, a floating diamond with no vertical face and no ground contact, is the
one object with no +24 twin. *Not yet watched happening in the original; the
evidence above is the sheet's own structure.*

Our camera has one pitch, so a rip takes the +0 drawings and ignores the rest.

**Facing is not the strip either.** Every sequence in `OTHER.SEQ` has a twin five
frames later, `01 06 / 03 08 / 01 04 / 02 04` against `06 06 / 08 08 / 06 04 /
07 04`, over and over, and frames 1-5 rendered beside 6-10 show the same creature
turned. So **frames 1-5 face the camera and 6-10 face away**, the same convention
`units.py` records for the human bodies, with the engine's mirror bit giving left
from right.

`other_actors.py --header` writes both creatures from this.

## Weapons: `BATTLE/WEP.SPR`, `WEP1.SHP/SEQ`, `WEP2.SHP/SEQ`

Read by `games/fft/tools/weapons.py`, which composes Gariland's armed blows.

## UI and icons: `EVENT/FRAME.BIN`

37568 bytes, **4 bits per pixel, 256 pixels wide, 293 rows**, no palette of its
own (use a flat grey ramp to read it, then pick the TI values by hand).

Contents, by source row:

| Rows | What |
|---:|---|
| 0-12 | window frames, digits, the small and medium fonts |
| ~60-140 | menu labels: Move, Job, Brave, Faith, Ability, Equip, Status, Help, Exp, Hp, Mp, CT |
| ~150-200 | large glyphs, the Units/Move/Auto header, counters |
| **208-222** | **status bubbles, row A**: glasses (Dark), a heart (Charm), "..." (Silence), a starburst |
| **222-236** | **status bubbles, row B**: "DW" (Don't Move), "DA" (Don't Act), a square in a square (Stop), a triangle |
| **236-250** | a chevron, the **five charge stars**, the **death-counter bubbles 3 2 1 0** |
| ~250-293 | Learned/Master banners, the Turn/Dir plate, arrows |

The status icons are about 14x11: a light speech bubble with a tail carrying a
dark symbol. At TI size they do not survive a direct downscale; `status_glyphs.py`
redraws their meaning at 5x5 instead.

## Text

| File | What | Note |
|---|---|---|
| `EVENT/ATCHELP.LZW` | item and ability descriptions | uncompressed despite the name |
| `EVENT/SMALL.OUT` | generic names | table at `0xec + u32 at 0x70`, split on `0xFE`/`0xFF` |
| `EVENT/*.EVT`, `EVENT/TEST.EVT` | event scripts and dialogue | censused by the campaign text tools |

## Maps: `MAP/MAPxxx.GNS`

Resource records; `extract.gns_records()` reads them. Record types: `0x2E01` the
primary mesh, `0x1701` the texture (256x1024, 4 bits).

Inside the mesh:

| Offset | What |
|---|---|
| `0x40` | pointer to the textured polygons |
| `0x44` | pointer to the 256-colour palette block |
| `0x68` | pointer to the terrain block |

Terrain block: `nx`, `nz`, then **two levels** of `nz*nx` tiles of 8 bytes.
Per tile: byte 0 bits 0-5 surface, byte 2 height, byte 3 bits 0-4 slope height,
byte 4 slope type, **byte 6 flags: bit 0 blocked, bit 1 crossed but never stopped
on**. That second bit marks a decorative prop, and such a tile carries the
**prop's own height, not the ground's** — Sweegy's four trees are tiles of
height 17-18 over ground near 4.

Level 1 is empty on most maps but carries real rooftops on others (Slums in
Dorter reaches height 15 there). **Always read both levels.**

Geometry, projection and the C2 treatment belong to `fft-map-extraction`.
