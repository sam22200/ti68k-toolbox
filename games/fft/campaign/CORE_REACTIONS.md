# Counter and Auto-Potion execution

Follow-up: [CORE_STRIKES.md](CORE_STRIKES.md) adds bounded consecutive primary
impacts before the response drain. ABI16/CoreAction34 stores a fixed tile and
one extra strike; the deferred reward byte now holds best earned EXP. The
measurements below describe the preceding ABI15 reaction checkpoint.

2026-10-26. L2 follow-up to CORE_CONTEXT.md. The first two reaction families now
execute through the bounded context, including packet suspension/restoration,
inventory costs and final settlement. This is not a certificate for all reactions,
weapon procs, Hamedo/Reflect/Mime, dual weapons or multiple strikes.

## Shared effect lifecycle

Counter uses the original ability's physical-reaction flag, the target's
pre-impact status and a Brave threshold. It may queue on a miss. Auto-Potion
uses successful HP-damage type after HP/KO/status resolution; zero damage can
trigger it, and a sleeping target can wake and then qualify. KO, Stop/Don't Act
and other represented prevention statuses block the appropriate stage. A
secondary effect cannot queue more reactions.

Pending actors resolve in slot order. Before preparation the engine checks live
status, HP/presence, Counter range and the current stock. Existing native ranged
trajectory policy is shared with targeting; the resident coordinator returns
from ACTION before loading AI for the line query, then reloads ACTION. It never
evicts an executing image. Full original bow ballistics/movement traits remain
in the L1/L3 backlog, not silently certified by this reuse.
At most one rejected trajectory is evaluated per update, spreading multiple
blocked ranged requests over frames instead of synchronously draining all of them.

Auto-Potion selects Potion, then Hi-Potion, then X-Potion. Friendly actors and
guests share the party bag; enemies use their independent Potion supply. The
item leaves stock once at secondary preparation, including a zero-effective-heal
case. Selection rechecks stock after earlier queued actors consume it. Duplicate
preparation cannot consume again. Team currently represents immutable affiliation;
Charm/effective-team work must preserve initial inventory ownership before enabling
that producer. Undead/item-affinity and mount/movement trait producers remain gated.

Counter eligibility comes from the archived source flag, not a broad native
"physical damage" category: the source Dash and Throw Stone records do not set
that flag. A ranged reaction is revalidated after the incoming action commits;
a newly intervening actor can cancel the return shot.

A20-byte packet preserves the primary actor/target, ability, timing, damage,
self-cost, hit/critical/recoil coordinates and reward presentation. Child actions
reuse the ordinary attack/item animation and commit once. Returning restores the
primary packet and invalidates affected presentation caches. It does not restore
HP, inventory, CT, command history, live positions or independently armed Charge.
Counter uses the existing attack roll/evaluator and recoil; Auto-Potion uses the
existing item effect. The reactor keeps its pending cast and turn credits.

When reactions are pending, primary EXP/JP wait until all responses finish.
The primary reward display then resumes once. A lethal Counter prevents the KO
primary actor from receiving that deferred award; reactions receive no EXP/JP.
Victory/defeat waits until responses and the root presentation settle. Unconfigured
ordinary actions retain their previous effect/reward timing and accepted images.

## Equipped data and packaging

FSV3 already stores equipped reaction IDs. Party loading now copies them into a
separate thirteen-entry source-ID array; generated/fixed encounter traits are
not inferred from learned bits. Counter/Auto-Potion are the only active new
families. Other reaction effect handlers remain future work. Initial save creation
preserves the equipped ID; existing party saves keep their selection and layout.

ACTION is the tenth existing-format overlay. It owns impact commit, start/reset,
reaction preparation/return and rewards; formula/clock/status requests keep FORMULA.
The evaluator is local to ACTION for splash effects, just as it is local to AI.
Resident hit/amount calls also reuse that evaluator while ACTION is loaded;
they do not copy FORMULA over it just to evaluate the same function.
The existing resident attack-roll callback is reused. Rewards move unchanged into
ACTION to remain below TTPack's input ceiling; neither ACTION nor its callbacks may
load another overlay while ACTION is executing. ABI15 rejects the previous bank.
Unit/Work/save layouts and the allocation count stay unchanged; the transient
reaction IDs plus packet require46 additional State bytes. No new art bank or
catalogue format is needed.

