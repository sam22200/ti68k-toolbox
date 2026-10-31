# Source solid spans and ordinary headroom

Subsequent unapproved intermediate-jump candidate: [CORE_JUMPS.md](CORE_JUMPS.md).
The sizes and unchanged169-image result below are the preceding checkpoint.

2026-10-25, workstream4, following [CORE_FULL_GRID.md](CORE_FULL_GRID.md).
Thirteen-actor/256-node storage remains the target. This checkpoint adds source
solid spans to the straight-ray AI terrain test and endpoint headroom to the
ordinary movement graph. Both retail chapter descriptors now carry validated
source geometry. The production gate remains closed.

## Original evidence and limits

`tools/surface_data.py` reads the local US disc, records terrain/resource hashes,
and runs the original BATTLE overlay instructions under Unicorn2.1.4 with explicit
fixture RAM. It checks the instruction hashes before execution. No replacement
MIPS instructions or trampoline are generated. The source decompilation is read
only, pinned at `83c1e00d410277fb0a3f9acf10ca2da1e8dabe22`.

| Original function | Address | Bytes | Purpose |
|---|---|---|---|
|battle_map_get_tile_data_pointer|0x80183FB4|104|Source lower/upper addressing|
|battle_effect_test_position_for_obstacle|0x801AF8B4|644|Point versus terrain solid span|
|battle_move_calculate_tile_ceiling|0x80176164|200|Ceiling above an exit edge|
|battle_move_update_candidate|0x80177794|584|Ordinary candidate headroom prefix|

`x/core-terrain/oracle/source.json` records complete function hashes, terrain
hashes and the exact tool version. The9294 queries use165 unique source/synthetic
columns drawn from MAP022,056,001,029,053 and asymmetric bridge/boundary fixtures.
Actor collision boxes are absent. The headroom queries isolate that prefix of
candidate validation; they do not certify the rest of original movement.
This executes original instructions with controlled inputs, not a full original
PS1 gameplay, GPU or timing trace.

**Observed point collision:** terrain occupies `bottom < height <= top`, with
positive-up heights in the TI model and negative vertical positions in FFT.
The lower record uses ceiling depth as thickness, falling back to its complete
base height when zero. The upper record instead falls back to one whole height
unit. Blocked upper records still collide: projectile geometry must survive even
when a record has no compact walkable node.

**Observed ceiling:** ignore blocked records; compute the requested slope edge
in half-height units, subtract twice the ceiling depth, and retain the smallest
candidate strictly above the exit, capped at127. Original byte wrap is retained.
Ordinary headroom requires source ceiling >= entry+body, destination ceiling >=
source exit+body, and source ceiling >= source exit+body. Equality passes. The
current adapter uses body6; body4 source boundaries are tested for later
Frog/Chicken support, without implementing those statuses here.

## Archive, ownership and module contract

The shared portable kernel is `../surface_geometry.h`. Its FGS1 section contains
an8-byte header (`FGS1`,u8 width,u8 height,u16 big-endian area), followed by8 bytes
per source column: two4-byte layer records `(base height, depth/half-height,
slope, ceiling depth low5 bits | blocked bit7)`. Reserved bits are rejected.
Maximum area is256 source columns, maximum section2056 bytes, axes at most18.
Source columns and compact walkable IDs remain independent.

`tools/chapter_desc.py` appends an aligned FGS1 section to each existing FCD3
body and a terminal8-byte footer (`SGE1`,u16 offset,u16 size). Header offsets
remain unchanged. The loader validates extents, alignment, exact dimensions,
reserved flags and the optional AMS six-byte trailer. Missing or malformed
geometry fails chapter loading. This extension is not the final frozen map/scene
v1 format. `check_core_boundaries.py` preserves the section when making13-actor
descriptors and tests missing, truncated and invalid spans.

The payload is borrowed directly from the archived descriptor: no geometry
allocation or decompression. Gariland adds1216 bytes, Orbonne1136, total2352.
Auxiliary `ffgt.bin`/`ffot.bin` extraction products are not extra packaged
variables. The group remains42 variables. `Work.surface` adds one4-byte native
pointer; an existing reserved byte becomes `surface_room_needed`.
`MOD_ABI=4` requires all seven FMB2 images to be rebuilt together. FSV3 saves,
Unit80 bytes, State1586 and Save3138 remain unchanged.

`battle_bind_grid` clears old geometry ownership; `battle_bind_surface` validates
before changing it and invalidates cached graph edges. Unbound synthetic fixtures
retain their explicit simplified column model. Normal retail chapter loading
requires source geometry and does not silently fall back.

## Tactical integration and performance

`edges` filters ordinary endpoint headroom using source column addresses and
world edge shifts `{0,2,6,4}`; the opposite side is xor2. Existing movement costs,
jump/height policy and occupancy stay in the shared tactical adapter. The source
water depth is added after ceiling calculation. Intermediate leap spans and
special movement still need their own source fixtures.

