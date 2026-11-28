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

