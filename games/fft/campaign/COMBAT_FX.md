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

## A staged action runs as long as its wash (2026-11-27)

The holds were a proportion because the action window was fixed at 39 ticks = 1.30 s,
shorter than one of the original's washes. Now a staged ability runs its action for as
long as its wash lasts (`fx_act_end`), so **the holds are the measured second itself**:

| ability | measured | ticks from `ACT_BLOW` | action ends |
|---|---|---|---|
| Stasis Sword | 1.10 s | 33 | 46 |
| Echo Grass | 1.40 s | 42 | 55 |
| Phoenix Down | 1.47 s | 44 | 57 |
| Potion, Antidote | 1.63 s | 49 | 62 |
| Night Sword | 1.83 s | 55 | 68 |
| Eye Drop | 1.97 s | 59 | 72 |
| Wish | 3.03 s | 91 | 104 |

An ability that is not staged keeps exactly the length it always had, and the plain
Attack is not staged, so the pacing the rest of the suite checks is untouched.

**The lengthening is cheap**: with no effect drawn it peaks at **341,684**, below the
350,714 of the previous state, because the wash starting at the blow saves more than
the extra frames cost.

### The spike had never been measured at its full height

Walking the action to its real end exposed this. The cycle scenario used to walk
`[ACT_WIND, ACT_END)`, and the frames it reached drew the spike at **32 rows, never at
82**: `grow` had not got there yet. The figures that said "the solid shape is cheaper
than the dots it replaces" were measured on a third of a spike.

At full height it costs about **1,220 cycles a row**:

| `FX_SPIKE_H` | peak |
|---|---|
| 82, the original's proportion | 424,734 over |
| 48 | 383,580 over |
| 32 | 364,110 over |
| 24 | 354,480 PASS |
| **20** | **349,588 PASS** |

So the budget affords **20 rows where E173 asks for 82** -- a quarter of the original's
proportion. The spike is now a cone at the target's feet rather than the column the
original raises. That is a measured limit, not a choice: to draw it at its true size
the cost has to come from somewhere, and the obvious reserve is the 55,000 cycles the
dimming spends every frame.

The scenario now walks to `fx_act_end()`, so the half where the wash and the spike
overlap is covered. **That is the fourth measurement in this file that reported a peak
for frames where the feature was not running**, after the missing `st.abil`, the
invisible unit pair and the rotation with no occluder.

## The dimming is at its floor in C (2026-11-27)

It costs 46,638 cycles a frame and gates everything else: the spike is capped at 20
rows of E173's 82 because of it, and the depth test was refused for the same reason.
So it was attacked. `experiments/gray/darken.c` under `ti-cycles`, fresh:

| variant | cycles |
|---|---|
| dark1 visible 160x100 | 49,212 |
| dark1 flat, 3000-byte plane | 70,534 |
| dark1 visible, pointers | 51,812 |
| **dark1 visible, unrolled (what ships)** | **46,638** |
| dark2 visible, unrolled | 25,234 |

Four things were tried and none of them works:

- **Dimming less area.** Measured on a real battle frame: **no row is entirely black
  and only 6 % of words are ones Dark1 leaves alone**. There is no wasted region to
  skip; the loop does useful work on 94 % of the screen.
- **A cheaper map.** Dark2 is 21,404 cycles cheaper but was rejected on the picture
  long ago -- it leaves dark grey and black alone and the relief stops reading. Every
  cheaper map collapses the four levels to two.
- **Dimming one row in two.** It halves the cost exactly, and at 160x100 it reads as
  **stripes**, not as a half-strength veil: the eye does not blend rows at this scale
  and the focused unit is lost in the banding.
- **A checkerboard** like the spike's. It changes which pixels are written but still
  reads and writes every word, so it saves nothing. Dismissed on the cost, not tried.

What remains is not a C change:

