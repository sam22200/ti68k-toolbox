# Combat effects: what the spec costs, measured

2026-11-27. The user supplied a combat-FX specification (focus modes, a phase
timeline, a catalogue of signature techniques and classic magic, reusable
primitives) and asked what it would cost, with a ceiling of 300 KB for effects
in the core.

The spec says itself that it inspected neither the code nor the memory budget.
This document supplies both. Nothing here implements an effect; it prices one.

## The answer on the ceiling

**300 KB does not fit, and the catalogue does not need it.**

[CHAPTER1_ARCHIVE_PLAN.md](CHAPTER1_ARCHIVE_PLAN.md) leaves a planning margin of
**294,559 bytes** (2,700,000 allowance, less the 131,072 reserve, less the
405,033 current core). Spending 307,200 on effects puts Chapter I **12,641 bytes
over**; even reading the ceiling as 300,000 bytes it is 5,441 over.

The catalogue costs about **12,600 bytes**, and roughly 38,000 if the sizing
below is optimistic by a factor of three. **40 KB is the recommended envelope**,
leaving 254 KB of chapter margin.

Sized with the ZX0 ratio measured on this game's own art, 0.68 (613 actor frames,
127,504 raw bytes, 86,673 packed), not an assumed ratio.

| Family | Frames | Raw | ZX0 |
|---|---|---:|---:|
| Slash arcs: Split/Crush Punch, Mighty Sword, physical | 24 at 24x24x3 planes | 5184 | 3525 |
| Crystals and shards: Ice 1-4, Stasis Sword, Lightning Stab | 32 at 16x16 | 3072 | 2089 |
| Orbs and explosions: Ultima, Flare, Dark Holy, Giga Flare, Holy Explosion | 12 at 32x32 | 4608 | 3133 |
| Columns: Holy, Raise, Bolt | 8 at 16x48 | 2304 | 1567 |
| Organic cloud: Bio 1-3 | 6 at 32x32 | 2304 | 1567 |
| Projectile heads: Wave Fist, Earth Slash, Arm/Leg Aim | 6 at 16x16 | 576 | 392 |
| About 45 `EffectDef` records of 8 bytes | | | 360 |
| **Total** | 88 | 18048 | **12633** |

Rings (Haste, Slow, Protect, Shell, MBarrier, Stop), particles (Cure, Raise, the
Night Sword drain), beams, auras, ground waves, flash and shake cost **nothing**:
the spec is right to want them procedural, and that is what makes the catalogue
cheap. Boss-exclusive effects can live in chapter data; the package format
already allows it.

## Storage is the emptiest of the four budgets

| Budget | Available | Estimated need | Source |
|---|---:|---:|---|
| Archive | 294,559 B | ~12,600 B | `CHAPTER1_ARCHIVE_PLAN.md` |
| Module execution zone (code) | 9,510 B | ~6,000 B | `campaign/data/modules.h` |
| Free installed RAM, 13 units | **436 B** | ~224 B | `INSTALLED_RAM_ABI99.md` |
| Frame budget during an action | 96,576 cy | ~11,660 cy | measured below |

Three consequences the spec could not know:

**The effect code cannot be resident.** `fftbat` has 476 bytes left under
TTPack's ceiling ([RESIDENT_BUDGET.md](RESIDENT_BUDGET.md)). The scheduler, the
primitives and the focus layer are kilobytes: they belong in a module image, as
`ffbm`/`ffbn` already are. The image must stay **under 9,510 bytes**, the
execution zone; below that the zone does not change and RAM is untouched, above
it the zone grows byte for byte against the 436 free.

**The darkening cannot have its own buffer.** 4,000 bytes against 436 free is
not a trade, it is a refusal. It must happen in place in the screen copy the
engine already keeps (`bg_usable`, `SAVE_K`) -- which is also what makes it free
on the following frames, since that copy is reused while the camera stands still.

**The spec's plane size is wrong for this build.** It assumes 3,840 bytes a plane
and 7,680 for two. This build uses `RT_DRAW_ROWS=100`: **3,000 a plane, 6,000 for
two** ([CORE_RAM_MARGIN.md](CORE_RAM_MARGIN.md)). That is exactly the check the
spec asks the reader to make.

## The focus layer, measured

