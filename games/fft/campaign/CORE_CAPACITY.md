# Descriptor-driven thirteen-actor core

Subsequent milestone: [CORE_GEOMETRY.md](CORE_GEOMETRY.md). The13-capacity
results below remain a historical checkpoint. Current Work/search and save-buffer
ownership change the allocation/budget figures; the save tail now shares the
longer-lived State allocation rather than remaining in a code overlay.

2026-10-24, workstream4, first implementation after the reduced-envelope audit.
The battle core now stores up to13 actors and reads each encounter's actual
count from its descriptor. Orbonne and Gariland still contain their original
eleven actors. This is a capacity milestone, not completion of the combat core.
The accepted next geometry contract remains256 compact8-bit nodes across both
floors; **that geometry is not integrated into gameplay yet**.

## Implementation

- `UNIT_CAPACITY` is13; `NUNIT` is the validated encounter count. State, pending
  actions, rewards, cover/feet caches, AI scratch and draw-order storage use the
  capacity, while active loops use the count. Unused State records are cleared
  as absent. Existing16-bit actor masks and four-bit forecast targets still fit.
- FCD3 adds the count and per-actor result records. The loader rejects0/14 actors,
  old FCD2 descriptors, misaligned/truncated definitions and out-of-range actor
  metadata spans. Source reward pairs now belong to the encounter; a13th actor
  cannot read past the former eleven-row flow reward table. Orbonne keeps its
  no-trophies behavior. Broader map/scene validation is still a separate gate.
- Module context carries the actual count. The signature includes the changed
  State/Work/context layouts; matching core, modules and descriptors must be
  distributed together. Existing source-slot/party-role fields are retained;
  this does not implement every conditional spawn or guest transition.
- Scene actor storage also uses the13-actor limit. Current scene beat counts and
  artwork are unchanged. Formation/save roster capacity remains separate.
- `State` moves from embedded program storage to one heap allocation. The
  initial expanded build exceeded TTPack's64936-byte input ceiling. Moving its
 1494 reserved bytes out lets the packed production build succeed. State stays
  alive after the large battle buffers are released so the supervisor can read
  the victory handoff; the final exit handler then frees it. Allocation failure
  returns through the existing error/escape path without dereferencing State.

No save migration, new artwork, new map or handwritten assembly is introduced.
The explicit `FFT_CAPACITY_CONTROL11` build is a regression control only.

## Verification

PC and actual68000 runs agree; no TiEmu input or live-session restart was used.

| Check | Result / scope |
|---|---|
|Root and campaign unit suites|Pass; existing ordinary movement, combat, UI and save checks|
|Core boundaries|1813 checks:551 Orbonne,561 Gariland,684 synthetic13 actors,12 malformed-descriptor checks and5 State-allocation-failure checks|
|13-actor boundaries|CT ties, all pending Charges due together, interruption, KO/revive/loot, final mask bit, no-exit path, actor12 through formula and resident AI, rewards from actors11/12|
|Real13-actor rendering|Board, F1/F5 rotations, Status and equipment of actor12; all four PC/native plane checksums agree|
|Fixed11 visual control|Nine combat pictures byte-identical with the same current data and fixed11 versus13-capacity builds|
|Campaign replays|37 native pixel/state replays pass|
|Opening checks|12 tactical replays,6 shared-font scene checks and6 legacy-strip controls pass|
|Flow and save|81 Gariland plus14 Orbonne checks; native split flow/save, receipt replay and trophy overflow checks pass|
|Cancel range / rotation|144 cancellation cases per map;144 rotation cases,80 framebuffers and81840 independent geometric corner checks|
|Charge reference|48 original trials / eight levels / native hash276A2090 pass|
|Input validators|16 source-census/compact-fixture tests pass; still input validation rather than layered gameplay|
|Native decompression|16 streams, three repetitions each; decoded checksums pass|

The rules-only native boundary and Charge harnesses omit their unused frame-loop
compositor to fit the native variable size ceiling. The separate renderer,
campaign and opening replays exercise the real rendering path.