`ai_clear` checks the sampled straight ray against both solid spans at the
source column, independently from compact node ordering. `battle_line_clear`
provides a bounded actor/target entry point; native dispatch uses AI module op4
through the resident context, with no overlay-to-overlay call. This is not a
bow-ballistic evaluator or integration of every player targeting mode.
Logical projectile collision does not change raster depth/cover masks.

A data-derived classifier bypasses the ceiling filter for dense lower-only
rectangles when every upper record is blocked and every possible lower edge
plus water plus body6 is <=127. It conservatively bounds slope side3 and never
uses map IDs. Compact custom grids keep the full query path. This bypass only
applies to movement ceilings; blocked upper projectile collision remains active.

The native cold Gariland graph fixture checks identical graph fingerprints:

| Variant | MC68000 datasheet cycles | CPU time at12MHz |
|---|---:|---:|
|Explicit old-rule control|1762548|146.879ms|
|Full source endpoint filter|3201412|266.784ms|
|Data-classified source filter|1762540|146.878ms|

These are cold graph-build CPU zones, not warmed input or LCD timings. Geometry
helper/edge compiler sections use shifts/adds and no long/float helpers. The
existing ray setup still contains `__mulsi3`/`__divsi3`; the inlined solid-span
loop adds none. Details: `x/core-terrain/compiler-review.json`.

## Regression and size evidence

- 9462 source-kernel/schema checks, hash `8A31E1BE`, agree PC, full ASan/UBSan and
 actual68000;9294 are original-instruction queries.
- 55 actual battle integration checks, hash `FF05E2DC`, agree PC/sanitizers/68000:
13 actors, six permuted lower/upper compact nodes, bidirectional rays, exact
 clearance, blocked-upper solids, invalid bind preservation and stale reset.
 The battle harness excludes legacy signed-shift sanitizer noise; the pure
 geometry suite has no such exemption.
- 3181 full-grid checks,128 canonical-cell checks,68 module-bank checks,
91186 frozen movement comparisons and1846 descriptor/boundary checks pass.
- 37 campaign and24 opening/scene/control replays pass, plus native flow95
 assertions,13-actor renderer checks, cancellation, rotation and Charge gates.
- 169 reference PNGs are byte-identical; nine fixed11 control pictures also
 match. No visual approval is inferred for future bridge render changes.
- All16 decoder streams, repeated three times, retain the previous cycle counts.
 Root/campaign/opening host suites and16 audit tests pass.

Final private `x/core-terrain/fftcampaign.89g`:469100 bytes,42 variables,
SHA256 `a309f0418b3e6341aa707105e17f92e736b71f4fa6ab9d8bed0736135a555db4`.
This is +3033 bytes versus466067 at the full-grid checkpoint:2352 source-data
bytes and681 other packaged bytes. Resident packed battle payload41081;
module archive payload33332, largest module image7406.

Native Work45514 (+4). Gariland's five requested heap blocks are
`[4724,32320,61674,10176,7200]`, total116094. Maximum256 nodes with the current
202-row scene requests121182; topology growth from150 to256 temporarily
requests128382. Largest individual block61674. These are allocation formulas,
not heap high-water, combined supervisor/program/module/grayscale residency,
stack, fragmentation or maximum future scene size. Code residency also changes.
The existing `ffca`64000-byte planning alert remains; no new alert appears.

`package.json`, `native-budget.json`, `source.json`, `results.json`,
`battle/results.json`, `image-regression.json`, `verification.json`,
`regressions/` logs and `compiler-review.json` live
under `x/core-terrain/`. `before/` preserves the466067 baseline and169 old PNGs
inside `screens.zip`; `controls/` preserves the unclassified terrain candidate.
Generated commercial data/oracle banks remain local ignored artifacts. Standard
bundle and the user's live TiEmu remain untouched; no hardware validation claimed.

## Reproduce and next gates

From the repository root, install the host-only oracle dependency once:

```sh
tools/pyenv/bin/pip install -r games/fft/tools/requirements-surface-oracle.txt
make -C games/fft/campaign core-surface-check
```

Run native builds serially; they share GCC4TI object paths. Full surrounding
regressions use the commands documented in CORE_FULL_GRID.md, plus
`check_core_boundaries.py`, `check_flow.py --native`, `check_capacity_render.py`
and the normal campaign/opening headless checks. Bundle privately with
`BUNDLE_OUTPUT=x/core-terrain/fftcampaign.89g` and
`BUNDLE_BUDGET=x/core-terrain/package.json`; run `tools/core_budget.py --native`
against that group, then `tools/core_audit.py`. Do not overwrite the standard
playtest group or a previous report before preserving the comparison baseline.

Still required before production: source upper-floor paths and special/leap
clearance; source knockback destination/ceiling policy; upper-floor raster cover
and feet in all four source-aligned views; player/ballistic targeting; a complete
action/status queue; source scene extrema; combined maximum residency; frozen
map/scene schemas and editors. Knockback's highest eligible floor and body4
ceiling convention are a static research lead, not an implemented rule here.