The spec leaves the darkening as "a LUT per byte or bitwise operations". From
`runtime/core/rt.h` -- `grey = light + 2 * dark` -- both of its levels are a few
bitwise ops, verified exhaustively over the four levels:

```
Dark 1   v -> min(v+1, 3)        d' = d | l       l' = d | ~l
Dark 2   0,1 -> 2 ; 2,3 -> 3     d' = ~0          l' = d
```

Measured with `experiments/gray/darken.c`, MC68000 datasheet cycles under
`ti-cycles`, 160x100 and both planes: **46,638** for Dark 1 row-unrolled, 25,234
for Dark 2. The full table, and the two general rules it overturned (pointers
lose to indexing on a five-iteration loop; widening the work to flatten the loop
costs more than it saves), are in
`.claude/skills/ti89-c-dev/reference/ti68k-performance.md`.

**A hand count is not a measurement.** The estimate offered before this ran was
37,000, low by 26%.

## The design falls out of the engine

`fft.c` already keeps its screen copy **a quarter of the rows per frame**
(`W->bg_part`, `RT_H/4`), charged `SAVE_K` = 13 kilocycles, over the same
five-words-per-row geometry; its comment records that halves at 26k went over the
budget. So the dimming is that same quartered pass with two more operations:
**11,660 cycles a frame**, about what the copy already pays. Four frames at 20 Hz
is 200 ms, which lands inside the spec's own "Focus, 150-300 ms, darken
progressively". The progressive dimming is not a feature to build; it is what the
existing structure produces.

TARGET mode follows: darken the copy, then draw the focused unit with the cover
mask `draw_unit_covered` already applies. The spec's warning -- do not simply
redraw the target on top of everything -- is satisfied by the existing pipeline.

## What is left for the animation

Measured baseline during an action, 13 actors, `check_capacity_render.py --case
battle-act`:

| | Mean | Ordinary peak | Free at the peak |
|---|---:|---:|---:|
| `battle-act` | 199,218 | **263,424** / 360,000 | **96,576** |

Dark 1 in one frame would reach 310,062 and still pass. That projection was
**optimistic by more than half**: the measurement of the built layer is below.

## Open, and deliberately not done

- The darkening is C. 46,638 cycles for 500 longs is 93 a long for six
  operations; a `movem` routine would plausibly halve it. That is assembly, and
  the project rule is to ask first.
- Per-effect RAM is estimated, not measured: about 224 bytes of the 436 free for
  the runtime record plus 32 particles. It means **one effect at a time**.
- `battle-act` is a 90-frame diagnostic case, not a played fight.
- Datasheet cycles throughout. No emulator or calculator run.
- No effect, primitive, asset or scheduler exists yet. This is a costing.

## Step 1 built and measured (2026-11-27)

`games/fft/fx_focus.inc`, behind `FFT_COMBAT_FX`, off by default. It dims the
finished picture and redraws one unit at normal brightness through the cover mask
the ordinary pass just used, so the scenery still occludes it. The HUD is composed
afterwards and stays bright. No effect, primitive or asset: this is the focus
layer, the palette and the restoration, and nothing else.

The second, deeper level of the spec was built, reviewed on the picture and
**dropped**: it leaves the scenery on dark grey and black alone and the relief
stops reading. With one level, four greys offer no fade, so the dimming arrives
in a single step; a gradual focus would need dithering, which is not done.

Restoration costs nothing. `W->bg`, the kept screen copy, is never dimmed, so
ending an effect is one assignment and the next frame simply stops dimming. That
also leaves `copy_view()`'s PC invariant -- the copy equals the view -- intact.

### Measured, 13 actors

| | Mean | Ordinary peak | Free at the peak |
|---|---:|---:|---:|
| `battle-act`, focus off | 199,218 | 263,424 | 96,576 |
| `battle-act`, **focus on** | 254,271 (+55,053) | **321,476** (+58,052) | **38,524** |
| `battle-menu`, focus on | 248,903 (+55,053) | 322,864 (+60,774) | 37,136 |

**The headroom for the animation is 38,524 cycles a frame, not the 85,000
projected above.** The projection added the 46,638-cycle dimming loop to
`UNIT_K`, 20 kilocycles for the redrawn unit; measured end to end the layer costs
55,053, of which the redraw is only about 8,400. The budget model now charges the
measured figure (`FX_DARK_K` 55), not the projection.

