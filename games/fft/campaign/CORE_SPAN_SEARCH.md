# Long-jump prerequisite: bounded wider-cost search

Subsequent checkpoint: [CORE_RESIDENT.md](CORE_RESIDENT.md) resolves the resident
packer ceiling through a ninth PARTY module and Status input relocation. The
wider-cost routine is now included in the current engine; long links and actor
record generation are still pending. Optional-only packaging statements below
describe the preceding checkpoint.

2026-10-25, workstream4. Follows [CORE_LONG_JUMPS.md](CORE_LONG_JUMPS.md).
**Status: optional search path verified; longer jumps are still not playable.**
The standard Work-bound build excludes this path. `FFT_NAV_SPANS` enables it
in the dedicated battle fixture; pure graph fixtures exercise both variants.

## Contract and storage

The existing four append-only Dial queues rely on edge costs1..3. A longer
jump can have a dry cost4, so extending link kinds without changing that proof
would lose future records in the current bucket. Ordinary searches retain that
fast algorithm; validation of the cost class happens once at graph binding,
not on every hot-loop edge.

The optional path uses eight linked FIFO buckets. Each pending node has one
next/previous record. A strictly improved distance removes its old record before
appending the new one; equal-cost entries keep insertion order. Bucket heads,
tails and counts express presence explicitly: ID255 remains a valid node.
Search count/need count and the254 maximum cost remain independent of IDs.
Zero-budget yields retain the current frontier/request count; restart resets
metadata without clearing all node links.

Two byte link arrays and bucket metadata fit inside the existing1024 queue
bytes in an anonymous union. Graph construction may continue borrowing all four
old queue rows for facing heights, provided no search is active. The next
initializer resets the chosen representation. There is no new heap request.
Fresh compiler measurements: CoreNavSearch1054 bytes, Work45514, State1586,
Unit80, FSV3 Save3138. ABI5/FMB2 layout and module count8 remain unchanged.
Gariland's five layout requests total116094 bytes;256 nodes/current202-row scene
121182. These are compiler/formula measurements, not full residency/stack peaks.

Link kind bits3..4 encode0..2 additional gap cells beyond the first. The optional
search adds that span to the existing base cost table, supporting abstract costs
1..5. `core_nav_graph_mode_valid` rejects wide kinds in the ordinary cost class,
unknown bits, a fourth gap, non-leap span flags and invalid destination/via IDs.
Callers must bind the matching cost class and set `CORE_NAV_WIDE` in the existing
out parameter; `CORE_NAV_OUT` retains its separate bit. The normal checked
initializer rejects wide mode when the optional path is not compiled.

This tests a bounded search representation, not a PS1 water-cost rule for longer
spans: original dry span costs2..4 are observed, while the wet extension still
needs source trials. The current via byte checks only one intermediate node.
Every intermediate actor/body span must be validated before such a link becomes
playable. Graph validation does not establish that missing geometry policy.

## Focused verification

-27592 checks, hash `D91623D8`, match PC, full ASan/UBSan and actual68000.
Independent O(V²) shortest distances, costs1..5, cyclic/nonlocal graphs, removal
of head/middle/tail queue records, stable predecessor ties, budget0/1/full,
abandoned searches, limits254, both old water directions and256 requested nodes
are covered. The original13887-case-count certificate is retained separately;
this rerun expands only the affected search fixture, not the full core audit.
- Actual Work-bound optional search: Gariland1219 checks (`AF1961E0`),
Orbonne1065 (`435FD706`), PC/sanitizers/native equal. Generated real-map graphs
produce the same distances and predecessors as the ordinary path, including
queue-to-edge-scratch-to-ordinary restarts. An explicitly abstract cost5 graph
checks the real spread adapter and path extraction. It is not a legal long-jump
geometry example.
- Native cost of a one-node yielded call in those current graph fixtures peaks
at6256 cycles on Gariland and5884 on Orbonne for the optional path. Whole12-point searches remain sliced. CPU datasheet counts
are not TiEmu/hardware FPS and do not certify future32-link rows.
- The pre-gate shared-loop change also passed the existing actual-map movement
control on Orbonne/Gariland/a13-actor descriptor. The final optional fixture and
accepted view checks exercise the final paths. No census,3M profile sweep,
font/management suite or decoder stream benchmark was rerun.

Artifacts: `x/core-span-search/kernel/`, `battle/`, `layout.json`, `logs/`.

```sh
make -C games/fft/campaign core-span-search-check
```

## Packaging result and next gate

An attempted build with the wider-cost routine resident exceeded TTPack's
64936-byte input ceiling. The error log is preserved in `logs/bundle-final.log`;
the preceding attempt also crossed the AMS variable-size limit. Keep this
optional path out of the shipping bound build until resident code is split or
reduced deliberately. Never route it through an overlay that can evict a still-
executing AI caller.

The private ordinary package was rebuilt successfully:
`x/core-span-search/fftcampaign.89g`,470444 bytes,42 variables, +37 versus the
470407-byte accepted endpoint/cache milestone. SHA256:
`5a4bbbea89e57d8b6b3a35aef5a4712155fc91583d16fc8a904c7c4a530454d0`.
All28 non-code variable hashes are identical. The standard group is unchanged,
and the user's live TiEmu was never driven. This package does not enable wide
costs or long jumps and must not be presented as doing so.

Next, address resident code capacity together with compact bounded link storage
and all-intermediate actor rules, then activate one coherent runtime path.
Static original `battle_move_check_horizontal_jump` also checks other-team body
spans on BOTH source layers, a standing-height+3 boundary, and frontier record
flags. A Boolean blocker on just the mover's current floor is insufficient.
Execute targeted original actor trials before choosing the native actor policy.
No map-ID exceptions or unnecessary upper checks on classified single-floor
maps should be introduced. Action/status, raster, full residency and production
format/editor gates remain open in CORE_STATUS.
