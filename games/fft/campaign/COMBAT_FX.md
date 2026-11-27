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

Dark 1 in one frame would reach 310,062 and still pass. Quartered it reaches
275,084, leaving about **85,000 cycles a frame for the effect itself** -- on the
order of 70 masked 16x16 blits. The particles and primitives of the spec fit with
room to spare.

## Open, and deliberately not done

- The darkening is C. 46,638 cycles for 500 longs is 93 a long for six
  operations; a `movem` routine would plausibly halve it. That is assembly, and
  the project rule is to ask first.
- Per-effect RAM is estimated, not measured: about 224 bytes of the 436 free for
  the runtime record plus 32 particles. It means **one effect at a time**.
- `battle-act` is a 90-frame diagnostic case, not a played fight.
- Datasheet cycles throughout. No emulator or calculator run.
- No effect, primitive, asset or scheduler exists yet. This is a costing.
