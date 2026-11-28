# Reading an original effect, and rebuilding it in four greys

How to find out what one of the original's combat effects actually *does*, and how to put
a recognisable version of it on a 160x100 screen inside a 360,000-cycle frame. Written
from Stasis Sword, where three successive attempts were each wrong in a way the method
below would have caught on the first.

## 1. Never read the motion off the atlas

`tools/effect_rip.py` decodes an effect's sprite atlas, and it is the obvious place to
start. It is also where the first two mistakes came from.

**A sprite's shape does not tell you which way it moves.** `E173`'s atlas shows a tall
cone with its point up. That was read as "a spike rising from the ground", then, when the
user said it comes from the sky, as "a cone descending" -- two guesses, both from the
same picture, neither of them evidence.

The animation records say it, and `effect_rip.py --frames` prints them:

```
E173  17 frames
    0  cell ( 56, 48) 40x136  corners (-20, -26, 20, -26, -20, 110)
```

The quad spans y **-26..110** about its anchor: 26 above it and 110 below. Almost the
whole sprite hangs *under* the anchor, so the anchor is up in the air and the effect
descends from it. That was on screen from the first day and went unread for three
attempts.

## 1b. Read ALL the records, not the first six

The choreography is in the file. `effect_rip.py --frames` prints the animation list, and
until 2026-11-27 it printed `fr[:6]` -- the first six records of however many there are.
Stasis Sword has **42**. Three versions of it were built from a seventh of what the disc
says, with video-watching filling the rest.

`--all-frames` prints them all, and for `E163` they say plainly what no single record can:

```
   16  cell ( 72,120) 40x48  corners (-16, 70,  9, 70, -16, 100)
   17  cell ( 72,120) 40x48  corners (-13, 71, 10, 73, -15,  98)
   ...
   24  cell ( 72,120) 40x48  corners ( -2, 83,  2, 85,  -5,  88)
```

Nine records, **the same cell**, with the quad's corners shearing and closing from a
spread of 25 pixels down to 4. Records 28..38 are another one doing the same. That
convergence *is* the effect -- blocks tumbling in onto the target -- and around it sit
three 16x16 cells and a handful of 8x8 ones: **distinct pieces, not one mass**.

So before anything else: count the records, group them by cell, and look at what the
corners do over a run. A cell repeated with moving corners is an animation; a cell that
appears once is a prop.

## 2. Watch it run, and find it by colour

`tools/effect_capture.py STATE FRAMES OUT` plays the original from a save state and keeps
the frames around each wash, reading the luminance and every unit's HP per frame. What it
gives you is a numbered sequence you can step through.

**Do not diff two frames to locate the effect**: the wash darkens the whole picture, so
every pixel differs and the box is the whole screen. Find it by what makes it an effect --
bright, and its own hue:

```python
a = np.asarray(Image.open(f).convert('RGB'), dtype=int)
bright = (a.sum(2) > 330) & (a[:, :, 2] > a[:, :, 0] + 30)   # bright and blue
ys, xs = np.where(bright)
```

On Stasis Sword that prints the effect walking in from the top of the screen:

```
f2919      8 px   x  85..88   y 115..117
f2925    252 px   x  83..134  y   0..116
f2937   1692 px   x  83..142  y   0..159
f2949   2191 px   x  83..142  y   0..169
```

`y` starting at **0** is the finding: it enters at the top edge of the frame, not above
the target.

## 3. Crop, zoom, and read the phases

Then crop to that box and look at consecutive frames at 3x. A full-screen contact sheet
is not enough -- at 1:1 the effect reads as "a blue column" and that is how two wrong
versions got built. Zoomed, Stasis Sword is plainly **two** movements:

| frame | |
|---|---|
| +24 | a faceted shard of ice enters at the top of the screen, point down |
| +28 | it falls, elongated |
| +32 | it reaches the target **and a pillar of light erupts upward out of it** |
| +36 | the pillar holds: white at the foot, blue climbing, sparks running up it |
| +44 | more shards keep falling into it |
| +56 | it thins away -- in width; the column stays tall |

