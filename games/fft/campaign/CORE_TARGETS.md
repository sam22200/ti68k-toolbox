# Ordinary radial formula08 target sets

Current follow-up: [CORE_BATCH.md](CORE_BATCH.md) adds gated compound preparation/
payment/commit and existing response/reward drain. No area producer is enabled.
Numbers below are the historical target-query checkpoint.

2026-10-27. Follow-up to [CORE_WEATHER.md](CORE_WEATHER.md). This checkpoint
adds a read-only footprint query, not live multi-target spell execution. Retail
Holy/Fire and all production AF_CAST records remain disabled. Player commands,
AI and forecasts do not yet consume this area query.

## Source evidence

`tools/area_oracle.py` executes the original MIPS green-panel, vertical, team,
presence and target-list instructions:79 footprint profiles and42 column
boundaries. Synthetic13-slot encounters include real Orbonne/Gariland terrain,
not a full original encounter or map census. Source/disc/function hashes, inputs,
marked panels and ordered source lists are in x/core-targets/oracle/source.json.
The FAT1 fixture is22116 bytes; FAL1 is426. Both stay outside production banks.
Their big-endian word fields and record starts are aligned for the68000.

Observed rules:

- A finite ordinary area is a Manhattan diamond. It ignores intervening walls
  and chooses one layer in each column, nearest to the centre's source height.
  Ties choose lower. Vertical tolerance doubles; reference height narrows to u8,
  candidate heights remain signed16. Raw water depth/half-height matter.
- A blocked layer uses finite -255 during selection, then stays unmarked. It
  must not simply be removed before comparing extreme heights. Highest-only
  selection (flags2 bit5) precedes tolerance. Hidden/no-cursor upper surfaces
  can select the upper layer and exclude a playable lower tile.
- Whole-map area255 bypasses vertical selection and can retain both layers.
  Source flags1/2/3/4 are raw offsets3/4/5/6. Cannot-hit-caster (flags2 bit0)
  removes the whole occupied cell; initial self-centre legality is separate.
- KO/Petrify/Wall units remain in the footprint. Formula finalization may reject
  their effects later. Crystal/Treasure/Jump/absent actors are excluded from the
  list. Unit-following centres still follow their coordinates after they become
  absent; nearby other actors may remain targets. Missing coordinates reject.
- Team filtering marks occupied panels, not individual actors. For overlapping
  occupants, a later rejected actor clears range without clearing an earlier
  selected mark. The bounded native grouping preserves that source result.
  Current scope is uncharmed Player/Guest versus Enemy; original initial-team
  versus current-team semantics need a fixture before Charm activation.
- Ordinary target lists are traversed in ascending source slots. Optional nearest
  sorting is reset by the ordinary strike caller. Pilot extractors preserve
  relative source order: Gariland0..5,16..20; Orbonne0..10. Native Unit.slot is a
  save-roster slot, not this battle order. An unordered result mask is not an
  effect/RNG ordering contract; future encounter formats must retain source order.

The source prepares every target's formula result before committing HP and
presentation. A loop around the old single-target skill() would repay MP and
change subsequent RNG/status/HP inputs. Batch preparation, commit, per-target
responses/rewards and presentation must be implemented before enabling spells.

## Runtime contract

`cast_targets(actor, ref, ability, follow)` returns signed16: mask0..0x1fff,
TARGETS_INVALID=-1 or TARGETS_UNSUPPORTED=-2. With follow0, ref is a compact cell;
with follow1, it is a stable battle actor ID. Tile255 is valid, and a16-bit count
can represent256 nodes. Coordinates, node IDs and layers remain separate.

This is footprint membership, not command validity: it does not check initial
range, MP, Silence or initial self-target legality, spend costs, draw RNG, mutate
State, or arm/resolve a cast. Disabled source ability records can be inspected
without changing normalized enablement. Formula families other than08, weapon
range, directional/random and direct shapes return unsupported explicitly.
Reflect, Math Skill, special families, mounts and other targeting producers are
not implemented by this query.

The normal path visits at most13 actors. Only team-filtered overlap grouping can
visit13 squared; no map-wide search or heap allocation is added. Upper raw bytes
are skipped when SURFACE_MOVE_UPPER is clear, including on one-floor maps; GRID_UPPER
alone is insufficient because an untargetable upper surface can affect selection.
The caller uses stable cells/status policy and the existing validated FGS1 data.

Native TARGETS is a new1564-byte cold image, loaded only by the resident wrapper.
It has no ModCtx callback or nested image-loader call. Existing transient cast
arguments are reused; follow travels as an argument rather than the opcode so255
cannot collide with the reserved preload operation. ABI26/ModCtx364/signature62276
rejects the previous12-image ABI25 bank. Context size and largest image6172 stay
unchanged. Existing12 images retain their sizes.

## Validation and cost

`make core-targets-check` / `check_targets.py`:1193 PC/ASan/UBSan/actual68000 checks,
hashFC02B091,79 source footprints and42 column boundaries. Tests reverse compact
node numbering, exercise13 actors/node255, four rotations, pure State/RNG/MP and
pending data, warm/cold module calls, rejected input and disabled source records.

Datasheet cycles: column average867/max978; cold query including image preload
average30764/max40642; warm query average18700/max28578. These are query zones,
not complete frame times. Compiler output has no image-loader call or software
wide multiply/divide/float helper. No assembly was authored.

Native State2192/Unit112/Work45516/ActionPacket22/SaveUnit176/FSV3 Save3138 remain
unchanged; host State2248. Five battle allocation requests remain unchanged,
first5330 native. Fresh layout probes verify these statements; allocation formulas
are not peak stack/fragmentation/high-water/combined-residency measurements.

Private group500606 bytes, +1564 from CORE_WEATHER: ffbcore payload-12, ffbm+1576.
All28 non-code banks,41086-byte FCJA and FSV3 remain identical. Raw module bank62070
contains13 images:6150/3544/4254/5786/2842/6132/4970/5588/6076/6172/5392/3438/1564.
Resident TTPack input64749 leaves187 bytes; future resident/bank growth needs a
budget before adding more code. Standard playtest package and live TiEmu stay
untouched. No image review is requested: existing gallery bytes/dates stay intact.

Targeted closure covers the preceding weather/AI/packet/forecast fixture, ordinary
actions, Charge, Gariland/Orbonne save-flow and twelve existing action/reaction
frame hashes. Unchanged geometry/catalogue/affinity/status suites keep their prior
evidence. Results and exact artifact hashes: x/core-targets/verification.json.

Next: batch formula preparation and one-cost multi-target impact, source response/
reward ordering, then reviewed spell presentation and actual command/AI integration.
C07/L4 still needs complete cold-HUD/frame accounting (previous cold forecast
max92032), stack/fragmentation/residency. Other jobs/status/campaign families,
formats/editors and production-content gates remain open. Jump stays deferred.
