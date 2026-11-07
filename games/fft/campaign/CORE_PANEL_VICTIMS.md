# Weapon panel victims and recipient previews

Subsequent integration: [direct projectile interception](CORE_DIRECT_INTERCEPTION.md)
replaces the historical intermediate-body blocking policy described below.

2026-11-06. The interactions delivery remains open; generic coverage stays
201 actions and78 R/S/M consumers. This follows CORE_ARCS.md.

## Behavior

Once weapon geometry validates a recipient panel, every targetable occupant of
that exact tactical cell participates, in stable unit-slot order. A different
terrain layer is a different cell. KO/Stone occupants remain in the list and
their ordinary effect guards still apply; absent, Jump/airborne and crystal/
treasure actors are omitted. Native mounted content is not added here.

One hand prepares every victim's hit, damage, critical, recoil and weapon-proc
packet before any impact changes live units. Existing scalar presentation then
shows those victims in order. All main-hand victims precede the second hand.
The second hand revalidates the original selected panel after recoil/KO; a
frozen victim within the current hand does not receive another formula roll.
Late invalid recipients reject at impact. There is one root command, payment
and reward settlement, and queued reactions wait for the whole sequence.
Each victim keeps its own deferred status/spell/drain packet. Reversed drain
can kill the actor; a later positive drain cannot revive that actor implicitly.

The original resolve-strike/list-units routines establish per-hand preparation
before effects and stable slot order. The native presentation deliberately
uses one existing animation per victim. It does not recreate simultaneous PS1
numbers or certify the complete original dispatcher/RNG schedule.

Direct weapon rays now treat endpoint cooccupants as members of the same
selected panel, while intermediate bodies retain their prior blocking policy.
The generic nonweapon/AI ray keeps its previous behavior. The same source
trace template supplies both variants.

The scalar preview uses the same lunge/arc recipient resolver. Its right-hand
portrait/name card now identifies that recipient. A fully blocked attack shows
`Miss 0%` with no recipient card. For several occupants, the first slot's
ordinary forecast is followed by `xN`, the number of listed occupants; this is
not an aggregate damage total or a new per-victim inspection menu. The existing
portrait bank, font, fitted box and renderer are retained. Queries preserve
units, pending casts, action ownership, inventory and RNG.

## Native ownership and capacity

ABI75/ModCtx348 intentionally supersedes the ABI74 preparation arrangement:

- PLANNER24 op1 resolves geometry and the panel mask; op0 keeps the AI geometry
  cache, op2 the pure line preflight, op3 the pure preview query, op4 preview plus
  a deferred NUMBERS6 request, and op5 the next victim/hand query.
- FORMULA5 op14 prepares or restores the frozen weapon packet, after PLANNER
  returns. It owns the former weapon RNG/formula body. No image loads another
  image while executing. A continuing frozen victim skips geometry and RNG.
- WEAPON22 keeps impact and scalar-cache op6. RESPONSE11 launches the existing
  animation with the stored hand. RESULTS7 no longer owns strike-target op4.

The existing post-owner request flags now include16=NUMBERS6 and32=FORMULA14.
A resident table preserves the old priority and return-value policy for4,
NATURE's combined3 and STATUS's8. Only STATUS's result propagates its KO/status
refresh flag. The action `left` byte keeps its low remaining-hand count and
uses bit128 while frozen occupants remain; never treat that bit as a hand count.

WeaponPanel stores thirteen10-byte packets plus six bytes of control. Together
with two preview bytes it adds138 transient Work bytes:46770. Unit112,
State2224, Save3202/FSV4, ModCtx348 and five heap blocks remain unchanged.
The execution zone shrinks8 bytes to8596; total Work/zone requests grow130.
25 images occupy8/9/8 shards58230/61718/56916,180 archived bytes more in total.
Actual non-BENCH raw64895/TTPack64897 leaves39 bytes; packed data39776.

## Evidence and boundaries

The original eligibility and panel-list instructions execute unchanged with
only map-location lookup isolated. The128 fixture executions contain24 unique
same-panel controls, with normal/KO/Stone, absent/crystal/treasure/Jump/mount
states and both stable tie-order modes. The mounted exclusion is a source
control, not new native mounted content.

All three focused carriers agree on PC, ASan/UBSan and actual68000:

- 527 behavior/preview checks,8864C62B;
- 33 actual HUD pixel/card checks on a neutral background,3FEC3500;
- 66 full13-slot checks with twelve panel occupants,C32255C0.

The plain-weapon full-panel cold preparation costs325340 datasheet cycles, including owner
loads. This is a preparation-zone measurement, not a complete rendered frame.
Frozen packet preparation and panel collection add no32-bit arithmetic helper.
The straight-ray setup retains its inherited32-bit divisions.
No authored ASM is added.

All16 initial affected regression suites pass.
The endpoint-ray correction changes only PLANNER24; the focused carriers and
five direct dependencies are rechecked on the final export. This adds the
weapon-reach suite. Five dispatcher/bootstrap dependencies also pass, for22
unique regression suites overall. The equipment fixture explicitly opts into
its existing attack_damage diagnostic API; no assertion or production code is
removed to repair its previously missing native symbol. Native
Gariland81/AE9CFD31 and Orbonne14/2CD70391 retain pixels, rules, exact settled
saves and repeated-payment/trophy-overflow checks. Loader parser140 and exact
bindings227 pass, including50 cold/50 warm loads and three chapter rebindings.
A real matching three-shard ABI74 export rejects before binding or execution.
The final active image/header bytes remain identical after chapter-only guards.

Direct projectiles retain their previous intermediate blocked-ray policy;
this slice does not add direct-projectile interception. Complete mounted/cooccupant height
producers, collateral/innate/Reflect AI utility and wider original dispatcher
fidelity remain in interactions. Full tactical-frame, worst stack, residency
and hardware checks remain the following stabilization/release delivery.
Standard groups, protected saves/proofs with bytes and dates, and the live
TiEmu session are preserved. Reports: x/core-panel-victims/verification.json.