**The count of movements is the thing to get right first.** Both earlier versions drew one
cone and so replaced the whole sequence with half of it. The second even lost the pillar,
which is what carries the effect, in order to gain the shard.

## 4. Rebuilding it: what four greys and 360,000 cycles allow

- **Span fills, not per-pixel calls.** A solid shape drawn with a dot routine costs a call
  a pixel. Filling whole bytes on both planes is about 1,220 cycles a row of a 24-row
  shape; per pixel it does not fit at all.
- **Dither instead of alpha.** The original's effects blend additively and the unit shows
  through them. Filled solid, the first version hid the very unit the focus had just kept
  bright. A checkerboard mask ANDed into the span costs nothing and reads as a veil.
- **Solid at the core, dithered away from it.** The original is white at the pillar's foot
  and blue as it climbs; four greys say that with density, not with colour.
- **Outline a silhouette, never a scatter.** A one-pixel halo round a one-pixel dot is four
  blacks for one white: it eats the effect. Round a filled shape it is the project's
  ordinary visibility rule and it costs two dots a row.
- **Powers of two for the phase lengths**, and masks rather than `%` for anything per
  frame: a 32-bit divide is about 900 cycles against the 1,220 a row costs.
- **Return early once a phase is over.** A held effect that draws nothing for the tail of
  a long wash pays nothing for it.

## 5. What you will have to drop, and how to choose

There is not room for all of it. Stasis Sword's shards keep falling through the whole
hold; ours fall once, because a second shard is 14 more rows on every frame of the hold
and measured 377,886 against the 360,000 limit.

**Drop the decoration, keep the structure.** The pillar is what the effect reads as; the
extra shards are texture. And measure the choice rather than guessing it: the height of
the pillar was swept (`FX_SPIKE_H` 36 passes with 434 cycles left, 24 passes with 7,250)
and the one with a real margin was taken. In this file a sub-thousand margin has twice
turned out to be a frame the scenario never reached.

## 6. Check the picture, not the checksum

The cycle scenario's screen checksum covers **one** frame. When the dissipation was added
the checksum did not move at all, because the frame it checks falls inside the hold. The
way to know an effect changed is to compare the rendered frames one by one:

```
t=30, t=36   0 pixels   (the hold, unchanged -- expected)
t=42       936 pixels   (thinning)
t=46+    1,728 pixels   (gone)
```

Four separate measurements in this layer have reported a healthy peak for frames where
the feature was not running: a missing `st.abil`, a pair of units `fx_play` rejects as
invisible, a camera orientation with no occluder, and a spike whose growth the walk never
reached. Assume a green number is covering nothing until a second reading says otherwise.

## 7. Where the cycles come from when an effect does not fit

Measured on Stasis Sword, peak out of 360,000, each against the same frame:

| lever | peak | won |
|---|---|---|
| the dimming loop rewritten in 68000 asm | 340,256 | 9,332 |
| **not drawing the units the action does not concern, during the pillar only** | 333,076 | **19,660** |
| **the same, for the whole wash** | 304,128 | **48,608** |

The last two are by far the biggest lever in this layer -- more than the dimming itself
costs -- and they are also the only ones with a visible price: the other units are dim
silhouettes during a wash and they simply vanish. On the original they stay. So it is a
fallback, not a default, and bounding it to the frames where the effect is heaviest keeps
two thirds of the win while cutting the absence from about 55 frames to 18.

Two that did **not** work, so they are not retried:

- **A composed copy of the dimmed picture**, built once and blitted after. It works -- 2
  compositions and 48 frames served out of a 55-frame wash, same checksum -- but it needs
  4,000 bytes and the Titanium has **426 free, 420 contiguous** (measured in TiEmu
  through `fftmem.py set heap on`). It cannot be taken conditionally either:
  `FFT_COMBAT_FX` is in neither `module.py` nor the campaign Makefile, so a `Work` member
  under that flag exists in the core and not in its modules, `MOD_SIG` diverges and the
  core refuses every image.
- **Depth-testing the effect.** `cover_bytes` per row is about 270 cycles and the shape
  is tall: 22,000 against a margin that has never exceeded 20,000.

## 8. Where the information lives

