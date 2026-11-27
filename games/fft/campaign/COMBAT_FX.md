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
