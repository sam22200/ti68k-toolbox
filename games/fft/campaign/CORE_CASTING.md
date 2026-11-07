# Casting contract: MP eligibility, payment and preparation policy

Follow-up2026-10-27: [CORE_INCANTATION.md](CORE_INCANTATION.md) adds actual cast
lifecycle and fixes a paid-effect gap here: the old test forced a miss, so it did
not detect execution rechecking MP after payment. The effect evaluator now skips
that second affordability test; the new exact-cost success test commits real HP
damage. Measurements and gate statements below describe this earlier checkpoint.

2026-10-27. Follow-up to CORE_DEFERRED.md. This milestone implements shared
MP eligibility/payment and source-checked preparation arithmetic. It does not
enable Holy, Fire or another new spell effect/targeting producer. The current
retail-normalized action subset still costs zero MP; a private synthetic carrier
exercises nonzero costs through the actual engine before a spell is enabled.

## Source evidence

`tools/casting_oracle.py` executes unchanged local-disc MIPS for33 MP cases,
12 preparation cases and a retail Holy arm-then-pay sequence. Original code
hashes, disc hashes, inputs and decompilation commit are recorded in
`x/core-casting/oracle/source.json`. External status-specific handlers return
without effects; dry/unmounted restriction checks and action-state setters run.

- Retail Holy sets its six-tick countdown/action-used state without spending MP
  or marking movement used. A later MP-resolution entry reduces999 MP to943.
  These are two direct original calls, not an elapsed-CT/end-to-end spell trace.
- Default-menu, non-Mime abilities pay the raw range record's MP byte. Other
  dispatch menus and Mime do not use that payment rule. Half MP shifts right,
  rounding an odd cost down. No source job in the160-record catalogue has innate
  Half MP; the native producer uses the already-loaded equipped support ID.
- Preview still checks affordability and records effective cost in original
  scratch, while skipping subtraction. Silence, Frog and Don't Act can reject
  before payment. This observation does not implement native Frog/mount/water
  restriction producers.
- Preparation masks CT to seven bits. Short Charge rounds half up; Non-Charge
  then makes the delay zero. Persevering actions ignore both. Synthetic odd and
  maximum-delay cases check the instructions' boundaries; these are not new
  retail ability records. This pure timing kernel is not yet a live caster or
  performance scheduler.

## Native resource ownership

`skill_cost` reads the existing archived range/menu data and the equipped support
ID. It is a pure resident callback: COMMAND, FORMULA and AI can query it without
loading an image or changing MP, equipment, status, CT or battle rng32. Existing
Silence checks and affordability now share `skill_blocked` across player commands,
hit forecasts and AI candidate construction. No formula's amount calculation is
turned into a resource mutation.

COMMAND validates first, then debits immediately before starting the effect.
Its existing deferred Charge arming returns before this point; a future caster
must do likewise and reach payment only at resolution. Failed or overlapping
orders preserve the whole State and RNG, including an independently pending
Charge. A miss still pays once; repeated impact or an overlapping command cannot
pay twice. Enemies use the same MP rule despite their separate unlimited-item
policy. Half MP uses source support ID0x1ce already carried through FSV3.

Native eligibility previews preserve world resources/RNG. Native FORMULA query
wrappers may write request scratch; that is distinct from the whole-State purity
required of a rejected command. The source resolution entry also clears action
bookkeeping, while the native pure eligibility query deliberately does not.

For nonzero costs the private test changes only Dash's raw MP byte to7 and adds
the Silence flag. This is a **synthetic resource carrier**, not retail Dash MP,
Silence behavior or a substitute Holy formula. The production catalogue stays
unchanged and Holy/Fire still have normalized kind255 (unavailable).

## Packing and image ownership

The initial inline query grew the largest image to5970 and exceeded TTPack's
64936-byte input ceiling. The final resident cost callback shares that math;
the unchanged range-construction body moves to COMMAND operation2. Its resident
wrapper returns before preview/AI can load another image. It is never called
from an executing FORMULA/AI image. No range style, ring membership or menu
layout changes. Invalid commands still reject before modifying request fields.

ABI19 adds one pure callback: ModCtx336 (was332), signature60245, twelve images,
largest5860. Reject ABI18 images so an old AI cannot bypass the new policy.
FCJ8/FSV3 and native Unit112/State2124/Work45516/Save3138/SaveUnit176 remain
unchanged, with five battle allocations. The copied image zone grows46 bytes
inside the resident program; the complete TTPack input shrinks114 bytes overall.
This is not a new combined-residency/stack certificate.

The private42-variable group is491065 bytes (+686), raw image bank53014
(+686), battle packed payload42260 (same length, changed bytes). Resident input
64743 leaves193 bytes of packer headroom versus79 before. All28 non-code banks
retain hashes. More resident growth still needs an image budget.

## Validation and remaining work

`check_casting.py` compares PC, address/undefined-behavior sanitizers and actual
68000 results for source resource/timing cases, real AI candidate lists, pure
eligibility, rejection before mutation, paid misses, repeat/overlap protection,
Half MP, enemy payment, Charge arming/resolution and cold range ownership.
Direct callers are actions, dual Charge, control, due-wave dispatch and the
existing Charge range/menu fixture. One native Orbonne/Gariland flow and fresh
native layout close ABI integration. Unchanged bitmap/geometry/timer evidence
is reused; no full census or live TiEmu input is used.

Next: an actual vulnerable-cast record with explicit tile/unit binding, live
interruption/payment order and a representative magical formula; then area
membership/commit/reward/reaction rules. Empty/absent-target payment, secondary
skillset dispatch, elemental/magic-evasion/trait producers, Quick255 and
all-permanent-freeze liveness remain open. Do not copy Archer's movement/weapon
cancellation or movement cost into ordinary magic. Jump remains deferred.

Final evidence: `x/core-casting/verification.json`.

```sh
make -C games/fft/campaign core-casting-check
```

## Final scoped results

| Evidence | Result |
|---|---|
| MP/preparation/real AI/cold range |370 PC/sanitizer/68000 assertions, `71AFAC0C`|
| Existing action / dual Charge |295 /586 assertions, unchanged `EDCEBFAE` / `0F008472`|
| Control / due-wave dispatch |594 /418 assertions, unchanged `15C96953` / `76F6E4AC`|
| Existing Charge |unchanged native/PC `276A2090`, including range/menu pictures|
| Gariland / Orbonne integration |unchanged `AE9CFD31` / `2CD70391`; native save equal|

All eleven flow PNGs retain bytes and modification dates. New ABI19 images load
and captured ABI18 images reject. The fresh native/host layout matches the
preceding checkpoint; no data schema, battle/save layout or allocation grows.
Only ffbcore (changed bytes, same compressed length) and ffbm change in the
private group. The standard466045-byte group and user's live TiEmu are untouched.

A private same-input native range benchmark compares the captured pre-move body
with actual COMMAND2. Charge-range hashes are identical (`B85507D0`): old queries
40332/40332 cycles; new cold/warm67888/43558. Thus relocation adds27556 cycles
cold (about2.30 ms at12 MHz) and3226 warm (about0.27 ms), while regaining packing
space. These are query costs, excluding range drawing/grayscale/input/full
frames. `ti-cycles` deliberately excludes call0 from its displayed max; the
recorded two-call sum and call1 recover the first/cold cost. No UI latency or
hardware FPS claim is inferred from a warm mean.

The two synthetic paid command starts average70748 cycles (effects/drawing not
included). Existing primary impact zones average16050 versus15982 before;
second-Charge preparation42650 versus42338. These scoped CPU costs and unchanged
pictures do not certify the complete scheduler or combined residency.