That is of the order of 30 masked 16x16 blits a frame while a focus is held. The
spec's particles and primitives still fit, but they have to be counted.

Reproduce: `check_capacity_render.py --case battle-act --combat-fx --actors 13`,
against the same command without `--combat-fx`.

### One byte, and why it must stay one byte

The whole state is a single `u8` in `Work`: 0 off, 1..n unit n-1 stays bright,
255 dim everything. It fills the padding byte the struct already had.

This is not tidiness. `MOD_SIG` (`module.h`) is derived from `sizeof(Work)`, so a
`Work` that grows makes the core refuse **every already-built module image** and
the battle never starts -- `game_render()` finds all eight of its pointers null
and draws "Missing data". A first version of this layer used two bytes and did
exactly that, and the failure was first misread as the carrier running out of
room. Ballasting the carrier without the feature disproved it: it still runs at
57,637 bytes, 260 more than the build that failed. Keeping the state to one byte
means enabling this feature never forces a module rebuild.

### Verified

- `make -C games/fft/campaign test` and `packages`: exit 0, `MOD_ZONE` unchanged
  at 9,510. The suite's `sizeof(Work)` assertions pass in configurations other
  than the carrier's, which is what proves the byte really is free everywhere.
- Flag off: checksums `311B`/`7262` and peaks 263,424/262,090, the figures from
  before the layer existed.
- The four reviewed pictures are byte-identical after the repacking.
- The dimming maps are exact on the tone histograms: light grey and dark grey
  land where the table says, and GLOBAL leaves 48 white pixels, which are the HUD.
- A unit outside the view is correctly left alone (`vis`); with the camera placed
  on it, all eleven brighten. Two earlier readings of this said otherwise and
  were wrong: one read a frozen picture, the other let the tick advance between
  the two renders it compared.

## Step 2 built and measured (2026-11-27)

`games/fft/fx_play.inc`, behind the same `FFT_COMBAT_FX`. The spec asks for an
effect scheduler, a result applied exactly once, and reusable primitives. Two of
those three already existed.

### The schedule was already there

`st.act_t` runs the action with named phases, and the spec's timeline maps onto
them without adding a second clock:

| Spec phase | Engine |
|---|---|
| anticipation | `act_t < ACT_WIND` |
| focus | `ACT_WIND` .. `ACT_BLOW` |
| animation | `ACT_BLOW` .. `ACT_HIT` |
| **impact** | **`ACT_HIT`** |
| result, return | `ACT_HIT` .. `ACT_END` |

**The single impact needs no work.** The combat core applies the result at
`ACT_HIT`; the renderer reads it. Skipping or speeding the animation moves
`act_t`, not the result. What had to be proven is the other half -- that
presenting an effect never touches battle state -- and that is what
`check_combat_fx.c` does.

### The primitives carry no state

Every primitive is a pure function of `act_t` and two screen points: a ground
ring (the 2:1 diamond the map's own tiles use, so it sits on the ground instead
of floating), an eight-spoke burst, and a swept slash. **No effect state exists**:
nothing in `Work`, nothing allocated, nothing to reset, and the PC and the 68000
compute the same picture.

That is not elegance for its own sake. `Work` cannot grow without invalidating
every built module image (above), and the installed battle has a few hundred
bytes of RAM free. A stateless primitive sidesteps both. The ring comes first
because Haste, Slow, Protect, Shell, Stop, MBarrier and Cure are all that shape
with another radius and cadence.

### Measured, 13 actors

`battle-act` is the action *menu*, not an action in flight, so pricing the
schedule needed a case that walks the real timeline. `battle-fx` does, **in both
builds**, so the difference is the effect layer and not the action state.

| `battle-fx` | Mean | Ordinary peak | Free at the peak |
|---|---:|---:|---:|
| effects off | 198,684 | 270,598 | 89,402 |
| **effects on** | 252,834 (+54,150) | **322,068** (+51,470) | **37,932** |

Adding the case costs the existing ones 26 cycles a frame (`battle-act` 263,424
-> 263,450, `battle-menu` 262,090 -> 262,116) for the one test that steps the
timeline. Checksums `311B` and `7262` are unchanged, so the pictures are not.

### The acceptance test, and one check deliberately removed

`make -C games/fft/campaign combat-fx-test`, 212 checks, in `test`:

