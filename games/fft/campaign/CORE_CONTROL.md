# Stop and shared command restrictions

Follow-up2026-10-28: CORE_SCHEDULER.md supersedes the historical Quick/liveness
limitations below. Quick priority/CT consumption, bounded frozen-clock resumes
and quit/cancel during idle wait now pass263 PC/sanitizer/68000 checks3056BFD2.
Retail producers and broader status/action families remain gated.

2026-10-26. L2 follow-up to CORE_STATUS_LIFETIME.md. Stop, Don't Move and Don't
Act are now accepted by the status API, with source durations20/24/24. Player
commands, forecast hit eligibility, AI candidates, movement starts and Charge
resolution share the same restrictions. This does not enable their spell/weapon/
terrain producers, Sleep or the complete incapacitation/reaction machinery.

## Shared policies

`status_control.h` derives a small cache from canonical current statuses.
`Unit.status_flags` retains rider/immortal context in bits0..1; upper bits cache
CT freeze, no movement, no action and no evasion. No field or allocation is
added. Application, expiry and command-state changes refresh the relevant bits;
general status filtering continues to read only its original context flags.

- Stop retains unit CT even when already ready, suspends both decrement and
resolution of a pending Charge, and blocks new commands. Its own duration still
expires on the global clock; clearing/expiry resumes CT and the retained Charge.
An active stopped unit yields at an uncommitted choice boundary without a key.
- Don't Move rejects new movement/path construction and limits actual AI
origins to the current cell. It leaves actions available.
- Don't Act rejects Attack, abilities/items and Defend before RNG, stock or
pending-state mutation. Forecast hit eligibility is zero; AI has no action or
Defend candidate. It leaves movement available. A pending Charge keeps counting,
then fails validation when due if Don't Act remains; clearing it before expiry
permits resolution. Rejection cannot leave `open_resolving` stuck.

Restrictions do not overwrite actual `moved`/`acted` history. Source end-turn
bonuses are suppressed for the blocked command, including the source Perform
condition; both blocked commands still permit Wait. A cure can restore a command
that was not actually spent. The CT sum uses16 bits before its60 cap.

Stop and Don't Act also prevent target evasion. The shared physical preview and
roll now implement the source no-evasion conditions, including an existing
Charging target, which previously still received ordinary evasion. Charging's
damage vulnerability remains unchanged. The cache also represents the source
Petrify/Crystal/Treasure/Sleep freeze and Confusion/Perform evasion conditions,
but those additions remain gated until their other gameplay handlers exist.

Accepted changes invalidate formula/menu/Status/movement/frozen caches and
restart an uncommitted AI selection. A newly illegal player targeting menu
returns to the turn menu. Existing greyed-menu styling is reused. Arbitrary
preemption of committed movement/strike animations, Sleep damage/wake behavior,
original status artwork/animation and general reaction contexts remain open.

## Packaging and safe call boundaries

The first resident build exceeded TTPack's64936-byte input limit. Cold turn-list
construction now belongs to FORMULA; arrow/death-counter cache initialization
belongs to PARTY. Both use existing images/Work storage. The glyph algorithms
and archived patterns are unchanged; the extractor reads their new source file
and preserves the archive bytes. No executing image may be evicted by a callback.

Moving the turn list exposed a callback boundary: BEATS used to call resident
`opening_battle`, whose `turn_start` now needs FORMULA. Scene steps now request
`OPEN_BEGIN` in the existing stage byte. Resident `map_start`/`map_update` resume
the battle only after BEATS returns. PC uses the same boundary. The setup image
tag is invalidated before binding it to a newly allocated chapter Work block.
This is a general phase/callback rule, not a map-ID exception or a second engine.

ABI12/nine images reject the captured ABI11 bank. Signature58100; Unit112,
State2014, Work45516, ModCtx320 and FSV3 save3138 remain unchanged. Native
allocation formulas still request114724 at Gariland,114364 at Orbonne and126732
at the maximum256-node/two-floor capacity. These are not combined peak-RAM or
stack/residency measurements.

Private `x/core-control/fftcampaign.89g`:478024 bytes,42 variables, +1340 versus
CORE_STATUS_LIFETIME. SHA256:
`33d69ac18951e001b79b23c5f5597ee09039a3458c42640e94955fa181af1848`.
Only module bank40050 (+1378) and packed battle payload (-38) change. All28
non-code banks, including FCJ7 and the contour tail, are byte-identical.
Largest image5742; FORMULA4904 and PARTY4712. Packed input64847 (+58) leaves89
bytes. Unchanged State/Work does not mean unchanged resident program bytes.

## Source evidence and targeted validation

`tools/control_oracle.py` executes unchanged original MIPS status-check, action
restriction, evasion, end-turn and AT-list routines. Main freeze-set initialization
also executes.48 status combinations (all40 IDs plus boundaries),84 end-turn
cases and two Stop AT lists supply the expectations. Inputs use dry terrain and
no mounts. Function/disc hashes and decomp revision are recorded; this is not
full water/mount, original action scheduling or Sleep/reaction certification.

`check_control.py`:594 PC/ASan/UBSan/actual68000 checks, hash15C696B3, with13
actors. Besides the source policies, these cover actual player menus/starts,
rejected-action side effects, AI origins/candidates, CT credit, ready-unit Stop,
held/resumed Charge, active-turn yielding, due-time Don't Act rejection/cure,
physical no-evasion, source AT order and actor12's timer-mask bit.

Existing lifetime4140/D0F9317E, protected bitmap4843/262BF732, actions295/
EDCEBFAE and Archer Charge276A2090 remain direct regressions. Native cold13-
actor AT construction costs50088 cycles, then18370 warm. The no-timer clock
fixture costs8390 (previous7502);13-actor cold expiry70598 (previous57674).
These include the changed cache/image dispatch conditions, not a full frame.
The renderless impact fixture now often starts with FORMULA already loaded by
the relocated AT builder; its lower number must not be advertised as an overall
action speedup. Full bounded scheduling/frame-budget certification stays open.
The emitted control classifier/unit-refresh code contains no software arithmetic
or division; existing formula/AT helpers are outside that claim.

Darkness/Silence433/A5131B09 also pass. Six ordinary action frames retain their
accepted PC/native hashes; all11 flow PNGs are byte-identical and retain their
dates. Gariland/Orbonne preserve81/14 assertions (AE9CFD31/2CD70391), matching
native save bytes, reward idempotence and trophy overflow. The standard466045-
byte playtest group and the user's live TiEmu remain unchanged. Final package,
source hashes and scoped results: `x/core-control/verification.json`.

## Next work

Implement Sleep's damage/wake and incapacitation interaction, then explicit
action/reaction contexts and safe interruption ownership. Keep Petrify, effective
teams, Death Sentence/Reraise and other effects gated until all their handlers
have fixtures. Trait/terrain/ability producers and original effect presentation
remain separate. Jump refinements stay deferred; full L2/content production
gates remain open.

```sh
make -C games/fft/campaign core-control-check
```
