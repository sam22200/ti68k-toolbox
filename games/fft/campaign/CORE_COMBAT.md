# Combat reactions and protections — delivery block C

New generic source IDs: Meatbone Slash434, Counter Magic435, Distribute444,
MP Switch445, Damage Split446, Weapon Guard447, Catch450, Blade Grasp451,
Arrow Guard452, Hamedo453, Concentrate469 and Throw Item474 (twelve).
Ordinary physical attacks also use the existing Abandon449 policy.

## Accuracy and interception

Ordinary Attack, Charge, Throw and applicable physical skills share the same
physical evade evaluator for forecasts, native AI and execution. Physical
accessory, right/left hand and class evades retain source order, front/side/back
filtering, final-product rounding and four draws even for zero evades. Weapon
Guard enables the exported weapon evade value when action restrictions allow
it; shields and accessories retain their own evades. FCJG repurposes the
previously zero weapon `Item.sev` byte without growing common data.
Concentrate resolves equipped/innate support and clears physical evades. It does
not remove Blade Grasp/Catch/Arrow Guard or ordinary magical evasion. Abandon
and Defending reduce the evade base; Dark/Confusion retain their physical stage.

Blade Grasp uses source flag4/0x10; Arrow Guard requires source weapon-range
flag1/0x20 and an actual bow/crossbow. Their primary-only Brave test precedes
ordinary evasion. Catch intercepts selected Throw items, excludes Mime throws,
and preserves its source Brave100 path without a Brave draw. Throw pays once;
a caught item enters the receiving unit's permanent blue inventory once,
capped99. Enemies cannot manufacture party stock. Its ordinary animation uses
the existing Catch number label. Repeated impact cannot pay or catch twice.
Pure forecasts never consume RNG or resources; MP Switch remains an execution
reaction rather than a guaranteed MP forecast.

Ordinary items now reach one tile, or four with equipped/innate Throw Item
(and Mime's source behavior). Throw itself still uses the actor's current Move.
Manual reach and AI share the effective range. No new menu/art bank is needed.

## Offensive responses

Meatbone Slash shares the existing post-HP-damage Critical/Brave trigger. A
surviving eligible reactor within ordinary weapon reach returns its own max HP
as damage, with no ordinary hit roll. Wall remains a miss. Its pending amount
and response do not earn an additional action reward.

Counter Magic freezes the eligible incoming source spell ID before impact,
including a miss. At response selection it rechecks incapacity, Silence/Frog
and effective MP, without requiring the spell to be learned or in the current
skillset. Its return centres on the original actor without the original spell's
range restriction, pays once (Half MP/Mime respected), and freezes all victims
before any response HP/status mutation. It uses the existing spell area and
status presentation. Secondary responses cannot provoke more responses.

Hamedo intercepts an accepted ordinary attack, Charge, selected Throw or
eligible scalar weapon-range action before root MP/stock payment. It replaces
the unexecuted root packet, cancels both root hands even if its strike misses,
and preserves the reactor's independently armed action, CT and turn history.
The native bounded policy currently requires a **melee reactor** and the
existing same-layer ordinary reach; distant ranged Hamedo/trajectory recovery
is not claimed. Availability/reach rejection occurs before the native Brave
draw. This is an explicit native adaptation, not a retail RNG claim for
rejected Hamedo candidates. Delayed Charge uses the same interception at due
resolution. Neither the cancelled root nor Hamedo grants EXP/JP.

## Ordered HP/MP effects

MP Switch tests before HP damage with positive MP. On success it redirects the
nominal damage, capped999, to MP; an MP shortfall does not spill into HP. HP
damage wake/cancel and post-HP-damage reactions are skipped. Knockback and other
prepared effects retain the shared command policy.

Damage Split stages ceil(damage/2) before the original HP damage. It does not
halve that original hit. A surviving reactor later damages the original actor
and heals itself by the frozen half. KO/late incapacity expires the response.
Distribute stages positive healing overflow. At response construction it finds
injured on-field allies by the reactor's current camp and each recipient's
permanent camp, excluding the reactor, dead/Petrify/airborne units. Wall counts
in the divisor and rejects its own result. Each receives ceil(overflow/count),
with no recursive overflow. Native player/guest camps retain their existing
blue-side mapping. Retail stages self-target requests but skips the acting
slot during this response drain; these three new secondary types do likewise.

Scalar and batch commits settle once. A batch MP redirect updates its owned
presentation packet and expires the old HP glyph; the existing area renderer
rebuilds one changed glyph per draw. An already prepared HP number must never
be displayed as the applied MP effect. Root rewards settle through the existing
response drain, and secondary effects preserve independent deferred actions.

## Native ownership and limits

ABI66 removes two redundant physical hit callback slots (ModCtx352).
COMBAT20 owns scalar command effect construction; COMMAND returns before the
resident dispatches it. MOVEMENT18 now owns cold reach/path/aim operations
5/7/8/10 as well as terrain preparation. EQUIPMENT19 constructs numerical and
self-status responses after selection returns. ACTION no longer retains the
whole scalar amount evaluator: sword-skill splash uses the same small pure
helper as its forecast. No executing image loads another image. Existing
FMB3/FMD2 packaging, five heap blocks, state/save layouts and archive-in-place
assets remain in use. Source art, portraits and camera handedness are unchanged.

This block does not certify all weapon formulas/procs, special actors, bow
weather fidelity, exact multi-floor geometry, whole-frame/stack/residency or a
full campaign. Automatic Mime imitation remains a separate final integration
feature. Block D contains the six remaining generic R/S/M producers.

## Verification

`make core-combat-check` runs unchanged retail MIPS fixtures, portable checks,
ASan/UBSan and the actual68000 owners plus representative existing battle
presentation. Local evidence is under `x/core-combat/`; final sizes, closure,
private native flows and preservation results are recorded in
`verification.json`. No live TiEmu session, user save or standard campaign
group is replaced. The oracle supplies controlled records/RNG and explicitly
isolates Hamedo target/MP/counter-preview callees; it is not a complete original
AI or whole-battle replay. See `oracle/source.json` and `offensive.json` for
exact source hashes, inputs, outputs and the isolation boundary.

Final focused result:2975 logic assertions (`2BBAAA20`) and12 UI assertions
(`0FA7DE36`), four framebuffer captures. All42 affected suites are green after
six focused rechecks. Private Gariland/Orbonne remain81/14 assertions and exact
FSV3/VAT bytes. Final ABI66/ModCtx352 has21 images/zone8730 and three shards
50728/50240/44748 bytes. Actual non-BENCH TTPack input64861 leaves75 bytes;
raw variable64859+2 matches it; packed data39763 bytes. Neither inspected hot
physical evaluator emits32-bit multiply/divide or floating library helpers.
All517 initially protected save/group/proof files retain bytes and dates.