- the units and the RNG are unchanged after **every** frame of the timeline;
- the focus follows the schedule, read back from the one byte that holds it;
- the animation skipped lands on the same state as the animation played, which
  is the spec's acceptance criterion;
- leaving the action releases the focus in one frame.

A fifth check asserted that two renders of the same `act_t` give the same pixels.
It failed, and it deserved to: the engine spreads its HUD and screen copy over
several frames and animates the damage number on its own clock, so consecutive
frames differ for reasons that have nothing to do with this layer. It was testing
the engine, not step 2. It is gone, with that reason written where it was.
PC/68000 equality covers the ground it was aiming at, in `battle-fx`.

### Still not done

- Only three primitives. Beam, aura, ground wave, projectile and particles, and
  the per-ability table that picks between them, are step 3.
- Effects draw over everything rather than sorted by depth.
- The pictures reviewed are from a harness that forces `M_ACT` without a resolved
  action, which is why the damage number reads "Miss" in them.

## Step 3 built and measured (2026-11-27)

The remaining primitives and the per-ability table. One of the three things this
step was supposed to contain is deliberately not in it; that is below.

### The table, which is the point of the step

`fx_table` gives four primitives per outcome category -- gather, travel, impact,
settle -- for 26 of them, four bytes a row, `const` in the module image. No RAM.

It is indexed by `st.effect_kind & EFFECT_KIND_MASK`, the category the engine
already computes for `act_kind()`, so **there is no second classifier**. Moving an
ability to another look is editing one row, and the renderer never learns what an
ability is.

Six primitives now: ground ring, slash, burst, beam, column, motes. All still
pure functions of `act_t` and two screen points, so step 3 adds no state either.
Motes scatter from a hash of the index and the frame, never from the RNG, because
the acceptance test requires the picture not to consume it.

The full dispatch costs **346 cycles** more than step 2's hardcoded sequence.

| `battle-fx`, 13 actors | Mean | Ordinary peak | Free at the peak |
|---|---:|---:|---:|
| effects off | 198,684 | 270,598 | 89,402 |
| step 2, hardcoded | 252,834 | 322,068 | 37,932 |
| **step 3, table and six primitives** | 253,180 | **322,414** | **37,586** |

Each of the six categories rendered at the same frame gives a different picture,
which is what says the table dispatches rather than decorating a fixed sequence.

### Depth sorting: built, then removed

`seen()`, the per-pixel depth test the water glints already use, was wired into a
depth-aware dot so ground effects would sit in the map instead of over it. It is
compiled out when the chapter's water bank is in (`FFT_WATER_BANK`) -- that is,
**in the configuration that ships**. A depth test that does nothing where it
matters reads like a feature, so it was taken out rather than left in.

Effects therefore draw over the scenery. Doing this properly means a depth test
that survives the water bank, and that is its own piece of work.

### Projectile and ground wave are not separate primitives

The spec lists eight. A projectile is a truncated beam that advances and a ground
wave is a ring that grows, so adding them as rows would have widened the table
without putting anything new on screen. They become two rows once their
parameters are chosen, not two primitives.

### Still open

- Depth, above.
- The table covers outcome categories, not individual abilities. The spec's
  signature techniques -- the Holy Sword family, Night and Dark Sword, Dark Holy,
  Ultima -- need their own rows keyed on the ability, not its outcome.
- No asset has been drawn. Everything so far is procedural, which is why the
  archive cost of the whole layer is still zero.

## Checked against the original (2026-11-27)

`games/fft/tools/effect_watch.py` watches the real game play an ability.

**The focus layer is right.** The original dims the scene to night blue and keeps
the caster lit: Wish for 3.02 s, Potion for 1.62 s. Step 1 was built from a
specification and now matches an observation.

**The step 3 table was wrong on one point, and now carries the measurement.**
Thirteen of Gariland's abilities were watched:

| plays in full light | dims |
|---|---|
| Dash, Throw Stone, Attack, **Heal** | Wish 3.03 s, Potion 1.63 s, Antidote 1.63 s, Eye Drop 1.97 s, Echo Grass 1.40 s, Phoenix Down 1.47 s |

**Heal and Antidote both strip a status and the original stages them oppositely.**
So what decides is not the outcome but the kind of action: a physical skill or a
Squire ability plays in full light, an item or an incantation takes the stage --
and `fx_table` is keyed on the outcome. The column is right where it is measured
and honest where it is not: six rows carry `D_NO`/`D_YES` from the original, the
other twenty are marked `D_GUESS`, "NOT OBSERVED". `fx_schedule` now consults it,
so a category that does not dim never takes the focus at any phase.