## Evidence and limits

`tools/reaction_oracle.py` executes unchanged original MIPS for27 finalize/staging
cases,16 stock choices,20 Brave boundaries and two EXP contexts. Actual original
HP/status resolution and Counter/Auto-Potion dispatch run. Graphics, equipment,
MP Switch and unrelated bookkeeping are isolated; the BIOS random veneer supplies
the fixture roll. The source action-phase code selects reacting units before
post-action rewards. This is not a complete original secondary animation,
JP/weapon-target or multistrike trace. Hashes/hooks/revision are retained in
`x/core-reactions/oracle/source.json`.

Twenty-six staging rows are producible by the currently enabled native ability
families; the remaining synthetic healing-plus-physical-flag row is source-only.

`check_reactions.py` covers the producible original staging subset, actual Counter
and all three Auto-Potion items, misses/zero damage/Sleep/KO, delayed root rewards,
lethal Counter/terminal conditions, same-stock competition, guest/enemy stock,
packet restore, duplicate commit/preparation, preserved CT/Charge, source save
loading and clear/blocked ranged overlay transitions. Prior context and primary
action/status/Charge fixtures remain direct regressions. No emulator input or
standard playtest bundle is changed. Final metrics and scoped evidence live in
`x/core-reactions/verification.json`.

| Scoped check | PC/sanitizers/68000 result |
|---|---|
| Reaction execution/packet/stock |267 assertions, `EF8107A7`|
| Root/impact ownership |348 assertions, `B05660E8`|
| Existing primary effects/rewards/AI |295 assertions, `EDCEBFAE`|
| Charge |`276A2090`, original preview/commit and delayed-action callers|
| Gariland setup/save/results |81 checks, `AE9CFD31`|
| Orbonne setup/handoff |14 checks, `2CD70391`|

The six ordinary action frames retain their previous PC/native hashes. Six
response-state snapshots also match between PC and68000: Counter and Auto-Potion
pre-impact, impact and restored primary states, using existing animation/art.
All eleven flow PNGs keep their bytes and modification dates. The clock/status
regressions were rebuilt at ABI15 before the final local-evaluator dispatch and
invalid-LoS yield optimization: lifetime4140, protected status4843, control594,
Sleep1070. Their policy/timer code and data/layout did not change; final
reaction/context/action/Charge callers and flow were checked after optimization.

| Storage | Previous context checkpoint | Reaction checkpoint |
|---|---:|---:|
| Private archived group |478570|482578 (+4008)|
| Raw image bank |40720|44920 (+4200)|
| Packed battle payload |42283|42091 (-192)|
| Battle TTPack input |64727|64605 (-122)|
| Remaining TTPack input headroom |209|331|
| Native State |2046|2092 (+46)|
| Native Unit / Work / FSV3 save |112 /45516 /3138|unchanged|
| Largest image / ModCtx |5742 /320|unchanged|

Only the battle payload and image bank change in the42-variable group. All28
non-code banks match the preceding checkpoint; the old ABI14/nine-image bank
rejects. Five-block requested battle storage is114802 bytes for Gariland,
114442 for Orbonne and126810 at the maximum two-layer topology. These are
allocation formulas, not peak stack/fragmentation/program-residency measurements.

Cold native Counter preparation is45374 cycles in the scoped fixture. Primary
start is42968 cold/6226 prepared; prepared impact is6492. Existing action
commit calls average13182, peak50576 in the23-call suite. Moving evaluator calls
onto the already resident ACTION image removes measured ACTION/FORMULA copy
ping-pong; these zones exclude rendering and do not certify a complete frame.
Generated chance/stock C wrappers have no software32-bit multiply/divide helper
or division instruction. No new assembler was authored.

Next: source-backed strike sequencing and further reaction families with distinct
target/cost/interrupt policies. General timing/residency, producers, campaign
contracts and content-production gates remain open; Jump refinements stay deferred.

```sh
make -C games/fft/campaign core-reactions-check
```
