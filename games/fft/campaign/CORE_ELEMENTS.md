# Typed elemental results, signed quantities and response ownership

Current follow-up: [CORE_MAGIC_EVASION.md](CORE_MAGIC_EVASION.md) adds magical
evasion/ordinary target validity and FCJA/ABI24; older numbers below are historical.

2026-10-27. Follow-up to CORE_AFFINITY.md. Cached elemental traits now drive
formula08 prediction, actual native AI ranking and single-centre impact.
Strengthen/Half/Weak modify the scalar; absorption selects the healing category;
nullification selects no effect. Production normalized Holy/Fire and all AF_CAST
producers remain disabled. Magic evasion, complete target eligibility, weather/
Oil/Float/mount/monster/Undead policy, areas and reviewed casting presentation
are still separate gates before retail spell enablement.

## Source observations

`tools/elements_oracle.py` executes unchanged original formula08 damage and
current-action finalization on77 profiles. The original HP/MP commit prefix then
runs through its signed stores; reaction dispatch/MP split are isolated and
execution stops before stat/status/KO/EXP handling. Normal weather, no Oil/Float,
unmounted ordinary humans and a non-evadeable carrier are explicit conditions.
Hashes, decompilation commit and all inputs/results live in
x/core-elements/oracle/source.json. This is not a full original battle/spell trace.

Holy's real source parameters are power50/element2/no magic evasion. Other
power/element combinations in the arithmetic fixture are synthetic, including
power255 to test an odd negative intermediate before division by two. Do not
call them retail Fire/Zodiac spell implementations. The source contains a
positive999 final cap, rather than a cap on the unsigned bit pattern.

Observed order: Strengthen5/4 on MA, existing support/status/zodiac modifiers,
XA*power narrowed to a signed halfword, Half (signed /2 toward zero), Weak
(double and narrow), Faith product/10000. Nullification wins over absorption;
Half precedes Weak even when both are set. Absorption changes the event category
to healing. Finalization caps positive damage/healing quantities at999.
The commit prefix reads both fields signed and clamps HP to0..max HP.

Large combinations can wrap a signed halfword before Faith. A negative damage
field can increase HP; a negative healing field can reduce HP. The native model
preserves this source behavior rather than interpreting a negative word as
large positive damage or incorrectly clamping it to999. The event category stays
separate from actual HP direction, including at these extreme boundaries.
Original source KO/status/reaction/EXP behavior after that prefix is not newly
certified here; existing native hooks and their earlier policy evidence are reused.

## Shared contracts

`magic_amount(actor,target,spec)` takes element<<8|power and returns the raw source
halfword bit pattern. `skill_amount` applies the source positive999 cap;
`skill_kind` is a pure resident lookup of damage/healing/null using the validated
cache and existing normalized element byte. Neither pure callback mutates State
or battle rng32, and neither loads an image. Existing raw non-elemental scalar
profiles retain their outputs; the shared capped query now intentionally changes
the old magic fixture's expected high quantities (B5895FED replaces4DCFDD40).

Existing verified MULS16 and DIVS32_16 handle the wide products: a source-narrowed
s16 times Faith<=10000 fits signed32, and its quotient fits signed16. The /3
support reciprocal remains bounded before scalar narrowing. No new assembly,
software wide arithmetic in this kernel, float or per-query equipment scan is
introduced. Compiler output is inspected alongside actual68000 equality.

Execution prepares a category and quantity once. Later trait changes, UI queries
and reaction presentation cannot rewrite them. `magic_hp` applies the same
signed HP transform in AI and commit. Forecasts show healing/KO/miss consistently
with that transform; existing floating-number style is reused. Normal absorbed
healing does not use damage-type Sleep wake or offer Auto-Potion. Nullification
pays at due launch, consumes no effect RNG and earns no effect reward. The source
initializer MP policy is reused; this is not a new full target-list/refund trace.
KO/empty/absent centres still use the preceding lifecycle policy and remain part
of the broader target-eligibility/presentation gate.

Damage/healing preserve the existing generic execute draw; nullification bypasses
it, as observed. No physical critical, Brave knockback or physical vulnerability
is inherited. Actual AI candidates use the same amount/HP transform and accept
beneficial absorbed healing on an ally while ignoring nullified targets. This
is the existing adapted potential-effect AI, not original complete probabilistic
or delayed-turn scoring. Broader healer-role/target policy remains separate.

`effect_kind` occupies the former State hand-alignment byte and the former
ActionPacket tail-alignment byte. Both primary and response-owned payloads retain
it. The healed-primary response test injects a queued Auto-Potion solely to
exercise transport; normal Holy healing does not naturally request that reaction.
Real damage/Auto-Potion remains covered by the earlier magic/action fixtures.

## Native ownership, size and CPU

ABI23/ModCtx352/signature61487 reject22. Twelve images remain; AI is largest6096.
Unit112/State2190/Work45516/ActionPacket22/SaveUnit176/Save3138(FSV3), the five
shipping allocation requests and their formulas are unchanged. Context storage
is inside each code image; no target list, queue or new heap allocation is added.
This layout is not a combined heap/stack/fragmentation/residency certificate.

Cold forecast formatting moves from the resident program into FORMULA10 and
uses local formula evaluators plus pure resident callbacks. The resident wrapper
passes its output buffer through the existing transient input union. No executing
image may call that loader. Actual HUD forecasts occur in M_AIM before action
wind-up; an explicit injected query after launch can change idle residency, but
still preserves the prepared result. No hot AI/math callback evicts its caller.

Private42-variable group496819 bytes (+1061), raw image bank58188 (+1220), packed
battle42006 (-159). TTPack input64795 leaves141 bytes below64936. All28 non-code
banks, including FCJ9, remain identical. The normalized reserved byte is named
element for the private carriers; no production record changes. A future retail
AF_CAST/data producer still requires its feature/version contract.

Scoped CPU evidence: raw non-elemental scalar/check zone39 averages2282 cycles
versus1972 before; typed query zone46 averages2950 over77 profiles. The three
live healing/null due launches average53324. Warm formatting averages7353;
one explicitly cold formatter costs46180, below the60k HUD-task envelope.
Charge-hand preparation rises42866->46546 in its scoped fixture; primary impact
zone15 averages17015 versus16268. These include fixture overhead and image-copy
tradeoffs, not a claimed speedup or complete worst-frame certificate.

## Validation and next work

`make -C games/fft/campaign core-elements-check`:1525 PC/ASan/UBSan/68000
assertions,92063E01.77 source profiles include modifier order, signed narrowing,
odd negative Half rounding, positive caps, actual HP stores, previews and execute
proc draws. Native fixtures exercise paid absorbed/nullified casts, Sleep/reaction
separation, prepared type preservation, injected response transport, actual source
item cache consumption and native AI healing/null classification.

Direct magic/affinity/incantation/action/dual Charge/ordinary Charge regressions,
one native Orbonne/Gariland flow, fresh layouts, old-image rejection and twelve
normal/response frame hashes cover the affected closure. All eleven flow PNG
bytes and dates stay unchanged. Geometry/status/timer/source-data evidence is
reused without a full census. Final report: x/core-elements/verification.json.
The standard466045-byte playtest group and live TiEmu remain untouched.

Next: source magic-evasion rules and complete target eligibility, then weather/
Oil/Float and area membership/per-target settlement with reviewed spell/empty-
centre presentation. Reflect/Mime, further reaction/status/weapon families,
Quick/all-freeze liveness, campaign, formats/editors and combined residency remain
open. This milestone does not close L2/L3/L4 or authorize bulk content production.
Jump refinements remain low priority.
