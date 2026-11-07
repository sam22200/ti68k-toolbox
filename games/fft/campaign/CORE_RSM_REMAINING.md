# Generic R/S/M inventory and remaining integration

Updated2026-11-05 after physical weapon formulas: [CORE_WEAPON_FORMULAS.md](CORE_WEAPON_FORMULAS.md).
All four user-requested delivery blocks are implemented. No new generic R/S/M
producer remains in this inventory; the final generic-core integration checkpoint
remains open.

## Scope and counting

The bounded scope is the20 generic jobs, source job IDs0x4a..0x5d. Read their
actual six R/S/M entries with `battle.Disc.job`/`battle.rsm`, deduplicate source
IDs, then inspect executable consumers. Menu/save presence alone is not support.
Counts describe equipped capabilities within their documented native policies,
not percentage completion of the full engine or game.

| Category | Distinct generic-job IDs | Native consumers | Missing producers |
|---|---:|---:|---:|
| Reactions |30|30|0|
| Supports |28|28|0|
| Movement |20|20|0|
| Total |78|78|0|

All201 generic action entries remain enabled. The final six R/S/M producers
were Finger Guard448, Train470, Secret Hunt471, Monster Talk473, Monster Skill478
and Move-Find Item509. Source monster jobs/skillsets and actual hidden-item/loot
records are present; the executable monster door uses source Chocobo actions.
Monster recruitment persists in reserve until compatible art/content exists.
Secret Hunt uses direct capped party inventory rather than a Fur Shop UI.
The pilot maps have no nonzero hidden items; positive discovery fixtures use
other real source map records. These bounds are explicit in CORE_DISCOVERY.md.

## Consolidated deliveries

The original50-ID plan is closed through four deliveries, not50 milestones.
The ten earlier technical units were internal implementation/check steps; do
not turn them back into separate user handoffs or restart an ID-by-ID queue.

| Delivery block | New IDs | Result and reference |
|---|---:|---|
|A — delivered: movement/terrain|15|Move/Jump bonuses, Ignore Height, terrain/water/lava, equipped Float/Fly, Teleport; CORE_MOVEMENT.md|
|B — delivered: equipment/progression|17|Eight Equip permissions, Two Hands, Equip Change, EXP/JP boosts, four movement benefits, Gilgame Heart; CORE_EQUIPMENT.md|
|C — delivered: combat/protection|12|Concentrate/Throw Item, guards/Catch, counters, HP/MP interception/sharing, physical Abandon; CORE_COMBAT.md|
|D — delivered: discovery/monsters|6|Move-Find Item, Secret Hunt, Train, Monster Talk, Monster Skill, Finger Guard; CORE_DISCOVERY.md|
|Final — open: generic-core integration|0|Mime/Hamedo, panel ranges and physical weapon bases delivered; effects/AI limits below, whole-frame/stack/residency checks|

Earlier baseline consumers plus these50 additions account for78 generic IDs.
Reflect440 and Non-Charge483 are additional implemented non-generic abilities.
Teleport2/499, Move underwater505 and Silent Walk508 remain outside the20
movement IDs counted here. Unique jobs, full monster content and unused/random
selectors are not missing generic producers in this table.

## Remaining interactions and explicit limits

The final checkpoint is bounded integration work, not a newly invented list
of generic families. Preserve and reuse the source/native cases already green;
check changed combinations rather than an exhaustive Cartesian product.

- Automatic Mime imitation is delivered outside the original50 R/S/M additions.
  It reuses primary/response owners with the bounds in CORE_MIME.md; no new
  timer, consumable payment or imitation chain is introduced.
- Ranged Hamedo and Brave-before-rejection ordering are delivered. Its shared
  strike/lunge/direct/arc panel ranges now follow the original. Exact projectile
  trajectories and complete overlapping-panel victim listing retain the bounds
  in CORE_WEAPON_REACH.md.
- Physical weapon bases, independent variable draws and ordinary guns are delivered.
  Weapon elements, healing/drain, magical guns and status/spell procs (including
  the remaining post-formula RNG) still need their checkpoint. Innate details
  and exact original future-CT/Reflect/AI simulation retain their named limits.
- Whole tactical-frame budgets, worst-case stack and resident/archive lifetime
  need their final combined checkpoint. A small formula-zone benchmark is not
  whole-engine certification.
- Full monster encounters/actions/art, original Fur Shop, traps/mounts,
  full multi-floor geometry, unique jobs and a complete campaign are separate
  content/engine scopes. Recruited monsters are intentionally kept in reserve
  rather than rendered as humans.

Representative accepted interactions already have focused owners: movement
and undo/benefits(A/B), hands and guards/KO between strikes(B/C), pending Charge
and Hamedo/Counter Magic(C), HP/MP sharing and root rewards(C), and inventory/
save migration/recruitment/discovery(B/D). Their actual evidence and remaining
bounds are recorded in the block notes and private verification manifests.

## Capacity and validation

ABI71/ModCtx352 has22 images in three7/7/8 shards51140/51952/53456,
with zone8566. Actual non-BENCH TTPack input64929/64936 leaves7 bytes.
FCJH49306 retains its layout with27 updated hand bits; FSV4/3202 remains; FSV3/3138 and FSV2/3110 migrate as before.
Unit112/State2224/Work45608 and five heap blocks stay fixed. Hamedo reuses
idle AI trajectory scratch. Weapon masks prepare the origin once; CT/pending
selection lives in ORDER. No new allocation or save field is introduced.
Keep archive/resident limits visible before expanding the weapon checkpoint.
Do not overwrite the standard group or disturb live emulator/user saves.

Block D supplies17635 logic and35 management assertions, identical PC,
ASan/UBSan and actual68000, plus catalogue, unit, affected closure and private
flow checks. The canonical handover and `x/core-discovery/verification.json`
record the final evidence.

The historical50..90 engineering-hour estimate applied before block A; it is
not a remaining ETA. The old201:50 count was never a work/time ratio. No measured
hours ledger supports a fresh numeric ETA or a percentage for the whole core.
The remaining final checkpoint and separate content scopes should be estimated
from their concrete acceptance tasks, not from the now-zero producer count.
