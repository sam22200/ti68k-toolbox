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
`EFF1/2`. No parser in the project yet; the human sequences were read by hand and
are recorded in `units.py`:

- battle idle, facing the camera: frames 11 10 9 10 11 12 13 12, at 6 8 10 8
  ticks of 60 Hz; facing away: 16 15 14 15 16 17 18 17.
- the walk uses the same frames at 2 4 6 4 ticks.
- frames 9-13 front, 14-18 back; then each pose front then back: hit 19/20,
  swing wind-up 27/29 and blow 31/33, kneeling 21/22, lying KO 23/24,
  throw 28/30.

FFT draws two directions and mirrors for the other two.

## `BATTLE.BIN`

Per-unit identity tables, addressed from the PS1 load base `0x80067000`:

| What | Address | Stride |
|---|---|---|
| shape id, 0 = TYPE1, 1 = TYPE2 | `0x80094748 + 4*identity` | 4 |
| sprite file LBA and size | `0x80094cd4 + 8*identity` | 8 |

Identities 1..52 are the named humans, 96..133 the generic job bodies
(`actor_storage_study.py` walks exactly that range).

## `BATTLE/OTHER.SPR` and `OTHER.SHP`

What a transformed or dead unit wears. The sheet holds, top to bottom: two rows
of **chicken**, two rows of **frog**, a row of **crystals**, and the **treasure
chest** down the right edge.

`OTHER.SHP` has **17 frames**: 0 a small marker, **1-14 the chicken**, 15-16 the
chest. **No frame addresses the frog** — its tiles are in the sheet but another
table or a page offset must reach them. `MON.SEQ` is the first lead.

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
