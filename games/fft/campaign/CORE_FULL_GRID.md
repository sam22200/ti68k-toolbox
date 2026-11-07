# Full256-node battle adapters

Subsequent source-span/headroom checkpoint: [CORE_SURFACES.md](CORE_SURFACES.md)
(ABI4, archived FGS1/SGE1, unchanged saved state). Historical sizes below belong
to this document's earlier checkpoint. Full source upper-floor gameplay remains open.

2026-10-24, workstream4, following [CORE_CELL_STATE.md](CORE_CELL_STATE.md).
The actual tactical core now accepts256 compact nodes across two floors, with
13 actors. Node255 remains usable in placement, cursor, path, Charge and AI;
counts and endpoints can represent256. This completes the bounded tactical
count migration, not source-correct upper-floor scenes or the production gate.

## Count and identity contract

Chapter `map_n`, module context counts, full-map iterators, range completion,
AI origin counts/indices and requested-node counts are16-bit. Payloads stay
bytes: cells, edges, predecessors, paths, actor/cursor positions and target IDs.
The existing255 unreachable *distance* is not an invalid tile identity.

AI selection returns a16-bit temporary256 for no candidate. A valid result255
must not trigger the nearest-node fallback. The256-origin sort and cooperative
candidate scan retain byte IDs while their counts and iteration endpoints stay
16-bit. Both setup and evaluation overlays carry the new count correctly. The old
quadratic insertion sort is replaced by a merge sort using the idle map-order
scratch. It preserves ascending score / descending node-ID ties without another
array or allocation.

Rotation counting-sort buckets and offsets are16-bit: even all256 cells in one
bucket must be retained in stable order. Keys are generated from each compact
node's coordinates, not its assumed rectangle index. The range cache compares
all node bits, its completion index reaches256, and contour neighbors resolve
coordinates on the current floor. The overlapping-tile API returns node+1 in
16 bits, so node255 returns256 rather than wrapping to "nothing".

Dense coordinate/trajectory lookup planes are built once with pointer walks and
small row fills. They use the current dimensions, including18-cell axes; they
no longer borrow the fixed10-column coordinate planes in the old contour bank.
No division is needed in this table initialization. A dense source rectangle
must still fit256 addresses; the compact count combines both floors. The loader
rejects unsupported geometry instead of truncating it. Scene crops still have
the independently validated202-row current-bank limit.

## Memory ownership and modules

Topology now has its own heap allocation. Work holds pointers to32-byte edge
rows and16-byte via rows; the allocation uses48 bytes per cached node. It grows
to the largest map visited, never past256 nodes. Growth allocates first, preserving
the complete previous binding if allocation fails; only then does it free the
old cache and bind the new rows. Release resets its pointer and capacity.

This removes the enlarged topology arrays from the combined scene/Work block,
keeping each allocation under the AMS per-block ceiling. Reports include the
fifth block, each current map, the256-node maximum and the temporary overlap of
old/new topology caches. These formulas do not measure combined program/runtime
residency, allocator fragmentation or stack high-water.

State and the borrowed pending FSV3 save retain their shared long-lived allocation.
Save remains3138 bytes; Work/topology/view/actor buffers can be released before
the runtime writes the pending save. New layouts require `MOD_ABI=3`, rebuilt
resident code and all seven matching FMB2 modules. No source save migration is
introduced by this tactical count change.

## Verification

- **3181 checks**, PC, ASan/UBSan and actual68000, hashC50D81D6, on13-actor
  descriptors with a16×16 reversed-ID lower grid and an8×16 two-floor grid.
  The upper node at(0,0) is255. Both fixtures use real battle stats/formulas;
  their geometry and floor connectors are synthetic TARGET inputs.
- All256 requested nodes, one-node-at-a-time search, movement/path through255,
  independent occupancy, ring membership/completion, one256-entry sort bucket,
  overlapping selection returning256, all256 AI origins and sliced iteration,
  the valid255/fallback distinction, Charge targeting255 and the highest actor
  slot/CT are checked. Four score distributions also match the frozen insertion
  ordering on every node, including reversed ties. Failed topology growth preserves the previous binding;
  release clears ownership. The battle sanitizer keeps the earlier exemption
  for legacy signed shifts; address and other undefined behavior checks run.
- Existing128 cell-state checks,68 bank checks,13887 compact-kernel checks and
 91186 frozen-path comparisons pass. All1837 battle boundary checks and the four
 13-actor render controls pass. Root/campaign/opening PC suites and16 audit
  validator tests pass.
-37 campaign and24 opening/scene/control native replays pass.169 reference PNGs
  remain byte-identical, including the newly rendered13-actor controls. Nine
  fixed11 control pictures agree. Cancellation288 cases, rotation144 cases/
 80 frames/81840 independent corner checks,48 original Charge trials and the
 95 full-PC flow assertions retain their outputs. Native flow verifies the
  settled save after Work/topology release, receipt idempotence and overflow.

