# Temporal and weapon-planner interactions

Current continuation: [CORE_MELEE_COLLISION.md](CORE_MELEE_COLLISION.md).
ABI74 deliberately splits scalar-cache ownership back to WEAPON22 op6 while
PLANNER24 owns strike preparation and origin geometry. The ABI73 ownership and
size checkpoint below is historical.

2026-11-05. Continuation of the remaining-interactions delivery after
CORE_WEAPONS.md. This closes its next-turn/weapon-utility slice, not the whole
interaction delivery. Generic coverage remains201 actions and78 R/S/M consumers.
There is no additional generic-family queue.

## Runtime behavior

The AI next-turn estimator now uses the battle clock's actual rules rather
than division by raw Speed. Finite Haste/Slow and Stop/Sleep expire before
that tick's gain. Slow wins simultaneous Haste; Speed1 plus Slow can truly
stop gaining CT, and Speed0 never divides by zero. Innate blocking effects
remain permanent. Mutable timer0 deliberately wraps and expires after256
steps. An already-ready frozen actor still waits for its wake tick. Absent,
KO, Petrified and airborne actors have no ordinary next-turn estimate.

The implementation skips constant-rate segments; it does not simulate every
clock tick per AI candidate. AI_SETUP caches all13 next-turn estimates at the
start of the plan. Charge's existing escape-risk discount reads that estimate.
The existing AT-list forecast keeps its separately source-checked display
contract, including its Speed0/Slow display clamps.

Weapon plans now add the actor's drain benefit or Undead drain cost and a
bounded19% expected value for conditional status changes and free spell HP.
Canonical source filtering removes immune/innate/incompatible status requests;
Frog toggling values removal instead of another infliction. Cancellation cannot
cure an innate ailment. Spell utility uses the post-physical HP, source Faith,
affinities and magical hit estimate, without drawing RNG or paying anything.
Lethal physical hits and Golem suppress follow-up value. Each hand and learned
Charge row has its own forecast; a dead actor/target stops the next hand.

This remains a native utility heuristic, not a claim of retail AI equivalence:
status weights, expected proc rounding, MP Switch/reaction probabilities and
compound future status transitions are not a full dispatcher simulation.
Reflected proc utility is deliberately omitted from this origin-independent
cache: its recipient depends on the candidate origin. Wider Reflect/area AI
and exact projectile/lunge/overlapping-panel behavior remain in the open
interaction delivery. The separate TI stabilization/release delivery remains.

A separate integration fix propagates a weapon spell's KO change back to the
monolithic impact owner. Both PC and native paths now refresh the AT list when
the free spell, rather than the physical blow, kills its target.

## Native ownership and capacity

ABI73/ModCtx348 adds PLANNER24, which owns cold weapon scalar/utility preparation.
The existing four actor/target pairs per update and PC/TI quanta are retained.
WEAPON22 retains physical preparation and impact; it never loads another image.
AI candidates read cached values and retain local range/hit/trajectory queries.
No running image evicts itself. The new image avoids growing the execution zone.

Unit112, State2224 and Save3202/FSV4 remain unchanged, with five heap blocks.
Work46118 grows260 transient bytes:234 utility bytes and26 next-turn bytes.
Twenty-five images share the unchanged8550-byte zone, in8/8/9 archive shards:
56268/55488/60238 bytes. The actual non-BENCH raw variable is64929 bytes;
TTPack input64931/64936 leaves5 bytes. The packed private data file is39902 bytes.
No standard group or live TiEmu session is rebuilt or modified.

## Evidence

make core-ai-interactions-check compares the estimator with the actual ct_next
scheduler on528 combinations of Speed0/1/9/50, CT0/91/100, finite/innate tempo
and blocking states, and timer0/1/3/17. An independently ticking frozen witness
counts actual elapsed ticks across scheduler quanta; this checks expiry order
and zero progress without duplicating the estimator's segment calculation.
The focused suite also checks pure cache preparation, drain/Undead, innate and
immune status requests, Frog removal, lethal/Golem suppression, Fire null/absorb,
Charge rows, both hands, a real planner decision and spell-KO AT-list refresh.

PC, ASan/UBSan and the actual68000 agree on21127 assertions, digest5F8E8515.
The largest observed four-pair cache quantum is41476 cycles. This is a scoped
measurement, not a complete tactical-frame certificate. Existing area AI keeps
756 checks/F9A74013,68 steps and its311562-cycle area quantum. Its general stage0
still peaks12334382 cycles, so the full-planner360k gate remains explicitly red.

Exact native loader checks cover227 bindings,50 cold loads,50 warm preloads,
three stale chapter rebindings and malformed/missing/swapped/signature failures.
An actual complete ABI72 three-shard export is rejected before any binding or
execution. Generated code for the clock helper, inlined AI scoring and planner
entry/scalar/HP utility contains no32-bit arithmetic or float helper call.

The final scoped closure, campaign flows, source hashes, packed size and protected
save/proof preservation report are recorded under x/core-interactions-ai/.

## Next interaction entry points

The source weapon target validator dispatches by raw weapon ARC/DIRECT/LUNGING
flags, whereas the native battle_line_clear entry still routes all ranged
returns through AI's ai_clear straight ray. Read these local reference functions
before changing geometry: battle_target_validate_weapon_target,
battle_target_validate_lunging_target, battle_effect_check_arc_trajectory_between_units,
battle_effect_set_and_validate_arc_trajectory and
battle_target_list_units_on_panels. The last lists every eligible actor on marked
panels in slot order (or stable distance order for line effects); finding one
unit_at_cell is not a complete overlapping-panel victim implementation.
These are remaining integration points, not new missing generic abilities.
