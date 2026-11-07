# Root actions, impact ownership and bounded response contexts

2026-10-26. L2 follow-up to CORE_SLEEP.md. The live primary-action path now owns
an explicit transient context. The response collection/drain primitives and
source eligibility query are checked prerequisites; no reaction ability or
multistrike executor/producer is enabled by this checkpoint.

## Runtime invariants

`CoreAction` adds32 bytes per battle, at the end of State. Unit/Work/save layouts
and allocation count are unchanged. It keeps the root actor, current actor,
phase, scan cursor, a13-bit pending mask and one16-bit source reaction ID per
actor. Beginning a root clears the mask without clearing unused array entries.
The mask owns validity. No dynamically sized queue or recursive callback stack
is introduced.

- Attack, ability/item, Defend and due Charge start a root before presentation.
  A merely armed Charge has no live impact context. A new order while the root
  is active rejects before RNG, items, pending-state cancellation or reset.
  Out-of-range actor/target IDs and KO users also reject before dereferencing.
- FORMULA consumes the impact phase once, including a miss. Repeating impact
  dispatch, replaying ACT_HIT or attempting a hit after a committed miss cannot
  reapply HP/status/recoil, costs, rewards or actor self-cost. The current live
  command still has one strike; this is not a multistrike implementation.
- Settlement closes the root before outcome checks, next-turn selection or
  returning to the menu. A pending response prevents closure. While waiting,
  the presentation frame is held at its end instead of wrapping its byte clock.
- Response collection accepts one request per non-root actor, up to twelve in
  the13-actor envelope. Invalid source IDs/actors and duplicate requests return
  explicit reasons without overwriting an accepted request. Collection closes
  when draining starts; a secondary context cannot generate more reactions.
- Drain order is actor-slot order, independent of insertion order. The caller
  supplies freshly revalidated eligibility; excluded requests expire with the
  root. A response must return ownership to the root before the next response
  or closure. The storage supports actor12 and source IDs above255.

`battle_reaction_blocked` checks live canonical status, current primary/secondary
context and an already-queued response through FORMULA. It adds native owner,
presence and HP checks. Its source policy currently covers dry, unmounted actors;
mounts, deep-water action restrictions and the Reflect exception need their own
adapters before those mechanics become playable. Global weapon-proc/reaction
conflicts are tested in the pure policy but have no live producer yet.

There is deliberately no live producer calling the response queue yet. The
integration fixture stages/drains contexts without claiming Counter effects or
animation. Next, the reaction executor must own a separate result/presentation
packet, restore the primary packet and actor command, revalidate the reactor,
apply that family's costs/target policy and only then settle the root. Hamedo,
Reflect, weapon procs, Mime and multiple strikes need distinct source fixtures;
they must not be inferred from a generic recursive damage callback.

## Original evidence

`tools/context_oracle.py` executes the original status-set initializer and53
reaction-block inputs, including all40 IDs, primary/secondary contexts and
existing global/unit reaction conflicts. The source prevention mask is
`64 95 0e 02 14`. Three original pending-selection fixtures verify slot order,
skipping the root and a stopped reactor, including actor12. The selected unit's
command save and context switch execute; counter construction, MP validation
and strike initialization return success through explicit runner hooks.

These observations are policy/selection evidence, not an original complete
two-strike/Counter/Auto-Potion battle trace or a Brave-probability certificate.
Function/disc hashes, hooks and revision: `x/core-context/oracle/source.json`.

## Packaging and tests

ABI14/nine images reject the captured ABI13 bank. Cold command-start/reset/facing
moves unchanged into FORMULA alongside context initialization; resident callers
invoke it only after any previous image has returned. No executing image is
evicted by a callback. Query/commit/start dispatch use existing scratch fields
and ModCtx320. FCJ7 and all non-code banks remain unchanged.

Native Unit112, State2046 (+32), Work45516 and FSV3 save3138. The five-block
allocation formulas grow by32 bytes: Gariland114756, Orbonne114396, maximum
256-node/two-floor board126764. These formulas do not certify combined program,
heap, stack, fragmentation or real peak residency. There is no additional heap
allocation. Private group478570 bytes (+434),42 variables. Module bank40720
(+496), FORMULA5564, largest image5742 and signature58710. The packed battle
payload shrinks62 bytes; its64727-byte input leaves209 below TTPack's64936 cap.

357 PC/ASan/UBSan/68000 context checks pass with hashF24887B4. Cold actual Attack
start costs41644 cycles, prepared5970; the prepared primary-impact case5960.
The cold start now loads FORMULA, so these are not a start-speedup claim.
Action/regression zones are scoped rule/dispatch costs, not whole-frame bounds.

Primary actions295/EDCEBFAE, lifetime4140/D0F9317E, protected bitmap4843/262BF732,
controls594/15C96953, Sleep1070/B6C26F11 and Charge276A2090 pass PC/native direct
regressions. The context fixture also runs actual AI_AIM dispatch through root
completion after its HUD wait. Six ordinary action pictures and all11 flow PNGs
retain their accepted bytes and original dates. Gariland81/AE9CFD31 and
Orbonne14/2CD70391 pass fresh PC/native rules, settled save equality, reward
idempotence and trophy overflow. All28 non-code banks remain byte-identical.
These checks do not certify every original reaction/strike or whole-frame budget.

`check_context.py` covers source guards, reversed/full-capacity queue insertion,
duplicate/invalid/nested requests, phase ownership, impact-once misses/hits,
blocked new orders and their side effects, runtime settlement, bounded waiting,
Charge, mutable status revalidation and native cold/prepared command starts.
Raw injected-impact fixtures now seed a context; independent successive action
fixtures close the previous committed context when skipping its presentation.
This preserves their original behavior expectations instead of weakening the
new ownership invariant for tests.

Final measurements, direct regressions, native layout, accepted action/flow
hashes and package comparison: `x/core-context/verification.json`. The standard
playtest bundle and the user's live TiEmu are left untouched.

```sh
make -C games/fft/campaign core-context-check
```
