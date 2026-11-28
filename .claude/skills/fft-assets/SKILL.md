---
name: fft-assets
description: Find any Final Fantasy Tactics asset on the PS1 disc and turn it into the TI's four-grey format - unit sprites, creatures, portraits, UI and status icons, weapons, fonts, item and event text, battle maps. Use when ripping new art for games/fft, writing or extending an extractor, or building the end-to-end asset builder from the ISO. Battle-map geometry and its C2 treatment have their own skill, fft-map-extraction.
---

# FFT assets: where they are, how to convert them

Everything below was read from the local retail US disc
(`roms/ps1/Final Fantasy Tactics (USA)/`, not in git). **Never commit extracted
artwork or a generated bank**: add the generated header to the nearest
`.gitignore`, as `map.h`, `units.h` and `chicken_art.h` are.

Read [reading an original effect](references/effects.md) before rebuilding any combat
effect: how to find what one actually does (its animation records, not its atlas picture),
and what four greys and a 360,000-cycle frame allow of it. `tools/effect_anim.py` decodes
and plays an effect's choreography straight from the disc -- cells, script and cast -- so
the timing no longer has to be read off a video of the console. `--camera` prints the
effect camera's keyframes and `--eye` plays them over the render; `--map-centre x,y,z`
gives it the one vector a preview cannot know, from the target to the middle of the
battlefield (Gariland's roof tile: `42,96,0`).

**The console is no longer in the loop.** The decoder plays an effect's whole
choreography from the disc -- cells, sequences, emitters and their children, the colour
tracks, the effect camera -- so an effect can be rendered, measured and budgeted without
running the PlayStation. Read [putting an effect on the TI](references/effects-on-ti.md)
for what that costs on the calculator: the load per frame of each decoded effect, the
three concessions that buy the budget (a spark is a pixel, a quad over 16 px is a
dithered flat, effects run at 15 fps), which four of the seven fit outright and what to
trim from the three that do not.

Read [the disc catalogue](references/disc-map.md) for file-by-file offsets and
formats. The rest of this page is the method and the traps.

**Before reversing any FFT format, check it against the decompilation.**
[adamrt/fft_decomp](https://github.com/adamrt/fft_decomp) is a byte-exact, 100 %
decompilation of SCUS-94221 -- the same North American disc this skill reads. Its
`include/fft/` headers name every structure with a `size_must_be_N` assertion, and its
source gives behaviour. It ships no game data, so it complements the local disc rather
than replacing it. `git clone --depth 1` is 18 MB; grep it first and guess second.

## The three questions, in order

1. **Where is it?** The catalogue. Do not search the ISO by hand; every known
   file and offset is listed there, with the tool that already reads it.
2. **Does a decoder exist?** Almost always yes. `games/fft/tools/` holds
   `extract.py` (disc access, maps, views), `units.py` (SPR/SHP/SEQ, human
   bodies), `weapons.py`, `other_actors.py` (creatures), `interface.py`,
   `management_assets.py`, `monster_body_assets.py`. Reuse before writing.
3. **How should it look on the TI?** The conversion rules below.

## Converting to four greys

The LCD is 160x100 in four greys and the first rule of the project is that
everything must read at a glance.

- **Quantise over the drawn pixels only.** Percentile thresholds over the opaque
  pixels, never over the whole canvas: a sprite on a transparent field otherwise
  collapses to one or two greys. `other_actors.quantise()` does this.
- **White outline, always, on anything that moves.** The mask dilated by one
  pixel, drawn white. It costs nothing at run time and it is what separates a
  sprite from any scenery. `other_actors.outline()`.
- **A second value cannot carry meaning at five pixels.** For a small icon, the
  fill carries the message and the shape carries the identity: black body with a
  white halo against white body with a black outline. A mid grey body vanishes
  on mid-grey scenery; measured, not guessed.
- **Human faces get their own treatment.** `units.py` lightens the face one grey
  and places the eyes by hand, because scaling merged them into a bar. That code
  is for humans only (see the trap below).

## The TI frame record

An actor frame is **208 bytes**: four planes of 26 big-endian `u16` rows, in
order light, dark, mask, inner mask.

| Plane | Meaning |
|---|---|
| light, dark | the two colour planes; (0,0) white, (1,0) light grey, (0,1) dark grey, (1,1) black |
| mask | **set bit means transparent** |
| inner mask | the body without its white outline, what the hit flash paints |

The sprite is 16 columns wide and the feet sit on row `UNIT_FOOT` (21), so a
creature shorter than 26 rows is placed with its bottom on that row, not at the
top of the record. `other_actors.frame_bytes()` writes one.

`RD16` has two definitions (`base.inc`): a native `u16` read and a portable
big-endian byte pair. **Emit bytes, not `u16`**, or the PC build reads the frames
byte-swapped.

## Traps found the hard way

**`units.sprite()` is human-only.** It looks for hair, a face, eyes and skin to
apply the portrait rules. Run on a chicken it produces floating bars above the
body. Share only the tile compositing: `other_actors.assemble()` repeats that
loop and applies the generic treatment.

**Check the pixel base before you believe a cut sprite.** Unit sheets start their
pixels at 0x200, but `OTHER.SPR` carries 512 colours and starts at **0x400**. Four
rows out, the chickens came out footless with two rows of palette bytes floating
above them, and the blame went to the quantiser for a while. The test is
geometric: sprite sheets bottom-align their art, so with the right base every
object sits flush on the last row of its cell.

**A shape table can describe more than it seems to.** `OTHER.SHP` has seventeen
frames for nine strips of artwork. One frame list is reused and the engine adds a
sheet-Y offset that the SHP never names: the offset picks the **object** (0
chicken, 48 frog, 96 crystal) and a further **+24 picks the game's second camera
pitch**, the same object redrawn with its vertical extent gone. Before concluding
that a sheet is unaddressed, compare the drawn row bands against the tile Y values
the frames use - and when a band turns out to be a near-copy of its neighbour, ask
what engine-wide state selects it, not what the unit is doing.

**Frames are grouped front then back.** FFT draws two directions, front
(down-left) and back (up-left), and mirrors them for the other two. The engine's
`ACTOR_KEY` carries the mirror bit, so a rip needs only half the facings.

**Shifting a shape down inside its own array draws nothing different.** An empty
top row produces neither pixel nor halo; only the placement of the whole glyph
relative to the unit changes what is seen.

**Removing part of a prop saves nothing.** What is left draws the same
silhouette. This matters for storage, not only for looks: see
`fft-map-extraction`.

## Before asking for a verdict

Every change that alters the picture needs the user's eye, and the images have to
be pinned first:

```sh
tools/bin/ti-view --decision /absolute/path/to/comparison.png
tools/bin/ti-view --clear-decision IMAGE...   # after the verdict, or when superseded
```

They then sit at the top of the gallery with a **Décision** badge. Show variants
side by side at the same scale, with a human unit beside a creature or an icon,
since size cannot be judged alone. An animated GIF is the right form for anything
that twinkles or cycles.

## Towards the builder

The end goal is one builder that takes the ISO and writes every bank. What it
needs, and what already exists:

| Asset | Source | Decoder | State |
|---|---|---|---|
| Battle maps, views, depth | `MAP/MAPxxx.GNS` | `extract.py`, `map_budget.py` | works; see `fft-map-extraction` |
| Human bodies | `BATTLE/*.SPR` + `TYPE1/2.SHP` | `units.py` | works |
| Monsters | `BATTLE/*.SPR` + `MON.SHP/SEQ` | `monster_body_assets.py` | works |
| Creatures (chicken, frog) | `BATTLE/OTHER.SPR/SHP` | `other_actors.py --header` | works |
| Weapons | `BATTLE/WEP.SPR` + `WEP1/2` | `weapons.py` | works |
| Portraits | unit sheets | `management_assets.py` | works |
| UI, status icons, numbers | `EVENT/FRAME.BIN` | none yet | read by hand so far |
| Item text | `EVENT/ATCHELP.LZW` | existing, uncompressed | works |
| Names | `EVENT/SMALL.OUT` | `management_research` | works |
| Status pictograms | drawn, not ripped | `status_glyphs.py` | works |

Keep each decoder a library function the builder calls, not a script with its own
`main` doing disc access. `other_actors.py` and `status_glyphs.py` follow that
shape: a `--header` mode writes the engine header, the functions stay importable.
