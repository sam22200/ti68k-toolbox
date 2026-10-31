# Source movement endpoints and cached graph

2026-10-25, workstream4, after [CORE_KNOCKBACK.md](CORE_KNOCKBACK.md).
**Status: user accepted the comparison with “continue” on2026-10-25.** The
Decision marker is cleared. The private package and focused native measurements
are complete. The standard bundle and
user-owned live TiEmu remain untouched.

## Source rule and original evidence

`tools/step_oracle.py` executes unchanged local-disc MIPS with explicit terrain,
frontier, source/destination panels and ample remaining movement budget. It
runs all of these original functions, checking their instruction hashes:

| Function | Address | Bytes | SHA256 prefix |
|---|---|---:|---|
|battle_move_set_tile_flags_for_pathfinding|0x80174E84|1028|ff6f1c4c293f3bbc|
|battle_move_transfer_tiles_height_halves_and_slope_to_scratch_pad|0x801755A8|64|dbfe81c711a9e41a|
|battle_move_init_source_panel|0x8017567C|732|30b7924ca24f1d23|
|battle_move_propagate_destination|0x80175958|1352|8934859d8d97de21|

The112 named cases cover28 semantic situations in four edge orientations:
height-limit equality/excess in both directions, source/destination ramps,
corner steepness, exact/short source and destination headroom, both layers,
blocked destinations, normal versus Move-in-Water source heights, water entry,
and level/descending/ascending horizontal-jump endpoints. Original terrain flag
classification executes rather than supplying expected steep flags by hand.
Actor-top paths are absent. The forward propagation has ample budget and empty
destination panels so acceptance isolates endpoint geometry, not complete costs.
This is a controlled original-instruction experiment, not a complete played
PS1 movement/AI trace. Full hashes/results: `x/core-steps/oracle/source.json`.

The terrain-flag loop spans512 source records; the oracle allows100000
instructions for this fixture. Existing source queries retain their default
10000 cap. No original code is modified or injected; only fixture RAM changes.

**Observed:** movement uses facing edges instead of the tile-center stand
height. Steepness is based on the source slope bits and half height; the source
exit rejects exit>=opposite when steep, whereas the destination rejects
opposite>entry (strict). These inequalities are intentionally asymmetric.
Both endpoint ceilings must also leave room above the destination's OWN entry
height, in addition to the entering character's source height. The old partial
headroom check omitted the destination's own-body condition.

Normal source exit height excludes water depth; Move-in-Water adds it. The
forward destination query adds destination water depth before its height-limit
test. On a horizontal jump with extra span, the landing cannot be HIGHER than
the source edge; level or descending landings can pass within Jump. The earlier
approximation used the reverse sense. Do not infer forward movement from the
reverse path-reconstruction candidate helper.

## Runtime integration

`../step_geometry.h` provides steepness and endpoint checks. Source-bound
`edges` uses these rules for ordinary neighbors and the existing one-gap jump.
The intermediate-jump volume query uses the same ordinary source exit height;
it no longer silently grants the water-surface ability to every character.
Legacy unbound fixtures retain their explicit simplified height rules.

The existing per-map flags still bypass unnecessary ceiling checks and choose
one or two source layers. This does not eliminate required lower-floor slope
and height checks. No map-ID exception, new archive field or heap buffer.
Current units use ordinary source-water mode0 and body6. The helper tests
Move-in-Water mode separately; importing per-unit movement abilities and other
body sizes requires extending both the policy and graph-cache identity.

Existing destination availability, actor blockers, terrain-cost table, final
water-point restriction and path search are unchanged. A water entry may cost
one point while still requiring more than the last available point; do not
replace that existing rule with an assumed two-point cost. Costs and complete
Fly/Float/Teleport/unit-top/lava policies remain separate work. Only one-gap
jumps are supported here; longer horizontal spans and cross-floor enemy body
volumes remain open. Raster cover/feet/projection are not changed by this patch.

## Focused checks and visual gate