1. **68000 asm.** `movem.l` moves five longs in one instruction where the C loop issues
   five moves. Estimated around 30 %, so roughly 46,638 -> 33,000, which would take the
   spike from 20 rows to about 31. An improvement, not a transformation, and the
   project asks before any asm is written.
2. **Not dimming every frame.** During a wash with a still camera the dimmed scenery
   and the unfocused units are **identical frame to frame**; only the focused unit, the
   effect and the damage number change. Composing that once into `W->bg` (which exists
   and costs no new RAM) and copying it per frame would remove the per-frame dimming
   entirely. It means the kept copy no longer equals the view, which is an invariant the
   PC build checks, and the unfocused units would be baked, so it is a redesign of the
   frame composition rather than a tweak.

### The composed copy: built, proved to work, and refused by the per-frame budget

The second route was tried in full. During a wash with a still camera the dimmed
scenery and the unfocused units are identical frame to frame, so the picture was
composed once into `W->bg` -- the screen copy the engine already blits back every frame
-- and the frames after it skipped both the unit pass and the dimming.

It works. Instrumented over one Night Sword action: **2 compositions, 53 frames served
from the copy** out of a 55-frame wash, and the PC/68000 checksum is **identical to the
version without it** (`D976`), which is the point: the redesign has to be invisible.

Three things it needed, all of them real findings:

- The focused unit must **not** be baked in. It animates, and a new pose drawn over a
  baked old one leaves the old one showing round the edges.
- The PC invariant check in `screen_scene()` calls `copy_view()` over the planes to
  compare and **never puts back what it drew**. That was harmless only while the copy
  and the view were always equal; with a composed copy the PC ended the frame holding
  the plain scene and the 68000 the dimmed one. A non-destructive check is the fix, and
  it is a latent bug for any future divergence of copy and view.
- The save is free if it rides inside the dimming pass, which already reads and writes
  every one of those words. A separate pass afterwards cost 43,000 cycles.

**It is refused on one number.** The composing frame pays the dimming *and* the save at
once: **397,286 against the 360,000 limit.** The frames after it pay nothing, but the
budget is per frame, not per action.

Spreading the save over four frames, the way the engine spreads its own, does not work:
`W->bg` is the live source `screen_scene()` blits from, so a half-composed copy is
blitted, and the frames that then dim normally double-dim the rows already saved.
Keeping the copy out of use while it is built means `copy_view()` at about 105,000 a
frame, which is worse than what is being saved.

**What would make it fit is a second screen buffer** -- compose there, blit from there
-- at 4,000 bytes, where 436 are free at 13 actors. That is the decision this is
waiting on, not more tuning.

**A note on the instrument.** The first run of the counter reported "0 frames served",
and the branch that serves them simply had no counter: a replacement had dropped it.
The conclusion "the cache never engages" was one print away from being written down as
a fact about the design. It is the fifth miscount in this file.

### Measured on the Titanium: there are 426 bytes, not 4,000 (2026-11-27)

The composed copy needs 4,000 bytes and the figure it was being weighed against -- 436
free -- was from 2026-11-24, when `MOD_ZONE` was 9,118. It is 9,510 now. So it was
measured again, in TiEmu, on a live Gariland battle from the `fftgar` state:

```
battle heap free 426 max 420
```

`HeapAvail` **426 bytes**, `HeapMax` **420** -- that is the largest contiguous block.
68 heap blocks, 237,294 bytes allocated, 10 units on the field. Read through
`fftmem.py set heap on`, which fires `DBG_HEAP` on the next cached command window
(`window_cache.inc`), and collected with `ti-mem log`: no screenshot.

**So the second buffer cannot come from RAM.** 4,000 bytes against 426 free and 420
contiguous is not a tight fit, it is an order of magnitude. Nothing in the composed-copy
design changes that, and no tuning of it will.

And it could not be taken conditionally even if the RAM existed: `FFT_COMBAT_FX` appears
in neither `tools/module.py` nor the campaign `Makefile`, so the module images are never
built with it. A `Work` member added under that flag exists in the core and not in its
modules, `MOD_SIG` diverges, and the core refuses all eight pointers -- the TI build
draws "Missing data" and the PC/68000 checksums part company. That is why this layer was
built to add **zero** bytes to `Work`, and the one byte it does use fits in padding the
struct already had.