Re-keying the table on the kind of action is **not** done. Thirteen observations
all come from Gariland and all fall on one side of the skill/item split; deciding
a new axis from them would repeat exactly the mistake just corrected. The
prologue, with Agrias and Gafgarion, is where other kinds of incantation live --
and that is finally a reason to go there.

Timing is still not resolved. The total is now observed -- about 3 s for Wish --
but relating that to an effect file's record count needs the effect number for
each ability, which is the file's name on the FFHacktics list.

## The DIM column is keyed on the wrong thing (2026-11-27)

Agrias's Holy Sword, watched in the prologue through `oracle.py --prologue`,
**dims the scene for 1.00 s**. It deals damage, exactly as a plain Attack does,
and the original stages them oppositely.

So the column cannot be a function of `st.effect_kind`. Its six measured rows
are right only because of which abilities happened to be watched; `AB_DAMAGE`
carries `D_NO` from Dash, Stone and Attack and is wrong for a Holy Sword.

The candidate key is the ability's own flags -- `abilities[a].flags` with
`AF_CAST` and `AF_CHARGE` -- which the renderer can already read through
`st.abil`. Re-keying is **not done**, and the column should not be trusted for a
cast ability until it is. `games/fft/RE_NOTES.md` has the measurements.

## The DIM column cannot be derived (2026-11-27)

Both candidate keys are disproved by measurement, each by a counter-example:

- **`st.effect_kind`**: a Holy Sword deals damage exactly as a plain Attack does,
  and the original dims one and not the other.
- **`abilities[].flags`, `AF_CAST`/`AF_CHARGE`**: Dash and Wish both carry
  `0x00`, and Wish dims for 3.03 s while Dash plays in full light. Night Sword
  dims without `AF_CAST`; nothing observed carries `AF_CHARGE`.

So the column should hold the fact **per ability**, as the original does -- an
ability points at an effect file and the file carries its own staging -- instead
of computing it from a property of the ability. `fx_table`'s `D_GUESS` rows
remain guesses and the measured ones remain right only for what was watched.

Measured so far: Stasis Sword 1.10 s, Night Sword 1.07..5.47 s, Wish 3.03 s,
Potion 1.63 s, Antidote 1.63 s, Eye Drop 1.97 s, Echo Grass 1.40 s, Phoenix Down
1.47 s; Dash, Throw Stone, Heal and Attack in full light.

And the staging is a **colour wash**, not a dim: blood red for Night Sword, night
blue for Agrias. Four greys cannot tint, so which grey each family takes is a
decision for the art, not a measurement. `fx_focus.inc` currently darkens by one
level for everything.

## The staging is stored per ability (2026-11-27)

`fx_play.inc` holds it the way the original does: **a fact against the ability**,
not something computed from one of its fields. Two bitmaps of 64 bytes, `const`
in the module image -- 128 bytes, no RAM:

| | |
|---|---|
| `fx_staged` | one bit an ability: does the original stage it? |
| `fx_known` | which of those bits is **measured** rather than defaulted |

The second map is the point. The table it replaces could not tell a fact from a
blank, and its six "measured" rows were right only by accident of which
abilities had been watched. A reader can now see exactly what was observed.

| measured, stages | measured, full light |
|---|---|
| Wish 3.03 s, Stasis Sword 1.10 s, Night Sword 1.07..5.47 s, Potion 1.63 s, Antidote 1.63 s, Eye Drop 1.97 s, Echo Grass 1.40 s, Phoenix Down 1.47 s | Dash, Throw Stone, Heal, the plain Attack |

**Anything unmeasured defaults to not staged**, and that is a choice with a
number behind it: the dimming costs 55 kilocycles a frame, so spending it on a
guess is worse than leaving an effect plain until somebody watches it. A missing
effect is visible and cheap to fix; a frame over budget is neither.

The header of the table carries both disproved keys with their counter-examples,
so neither is tried again: `st.effect_kind` falls to a Holy Sword and a plain
Attack both dealing damage and being staged oppositely, `abilities[].flags` to
Dash and Wish both carrying 0x00.