- 451 geometry checks, hash `8BB8BBE5`, agree PC, full ASan/UBSan and actual68000:
112 original queries, both steep flags, accept/reject, and full versus cached
single-floor equality. No exhaustive map/profile sweep was rerun.
- 80 actual battle checks, hash `7F8503F0`, agree PC/sanitizers/68000:16 cases with
3/6 nodes and the13-actor descriptor cover edges versus cached stand, steep
exit/entry asymmetry, destination self-headroom, lower/upper transitions, ordinary
water height and one-gap endpoint restrictions through reach/path extraction.
The battle harness excludes legacy signed shifts; the pure kernel does not.
- Two affected movement-range views agree PC/native in both checksums and the
same viewport's fixed shade IDs. Orbonne remains identical to its prior image;
Gariland changes, checksum `BBB7` (Orbonne `BA40`). Existing gallery files and
reference timestamps are preserved. Only the side-by-side comparison is new.

PC screenshots are160x100 at shades24/88/152/216; native snapshots are240x128 at
0/85/170/255. The checker crops the viewport and maps those known palettes;
PNG bytes or raw RGB without that normalization are not valid equivalence tests.
This tool-format fact is recorded in the platform patterns reference.

## Graph preparation and measured cost

Ordinary Jump is capped at7 before endpoint checks and graph-cache lookup. This
comes from static original `battle_move_store_unit_movement_to_scratchpad` code;
the fixture configures that policy before executing the original queries. It
does not execute the unit configuration routine. Raw Jump8/255 reuse effective
Jump7 and cannot collide with the invalid-cache key255. Special Fly/IgnoreHeight
branches require their own policy before integration; displayed stats are intact.

Graph construction temporarily borrows the existing four navigation queues for
four facing-edge bytes per node and the existing actor-block table for steepness
and water depth. There is no live search during this phase. `spread_start` clears
the actor table afterwards; `core_nav_begin` resets queue lengths before reuse.
The cache checks also abandon a partial search and rebuild, including node255
on a256-node grid. Do not access these borrowed buffers concurrently.

The accepted uncached algorithm is frozen in test-only
`step_reference_edges.inc`, never linked into the shipping game. Exact graph
fingerprints agree for both maps at Jump1..4; budget1 and full-budget searches
agree. PC/sanitizers/actual68000 checks: Gariland628 (`C0A3183C`),
Orbonne588 (`3AB95F62`). No exhaustive profile/asset/UI sweep was rerun.

|Native cold graph, mean over Jump1..4|Accepted uncached|Cached|CPU reduction|
|---|---:|---:|---:|
|Gariland|3266702 cycles /272.23ms|2261617 /188.47ms|30.77%|
|Orbonne|2813882 cycles /234.49ms|2020864 /168.41ms|28.18%|

These are MC68000 datasheet CPU times at12MHz, not TiEmu wall time, full-frame
latency or hardware FPS. Each mean represents four graph builds. Earlier Jump3-
only figures have a different denominator. Artifacts: `x/core-steps/cache/`;
pre-cache package/cold logs are retained separately. Generated `edges` assembly
has no32-bit multiply/divide or floating-point helper calls.

## Private package and accepted images

`x/core-steps/fftcampaign.89g`:470407 bytes,42 variables, +178 versus the
470229-byte recoil group. SHA256:
`b30d8b5b69bc119e257fa208a22c55c4c7570a75176f6f0b2a6b2572c35b0038`.
The28 non-code variables are hash-identical. ABI5/eight FMB2 modules, FSV3,
Unit80, Work45514 and State1586 fields are unchanged; no new heap allocation.
The known five-block layout requests remain116094 bytes for Gariland and121182
at256 nodes/current scene; these are layout accounting, not a fresh full-residency
peak/stack/fragmentation measurement. `package.json` deliberately does not copy
old native measurements into a new report. The existing ffca planning alert
remains; it is not introduced by this rule-only patch.

`check_step_views.py --verify-accepted` checks the two affected PC/native views
against the accepted pixels. It preserves the before/after proof, archives the
old Gariland pair once, and promotes only that accepted reference pair. Orbonne
and unrelated image dates stay unchanged. This does not grant visual approval
to future changed pixels. Standard group and live TiEmu remain untouched.

Reproduce only the affected modules:

```sh
make -C games/fft/campaign core-step-check
make -C games/fft/campaign core-step-cache-check
# Matching current module banks, only two affected pictures:
tools/pyenv/bin/python games/fft/campaign/check_step_views.py --verify-accepted
```

Next: [CORE_LONG_JUMPS.md](CORE_LONG_JUMPS.md). Its full-spread geometry prototype
is verified separately, but longer jumps are not yet connected to game search.
