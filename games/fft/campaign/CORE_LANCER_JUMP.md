# Lancer Jump

2026-11-03. The user re-enabled Jump and explicitly authorized simpler terrain
rules instead of an engine rewrite. This is the Lancer action, separate from
ordinary movement jumps (`CORE_JUMPS.md`).

## Model and source evidence

The twelve learned source selectors394..405 extend one Jump command. Base reach
is1 tile/1 level, the largest learned horizontal and vertical upgrades combine,
and Mime uses8 tiles/8 levels. Management/Status retain the original learning
IDs. Combat lists one command named Jump. The normalized formula is100 (the
source actual damage handler); selectors have no ordinary range/formula row.

`tools/lancer_jump_oracle.py` executes unmodified retail MIPS for14 preparation
profiles and16 damage profiles. The source preparation canonicalizes to394,
reserves Move+Act and uses max(1,floor(50/Speed)) ticks. Short Charge/Non Charge
do not alter Jump. Native Speed0 is clamped to1 for liveness, without executing
the original division by zero. Formula100 uses PA*WP; a polearm first multiplies
PA by3/2. Unarmed YA derives from PA*Brave/100 (minimum1). Defense Up, Protect,
Charging/Sleep, Frog/Chicken, attacker Berserk/Frog and zodiac retain source
order. Damage is capped by the native shared999 limit. No evasion, critical or
knockback draw is added. Source hashes and fixture boundaries are in the private
`x/core-lancer-jump/oracle/source.json`. This is not a full original UI/AI replay.

Jump binds the selected compact terrain cell, not its current occupant. A moving
target can escape; another eligible occupant is hit instead. An empty cell
finishes without damage or rewards. The jumper remains logically at its origin,
with that movement cell reserved, but is absent from targeting and drawing.
Its ordinary CT and status timers do not advance during flight; its independent
deferred timer does. On landing it becomes visible at its origin and uses the
existing action/reward/response pipeline once. KO/invalid actor cancellation
clears the pending command and airborne state.

## Deliberate simplifications

Only cells on the same terrain layer may be selected. There is no bridge,
ceiling obstruction, preferred higher layer, midair trajectory, new sprite bank
or PS1 jump animation. The actor disappears during flight and reuses the existing
wind-up/impact poses at landing. Initial manual targeting selects an occupied
cell, through the existing command UI. The ordinary immediate-utility AI can
choose Jump using its true reach and current damage; it does not predict target
movement or future CT. Full source counter/special-actor/mount dispatch remains
outside this slice.

## Ownership and storage

ABI60 retains ModCtx360 and the existing deferred record, Unit, State, Work and
FSV3 layouts. A derived airborne flag uses the previously free status-cache bit7.
COMMAND validates and returns before RESPONSE5 constructs a Jump order; no
executing image can load another. The resident pure callback shares learned
limits/reach without image loads: target_query252 returns packed limits, and
charge_query with a Jump ID treats target/flags as origin/cell (no RNG). TURN4/5/6 own the unchanged target validity,
pickability and projected twin-cell queries at input time.

FMD2 archive records omit only the358 verified zero bytes in each ModCtx.
The signature stays in the record. The loader copies code before/after the hole
to its original logical offsets and initializes the same360-byte context from
the existing resident template. Code addresses, direct callbacks, execution
memory and gameplay allocations retain their previous model. MOD_ZONE is based
on the logical image size, never on its smaller archived length. Each shard
retains its64000-byte limit; there is no decompression or new assembly.

## Validation

The targeted command is `make core-lancer-jump-check`. Logic and UI fixtures
compare PC, ASan/UBSan and actual68000 modules, with private descriptor/VAT paths.
The family evidence, packing, regressions and preservation results are recorded
under `x/core-lancer-jump/`. Live TiEmu and the standard campaign group are not
part of these checks. Whole-frame, stack/residency and full R/S/M release gates
remain separate.


Current family result:298 logic assertions0D8F8FB5 and24 UI assertions65782A40
(322 total) agree on PC, ASan/UBSan and68000. Sixteen full-plane pictures cover
four orientations, with256 render calls. Learned slot order, range, a synthetic
second-layer rejection, actual AI choice, escaped/replaced target, no flight CT,
KO, exactly-once impact, resource/reward guards and reserved-origin recoil/
legacy-splash immunity are covered. Native marked cold arm peaks100218 cycles,
amount query55492 and due dispatch112812. Cold render still reaches4838752;
these are bounded zones, not a360k full-frame certificate.

The final18 logical images use an8390-byte zone and9+9 shards of62018/62010
bytes, leaving1982/1990 under64000. All18 archived FMD2 records independently
reconstruct the complete compiler image byte for byte;6444 zero bytes are omitted
in total. Context binding retains170 checks65F2A9C4, parser130/00000047,36 cold
loads/36 warm preloads/three stale chapter rebindings, plus eight rejected bank
variants including the actual ABI59 export. Exact logical size, offsets and
hashes are in `reconstruction/results.json`.

The raw program is64881 bytes; actual TTPack input64883 leaves53 below64936.
The private compressed data file is40025 bytes. Unit112/State2210/Work45576,
SaveUnit176/Save3138/FSV3 and the allocation formulas equal the prior checkpoint.
Both private native chapter flows retain81/AE9CFD31 and14/2CD70391.
The catalogue-driven actions digest changes toFF0977D9 only because its forecast
loop now includes the twelve Jump rows: restoring just those rows in a private
catalogue reproducesE565DC51, with the current engine and all295 assertions.


All34 scoped suites are green (Jump plus33 older suites). The first closure's
two control-flag comparisons included the new native airborne cache bit in the
original flag domain. The control fixture now excludes that internal bit, as it
already excludes the separately tested loss-of-control domain; the explicit
Jump checks remain. Its complete594 assertions recover15C96953 on PC,
sanitisers and68000 in the targeted rerun. No production rule was changed for
that fixture correction. Old area/Math/Throw/stat-save/critical images retain
F9BACFF5/138E34BC/38B87D4D/235C9F0E/0EDD7883.

All481 protected save/group paths retain bytes and modification dates; eight
identical test-fixture dates were restored after the final runs. Final evidence
is consolidated in `x/core-lancer-jump/verification.json`. No live emulator,
standard campaign group, commit or push is part of this work.
