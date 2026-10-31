# Status lifetime and Poison/Regen upkeep

2026-10-26. L2 follow-up to CORE_STATUS_STATE.md. Timed status bitmap commits
now initialize/reset/clear their source counters. Poison and Regen are enabled
through the status API, expire on the battle clock and affect HP at turn end.
Other status effects, incapacitation and reaction contexts remain separate work.

## Rules and ownership

Each chapter Unit has sixteen byte counters for source IDs24..39. Source rows
already archived in FCJ7 supply durations, including36 ticks for Poison/Regen;
no new catalogue or map bytes are needed. Accepted timed applications reset
duration, immunity blocks applications, and removal/cancellation clears the
matching counters. Innate effects remain protected. The source Death Sentence
setter preserves an existing nonzero counter; its separate own-turn decrement
and consequences remain unsupported.

Only IDs24..38 use the ordinary tick path. Inflicted, non-innate bits decrement;
zero underflows to255 as in the original, and reaching zero stages removal.
Slot15 does not tick. The runtime skips absent and Jump actors. A13-bit candidate
mask avoids loading FORMULA when there are no mutable timed statuses. Within
each candidate, the loop stops after the last active bit in each byte; it does
not scan forty statuses every frame. Runtime mutations use `battle_status_apply`
and the shared cure/KO owners so the timer mask and canonical state agree.
`unit_status_seed` is a construction/fixture helper, not an application API.

The scheduler expires statuses after all due Archer Charges and before its
next unit-CT increment. Menu idle frames do not advance timers. Poison/Regen
run once after the unit ends its turn, before searching for the next actor.
The amount is `max_hp >> 3`, rounded down, with ordinary HP saturation. Poison
precedes Regen when both are effective. Absent/Crystal/Dead/Jump/Petrify/Treasure
actors do not receive upkeep. Native HP-based KO is also checked because the
older battle lifecycle does not yet synchronize all canonical Dead handlers.

Lethal upkeep cancels pending Charge, resets the existing death counter and
checks terminal outcome before another CT search. It gives no item/EXP/JP or
attack rewards. Shared KO cleanup applies the original Dead cancellation row
to mutable timed/command effects, preserving innate effects and unrelated
Darkness/Silence. Ordinary damage, splash and Wish self-cost use the same cleanup;
Wish self-KO is counted once. Revival cannot restore a removed poison timer.
This is not a complete canonical Dead/Crystal/Reraise implementation.

Upkeep currently commits engine HP without the original result animation or
floating number sequence. No new spell, item, terrain-Poison producer, Regen AI
spell choice or status artwork is enabled. Existing ordinary pictures are the
presentation regression gate; new effect presentation remains separate work.

## Original evidence and targeted checks

`tools/status_lifetime_oracle.py` executes unchanged original MIPS:

- `battle_status_update_expiring`,0x8018D910;
- `battle_status_apply_poison_and_regen`,0x8018D7C8;
- source bitmap filter/remove/store and `main_status_set_ct`.

78 tick cases cover all fifteen ticked IDs, simultaneous expiry, innate
protection, underflow and the untouched Death Sentence slot.85 timed bitmap
commits cover new/repeated/immune/removed/innate statuses, opposite effects and
the KO cancellation row.42 upkeep stages cover rounding, priorities, innate
effects and blocked/absent actors. The original routines and source tables are
hash-checked. External status-specific callbacks are isolated; timer setters
and removers execute unchanged. This oracle does not certify full original
scheduling, HP commit, reactions or presentation. Ordering is taken from the
source `battle_action_run_between_turn_control` phases and checked in the
native scheduler adapter.

`check_status_lifetime.py` passes4140 assertions on PC, ASan/UBSan and actual
68000, hash D0F9317E, with an explicit13-actor descriptor. It covers the source
cases and actual FORMULA application/renewal/cure/expiry, menu pause, end-turn
HP/rewards, Charge-before-expiry, absent/Jump suspension, damage/Wish/upkeep KO,
revival, last-player defeat and all13 timer-mask bits. Ordinary clock work is
checked to leave the status image unloaded when there are no timers.

The protected-status suite now has4843 checks/hash262BF732: the added assertion
reflects Poison becoming supported and receiving its source duration. Earlier
bitmap/cure/AI/command cases retain their meaning. Existing295 action checks
retain EDCEBFAE. Shared catalogue bytes and their prior B9686645 reader evidence
are unchanged; no new catalogue sweep or exhaustive geometry audit is needed.

## Native memory and measured CPU

ABI11 rebuilds all nine FMB2 images. Unit112 (was96), State2014 (was1804);
the additional210 bytes are13 times16 counters plus the two-byte candidate mask.
Work45516 and FSV3 save3138 remain unchanged. No new allocation is added: the
State/save block becomes5152 bytes. Native allocation formulas request114724
bytes at Gariland,114364 at Orbonne and126732 at the maximum256-node two-floor
capacity. These exclude stack, fragmentation, allocator overhead and combined
supervisor/module/grayscale residency; they are not peak-RAM measurements.

The13-actor expiry batch costs57674 datasheet cycles including a forced cold
FORMULA load, versus88166 before sparse-bit iteration (34.6% less). Two actors'
expiry plus the next clock step costs12070; a prepared Regen upkeep call1848.
One ordinary no-timer clock tick costs7502 and loads no status image. These are
scoped CPU calls, not LCD timings, a complete frame or a worst-case synchronous
`ct_next` certificate. Full bounded scheduling remains an L2/performance gate.
The existing primary-impact fixture peaks49176 cycles versus40876 previously;
the larger FORMULA image adds CPU at impact. Sparse iteration does not make the
whole action path faster. The generated changed status helpers and inlined
timer/upkeep entry contain no software arithmetic calls or division; the older
`formula_amount` retains its existing multiply callback.

## Package and integration

Private `x/core-status-lifetime/fftcampaign.89g`:476684 bytes,42 variables,
+1168 versus CORE_STATUS_STATE. SHA256:
`6dac9e435ae502b53bf127ef4fae128e695ab92f1269652d200e0b9c97ab7a89`.
Only the module bank (+1042, now38672) and packed battle (+126) change; all28
non-code banks are identical. Nine image sizes are5404/3500/4210/5732/2720/
4108/4564/4100/4220. Signature57843 accepts the new bank and rejects captured
ABI10. Largest image5732; packed battle input64789 leaves147 bytes of the
64936-byte TTPack input ceiling. Budget further resident growth before hookup.

Six ordinary action frames and all11 flow PNGs retain their previous bytes/
hashes and image dates. Gariland/Orbonne retain81/14 assertions, AE9CFD31/
2CD70391, matching native save bytes, reward idempotence and trophy overflow.
Archer Charge retains276A2090. The standard466045-byte playtest group and live
TiEmu are untouched. Current provenance/results/source hashes are in
`x/core-status-lifetime/verification.json`.

## Remaining work

Continue with incapacitation/cast interruption and explicit action/reaction
contexts. Other timed effects still need their gameplay hooks before the API
can accept them. Equipment/job innate/immunity producers, Death Sentence/Reraise,
team/control/occupancy policies, terrain status producers and original upkeep
presentation remain open. Jump refinements stay deferred. L2 and the content
production gate are not closed by this slice.

```sh
make -C games/fft/campaign core-status-lifetime-check
```
