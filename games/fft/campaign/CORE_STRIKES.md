# Consecutive primary strikes before reactions

Follow-up: [CORE_HANDS.md](CORE_HANDS.md) connects ordinary supported Two Swords
hands, Counter, management and shared forecast/AI. ABI17/twelve images and FCJ8
supersede this page's ABI16/FCJ7 measurements. Broader weapon/skill families
remain gated; the earlier sequencing evidence below keeps its original scope.

2026-10-26. L2 continuation of CORE_REACTIONS.md. The shared engine now accepts
one or two ordinary Attack strikes under one root. This is the sequencing
foundation, not an enabled Two Swords passive: independent hand equipment,
weapon formulas, hand-specific preview/AI and source target rebuilding still
need their own fixtures before a content producer may request two strikes.
Normal player/AI Attack continues to request one. Charge and other abilities
do not inherit repeat behavior from this entry.

## Original evidence

`tools/strike_oracle.py` runs unchanged local-disc MIPS instructions for32
strike initializations, five continuation/phase branches and ten best-EXP pairs.
The initializer distinguishes both empty hands, both weapons and a single
weapon moved from the left slot; the Two Swords support flag yields two only
for eligible menu/hand combinations. Jump and items do not simply inherit it.

The resume handler takes its continue-strike branch before advancing to the
reaction phase. Once that phase is exhausted it reaches reward handling.
Animation, UI, object getters, announcements and reward callbacks are isolated
and logged. This is a phase-control observation, not a complete original
double-strike battle/animation/retargeting certificate. The source EXP function
keeps the highest successful award across the action, rather than summing every
strike; kill-count and level-difference/clamp boundaries are included.
Fixture inputs, hooks, disc/function/decomp hashes stay in
`x/core-strikes/oracle/source.json`.

## Native ownership

`attack_sequence(actor,target,count)` rejects counts outside1..2 before pending
casts, RNG or command state change. `CoreAction` adds two bytes: extra primary
strikes left and the fixed compact tile. Tile255 remains a valid byte identity.
The response drain and root close both refuse while strikes remain.

After a committed primary animation, FORMULA finds a live target on the stored
tile and validates the current actor/range. It returns before the resident
coordinator requests a ranged AI trajectory. Only then does FORMULA prepare the
next animation/roll and rearm the primary impact. It cannot load another image
while executing. This uses existing poses and redraw invalidation; there is no
new sprite or UI style. The bounded ordinary-Attack adapter skips an empty/KO
tile or an actor that can no longer act. It does not follow a recoiling unit.
These adapter choices are checked native policy, not claimed as the original's
complete ability-specific targeting rules.

Each impact commits once. Queued Counter/Auto-Potion remains pending until all
primary strikes finish; duplicate requests do not reroll Brave or use a second
queue entry. One reaction per actor still resolves in slot order without
recursion. CT, inventory, command history and independent pending casts remain
world state. A blocked/empty follow-up settles previous effects and responses.

The deferred reward byte now stores the best earned EXP amount, not a kill
marker that could later be recomputed against a different target or KO count.
JP/EXP are applied once after responses. A first hit followed by a miss retains
its award; a lethal Counter prevents the dead primary actor from receiving it.
The immediate single-strike path and existing JP/spillover formulas are retained.
The source zero-KO-count fallback also avoids dividing by zero on injected
reward records. ABI16 rejects older banks despite unchanged packet/save layouts.

## Validation and remaining scope

`check_strikes.py` checks actual two-impact animations/commits, Counter and
Auto-Potion after the last strike, one stock consumption, duplicate impacts,
miss/miss, original best-EXP pairs, early KO/recoil/Don't Act/dead actor,
invalid counts without side effects and one final root settlement.
Fresh replacement occupants and clear/blocked ranged continuations exercise
the return boundary between FORMULA and AI on the actual68000.
Direct regressions are reactions, root contexts, primary actions and Charge, plus
normal action pictures and one pilot flow after the shared ABI change.
No all-map census, exhaustive geometry sweep or live TiEmu input is involved.

Final package/layout/CPU and hash evidence: `x/core-strikes/verification.json`.

| Targeted evidence | PC/sanitizers/68000 result |
|---|---|
| Consecutive strikes |250 checks, `9B0AAD44`|
| Counter/Auto-Potion |267 checks, `EF8107A7`|
| Root contexts |348 checks, `B05660E8`|
| Primary effects/rewards/AI |295 checks, `EDCEBFAE`|
| Charge |`276A2090`|
| Gariland / Orbonne flow |81 /14 checks, `AE9CFD31` / `2CD70391`|

All six normal and six response-state PC/native plane hashes match the preceding
checkpoint. Eleven flow PNGs keep their bytes and modification dates. Source
catalogue/geometry/bitmap/timer data and code are unchanged, so their earlier
evidence is reused; no exhaustive audit was repeated.

| Storage | Reactions checkpoint | Strikes checkpoint |
|---|---:|---:|
| Private42-variable group |482578|483414 (+836)|
| Raw image bank |44920|45508 (+588)|
| Packed battle payload |42091|42339 (+248)|
| Battle TTPack input / headroom |64605 /331|64923 /13|
| Largest image / ModCtx |5742 /320|5830 /320|
| Native State |2092|2094 (+2)|
| Unit / Work / FSV3 save |112 /45516 /3138|unchanged|

Only `ffbcore` and `ffbm` change; all28 non-code banks match. ABI16/signature59368
accepts the rebuilt ten-image bank and rejects captured ABI15. Five requested
battle blocks total114804 bytes for Gariland,114444 for Orbonne and126812 at
the maximum topology. Allocation count is unchanged; this is a layout formula,
not stack/fragmentation/combined-residency measurement. The13-byte TTPack margin
is an active constraint: relocate further resident growth before adding it.

Native follow-up preparation costs37852 cycles in both measured melee cases;
ordinary start is43706 cold/6436 prepared and prepared impact6758. The23-call
primary suite averages13931 cycles, peak53892. Zones exclude rendering and do
not certify a complete frame. A first all-in-ACTION layout grew its image to6150;
putting follow-up preparation in FORMULA reduces the largest image to5830.
Both callers return before ranged LoS can load AI. Compiler-generated context
wrappers contain no software32-bit arithmetic helper or division instruction;
no new assembler was authored.

The standard playtest bundle remains separate. Next: independent weapon hands
and shared preview/AI before enabling Two Swords, further reaction families,
then the remaining action/status/scheduler contracts in CORE_ROADMAP.md.
Jump refinements stay deferred. This slice does not close L2/L3/content gates.

```sh
make -C games/fft/campaign core-strikes-check
```
