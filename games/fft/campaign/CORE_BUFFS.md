# Buffs and timed status spells: 22 retail producers together

2026-10-29. Follow-up to CORE_HEALING. One validated formula0a/0b family
adds 22 actual source records, rather than one named spell per milestone.
The approved area/forecast/number renderer, fonts and menu style are reused.

## Delivered scope

Regen; Protect 1–2, Shell 1–2 and Wall; Poison; Haste 1–2, Slow 1–2, Stop and
Don't Move; Reflect; Blind; Pray Faith and Doubt Faith; Silence Song,
Paralyze (Don't Act) and Sleep; Carbunkle and Silf.

Wall is the Protect+Shell spell, not the separate source Wall status 31.
Raw flags retain source costs, CT, evasion, Reflect eligibility, radii,
vertical tolerance and ally/enemy exclusions. Radius 0 spells also use the
same owned area packet. No named-spell runtime dispatcher is introduced.

- Formula0b applies Magic Attack Up, zodiac(MA), separately rounded zodiac(X)
  and both effective Faith values. Formula0a additionally applies Magic
  Defense Up, Shell and Chicken/Frog to MA, before zodiac and chance.
- Evasion is resolved before the success percentage, with the original draw
  order, including zero evasion and chance 0. A preview consumes no RNG.
  Failed or fully immune status requests have canonical null effect/zero value.
- The offline compiler accepts only all-or-nothing addition sets with exactly
  one nonzero status byte and implemented flags/status bits. The prepared
  u16 amount carries byte/mask for AB_STATUS, not an HP quantity. The pure
  resident query specializes the existing full bitmap filter to that shape;
  commit still uses the shared source cancellation/immunity/timer kernel.
- One cost/payment, Unit/Panel binding, charged timing, once-only commit,
  counter/turn/reward/outcome draining and original caster Reflect semantics
  remain shared. Reflected statuses affect one endpoint, without a second AoE,
  payment or bounce. Normal status impacts do not consume HP or wake Sleep.
- Existing status kernels provide Poison/Regen upkeep, Haste/Slow CT changes,
  Stop/Sleep/Don't Move/Don't Act controls and Dark/Silence behavior. Protect's
  physical XA reduction is applied before Charging/Sleep and zodiac, including
  actual weapon/dual/Charge paths. Shell/Faith/Innocent use the shared magic
  arithmetic. Timed effects refresh and cancellation clears their timers.
- Forecasts show Status plus probability; the existing seven-row number
  renderer shows short status labels, never the packed bitmap as HP damage.
  No new bitmap/font bank or original spell/summon cinematic is added.
- AI uses conservative immediate status utility with ally/hostile sign,
  existing-effect suppression, near-expiry refresh and caster checks for
  Faith/Innocent. This is an adaptation, not the original future-turn AI.

`magic_heal` retains its historical callback name: query 0 is HP amount or
packed status byte/mask; query 1 is success percentage; query 2 is the HP-family category.
It performs no image loads or mutation. FORMULA owns status commit; RESPONSE
owns the old HP commit. Both instantiate the same bounded batch-commit loop.
No new State/Unit/Work/save field or core allocation is introduced.

## Memory work included in the batch

The unchanged cached menu compositor moves to RESULTS with a resident wrapper;
its source label lookup is local to that image, with no nested module loading.
The raised tab/frame, dimensions, row schedule and cursor remain unchanged.
An old reference checks 28 incremental menu parts against identical seeded
planes on PC and 68000; screen-plane pointers must be restored on return.

ABI 40/context 376/signature 410 rebuilds all 17 consumers, split 8+9.
Raw code shards are 43270/44990 bytes; maximum image/zone 7020 (+370).
The existing static executable-image zone therefore grows 370 bytes inside
the resident program; this is not an additional heap allocation.
The packed resident input is 64915, with 21 bytes below the 64936 limit.
Further resident growth must include measured relocation in its feature batch.
Unit 112/State 2192/Work 45576/FSV3 save 3138 and five core allocations are retained.
This does not certify the full worst-case stack, RAM residency or frame budget.

Private group `x/core-buffs/fftcampaign.89g` is 526017 bytes (+2753). Only
ffbcore (-285), ffbm (+2100), ffbn (+938) and ffcj (same length) change.
The 27 other non-code banks/assets remain identical. FCJA stays 41086 bytes and
changes exactly 22 previously disabled normalized records, with other sections
identical. The standard playtest remains the previous 466045-byte group.

## Verification

`make -C games/fft/campaign core-buffs-check` runs production records, PC,
ASan/UBSan and actual 68000 image dispatch. The main fixture records 5221
assertions, digest
3C724297: all 22 costs/CT/learned lists/AI/typed impacts, Unit/Panel binding,
immunity/innate/current status, timer initialization, repeat-impact protection,
Reflect endpoints and unchanged cached menu parts. A second native carrier
checks the existing numeric-label cache, Protect through real attack/Charge
queries and a synthetic 13-actor same-team fixture on legal distinct Gariland
cells. This is a capacity test, not a canonical party/deployment composition.
The second carrier passes 461 assertions E318A9A7; combined count 5682.
The split keeps each test executable below the AMS variable transport limit.

The bounded original oracle executes 67 actual MIPS formula/filter/finalizer
profiles and 10 Protect XA-order cases. Native-equivalent random draws are
supplied to the original RNG getter and percentage roller; target-list sorting
is isolated. These traces do not claim a full original cast/render/AI replay.
The shared bitmap kernel skips entire empty bytes while retaining source row
order. Existing 177 original bitmap profiles/public adapters pass 4843 assertions
262BF732. The 13-actor impact drops 441270 to 296906 backend cycles (about 33%);
preparation costs 289686. The full frame/stack/residency gate remains open.
The single-byte query is also compared with the existing full bitmap filter
on 198 compatible producer/state combinations, not a repeated broad census.

Affected call sites, actual prior-ABI 39 export rejection, layouts,12 old
screens and both native pilot save/results flows are recorded under
`x/core-buffs/`. Unchanged geometry/Jump proofs are reused; the bitmap oracle was rerun
because its kernel changed.
The standard playtest group, user saves and live TiEmu remain untouched;
old gallery/group bytes and modification dates must remain identical.

## Remaining work

The production catalogue now contains **77 executable normalized records**,
including **55 casts** (41 spells, 14 summons). Generic careers expose
**74/201 action-table entries**;127 unavailable entries are not 127 effects.

| Shared work group | Remaining generic entries |
|---|---:|
| Other status/control/barrier behavior | 12 |
| Percentage HP/resource transfer (09/0f/10) | 5 |
| Other default formula/job families | 69 |
| Special dispatch/items/upgrades/categories | 41 |
| Total | 127 |

The 12 status-family entries still gated are Reraise, Esuna, Frog, Death,
Float, Quick, Zombie, Blind Rage, Confusion Song, Dispel Magic, Petrify and
Golem. They require additional behavior, not just another addition mask.
Next take the compatible percentage/resource-transfer family together.
Broader R/S/M, special jobs/monsters, campaign/objectives, formats/tools and
full 13-actor worst-case memory/performance gates remain open. The core is
**not complete** and mass content production remains gated. Keep Jump deferred.
