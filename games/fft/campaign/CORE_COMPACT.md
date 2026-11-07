# Compact common topology and six-point search costs

2026-10-25. **User priority correction:** Jump is minor. This checkpoint closes
common storage work already changed, then hands priority to L2 actions/statuses.
Long-span movement/body refinement is deferred; ordinary one-gap gameplay stays.

## Implemented storage contract

Chapter topology now stores two-byte destination/kind pairs and four cached
first-neighbor IDs per source node. Direction replaces one via ID per link.
Explicit presence remains in kind bit2, so node255 is never an invalid sentinel.
Node coordinates/layer still select the real source neighbor, not arithmetic
on compact IDs. Source/target occupancy rules are unchanged in live gameplay.

Rows reserve16 links on a lower-only grid,32 when the bound grid has any upper
nodes. `battle_bind_grid` chooses32/64-byte row stride from validated data.
Four extra neighbor bytes make36/68 bytes per node. This is capacity; the current
builder still emits ordinary and one-gap links. Longer injected links in the
fixture do not establish playable all-intermediate collision policy.

Allocation capacity is now retained BYTES, not node count. Via rows begin after
the CURRENT edge rows, including when a smaller map/stride reuses a larger
allocation. Grow-then-free failure rollback preserves the previous binding.
No sixth heap block is introduced. All consumers rebuild with ABI7; FMB2 still
has9 images. Removing the old Status ability/count callbacks moves those two
private helpers into the existing SCREENS source, reducing ModCtx by8 native
bytes. Shared Status input/render behavior is unchanged; no hot-loop overlay
copy is introduced. The reserved module zone is5726 bytes.

|Fresh native allocation/layout|Previous|Current|Difference|
|---|---:|---:|---:|
|Work|45514|45516|+2|
|Gariland topology,150 nodes|7200|5400|-1800|
|Gariland five heap requests|116094|114296|-1798|
|Orbonne five heap requests|115614|113936|-1678|
|256 nodes, two-floor rows/current scene|121182|126304|+5122|

The maximum row reserve grows to support32 links, while the pilot lower-only
maps shrink. These are layout/allocation formulas, not a certified combined
RAM/stack/residency peak. A previous larger graph remains allocated until release.
Current pilot→maximum growth transient is22808 topology bytes. FSV3/Unit80/
State1586 remain unchanged; native Search is1054 bytes.

## Performance decision

The first candidate recomputed a neighbor through `node_at` during every leap
check and slowed ordinary search by about35% in the Gariland fixture. It was
rejected. Its logs/package are retained in `rejected-direct/`, never shipped as
the current candidate. A four-neighbor cache removes that conversion from the
search loop while keeping a memory saving.

A source-stage benchmark rebuilds the exact captured pre-change Work/search
and original ABI6 module bank in a temporary tree. Both versions run the same
two12-point real-map searches, one-node slices, outward modes and fixed units.
All costs/predecessors retain the same hashes. MC68000 datasheet CPU totals:

|Map / two searches|Previous|Cached compact|Change|
|---|---:|---:|---:|
|Gariland ordinary|701196|698820|-0.34%|
|Orbonne ordinary|600340|600304|-0.01%|
|Gariland wider-cost path|785990|772818|-1.68%|
|Orbonne wider-cost path|673636|662848|-1.60%|

These bounded measurements are not whole-game FPS or hardware timing. No
speed claim is extrapolated to future32-link gameplay rows. Artifacts:
`x/core-compact/baseline/`, `logs/span-cache.log` and `performance.json`.
`benchmark_compact_graph.py` requires the explicit captured baseline snapshot;
that research source stage is not a runtime fallback.

## Six-point cost contract

Generic wide-link kind bit5 marks explicit destination terrain cost3; the gap
is added independently of source water. Bits3..4 keep0..2 extra gap cells beyond
the first; unknown flags and a fourth gap fail generic validation. Compact Work
rows additionally use bits6..7 for cardinal direction. The eight linked buckets
now exercise costs through6, within their existing storage. Ordinary graphs keep
the previous four-bucket fast path.

The previous source-cost fixture proves the forward six-point case. Current
core tests cover it in the independent distance oracle, mixed cyclic graphs,
yielded/full searches and the actual Work adapter. The live builder does not
emit storm/terrain3 or longer spans yet; this does not implement weather or
change existing water policy. CORE_JUMP_COSTS.md retains producer scope limits.

## Focused verification

- Generic graph/search:27624 PC/sanitized/68000 checks, cost6 boundaries plus
mixed cyclic weights, FIFO ties,256 targets and node255.
- Compact Work fixture:170 checks,32 outgoing links on a9x14/two-layer252-node
board, both landing floors, four directions, true first-neighbor identities,
cost6/path extraction and reuse of a larger allocation on a lower-only rebind.
- Full-grid adapter:3185 checks, including reversed compact IDs, upper node255,
allocation-growth failure/rollback, AI ordering and cleanup.
- Frozen accepted graph/cache checks:629 Gariland and589 Orbonne; source rules
remain identical while row/via storage changes. Both maps retain1219/1065
fast/wide caller checks and hashes AF1961E0/435FD706.
- The direct one-gap caller,128 cell transitions and68 loader-boundary cases
pass. Matching ABI7 images validate and execute through setup/scene/save/results;
PC/native flow checks retain81 Gariland/14 Orbonne and identical settled saves.
- Status input/render replay and the two accepted range views remain identical.
No census,3M profile sweep, font/management suite or decoder-stream benchmark
was rerun. Live TiEmu and the standard playtest group were untouched.

```sh
make -C games/fft/campaign core-compact-check
```

## Package and next work

Private `x/core-compact/fftcampaign.89g`:472001 bytes,42 variables, +119 versus
471882. All28 non-code variable hashes are identical. Packed battle input64825
bytes leaves111 below the64936-byte limit. This remains tight; future L2 code
needs explicit module/resident budgeting. See package.json/verification.json for
hashes, actual module sizes, source snapshots, layouts and targeted evidence.

**Next: L2 shared actions/statuses**, per the user's priority correction. Keep
horizontal Jump research/refinements deferred. Later integration can reuse the
32-link/cost6 contracts, but must produce the actual actor/status/movement
records and validate every intermediate column before enabling those links.
This checkpoint does not close all L1 geometry or the production gate.
