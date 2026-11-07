# Accepted reduced TI core envelope

Current map profile census: [CORE_MAP_PROFILE.md](CORE_MAP_PROFILE.md),109
referenced event/random maps;77 lack upper stop cells but only66 permit skipping
all upper movement geometry. This does not change the13-actor/256-node envelope.

Subsequent source-span/headroom checkpoint: [CORE_SURFACES.md](CORE_SURFACES.md)
(ABI4, archived FGS1/SGE1, unchanged saved state). Historical sizes below belong
to this document's earlier checkpoint. Full source upper-floor gameplay remains open.

2026-10-24 user decision. This supersedes the21-slot/512-node **target** proposed
in `CORE_AUDIT.md`; the original-data census remains valid as research evidence.

## Limits and rationale

| Domain | Accepted port limit | Reason |
|---|---|---|
|Battle actors|13, including player, enemy and guest actors, KO and staged actors stored by the battle core|User-selected Zirekile Falls envelope; omit the eleven-Monk random encounter. Other content above13 must fail validation, never lose actors silently.|
|Unit identity / masks|8-bit compact runtime identity;16-bit unit masks remain sufficient|13 indices0..12 fit the current mask and four-bit forecast target encoding. Source slot IDs stay a separate8-bit reference, preserving source order.|
|Tactical tile nodes|256 compact IDs across lower and upper together|Keep8-bit arrays/edges/predecessors. ID255 is valid.|
|Node counts and iteration endpoints|16-bit|Count256 must not wrap to zero. All full-node loops need an endpoint-capable type.|
|Missing tile|Separate validity/state flag or bitmap|No `INVALID_TILE=255` in the new compact-node contract. An existing16-bit temporary may still use an explicit out-of-domain value.|
|World coordinates|Separate x, y and elevation; observed axes up to18|Compact index is not a coordinate. In the proposed2-byte position, x is one byte, y uses five bits and elevation one bit in the second byte.|
|Persistent roster|16 Formation records plus separate4 guest roles, unchanged research scope|Battle capacity13 does not reduce permanent roster capacity.|

`../core_limits.h` declares these target types/constants. The battle core now
supports13 actors with descriptor-driven counts; existing encounters retain11.
See [CORE_CAPACITY.md](CORE_CAPACITY.md) for native tests and the private package.
The compact identity/search kernel now passes256-node/two-floor native fixtures
([CORE_GEOMETRY.md](CORE_GEOMETRY.md)). Actual unit/cursor/path/Charge/AI cell transitions also pass native fixtures
([CORE_CELL_STATE.md](CORE_CELL_STATE.md)). The actual13-actor/256-node tactical adapters now pass native fixtures
([CORE_FULL_GRID.md](CORE_FULL_GRID.md)). **Source upper-floor movement/LoS/cover
and full scene/memory integration remain a target.**
The user's live TiEmu and standard group remain unchanged.

## Retail data check after excluding test maps

`tools/port_limits.py` re-reads every terrain-bearing resource and variant from
the local retail-US disc. Exclude known Test Maps0..4: MAP000,117,118,119,125.
This is deliberately a conservative retained-resource census, including scene
and optional maps; it is not a claim that every retained map is a story battle.

With the raw tile flag filter `(flags &3)==0` used by the original initial
placement lookup, the records are:

| Record | Map / location label | Dimensions | Lower | Upper | Total |
|---|---|---:|---:|---:|---:|
|Most retained stoppable cells|MAP100 / Public Cemetery|15×15|198|0|198|
|Most stoppable cells with an upper floor|MAP029 / Colliery Underground Second Floor|12×13|155|4|159|
|Most upper stoppable cells|MAP053 / Entrance to Death City|9×9|65|18|83|