What is left for the dimming, then, is the asm route (`movem`, about 30 %) or nothing.

### The asm route, taken and measured (2026-11-27)

Authorised, written and measured rather than estimated. `games/fft/fxdark.s`, benchmarked
against the very loop it replaces, same 100 rows and both planes, under `ti-cycles`:

| | cycles |
|---|---|
| the C loop that shipped | 46,638 |
| `fx_dark_asm` | **42,010** |

Identical plane checksums and an equality marker in the bench, so the two produce the
same bytes. **4,628 cycles, 9.9 %** -- not the 30 % that was guessed before anything was
written. The guess was a hand-wave; an instruction-level count said 5 to 8 %, and the
measurement landed at 10.

The reason is structural and worth keeping: the map wants **ten long words at once**,
five of each plane, and the 68000 has eight data registers. `movem` cannot carry both
planes in one go, so the routine works three words at a time and then two, with one
register as the shared temporary. A formulation needing no temporary exists -- `d' =
d | ~l'`, which is exact -- but it costs two extra `not.l` a word, 12 cycles against 4
for the move, so the move wins.

On the real frame, the peak falls from **349,588 to 340,256**, leaving 19,744 cycles of
margin instead of 10,412. The PC build keeps the C loop and the 68000 takes the asm, so
the PC/TI cross-check compares one against the other: it passes with the same checksum
`D976`, which is the equivalence proof on a real picture rather than on synthetic data.

### The asm's gain spent on the spike (2026-11-27)

The 9,332 cycles the asm freed on the real frame go into `FX_SPIKE_H`, measured again
now the curve has moved:

| `FX_SPIKE_H` | peak | margin |
|---|---|---|
| **28** | **349,936** | **10,064** |
| 32 | 354,778 | 5,222 |
| 36 | 359,566 | 434 |

**28**, not 36. 36 passes, and passes with 434 cycles left, which in this file is not a
margin but a bet that the cycle scenario reaches the worst frame -- and four measurements
here have already turned out to cover frames where the feature was not running at all.
28 restores exactly the margin the layer had before the asm: the gain goes into the
effect, not into the risk.

E173 asks for 82 rows. The spike is now a third of the original's height instead of a
quarter, and the comment that called 28 "136 * 0.6" has been corrected -- that is 82.

### It falls, and it thins away (2026-11-27)

Two faults of movement, both found by looking at the captured original rather than at
our own picture.

**The direction was inverted.** The spike grew upward out of the target's feet; the
original's column comes down out of the sky. `E173`'s frame 0 had said so all along --
its quad spans y -26..110 about its anchor, almost entirely **below** it, so the anchor
is up in the air and the effect hangs from it. What was read instead was the atlas
picture, a cone with its point up, and that was taken for the direction of travel. A
sprite's shape never tells you which way it moves.

**It never ended.** `grow` saturated at `FX_SPIKE_H` and the column then held, unchanged,
for some forty frames. The original is gone by wash+54 while the wash itself runs to +66.
At the old 39-tick action this lasted a few frames; making the duration faithful made the
frozen animation the most visible fault of the sequence.

So the effect now has the original's three phases, in its proportions -- twelve of its
frames is eight of our ticks:

| phase | ticks | |
|---|---|---|
| fall | 8 | the foot descends from a fixed top to the target |
| hold | 8 | full width |
| thin | 8 | the width decays to nothing |

The thinning is in **width**, not height: at wash+54 the original still has a tall
column, only a narrower one. It does not retract towards the sky.

Eight each, so all three are shifts: this runs on every frame of a wash and a 32-bit
divide would cost about 900 cycles against the 1,220 a row already costs. Past the third
phase it returns at once, so the tail of a long hold draws nothing where it used to pay
28 rows a frame for a frozen picture.