`check_combat_fx.c` drives by ability now -- Wish on one side, Dash on the other
-- so it checks two measured facts instead of an invented rule, and one check
holds the two maps together: **every bit of `fx_staged` must also be set in
`fx_known`**. Without it the second map would be a comment, and a staged guess
could be written without anything noticing. 257 checks.

The other half of `fx_table`, which primitive plays at which phase, is **not**
measured: those were chosen. Only the staging comes from the original.

## One wash, and the duration per ability (2026-11-27)

**The grey does not vary per family, and the measurement is why.** The original
stages with a colour wash -- blood red for Night Sword, night blue for Agrias --
and four greys carry no hue. The one axis the LCD does carry is luminance, and
`tools/effect_stage.py` fires on `lum < bright * 0.70`: a **darkening** of at
least 30 %. All eight measured abilities, the five items included, were measured
moving the same way. Giving items a pale wash would have been a pleasing art
rationale contradicting the data it claimed to rest on.

The five candidate maps were rendered on the same Gariland frame and compared
side by side (`ti-view --decision`, 2026-11-27):

| map | verdict |
|---|---|
| **DARK1** `d'=d\|l, l'=d\|~l` | **kept, for every family** |
| LIGHT1 `d'=d&l, l'=d&~l` | contradicts the measurement; the focused unit also loses its white outline against a pale scene |
| DARK2 | already rejected on the picture: the relief stops reading |
| BLEACH | fails symmetrically on the light side |
| INVERT | the unit does not detach; reads as a flash, not a focus |

**What the measurements do separate per ability is the hold.** It is carried over
as a proportion, not as the second itself:

> this engine's action is `[ACT_WIND, ACT_END)` = 39 ticks = 1.30 s at 30 fps,
> and the longest measured wash is Wish at 3.03 s = 91 ticks.

Six of the eight last longer than the whole action, so literal seconds saturate
and stop telling the abilities apart -- the one thing the duration is for. Each
hold is `round(39 * seconds / 3.03)`:

| ability | measured | ticks |
|---|---|---|
| Night Sword `0x0a5` | 1.07 s | 14 |
| Stasis Sword `0x09b` | 1.10 s | 14 |
| Echo Grass `0x178` | 1.40 s | 18 |
| Phoenix Down `0x17d` | 1.47 s | 19 |
| Potion `0x170`, Antidote `0x176` | 1.63 s | 21 |
| Eye Drop `0x177` | 1.97 s | 25 |
| Wish `0x098` | 3.03 s | 39 |

A hold ending before `ACT_END` is not a cut: the kept screen copy is undimmed, so
the next frame simply stops dimming. A sword art's wash now clears just after the
impact and the settle plays in full light.

24 bytes of `const`, no RAM: `Work` is full and growing it changes `MOD_SIG`.
The eight-entry scan runs per staged frame and measured **6 cycles** on the peak.

**Two gaps this closed, and a number to correct.** The `battle-fx` cycle scenario
set no `st.abil`, so once the key moved to the ability it stopped dimming
altogether -- its 271,042-cycle peak was a frame with no wash. Driving it with
Night Sword put the dimming back, at 324,462.

**That second figure was still wrong**, and in the same way: the scenario cast
from unit 0 at unit 1, a pair `fx_play` rejects as not both visible, so the
primitives never drew either. It measured the wash alone. Casting from 8 at 10,
the pair the effect actually plays between:

> **peak 351,090 / 360,000 -- 8,910 cycles of margin.**

That is the real cost of the layer, and it is much closer to the ceiling than
either earlier number suggested. Any further effect work starts from 8,910, not
from the 35,538 the previous line claimed.

**The outline this margin refused (2026-11-27).** The effects are drawn white and
vanish over the pale half of a washed picture, so the project's own visibility
rule -- a one-pixel outline, as every unit sprite gets -- was implemented and
measured. Every primitive is a pure translation of its anchor points, so the
dilation is exact: the same shape drawn black at the four neighbours, all four
before the white centre.

| passes | peak |
|---|---|
| 1, as shipped | 351,090 PASS |
| 2, a single offset | 362,072 over |
| 3 | 373,268 over |
| 5, the full outline | 407,284 over |