## Measured size and CPU

| Item | Previous cell milestone | Current256-node milestone | Change |
|---|---:|---:|---:|
| Unit |80|80|0|
| State |1480|1586|+106|
| State plus pending Save block |4618|4724|+106|
| Work |48046|45510|-2536|
| Combined scene/Work block |64206|61670|-2536|
| Separate topology, Gariland |embedded|7200|fifth allocation|
| Sum of requested Gariland blocks |111320|116090|+4770|
| Private campaign package |465566|466067|+501|

Gariland's five blocks are4724,32320,61670,10176 and7200 bytes. Orbonne's
140-node cache uses6720 bytes, giving115610 total requested bytes. The256-node
cache uses12288, giving121178 bytes with the *current*202-row scene allocation.
The largest individual block is61670. Growth from150 to256 nodes temporarily
retains both7200- and12288-byte caches; total requested memory during that
transition is128378 bytes. Larger scene crops, program/runtime residency,
allocator overhead, fragmentation and stack are excluded: these are formulas,
not a measured combined memory high-water or a hardware-fit claim.

The pathological256-origin setup zone averages561409 cycles (~46.8ms at12MHz),
versus5191343 (~432.6ms) for the frozen insertion version: about9.25× faster on
these four synthetic setup inputs. The new ordering also matches the frozen
insertion reference for four complete score distributions on each geometry.
Cooperative iteration over all256 origins costs176280 cycles in total in the
no-action fixture; this is not a complete enemy-turn latency measurement.
The worst256-cell single-bucket rotation sort costs83766 cycles.

The cold graph fixture averages7221400 cycles (~602ms), including topology
construction and a one-node-at-a-time search over all256 requested nodes. This
must not be presented as an ordinary frame or warmed Move latency. Map-owned
offline topology remains a separate format/performance gate. Current ordinary
13-actor board/rotation peaks are266748/329474 cycles, Status/equipment
241534/265366. Current limit4 movement comparisons peak79400/82176 cycles on
Orbonne/Gariland. Cold/turn/full-fight outliers remain above360k; no universal
30fps or LCD timing claim. All48 native decode repetitions retain their previous
cycle counts. Compiler output for eight relevant resident/module functions
(including inlined module entries) contains no32-bit multiply/divide or float
helpers; details are in `compiler-review.json`.

Private package: `x/core-256/fftcampaign.89g`, **466067 bytes**,42 variables,
464977 payload bytes plus1090 transport bytes;501 bytes above the preceding
cell milestone and22 bytes above the unchanged466045-byte standard group.
Packed battle/module payloads are40463/33270 bytes. SHA256:
`1a32e5b777959f553e3dccf3c19f061b2722ff49aa089055ed88eb74e1e78b95`.
`package.json`, `native-budget.json`, `source.json`, `image-regression.json`,
`results.json` and fixture logs are under `x/core-256/`; `controls/` retains the
pre-merge-sort package/layout and observed timing. The existing ffca64000-byte
planning alert remains unchanged. The standard groups and live TiEmu were not
replaced.

## Remaining gates

The tactical envelope is implemented, but source upper-floor traversal, special
movement, solid-span line of sight, bridge occlusion, depth ordering and matching
archived geometry banks still need original traces and asymmetric render
fixtures. The synthetic test intentionally draws no mismatched scenery. No new
map art, full-scene size claim or source-floor fidelity claim is made here.

The16-node walking-animation path remains bounded: oversized ordinary paths
fail explicitly. Streamed paths or special movement need their own verified
contract. Source action/status families, persistent guests, maximum combined
residency/stack/fragmentation, full scene extrema, map/scene v1 validators/editors
and the production gate in [CORE_AUDIT.md](CORE_AUDIT.md) remain open. B06/B07 LCD
reports still need hardware investigation. The user's live TiEmu and standard
bundles are untouched.

```sh
rtk proxy make -C games/fft/campaign core-full-grid-check
rtk proxy make -C games/fft/campaign core-cell-check core-grid-check
rtk proxy python3 games/fft/campaign/check_core_boundaries.py \
  --out games/fft/campaign/x/core-256/boundaries
rtk proxy python3 games/fft/campaign/check_capacity_render.py \
  --out games/fft/campaign/x/core-256/render
rtk proxy python3 games/fft/campaign/check_flow.py --native \
  --out games/fft/campaign/x/core-256/flow
rtk proxy make -C games/fft/campaign bundle \
  BUNDLE_OUTPUT=x/core-256/fftcampaign.89g \
  BUNDLE_BUDGET=x/core-256/package.json
rtk proxy python3 games/fft/tools/core_budget.py --native \
  --group games/fft/campaign/x/core-256/fftcampaign.89g \
  --out games/fft/campaign/x/core-256/native-budget.json
```