There is **no PS1 Final Fantasy Tactics decompilation**. `Nax/wotl` decompiles the PSP
*War of the Lions*, which is a different build. What exists for the PS1 is modding work:
`Glain/FFTPatcher` patches effect files by offset, and FFHacktics documents the directory
and the graphics.

For an effect's choreography none of that is needed: section 0 of the `.BIN` holds it,
and `effect_rip.py --frames --all-frames` prints it. FFHacktics describes the format as
carrying **animation scripts** as well as the frame records, so there is more in the file
than the 28-byte records -- timing in particular, which is still read off captured video
here. Decoding that script is the way to stop inferring timing from frames.

## 9. The criterion: let the effect's height decide, not the budget

An effect either fits the ordinary picture or it earns a cut-away. The test is **its own
requested height**, because that is a property of what is about to be drawn:

```c
#define FX_TALL 28                      // above this, the picture gets out of the way
static u8 fx_tall(void)
{
    return st.mode == M_ACT && FX_STAGES(st.abil) && FX_SPIKE_H > FX_TALL
           && st.act_t >= FX_ICE_FROM && st.act_t < FX_ICE_FROM + FX_ICE_LEN;
}
```

**Do not gate it on the frame budget.** `W->left` was tried and it is negative all
through a battle scene, so a budget test fires always or never. The height is honest.

**Both halves of that test matter.** Keyed on the ability alone, the cut-away stayed up
for the whole wash, and since the effect plays 23 ticks of a 55-tick hold that left 25
frames of empty grey field with nothing on it. The picture goes away for the *effect*,
not for the ability.

What a cut-away is: the map becomes a flat field, nothing is dimmed (a uniform field has
nothing to dim) and only the two units the action concerns are drawn. Measured, that frees
99,020 cycles -- 28 % of the whole frame budget -- and it is what pays for `E173`'s real
82 rows, where the ordinary picture affords 28.

| | peak | margin |
|---|---|---|
| ordinary picture, 24-row pillar | 352,736 | 7,264 |
| **cut-away, 82-row pillar** | **337,234** | **22,766** |

The sequence reads as wash on the map, cut to the effect, back to the washed map. Note
what it costs in fidelity: the original keeps its scene visible throughout. This is a
trade taken deliberately to get the effect's size right, and it is bounded to the frames
the effect draws on.

## 10. VRAM, the records, and what is still missing

**The console's VRAM can be read**, and `effect_capture.py` now dumps it at every wash
(`vram-NNNNNN.bin` and `.png`, the 1 MB seen as 1024x512). It answers *what is loaded*
while the effect plays, which the file cannot.

It is not the best source for *what is drawn*, though. The pages come out as indexed 4-
or 8-bit data viewed as 15-bit colour, so reading them means recovering each page's depth
and CLUT first. The animation records give cell and quad directly, and there are only 42
of them: start there, and keep the VRAM for questions about residency.

**So no, each effect does not have to be rebuilt by eye.** Its choreography is in its
file, and section 11 reads it.