Verified on the PC render rather than assumed, pixels changed against the version before:
t=30 and t=36 identical (the hold), t=42 936 pixels (thinning), t=46 onwards 1,728 (gone).
The cycle scenario's own checksum does not move because the frame it checks falls inside
the hold -- which is why the picture was compared frame by frame instead.

### The disc said it all along: 42 records, six of them read (2026-11-27)

Three versions of Stasis Sword were built -- a rising cone, a falling cone, a falling
shard plus a pillar -- and each was wrong about what the effect *is*. The reason was not
that the information is hard to get. It was this, in `tools/effect_rip.py`:

```python
for i, f in enumerate(fr[:6]):
```

`--frames` printed the first **six** animation records of however many a file has. E163
has **42**. The choreography was in the file from the first day and was read at one
seventh, with frame-by-frame video filling the rest.

With `--all-frames`, records 16..24 are the same 40x48 cell over nine frames with its
quad corners shearing and closing from 25 pixels of spread to 4; records 28..38 are
another doing the same. **That convergence is the effect**: blocks tumbling in onto the
target. Around them sit three 16x16 cells and a handful of 8x8 ones -- distinct pieces,
not one mass, which is exactly what "the blocks do not read" meant.

So `fx_spike`'s fall is now three solid outlined diamonds converging on the target
(`fx_blocks`), and the pillar rises out of them. **352,736 / 360,000**, margin 7,264:
the three blocks cost what the single shard they replace cost.

The method, so the next effect does not repeat it, is in
`.claude/skills/fft-assets/references/effects.md`, and its first rule is now "read all
the records, not the first six": count them, group them by cell, and look at what the
corners do over a run. A cell repeated with moving corners is an animation; a cell that
appears once is a prop.

### The effect's height decides whether the picture stays (2026-11-27)

An effect either fits the ordinary picture or it earns a cut-away, and the test is **its
own requested height**: a property of what is about to be drawn.

```c
#define FX_TALL 28
static u8 fx_tall(void)   /* fx_play.inc */
{
    return st.mode == M_ACT && FX_STAGES(st.abil) && FX_SPIKE_H > FX_TALL
           && st.act_t >= FX_ICE_FROM && st.act_t < FX_ICE_FROM + FX_ICE_LEN;
}
```

**Not the frame budget.** `W->left` is negative all through a battle scene, so a budget
gate fires always or never -- that was measured when the composed copy was tried.

**Both halves of the test matter.** Keyed on the ability alone, the cut-away stayed up for
the whole wash; the effect plays 23 ticks of a 55-tick hold, so 25 frames showed an empty
grey field. The picture goes away for the *effect*, not for the ability.

A cut-away is: the map a flat field, nothing dimmed (a uniform field has nothing to dim),
and only the two units the action concerns drawn. The levers measured separately:

| lever | peak | won |
|---|---|---|
| flat map, still dimmed | 344,178 | 8,558 |
| flat map, not dimmed | 302,156 | 50,580 |
| only the two units, whole wash | 304,128 | 48,608 |
| **both** | **253,716** | **99,020** |

The flat map is worth almost nothing on its own -- the scene blit already comes from
`W->bg` and is cheap. What pays is that **a flat field does not need dimming**, which
removes the asm loop's 42,010 cycles. The gain is in what stops happening, not in what
replaces it.

That is what buys `E173`'s real height:

| | peak | margin |
|---|---|---|
| ordinary picture, 24-row pillar | 352,736 | 7,264 |
| **cut-away, 82-row pillar** | **337,234** | **22,766** |

`FX_SPIKE_H` is 82 at last -- 136 * 0.6, the original's own height.

**What it costs in fidelity**, since nothing else in this file does: the original never
cuts away. It keeps its scene visible and dimmed for the whole effect. This is a trade
taken to get the effect's size right, bounded to the frames the effect draws on, and
`FX_TALL` above 82 undoes it.