The source resource SHA256, record number and dimensions accompany every row in
`x/core-audit/port-limits.json`. Names/test-map labels come from the
[original-disc map viewer](https://mmatyas.github.io/fft/mapviewer/); dimensions
and counts above come from our disc parser. The18-cell axis remains necessary
(e.g. MAP076 Zeklaus Desert); do not enforce a15×12 rectangle on every map.

Stoppable cells are only one budget. A tile forbidden as a destination can still
affect traversal, targeting, jumps, visibility or terrain scripts. Therefore the
census also checks a larger candidate set: **the complete lower rectangle plus
all upper cells whose source blocked bit is clear**. Its retained-resource
maximum is225 nodes, also MAP100. This fits256 with31 spare nodes. It is a
conservative geometry input set, **not a proved complete movement graph**:
special movement and terrain mutations still need fixtures/validation.

This gives evidence for the256-node budget without removing tree/wall/transit
coordinates just to match a walkable-cell statistic. Scenery polygons and cover
data remain independent of the tactical-node budget. An unsupported resource or
mutation exceeding256 must be reported explicitly by the content validator.

## Community JSON cross-check

Reproduced the user's122-file census from
[`fft-map-json` at c4f8252](https://github.com/rainbowbismuth/fft-map-json/tree/c4f8252d1ca8446006e6cbd751c2f02b776e5d66),
using exactly `not no_walk and not no_cursor`. After excluding the five test-map
IDs, its largest count is MAP100:221. Its largest upper-bearing count is the
reported MAP120:156+12=168; MAP053 again has18 upper cells.

Important provenance differences:

- MAP120's own `gns` field is **`MAP120.GNS (fake)`**. No MAP120.GNS exists on
  the local retail-US disc. It cannot define that disc's maximum real map.
- MAP100 has221 cells under the JSON filter, while the raw retail flag filter
  gives198. Several cells share source flags but differ in JSON `no_walk`;
  matching dimensions alone does not prove identical blocking semantics.
- JSON test-map geometry also differs from this disc. The README mentions
  MAP000/MAP062 parser issues; the pinned revision actually contains both files.
- 24 JSON entries differ in dimensions or lower/upper counts from the selected
  retail primary resource. Some may reflect variants or export policy; they are
  comparison results, not24 automatically proven broken maps.

The optional report stores both sets and each JSON SHA256. It does not silently
replace source flags with community classifications. Neither198 nor221 is a
universal reachable-cell count under every job/status/movement mode.

## Zirekile source check

Retail MAP083 scenarios128..136 reference ENTD`0x195` (405) and squad270.
ENTD contains eleven populated definitions: **nine always-present records and
two initially absent records**. The previously observed original deployment
overlay gives squad270 a capacity of four. Thus9+4 supports the user's13-actor
initial envelope. It is not a full played trace of every conditional activation.
Keep source IDs/roles as data when folding definitions into13 runtime slots;
check script transitions instead of counting every populated definition as live.

## Representation and next migration

The source stores two fixed256-cell layers. That source address stays an offline
or archive coordinate; it is not the runtime node ID. Generate a deterministic
coordinate/elevation-to-node lookup and reverse positions offline. For example,
on a4×18 lower grid with one upper tile, upper(0,0) can have compact node72;
its source address256 must not leak into an8-bit runtime index.

`(y<<4)|x` is correct for a16-wide grid. For the general map, use generated row
offsets/lookups and compact topology; do not force width16 or multiply/divide
coordinates in hot loops. Validity must also distinguish absent upper cells
from a present node255. Pending actions use their state/target-valid flag.

Draft fixture schema2 now generates:

-13 actors, all camps, two ordinary KOs, upper nodes and equal-tick pending work;
- a256-node boundary with node255 referenced as a valid target;
- a4×18/two-floor fixture with two distinct nodes at the same x/y.

The validator rejects actor14, node257, duplicate positions/IDs, absent targets
and source-address/compact-ID confusion. These are input tests; separate actual
13-actor PC/native rules/rendering and compact geometry/search fixtures now pass.
Unit/cursor/target/cover floor/count adapters remain the next engine migration.

Measured native eleven→thirteen growth is180B in State and150B in Work for
existing unit-shaped arrays. Current Work is44434B. State1494B and its deferred
save tail3138B share a4632B allocation; the four battle blocks request107722B,
excluding executable/module/
supervisor/grayscale residency, stack and allocator overhead. No16→32-bit
unit-mask widening is needed. The old41B/node estimate is historical: the fixed
queue and explicit via rows changed the layout. Future expansion must measure
node arrays, lookup metadata, scene bounds and topology together; simply growing
the60594B scene/Work block leaves little room under the native block ceiling.
Measure actual block sizes and cycles after that
migration, preserving current-map behavior.

## Reproduce

```sh
rtk proxy make -C games/fft/campaign core-audit-test port-limits
rtk proxy tools/pyenv/bin/python games/fft/tools/port_limits.py \
  --community /path/to/pinned/fft-map-json/data \
  --out games/fft/campaign/x/core-audit/port-limits-community.json
```

The optional community corpus is a local research dependency, not bundled game
data. No runtime downloads or JSON parsing are introduced on the calculator.
