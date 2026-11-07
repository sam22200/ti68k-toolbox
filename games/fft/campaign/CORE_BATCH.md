# Formula08 compound preparation and impact

Current runtime follow-up: [CORE_AREA_CT.md](CORE_AREA_CT.md) owns a packet in Work
and routes automatic impact for cast_order/due casts. Isolated caller packets
still require their explicit commit; retail spells/presentation remain gated.

Current follow-up: [CORE_BANKS.md](CORE_BANKS.md) splits the executable archive
without changing effects/save state. Numbers below describe the prior batch build.

2026-10-27. Follow-up to [CORE_TARGETS.md](CORE_TARGETS.md). The gated effect-stage
backend now prepares a bounded batch, pays once and commits its prepared targets
once. It reuses the existing response/reward drain. This does not activate retail
spells, initial area command selection, area AI or reviewed spell presentation.

## Source and scope

`tools/batch_oracle.py` supplies thirteen groups of thirteen original source slots
to original MIPS presence/formula08/finalizer instructions. The compound driver
reconstructs the ascending ordinary strike order observed in
battle_action_resolve_ability_strike: all target formulas precede HP commits.
It supplies the port's deterministic pass/fail stream with original scaling and
normal weather. It does not execute the whole original dispatcher, payment,
HP/status/reaction/reward/presentation tail. Those boundaries retain their scoped
prior evidence; do not present this as a complete original AoE battle trace.

The4920-byte FBA1 fixture records per-target inputs, presence, hit, signed amount,
typed outcome/Oil removal, final RNG and copied draw lists. Cases include caster
self-target,13 present actors, mixed evasion/late KO/Petrify/Wall rejection, absent
objects/Jump, Faith, elemental nullification/absorption/resistance and signed
power255 probes. Extreme carriers are synthetic; no retail record is enabled.
Source/disc/function hashes and decomp commit: x/core-batch/oracle/source.json.
Raw source slot order is explicit in the fixture. Pilots preserve relative source
order, but future encounter formats must retain it before broader activation.

## Ownership and lifecycle

`CastVictim` is4 bytes: amount16, hit8, effect8. `CastBatch` is60 bytes with thirteen
victims and an8-byte request/result header. The caller owns its storage; it is
not embedded in State, Work or a save, and no batch heap allocation is introduced.
Keep it alive and unchanged from preparation until commit returns. It may then
be released: existing State/ActionPacket own the response drain. This is trusted
internal engine storage, not a serialized/untrusted packet format.

`cast_batch(CAST_BATCH_PREPARE, &batch)` validates an EMPTY request, actor/root
availability, source-compatible normalized formula08 damage/area/cast record,
resource/restriction eligibility and supported footprint. Rejection changes no
State/RNG/MP/output. The effect-stage entry assumes initial command range, cast
binding choice and CT timing were already validated; it is not a new command.
On success it starts one CoreAction root, pays effective MP once, prepares all
present targets in ascending native IDs and marks READY. HP, Oil, KO, response
stock, rewards and other pending casts do not change during preparation. It also
handles a genuinely empty footprint and entirely nullified/missed effects.

The shared magic_prepare helper is used by both the existing single-target
COMMAND and the batch TARGETS path. It preserves sequential evade/proc draws,
positive999 cap, signed quantities, final eligibility and protected latched Oil.
It uses resident pure scalar/evasion callbacks, not the evicting skill_amount
wrapper. The supported formula has no enabled infliction/proc/status family.

`cast_batch(CAST_BATCH_COMMIT, &batch)` requires READY plus the same owned root,
actor and ability. It consumes the impact and marks APPLIED before applying any
victim. Repeated calls reject without effects or RNG. All prepared victims commit
even if the caster was KO by its own earlier result; no formula/affinity/weather
query can recompute their frozen payload. The shared magic_commit helper keeps
single-target and batch HP/status/KO/Oil/response semantics together. Late innate
Oil protection is rechecked. Later scalar/weather changes do not replace damage.