Two obsolete test assumptions were corrected without changing production
presentation: the range-cancel fixture still expected deferred cleanup, and the
standalone scene fixture omitted the accepted shared font bank and used the old
90-frame title duration. Scene controls now supply that bank and compare the
current150-frame timing; post-title pictures and scene state still agree.

Artifacts: `x/core-audit/boundaries/results.json`,
`x/core-capacity/render/results.json`, `x/core-capacity/control11/results.json`,
`x/core-capacity/flow/`, `x/check/` and the opening's native check output.

## Measured storage and CPU

| Native item | Bytes |
|---|---:|
|Unit|80|
|State|1494, previously1314|
|Work|42786, previously42636|
|Four requested battle heap blocks|1494 +32320 +58946 +10176 =102936|
|Largest battle heap block|58946|
|Serialized SaveUnit / Save|176 /3138, unchanged|

The current actor-array expansion is330 bytes across State/Work. The102936-byte
heap sum includes the newly separate State allocation. It excludes executable,
supervisor/module/grayscale residency, stack, allocator overhead, fragmentation
and optional allocations. It is not a measured combined heap high-water.

In the synthetic13-actor fixture, ordinary board frames average214113 cycles,
peak268254; ordinary frames in the rotation replay average246369, peak328770.
Status/equipment peaks are241410/265242. Cold/rebuild/turn-classified frames peak
5896354 cycles. Existing opening fight replays also retain ordinary outliers
above360k cycles. These are datasheet68000 CPU measurements, not a blanket30fps
or hardware/LCD timing claim. Decoder-only costs are914830..1135306 cycles per
stream; loading includes additional work.

Private candidate: `x/core-capacity/fftcampaign.89g`, **467425 bytes**,42 variables,
466335 payload bytes plus1090 transport bytes; **+1380 bytes** versus the466045-byte
baseline. SHA256:
`1ff5c99c80f3cc58df311b77e1aaff1d1f9cb21edd34c5f13eef70bb27064810`.
The packed battle core payload is39561 bytes; its FFBM module bank is35530 bytes
including the data trailer. Raw FCD3 descriptors are2521/2082 bytes.

`package.json` records the previous-package delta; `native-budget.json` records
compiler layouts, source hashes and48 decoder runs. `source.json` records the
retail census alongside this exact candidate's package inventory. The pre-existing ffca bank
still triggers the report's conservative64000-byte planning guard. The standard
466045-byte group and user-owned live TiEmu session are unchanged.

## Reproduce and continue

Run GCC4TI builds serially. No emulator is needed for these checks.

```sh
rtk proxy make -C games/fft test
rtk proxy make -C games/fft/campaign test core-audit-test core-boundaries-check core-capacity-render-check
rtk proxy python3 games/fft/campaign/check.py
rtk proxy python3 games/fft/campaign/check_capacity_control.py
rtk proxy make -C games/fft/campaign bundle \
  BUNDLE_OUTPUT=x/core-capacity/fftcampaign.89g \
  BUNDLE_BUDGET=x/core-capacity/package.json
rtk proxy python3 games/fft/tools/core_budget.py --native \
  --group games/fft/campaign/x/core-capacity/fftcampaign.89g \
  --out games/fft/campaign/x/core-capacity/native-budget.json
```

Next: integrate compact nodes/layered topology, node255 validity and16-bit counts
across movement, targeting, occupancy, cover and all four projections; exercise
the asymmetric4×18/two-floor and256-node fixtures before content production.
The runtime still has150 one-floor tile slots and its old invalid-tile markers.
Do not call the input fixtures a completed geometry migration.

Then stabilize the common action/status/scheduler families, source-trace the
conditional encounters, and finish progression/campaign mechanisms and versioned
map/scene formats/tools. Complete peak-memory/fragmentation/stack/performance
gates and a hardware playthrough. B06/B07 remain open. The full matrix and closed
production gate remain in [CORE_AUDIT.md](CORE_AUDIT.md).
