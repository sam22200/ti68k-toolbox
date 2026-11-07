# Complete native weapon effects

Current panel/preparation ownership: [CORE_PANEL_VICTIMS.md](CORE_PANEL_VICTIMS.md).
The checkpoint below is historical.

Current ownership and collision integration: [CORE_MELEE_COLLISION.md](CORE_MELEE_COLLISION.md).

Historical continuation: [CORE_AI_INTERACTIONS.md](CORE_AI_INTERACTIONS.md) moves
cold forecasts to PLANNER24 and adds temporal/drain/conditional utility. The
weapon-effects checkpoint and its measurements below are historical.

This is the first of the three final combat deliveries: weapon effects together,
remaining interactions, then TI stabilization/release. Generic coverage stays
201 actions and78 R/S/M consumers; there is no new generic-family queue.

## Delivered rules

The existing physical bases now share the source weapon formula records with:

- Physical elements, XA strengthening, half/weak/null/absorb affinities.
- Healing weapons23/93, their pre-Charge zodiac stage and Undead reversal.
- Draining staff61, including reversed actor/target effects on Undead.
- Magical guns74..76: source10%/30%/60% spell tiers, WP plus Charge, magical
  supports, Shell/Frog/Chicken, zodiac, weather, Faith/Innocent and affinities.
- The source19% conditional draw for physical formulas1/2, including weapons
  whose proc ID is zero. Nullification stops before that draw. Formula1 adds or
  removes the source status set; formula2 schedules its actual spell ID.
- Conditional ailments/buffs, immunity, innate preservation, cancellation and
  timers through the canonical status primitives. Charm also uses the existing
  camp/origin/restoration lifecycle. Frog removal is frozen at preparation.
- A forced single-target spell after a surviving physical impact, with existing
  magic/evasion/Reflect/resource/reaction rules and one root reward settlement.
  It spends no additional MP, inventory or CT. Golem and lethal physical damage
  suppress it; MP Switch retains its ordinary per-impact eligibility.

Ordinary positive scalar results retain the native999 cap. Charge retains its
existing uncapped scalar contract, including forecasts, before bounded HP
settlement. Signed halfword results keep the existing magic_hp interpretation.
Healing weapons and both gun formulas bypass the four ordinary evade draws
only after applicable source guards. Healing and magical guns draw no critical;
magical guns draw their tier instead. Variable physical bases precede criticals.

Twenty-one further source-compatible weapons now permit Two Swords, including
those with elements/procs/healing/drain. Each hand independently selects metadata,
RNG and outcome. The second hand is re-targeted by the existing panel executor;
only a damaging first hand predicts mutable Sleep waking. An absorbed/healing
or null first hand does not falsely wake it in the forecast.

## Owners, data and memory

FCJI50330 appends128 raw8-byte source weapon records (+1024 archived bytes) to
FCJH. Both opening/campaign readers and wrappers agree; the raw rows stay in
Flash. The existing item hand bits change for those21 weapons. No Unit, State,
SaveUnit, save-version, inventory, art or portrait field is added.

ABI72 removes the unused resident attack_prediction callback, retaining pure
weapon_amount. RESPONSE/COMBAT request preparation and return before WEAPON22
loads. WEAPON owns preparation, primary HP/drain/status commit and pure weapon
forecasts. STATUS23 owns the existing scalar/batch bitmap commits and explicit
status API, plus the deferred spell follow-up. The loader drains requests only
after the executing owner returns; no image replaces itself on its own stack.
The prepared effect fits the existing ActionPacket.self word, so Counter,
Hamedo and Mime already preserve it without a larger packet.

AI forecasts remain local to the executing image. WEAPON precomputes ordinary
and learned Charge scalar rows for the current actor/targets, four pairs per
update. Only origin-independent amounts/kinds are cached; reach, facing, hit
chance and obstruction remain candidate-local. This adds250 transient Work
bytes, avoiding a larger execution zone and repeated candidate image swaps.
Previews/cache construction consume no battle RNG.

Native layouts: Unit112, State2224, Work45858, Save3202/FSV4, ModCtx348;
five allocation blocks.24 images share one8550-byte zone in8/8/8 shards.
The private actual non-BENCH packed input is64927/64936 (9 bytes left), versus
64929 before this delivery. The paired raw variable is64925; TTPack also packs
its two-byte length prefix. Resident growth must still be budgeted carefully.

## Evidence

weapon_oracle.py executes unchanged hash-verified retail formula instructions,
including conditional proc selection and canonical bitmap filtering. Only libc
rand values, weather lookup, recoil and the external drain finalizer are
isolated. Fixtures cover all125 raw weapon entries0..124 (including bare hands), deterministic previews,
all special records, signed/scalar modifiers, exact magical-tier and19% boundary
values, Charge and high legal stats. All128 archived raw records are compared
against the disc by check_common.py.

make core-weapons-check compares source fixtures and real commands on PC,
ASan/UBSan and the actual68000 runner. Integration includes repeated impact,
healing/drain/overkill/Undead, cancellation, late immunity, Charm/Frog lifecycles,
Oil, Golem, MP Switch, one-time payment/rewards, both hands, due Charge,
Counter/Hamedo, borrowed Mime weapons, Reflect and bounded AI cache queries.
The770 original profiles feed4741 assertions (E4AB8000), equal across all three
execution modes. The earlier288 physical primitive profiles and their2421 assertions are retained;
only the execution fixture's newly implemented conditional draw and raw primitive
contract are updated, without dropping an assertion.

Closure covers49 distinct affected suites:47 broad regressions,19 targeted
rechecks and final repaired carrier runs. The64 native area-effect pixel samples
retain F9BACFF5. Loader checks cover219 exact bindings,48 cold/warm loads and an
actual prior-ABI71 three-shard rejection before execution. Gariland/Orbonne keep
81/14 checks (AE9CFD31/2CD70391), exact settled saves and repeat/overflow protection.
All560 pre-existing protected save/group files retain bytes and dates.

Production-style renderer carriers explicitly omit unused diagnostic weapon
APIs; direct fixtures opt in with FFT_WEAPON_TEST_API. The two size-bound
carriers compile their unused runtime loop as --frames0 and omit only unused
loop zone names. Their own scripted frames, all assertions, all pixel hashes
and their timing markers remain. Production archives and the final raw build
are byte-identical after this diagnostic-only cleanup.

Final counts, regression scope, native loader binding/rejection, campaign flows,
codegen, paired size measurements and preservation are recorded in
x/core-weapons/verification.json. Source/retail fixture bytes stay local.

## Native bounds and the two remaining deliveries

Conditional spells resolve within the existing impact/response lifecycle. The
native renderer retains its current weapon presentation and HP/status panels;
this does not recreate the PS1's separate spell-proc animation script/timing.
Reflect reuses the existing one-bounce layer rule and bounded reaction policy.
Source actor/element/effect rules are shared, while the original full dispatcher
and complete action-RNG stream are not certified by a primitive oracle.

The remaining interaction delivery includes exact projectile/lunge collision and
complete overlapping-panel victims, broader innate/AI/future-CT/Reflect fidelity
(including proc utility in planning). TI stabilization remains the full tactical
frame, worst-stack, heap/residency and release/hardware checkpoint. A full
campaign and remaining monster content/art are separate content work. Standard
groups and the live TiEmu session are not changed by this core delivery.
