# Vulnerable incantations: binding, interruption and paid execution

2026-10-27. Follow-up to CORE_CASTING.md. The core now arms and resolves
single-centre vulnerable casts through the existing CT/action/reaction pipeline.
Tile and unit binding are explicit; retail Holy/Fire formulas, area targeting
and a player binding-choice UI are still gated. Private carriers exercise the
backend without changing the production catalogue or reviewed presentation.

## Correct the paid-effect boundary

The preceding MP milestone could debit a command and then call `skill_hit`,
which rechecked affordability against the remaining MP. An exact-cost command
could therefore pay and incorrectly miss. Its earlier test forced a miss and
did not prove a successful paid effect. `skill_effect_hit` now checks effect/
status eligibility without another affordability check; `skill_hit` retains the
pure preview check. COMMAND calls the former after its one payment. The new
test pays7 from7 MP, observes a natural successful hit, commits real HP damage
once and rejects duplicate execution/impact. No retail-zero-cost behavior changes.

## Source and target policy

`tools/incantation_oracle.py` executes four unchanged original initialization/
MP/binding sequences and twelve Charge-only recoil predicates. With an empty
tile, a living/KO unit or an absent target record, the observed initializer pays
Holy's56 MP before strike setup and live/stored coordinate binding. Strike setup
and external status handlers are isolated; payment/binding hooks only observe.
This does not certify a complete original target-list/effect/presentation/refund
trace. Provenance/functions/disc hashes are recorded in
`x/core-incantation/oracle/source.json`.

The original recoil-cancellation predicate applies to skillset Charge, not White
Magic. Reused CORE_DEFERRED.md evidence distinguishes live-unit coordinates from
stored tiles; CORE_CASTING.md supplies source timing/MP rules, and existing
bitmap evidence supplies Sleep/KO cancellation and Stop freeze.

## Core contract

`cast_order(actor, reference, ability, follow)` returns0 for rejection,1 for an
immediate/empty resolution or2 when armed. It requires an enabled AF_CAST record,
a present actor, resources, a valid initial compact cell, normalized range/height
and self-target policy. Areas and persevering actions reject until their own
target-set/pulse pipeline exists. No unsupported spell becomes usable merely
because its source ID has a range/CT record.

The eight-byte pending record is unchanged. States4/5 distinguish fixed-tile
and unit binding; `tile` holds the compact node or actor slot respectively.
Charge alone reads origin/weapon/off_weapon. Counts/invalid results remain16-bit,
so node255 stays valid, including on the upper layer. Actor IDs are fixed slots
for the lifetime of an encounter; slot recycling requires explicit identity/
binding invalidation in the future lifecycle producer.

Arming latches CT, sets Charging and spends Act, preserving Move. Short Charge
and Non-Charge use the source arithmetic with equipped support IDs. No MP/RNG/
effect is consumed on arm. Movement or weapon changes do not cancel the cast;
the due centre follows the bound unit even outside the original range. A fixed
tile can affect a different occupant after movement. The source delay is not
converted into completed character turns, and menus do not tick it.

At due time, KO/Sleep cancel, Stop retains/fixes the countdown, and resources,
Silence and Don't Act are revalidated. Ordinary binding removal never retargets
another actor at the old coordinates. This native single-centre stage pays for
empty/KO/absent centres and produces no reward; empty-centre spell animation is
deferred. This follows the observed initializer payment order, without claiming
full original empty-spell presentation. Live effects use current formulas/stats,
then one root owns impact, reactions, rewards and the CT resume.

Counter/Auto-Potion and a later due cast cannot overtake the current root. The
same bounded thirteen records feed both Charge and casts in slot order; no event
queue or extra allocation is added. Airborne Jump/performance producers remain
reserved, and all-permanent-freeze liveness/Quick255 priority remain open.

## Image ownership and budget