**There IS a PS1 decompilation**: [adamrt/fft_decomp](https://github.com/adamrt/fft_decomp),
byte-exact on SCUS-94221, 100 % decompiled. `include/fft/effect/` names every structure
this page reverse-engineered. Read it before guessing at a format again. (`Nax/wotl`
decompiles the PSP build, which is a different thing.)

## 11. The animation decoder: an effect read instead of watched

`tools/effect_anim.py` decodes sections 0, 1 and 3, and plays the result. The video is
no longer where the timing comes from.

    effect_anim.py 163 --script      the byte-code, one block per sequence
    effect_anim.py 163 --timeline    tick by tick: which cell is drawn where
    effect_anim.py 163 --render DIR  PNG per frame and a GIF, from the disc alone
    effect_anim.py 163 --render DIR --ti     the same at 160x100 in four greys
    effect_anim.py 0 --audit         decode all 291 data effects and report the misses

**Section 1 is the script**, and it is four opcodes wide:

    0x00..0x7f  <u8 ticks> <u8 flag>   draw that cell of section 0, hold `ticks`
    0x81                                end of sequence
    0x82 <s16 x> <s16 y>                set the position
    0x83 <s8 dx> <s8 dy>                move by (dx, dy)

Its head is `u16 count, u16 0` then `count` u16 offsets **relative to byte 4**. Ticks are
60 Hz, so the common `02` is one frame at 30 fps. **Section 3 is the cast**: `u16 2, u16
count`, 16 bytes, then 196-byte records whose *byte 1 is the sequence they play* -- one
sequence can be cast twice. The rest of the record is still unread.

**Section 0 had been read wrong twice.** An offset points *four bytes before* its record:
the bytes it lands on are the previous record's last corner. A record is `u16 marker, u16
npoly` then `npoly` polygons of 24 bytes -- page, palette, `u, v, w, h`, then **four**
corners, TL TR BL BR. Three consequences, each of which had cost a wrong version:

* records of 52, 76 and 100 bytes are the two-, three- and four-polygon frames. Keeping
  only the 28-byte ones leaves 42 of Stasis Sword's 128 cells, and the dropped ones are
  the interesting ones: the shards are drawn three polygons at a time;
* the fourth corner was being taken from the next record;
* **`w` and `h` are signed**, and a negative one mirrors the cell. 23 of Stasis Sword's
  298 polygons carry one, and read unsigned they index 224 columns into a 128-wide atlas.

**What it buys, measured on the whole disc**: 291 data files, 11,233 cells, 1,537
sequences, 1,893 cast members. Every sequence parses to its `0x81` with no fifth opcode.
One file (E323) has a frame record that is not `4 + 24n`, and six (E105..E110) cast a
sequence their table does not hold -- report those, do not paper over them.

**Stasis Sword, read rather than watched.** Eleven sequences: a sword flash at the
caster, one 48x104 blade falling from y -276 in seven steps of 32 px, four 8x8 sparkles
falling beside it, and five more standing the column. The blade's own sequence is
`move to 0,-276`, seven `cell 2 / move by 0,+32`, then `cell 3` held 30 ticks, then
thirteen cells with `move by 0,-2 .. +2` -- a settle. Total 72 ticks, 36 frames.

**Place the origin, or you will mistake it for a scale problem.** The composite sweeps
y -283..+99 about its origin, so with the origin on the target's feet almost all of it
draws below the screen and the standing column looks three times too short. Called
without `--anchor`, the tool sits the sweep on the canvas and prints where it put it.

## 12. What the decompilation corrected

Every layout section 11 derived by hand matched `fft_decomp`'s headers -- section map,
24-byte polygon with four corners, signed cell size, the four sequence opcodes, 196-byte
emitter records whose byte 1 names a sprite sequence, 6278-byte timeline, 600-byte timing
curves, 24-byte flags, `4 + 15*160` curves. Three things were wrong, and they are the
three that mattered:

* **`0x81` is LOOP, not END.** A sprite sequence runs back to its first byte. Nothing in
  it ends an effect, so "the decoded animation is longer than the game's" was never a
  timing discovery -- it was a missing stop.
* **The third byte of a frame is the ordering-table depth mode**, not a flag: 0
  unchanged, 1 nearer 8, 2 fixed 8, 3 farthest, 4 fixed 16, 5 nearer 16. The original
  sorts its effect sprites, and says so per frame.
* **The atlas is 128 or 256 wide**, and byte 0x403 of section 9 (`wide_image`) says
  which. 20 of the 291 effects are 256 wide; read at 128 they come out as a 512-row
  ribbon with every sprite cut in half. `effect_rip.py` had assumed 128 since the start.

And the start delay that section 11 could not find is **`spawn_delay` in the timeline**
(section 7, offset 0x06), "frames between child spawns", beside `phase1_duration` and
`phase2_duration` and, per particle channel, a 25-keyframe table. It was never in the
emitter record, which is why reading that record harder was never going to find it.

Section 2, which this page had dismissed as 64 constant bytes, is the **effect script**:
a 9-bit-opcode interpreter with counters, branches, child spawning and camera control.
Decoding sections 2 and 7 is what would make the playback the game's.



## 13. The particle model, and how far it was checked

`effect_anim.py --render` plays an effect from its timeline. What that took, and
what each piece is worth, measured rather than argued:

* **the spawn** -- a keyframe's emitter spawns on the keyframe's own frame count,
  which `state.child.step[i] = 0` resets on entering it, so step 0 always passes
  and the gate `step % spawn_interval` lets the first frame through. Stasis
  Sword's shards have interval 49 inside 2-frame keyframes: one spawn each;
* **the death** -- lifetime -1 is `ANIMATION_DRIVEN`, and the particle is freed
  the frame its sprite node's `frame_timer` reads 0. `draw_particle_sprite` runs
  the sequence step *before* that check and reloads the timer with the hold it
  just read, so the timer is 0 only on a `hold 0` op. Every sequence ends on one;
* **the spread** -- `EFFECT_EMITTER_BOX_SPREAD` clear means **on the ellipsoid's
  surface**, not inside it, and that is 1,429 of the disc's 1,893 emitters;
* **the blend** -- bits 5-6 of a part's tpage: 11,743 polygons B+F, 2,041 B+F/4,
  499 B-F, 341 B/2+F/2. One uniform add burns the quarter-strength parts;
* **the tint** -- three colour curves a particle, read at its own frame counter,
  128 neutral. They brighten as well as dim: Night Sword's ball curve peaks at
  255, averages 153.

**Where it lands.** Night Sword's ball in the game measures (240, 177, 171)
against (79, 54, 42) for one cell of it -- **three layers**, which is exactly
what its emitter spawns. The render gives (237, 220, 208): the red matches to
within a percent and green and blue run about a quarter high, which is the
tint's x1.19 average. The model is right; the remaining error is exposure, and
comparing a clean render against a photograph of a television is at the edge of
what the measurement can settle.

**Not modelled**: gravity (a global an opcode sets), and the bloom the console
spreads over a lit scene -- which is most of what still separates the two
pictures.


## 12. Code is not "no data": 110 effects hide theirs behind their MIPS

`effect_rip.py` used to answer "a MIPS routine, not data" for 110 of the 401
non-empty effect files, Holy among them, and that was taken to mean those
abilities have no art to read. It is wrong. **Every one of the 110 carries a
complete data file after its code.** The header is the landmark: `frame_data`
is 40, because section 0 follows the ten-word header, and the nine other
offsets never decrease and stay inside what is left of the file. `data_base()`
scans for it; for the 291 plain data files it returns 0 on the first word.
**401 of 401 now decode.**

### Which file an ability plays, from the disc rather than from its name

Two tables in BATTLE.BIN answer it, and neither was being read:

| symbol | address | contents |
|---|---|---|
| `g_battle_effect_ability_ids` | `0x801b63f0` | 492 s16: ability id -> effect id, masked `& 0x1ff`, negative = no effect file |
| `g_battle_effect_disc_entries` | `0x801b53e8` | `{sector, size}` per effect id, what `battle_effect_load_lba_and_size` reads |

Resolving the sector back to a file name proves the name convention instead of
assuming it: ability 15 (Holy) -> effect 15 -> lba 61383 -> `EFFECT/E015.BIN`.

### What a code overlay still will not give you

Holy decodes to 60 cells, 11 sprite sequences, 16 emitters, 5 particle channels
and a 229-frame timeline -- and its *main* image is in none of them. Its script
registers three native handlers, and `fft_decomp` names them:
`effect_e015_update_ring_mesh_state`, `..._ring_stack_mesh_state` and
`..._textured_quad_grid_4x4`. The first builds a ring of 2x32 textured quads
around the target from three lattice rows of nine points, and spins it:

    work->spin = (work->spin + lerp(spin_speed_start, spin_speed_end, j)) & 0xfff

`0xfff` is 4096, a whole turn, so the ring comes round every revolution -- that
is the full 360 you see on the console, and it is 64 textured quads rotated in
3D per frame. **Read the overlay's handlers before promising an effect**: the
data sections tell you what accompanies the effect, not what it mostly looks
like. For the TI the arbitration is easy here, because the part the renderer
cannot play is also the part that has no answer in four greys at 160x100.


## 13. "Too transparent" was not a layer bug: five checks against the decompilation

A main layer that reads as a ghost -- Stasis Sword's ice block, Night Sword's
blade -- sends you looking for a lost layer. It is worth checking the pipeline
against `fft_decomp` before changing anything, because every lever is named
there and each one can be counted on the disc:

| what | where it is decided | measured |
|---|---|---|
| is the part blended at all | `flags & 0x200` -> POLY_FT4 code 0x2e, else 0x2c | 22,512 of 22,916 blended; **all** of E163, E173, E015 |
| which blend mode | the part's tpage bits 5-6 | 19,092 B+F, 2,630 B+F/4, 727 B-F, 467 B/2+F/2 |
| which palette | `flags & 0xf` CLUT row, `& 0x10` bank, **computed, not stored** | row 0 throughout those three |
| the tint | `battle_effect_update_node_from_work`: a curve, or neutral `0x80` | neutral is x1, as modelled |
| per-texel opacity | the texel's bit 15 (STP) | 252 of 252 palette entries set it: nothing is opaque |

All five came back negative. Note the field order while you are there:
`battle_effect_sprite_part_t` is **`u16 flags` then `u16 tpage`** -- a decoder
that calls the first word `tpage` has them swapped, which is survivable only as
long as nothing reads the first word's bits.

### What was really wrong, and what was not

Really wrong: the renderer accumulated every part in float and clipped once at
the end. The console writes **15 bits**, so each primitive saturates and is
quantised to 5 bits a channel as it is drawn, in order. `_blend` does that now.
It changes little on these two effects -- they rarely cover a pixel twice -- but
it is the hardware's rule rather than a convenient one.

Not wrong at all: the look itself. These are additive sprites, and the render
has **nothing behind them**. Add the same frames to a background of 48 or 96 and
the ice block reads as a solid bright mass, which is what the console shows over
a lit battlefield. Judging an additive effect against black will always call its
broad, dim shapes transparent. Put a floor under them before deciding.


## 14. Transcribing the sprite path: nine levers, eight already right

`schedule()` was written by inference before the decompilation was found, so it
was read back line by line against `battle_effect_render_particle_sprite` and
`battle_effect_submit_sprite_to_ordering_table`. Eight of its nine levers were
already faithful, and each is countable, so the check is repeatable:

| lever | the source's rule | on the disc |
|---|---|---|
| sequence stepping | `frame_timer -= 2` a frame; ops 0x81 loop, 0x82/0x83 move | already transcribed, hold 0 freezing included |
| the spawn gate | `time % lerp(spawn_interval_start, _end, factor)` | every curve nibble is 0, so it is the start value |
| the particle count | `lerp(particle_count_start, _end, factor)` | likewise: 1..3, never curve-driven |
| placement | `work_kind_flags` bits 12-15 pick the list-node kind | the catalogue has only WORLD (2,672) and WORLD\_UNSCALED (555) |
| the camera zoom | `g_battle_camera_zoom` | `battle_camera_init_defaults` sets it to `ONE`; zoomed out is 0xC00 |
| the primitive code | 0x2c, or 0x2e when `flags & 0x200` | 22,512 of 22,916 polygons blended |
| the blend mode | the tpage's bits 5-6 | 19,092 B+F, 2,630 B+F/4, 727 B-F, 467 B/2+F/2 |
| the tint | a curve, else neutral `0x80` | as modelled |
| the frame buffer | 15 bits, saturating per primitive | **this one was wrong**, see section 13 |

### The lever that was not in the engine at all

The renderer shrank the effect to fit its frame: Night Sword's 114x275 sweep
scaled by **0.84** into a 256x240 default. That thins its blade to two pixels
and takes the contrast with it, and the blade then reads as too faint when
nothing about it is. The console draws at 1:1 and lets what overflows overflow.
`--render` now grows the frame instead, and says so. Judging an effect through
a shrink is the same mistake as judging it against black.

### The last two, since transcribed

`node->screen_rotation_angle` and the ordering table's depth modes. Neither
changes how bright anything is -- which is why neither explained a faint layer
-- but both decide what a frame looks like:

* **the turn.** `work_kind_flags` bit 1 orients a particle to its velocity, and
  `submit_sprite` then turns the part's own corners by
  `[[cos, -sin], [sin, cos]] >> 12` before adding the position. **557 of the
  disc's 3,227 emitters set it, across 225 effects** -- Flare turns 2,862 of its
  6,334 draws over a 180-degree spread, which is what gives its ring its
  tangential streaks;
* **the depth.** A sprite's ordering-table index is its projected z over four,
  which the sequence op's third byte overrides: 1 and 5 pull it forward by 8 or
  16, 2, 3 and 4 pin it to 8, the far end or 16. Catalogue-wide: mode 1 on
  21,265 draws, 2 on 3,960, 0 on 1,560, 4 on 664, 5 on 109, 3 on 10. **A
  negative index is not clamped, the sprite is dropped** (`if (depth < 0)
  return;`). Drawing by channel order instead put the 4,743 pinned sprites in
  the wrong layer.


## 15. Three of the twenty-five families, and the limit of an inference

`effect_mesh.py` now carries three of the 25 native renderer families,
transcribed from `src/effect/effect_e015_*`:

| family | what it builds | overlays |
|---|---|---|
| `RingStack` | 9 lattice rows of 8, 8 rings of 8 quads, texture scrolling in v | 31 |
| `RingMesh` | 3 rows of 32, 2 rings of 32, **height shifted left 2** | 32 |
| `QuadGrid4x4` | 16 quads of 64 px in **screen space**, never projected | 27 |

The ring mesh is the stack's renderer with other counts, so they share one
lattice; the source builds one quadrant and rotates it into the other three,
which is the same 32 points as walking the whole turn in 0x80 steps.

### Which routine is which handler: read it, do not infer it

`step_child_timeline` gives `slot = (selector & 7) - 1`, and script opcode 0x06
fills the slots (`command[1] >> 2`). Holy's four registrations hold routines 7,
8, 9 and 8, and **`g_battle_effect_file_ptrs` at 0x801b50d0 in BATTLE.BIN is a
static table of 198 routine addresses** that answers the rest outright:

    [7] 0x801C2500   effect_e015_update_ring_stack_mesh_state
    [8] 0x801C39F8   effect_e015_update_ring_mesh_state
    [9] 0x801C4FC8   effect_e015_update_textured_quad_grid_4x4

It had been inferred -- ids ascending onto the overlay's functions in address
order -- and the inference was right, which is luck rather than method. The
table costs one read and needs no luck.

### What actually made them slabs

Not the mapping. **These are POLY_GT4**: each vertex carries its own colour and
the hardware interpolates it across the primitive. Painting the whole quad in
one of the four turns a graded wash into a flat plate, which is what the 4x4
grid and the ring mesh looked like, and it flattened every sprite quad too.
`_tri` interpolates the three corner colours barycentrically now, 0x80 neutral,
and all three renderers run by default.


## 16. Where an emitter sits: the spawn position and its seven anchors

`battle_effect_interpolate_emitter_spawn_position` lerps three words --
`spawn_position_start` at 0x14 against `spawn_position_end` at 0x1a -- by the
curve the caller read, and `placement_flags` bits 9-11 then name what that
position is measured from:

| anchor | emitters | a preview can place it |
|---|---|---|
| target | 1,717 | yes, it is the point drawn about |
| caster | 525 | yes |
| parent particle | 347 | **no** |
| map centre | 267 | **no** |
| cursor tile | 186 | yes |
| world | 132 | yes |
| tracked entity | 53 | yes |

**614 of the 3,227 anchor somewhere a preview has no way to find**, and saying
so is better than drawing them at the origin and calling it done.

### Read the curve before believing the pair

3,154 of the 3,227 have `start != end`, which looks like almost every emitter
walking across its effect. It is not: only **262 carry a spawn-position curve**,
and without one the factor is 0 and the lerp returns the start for ever. The
pair that actually moves anything is the **260** that have both, across 138
effects -- Holy and the three Cures among them, neither sword. Counting the
pairs alone would have overstated it twelve-fold.


## 17. The unit system, and why nothing was oversized

`battle_effect_resolve_target_render_coords` is the last place a scale factor
could have hidden, and there is none in it. It is a pure translation:

    tile centre = (tile_x * 28 + 14, -(tile->height * 12), tile_y * 28 + 14)

A unit target copies the unit's own screen-coordinate record; with the terrain
flag set, Y is lowered to the tile's surface including its depth. **A tile is 28
world units across and a height level is 12**, Y negative upward. A preview that
draws about the target is already standing on that point, so it needs nothing
from this function -- which is the useful answer, not a disappointing one.

### Measure a mesh in tiles, not in pixels

The ring mesh looked alarming at "511 x 650 px" until it was put back into the
game's own units, where it is nothing of the kind:

| what | at its widest | in tiles |
|---|---|---|
| the beam (ring stack, f10..110) | 30 x 368 | 1.1 wide, **13.1 tall** -- 30.7 height levels |
| the second beam (f152..176) | 128 x 304 | 4.6 x 10.9 |
| the flash (ring mesh, f80..116) | 510 x 472 | **18.2 x 16.9** |
| the ring on the victim (f132..152) | 68 x 32 | 2.4 x 1.1 |
| the halo (4x4 grid) | screen | 256 x 256, no world at all |

A column thirteen tiles tall rising out of the victim and a ring eighteen tiles
across are exactly what the console shows: the beam leaves the top of the frame
and the flash fills it. **Pixels on a canvas you chose the size of say nothing**
-- the frame was 256x618 here and 256x240 there. Convert to tiles before calling
a mesh too big.


## 18. Five families of twenty-five, and what wiring one still needs

`effect_mesh.py` carries five now, transcribed from their own sources:

| family | what it builds | where | overlays |
|---|---|---|---|
| `RingStack` | 9 rows of 8, 8 rings of 8 quads | world | 31 |
| `RingMesh` | 3 rows of 32, 2 rings, height `<< 2` | world | 32 |
| `SummonMesh` | 9 rows of 17 that **narrow by the cosine and climb by the scaled sine** -- a dome, 8x16 quads | world | 12 |
| `QuadGrid4x4` | 16 quads of 64 px | **screen** | 27 |
| `PolarMesh` | 6 rings of 5 mirrored into four quadrants, 80 quads, the centre drifting by a velocity that gains an acceleration **between rings** | **screen** | 23 |

That is 125 of the 198 handler instances. The dome's view is the ring mesh's
but for the row-radius terms a dome has no use for, so they share a class; the
polar mesh has its own, where `radius_start` sits at 0x20 and the angle at 0x44.

**One detail that is easy to lose**: in the polar mesh every curve is read at
the handler's own frame counter, which advances once an update -- except the
scroll, which the source reads at the caller's frame. They are not the same
number once a keyframe repeats.

### Wiring them all: one offset and one address an overlay

**Every one of the 110 overlays loads at the same address, 0x801c2500.** So a
routine pointer in `g_battle_effect_file_ptrs` is really an *offset into
whichever overlay is loaded*, and `target/effect.yaml` says which function sits
at that offset in a given overlay, and where that renderer's own brightness
table is. `effect_handlers.py` reports the pair into `effect_handlers.json`,
committed so the decoder does not need the decompilation checked out:

    offset = g_battle_effect_file_ptrs[id] - 0x801c2500
    family, table, width = overlays[effect][offset]

125 handlers over 85 overlays, and no ordering, no guess.

### Walk the phase channels, not only the child's

With that table in hand only **25** effects resolved a renderer, which was the
reading and not the table: `mesh_actors` walked `child_channels` alone. The
parent timeline runs phase 1, then the child, then phase 2, and each has its own
five channels; a handler called from a phase channel is called the same way.
Walking all three took it to **84 effects and 195 wired instances** -- ring stack
61, polar mesh 43, ring mesh 40, 4x4 grid 38, summon dome 13 -- and fixed the
child's own windows, which were short by phase 1's frames.

**An effect's picture can be entirely native.** E071 has no sprite timeline at
all, so a renderer driven by that timeline produced nothing for it; its length
is its handlers' windows instead.