The commit collects the maximum existing earned_exp value once and queues each
supported Counter/Auto-Potion at most once. Existing root completion drains
responses in stable actor order, rechecks life/stock, grants the deferred reward
once, and only then evaluates victory/next CT. The fixture actually runs two
Auto-Potion responses from one batch. This reuses the current ordinary-human
response/reward subset; full original Reflect/Hamedo/other magical reactions,
all reward corner cases and additional status families are not newly certified.

This internal API is not yet called by attack_step/command dispatch for area
spells. Do not advance attack_step between PREPARE and the explicit COMMIT: its
single-target path would consume the root impact. A live integration needs a
durable batch owner and explicit presentation/impact routing, plus appropriate
per-target numbers/poses. The current single-target cast centre binder remains
unchanged; area dispatch must use the target-set follow-centre contract.

## Native modules and budget

ABI27/ModCtx364/signature62533: same-size transient input union now accepts the
caller batch pointer. TARGETS1 owns preparation; RESPONSE10 owns compound commit.
Each resident call returns before loading another image. Neither executing image
loads or calls an evicting module. ACTION and RESPONSE compile the same small
magic_commit helper; enlarging ACTION with the whole batch was rejected because
it raised the resident module zone unnecessarily. No assembly was authored.

`make core-batch-check` / `check_batch.py`:1202 PC/ASan/UBSan/actual68000 checks,
hash94558E80. Rejections cover wrong phase/actor, insufficient MP, unsupported
normalized drain and raw persevering performance, disabled retail records and
invalid bindings. Actual batch tests cover exact-cost casts, thirteen present
actors, empty/null-only sets, late weather/MA changes, Oil protection and two
sequential Auto-Potion responses with one reward/terminal drain.

Measured cold preparation average84287/max97158 cycles; compound commit including
cold module load/possible AT refresh average57485/max103810. These are backend
zones, not animation/full-frame budgets. Current target query with the larger
TARGETS image remains separately measured in the verification report. Do not
reuse the older CORE_TARGETS cold-load timings for this larger image.

Native State2192/Unit112/Work45516/ActionPacket22/SaveUnit176/FSV3 Save3138 remain
unchanged; host State2248. Five battle allocation requests remain unchanged,
first5330 native, plus caller-owned60-byte storage while a batch is pending.
No event queue or heap allocation is introduced. These are compiler layouts and
allocation formulas, not runtime stack/fragmentation/high-water/residency probes.

Private group503633 bytes, +3027 from CORE_TARGETS. Payload deltas: ffbcore+113,
ffbm+2914. All28 non-code banks and41086-byte FCJA remain identical. Raw module
bank64984 has13 images:6150/3542/4254/5786/2842/6132/4968/5588/6076/6204/5416/5002/
2862. Largest6204 (+32); resident TTPack input64783 leaves153 bytes. The module
bank is now close to its variable limit and exceeds the64000-byte planning guard;
future growth needs a split/budget rather than silently overrunning it. Existing
old-bank rejection and all13 context signatures are checked.

Final evidence, packet/layout probes, prior weather/targets/responses/actions/
Charge/flow closure and twelve reviewed frame hashes: x/core-batch/verification.json.
The production resident wrapper remains subject
to dead-code removal until a producer references it; its future live integration
must budget its linked code and durable caller storage explicitly. File sizes are
not a complete RAM/stack/fragmentation/residency measurement.

The existing C07/L4 cold-HUD/frame/residency gate remains open (previous forecast
max92032 exceeded its old60k estimate). Allocation/layout checks and small query/
commit zones do not close it. Retail AF_CAST records, area UI/AI/presentation,
Reflect/special families and broader jobs/status/campaign/formats/editors remain
open. Jump stays deferred. Live TiEmu, standard playtest group and existing gallery
bytes/timestamps are preserved; tests are scoped to affected dependencies.