COMMAND owns initial validation, arming and due preparation (operations3/4).
Its four-byte input lives in ModCtx rather than gameplay State/formula scratch.
The resident wrapper preloads COMMAND, writes those arguments and invokes it;
only accepted orders mutate State. The module returns before ACTION preload or
AI continuation. Due `pending_next` remains resident, so ordinary CT polling does
not copy an image. Pure resident `unit_at_cell`/MP/hit/recoil callbacks never
evict the executing module.

An initial resident validator exceeded the packer limit. Moving validation and
due preparation into COMMAND resolved it. ABI20/ModCtx344/signature60510 reject
ABI19; twelve images, largest5862. State2124/Unit112/Work45516/FSV3 and the five
battle allocations remain unchanged. The pending record still occupies8 bytes;
no generation array, event queue or per-cast target array is allocated.

Private42-variable group492546 bytes (+1481), raw image bank54480 (+1466), packed
battle42275 (+15), TTPack input64859/headroom77. All28 non-code banks remain
unchanged; FCJ8 remains valid. Further resident growth still needs an image budget.
Allocation formulas and image sizes are not full stack/fragmentation/residency
or complete-frame certificates.

## Evidence and remaining work

`check_incantation.py` includes the earlier resource fixture and successful paid
effects, fixed/replaced/followed targets, initial rejection purity, late status/
resource rejection, caster displacement/weapon changes, real CT ticks/menu pause,
support timing, thirteen casts, Auto-Potion ownership and actual node255/upper-
layer binding. Private data changes Dash into a cost carrier and Throw Stone into
a cast-policy carrier with evasion disabled; these are not retail spell formulas.
Production normalized Holy/Fire remain kind255, and no AF_CAST record is shipped.
A future production AF_CAST producer needs a catalogue feature/version contract
so an old engine cannot read the new flag as an immediate ordinary action.

Direct action/dual Charge/control/due-wave/Charge regressions and one native
Orbonne/Gariland flow cover integration. The native layout is refreshed; unchanged
bitmap/geometry/catalogue evidence is reused without a census or live TiEmu use.
Final results: `x/core-incantation/verification.json`.

Next: a source-backed magical formula and target eligibility/traits, then area
membership/commit/reactions/rewards and reviewed cast/empty-spell presentation.
Secondary skillset dispatch, original slot-reuse behavior and full target/layer
validity still need their own evidence. Jump refinements remain low priority.

```sh
make -C games/fft/campaign core-incantation-check
```

## Final scoped results

| Evidence | Result |
|---|---|
| Incantation/resource/paid-hit/layer test |799 PC/sanitizer/68000 assertions, `FD122519`|
| Ordinary/dual Charge |586 assertions, unchanged `0F008472`|
| Actions / due-wave / control |295 /418 /594 assertions; unchanged `EDCEBFAE` / `76F6E4AC` / `15C96953`|
| Existing Charge |unchanged native/PC `276A2090`, including range/menu pictures|
| Gariland / Orbonne flow |unchanged `AE9CFD31` / `2CD70391`; native save equal|

The fresh native/host layouts are identical to CORE_CASTING.md. All eleven flow
PNG bytes/modification dates remain unchanged. New ABI20 images load and old19
images reject. Only ffbcore/ffbm change; data and the standard466045-byte group
are untouched. The already-reviewed normal/response style and unchanged geometry/
bitmap evidence are reused; no live TiEmu input or new assembler was used.

The two scoped unit/tile due launches average48808 cycles. Synthetic paid command
starts average80250; subsequent Charge-hand preparation42722 versus42650 before.
Moving empty Charge preparation to COMMAND adds copying: the thirteen-empty
record dispatch zone is225232 versus126714 previously. Thirteen ordinary due
transitions average83447 versus78249. These CPU zones exclude drawing/grayscale;
this is a packing/ownership tradeoff, not a claimed speedup or full-frame latency
certificate. Full combined residency/stack/fragmentation and worst-case frame
budgets remain open before content production.
