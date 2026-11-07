# Intermediate horizontal-jump clearance candidate

2026-10-25, workstream4, following [CORE_SURFACES.md](CORE_SURFACES.md).
**Status: user accepted the Orbonne comparison on2026-10-25.** The Decision
marker is cleared. The169 images after that review are the accepted reference.
The subsequent single-floor optimization is recorded in
[CORE_MAP_PROFILE.md](CORE_MAP_PROFILE.md); the preliminary figures below are
historical. No launch or input in the user-owned live TiEmu was authorized.

## Source behavior and target boundary

The original `battle_move_check_horizontal_jump` at0x8017622C is684 bytes,
SHA256 prefix `551a8a31fcf7897c`. `tools/jump_surface_oracle.py` adds that
routine to the existing original-instruction harness. It executes unchanged
local-disc MIPS with controlled frontier/config RAM, no actors and no injected
instructions. Full function hashes and Unicorn2.1.4 provenance are recorded in
`x/core-jump/oracle/source.json`. The source repository remains read-only.

The query tests the whole body's interval at the source exit height, across
both slope-facing edges of both layers of an intermediate source column. It
includes water and ceiling depth, byte wrap, strict endpoint inequalities and
the original127 minimum sentinel. A terrain span strictly inside or containing
the body rejects the jump. The final minimum-height rule also rejects terrain
wholly above the jumper. Merely touching a span endpoint can pass.

Unlike straight projectile collision, this movement query skips blocked terrain
records. Do not share their differing zero-depth/thickness semantics implicitly.
The portable `surface_jump_clear` model in `surface_geometry.h` accepts the
caller-supplied body size; current battle integration uses6. Body4 is tested in
the kernel but Frog/Chicken support is not added by this candidate.

`edges` now calls it for the intermediate SOURCE column of an existing one-gap
jump, even when that column has no compact node on the moving floor. It resolves
the source layer and direction from tactical coordinates, independent of compact
ID order. In bound retail/source fixtures this replaces the old single-floor
height comparison; explicitly unbound synthetic/legacy fixtures retain that
simplified rule. Both normal chapter descriptors already require source data.
No map-specific override or additional archive field is introduced.

**Still incomplete:** endpoint height/steepness/water policies remain the earlier
subset. Horizontal spans longer than one intermediate cell, Fly/Float/Teleport,
unit-top paths/mounts and enemy body spans on either floor need separate work.
The existing dynamic occupancy check still follows the explicit intermediate
node of the moving floor; the terrain kernel must not be described as full
source jump occupancy. No changes to raster bridge masks, projection or sprites.

## Evidence

- 47301 checks, hash `E23CBC36`, match PC, full ASan/UBSan and actual68000.
47264 are original-instruction queries over302 source/synthetic columns,
 four directions, body4/6, exact boundaries, water, blocked upper and byte wrap.
 Twelve test-only banks hold the oracle rows; none is part of the game package.
- 156 actual battle checks, hash `B8383FD2`, match PC/sanitizers/68000. Thirteen
 actors are available; dense six-node and sparse four-node grids exercise both
 source floors, permuted IDs, blocked/clear/just-touching upper spans, lower
 slabs/slopes, reach/path extraction and the Jump1 lower boundary. The battle
 sanitizer excludes pre-existing signed shifts; the pure geometry kernel does not.
- 298 changed current-map edges for Jump1..4 all agree with independent original
 queries: Orbonne136 (130 removed,6 added), Gariland162 (132 removed,30 added).
 This deliberately changes the graph; it is not old-graph equivalence. The
 old max-height shortcut both admitted invalid jumps and rejected valid ones.
- 91186 search comparisons still pass. Both search implementations consume the
 new graph; this proves search agreement, not historical graph/path equality.
- 55 source bridge/ray checks,3181 full-grid checks and1846 boundary checks pass.
 The bridge cold control now expects the source graph to differ from the old
 approximation and verifies equality of full versus classified ceiling paths.
- 37 campaign and24 opening/scene/control native replays pass with matching PC/TI
 pixels/state. Root/campaign/opening host suites pass. The opening end-to-end
 test reaches the ending after3942 battle frames.

Image comparison with the preceding checkpoint:165 of169 PNGs remain identical.
Four differ: `opening/x/check/fight-{first,complete}-{pc,ti}.png`. These are the
Orbonne auto-battle replays, sampled at the same requested frame limits after
the graph changed. Their PC/TI pairs agree. The user subsequently accepted these four captures; the old PNGs are preserved inside `x/core-jump/before/screens.zip`.
The comparison shows both old/new captures at the same scale. A passing CPU
checksum is not treated as user approval.

## Preliminary cost and pending package report

The existing native cold Gariland fixture measures:

| Path | MC68000 datasheet cycles |
|---|---:|
|Explicit historical unbound terrain control|1797760|
|Full source ceiling and jump checks|3468828|
|Classified ceiling plus source jump checks|2050356|

The classified result is about170.86ms at12MHz, versus146.88ms at the previous
source-surface checkpoint: about24ms more for a cold graph build. It is not
warmed input latency, LCD timing or actual hardware FPS. This is preliminary;
no post-review optimization or new package-size report has been run.

No fields, allocation, save layout or module ABI were added. The candidate keeps
ABI4/FSV3 and the same topology bounds. Native total residency, stack high-water,
compressed code delta and final combined package size remain unmeasured here.
Do not quote the old469100-byte group as the size of these new source changes.

## Reproduce and resume

With the existing pinned host-only Unicorn requirement installed:

```sh
make -C games/fft/campaign core-jump-check
```

This serially checks original queries, actual battle reach/path and all changed
current-map jump edges. Additional regression artifacts and logs are preserved
under `x/core-jump/`; `before/` includes old package/native reports and source
snapshots. Synthetic data and original bytes stay local ignored artifacts.

Serialize native builds AND tests sharing `campaign/data/ffsave.sav`: host save
unit tests delete/rewrite that fixture, while campaign replay checks read it.
An initial parallel test attempt collided on that temporary save; rerunning the
complete checks serially passed. This was a test orchestration issue.

The user accepted this checkpoint; the Decision marker is cleared. Next,
finish compiler review, flow/render/non-regression gates and private bundle/
native-budget/source reports against the469100-byte baseline. Measure before
optimizing. Then continue endpoint source movement policies, longer spans and
cross-floor actor obstruction before claiming complete source upper traversal.