**Not even one extra pass fits.** Halving the dot counts to pay for a 2-pass
outline does fit, at 355,368, but the picture does not improve: a one-pixel halo
around a one-pixel dot is four blacks for one white, which eats a scatter instead
of framing it. An outline makes a *silhouette* read, and these primitives have no
interior. The route to readable effects is solid shapes first -- the original's
own crescents, cone and orbs are in `EFFECT/E173.BIN` (Night Sword) and
`E163.BIN` (Stasis Sword), decoded by `tools/effect_rip.py` -- and only then an
outline around them.

`check_combat_fx.c`: 302 checks, including a staged ability whose hold ends
before the action, and an assertion that every staged ability carries one.

**Limits.** The abilities classed "full light" mean "no 30 % dip", not "no wash":
a brightening effect would have been invisible to the detector. And Night Sword's
1.07..5.47 s spread is wide enough to deserve a second pass before it is trusted.

## The first solid shape, read off the original's atlas (2026-11-27)

The outline failed on a scatter of dots, so the shapes came first. `E173.BIN` is
Night Sword (the effect belongs to an ability by its file **name**: there is no
table, and `effect_names.json` holds the list). `tools/effect_rip.py --frames`
gives its animation, and frame 0 is the whole look:

> cell (56, 48), **40x136**, quad spanning y -26..110 about the anchor

A tall spike, point up, base at the target, about three and a half times a unit's
height. At the project's 0.6 sprite scale: **24x82**. `fx_spike` draws that.

**It is cheaper than the dots it replaces**, which is the opposite of what adding
an outline to the scatter did:

| | peak |
|---|---|
| the dot scatter, no outline | 351,090 |
| the dot scatter + a 1-pixel outline | 362,072 (over budget) |
| **the solid spike, outlined and dithered** | **350,714** |

Two things buy that. `fx_span` fills **whole bytes** instead of calling `fx_dot`
per pixel -- one touch per eight pixels on each plane -- and the silhouette's
outline is two dots a row rather than four per lit pixel.

**The body is a checkerboard, not an aplat.** Filled solid, the spike hid the very
unit the focus had just kept bright. The original's effects blend additively and
the unit shows through them; four greys have no alpha, so every other pixel is
left as it was. It costs nothing -- the pattern is ANDed into the span's mask.

Still open: the effect is not depth sorted, the other five primitives are still
the chosen ones, and only `AB_DAMAGE`'s impact and settle use the spike so far.

## The original's order, captured (2026-11-27)

`tools/effect_capture.py` runs the original from a save state and, around each wash,
keeps the frames and reads two things **per frame**: the luminance, which says when
the picture darkens, and every unit's HP, which says when a blow lands. The question
was an ordering, so it was measured as one rather than judged off a screenshot.

Stasis Sword, Orbonne, wash at frame 2901:

| | |
|---|---|
| wash −22 | the scene normal, Agrias at rest |
| wash −12 | she draws |
| wash −2 | **the blade fully raised, in full light** |
| wash +0 | the swing goes out, and **the picture darkens on that frame** |
| wash +36 | **the effect erupts** on the target, 0.6 s later |

Two corrections follow, and both are now in `fx_play.inc`:

1. **The wash starts at `ACT_BLOW`, not `ACT_WIND`.** Starting at the wind-up dimmed
   a preparation the original plays lit, and charged `FX_DARK_K` for eight frames
   that should not have paid it.
2. **The spike plays at phase 3, not at the impact.** The original leaves 0.6 s
   between the darkening and the effect; phase 2 is the four flash frames, so the
   spike belongs after them, from `ACT_HIT + 4`.

The caster stays lit until `ACT_HIT` and the target takes over there: on the original
the lit figure is the caster while the blade travels.

Peak **358,252 / 360,000**, 302 checks. The margin is down to **1,748 cycles** -- the
wash now covers frames where the spike also draws, where before the two barely
overlapped. That is tight enough to be the first thing to look at before any further
effect work.

**Durations, measured again on this run:** Night Sword 1.83 s and 2.52 s, Stasis Sword
1.10 s. The 1.07 s in the hold table was the short end of the spread, and our whole
action window is 1.30 s -- shorter than one of the original's washes. The effects
reading as too quick is a consequence of that compression, not of the hold table.

**A trap on the way.** The first capture returned exit 0 with no events at all. What
said it was wrong was not the absent wash but the absent **HP changes over 3,100
frames**: the save state held a battle that was not advancing. Without that second
reading, "no wash detected" would have looked like a fact about the game instead of a
fact about the state.

