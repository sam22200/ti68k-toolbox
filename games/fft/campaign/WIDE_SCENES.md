# Per-map scene width: promoted ABI99 core

Latest follow-up: [RENDER_OPTIMIZATION.md](RENDER_OPTIMIZATION.md) implements
the user's human-rendering/HUD priorities. ABI99 passes selected ordinary
360k gates at12 and13 units (Zeklaus worst331842/356832). The [installed-RAM milestone](INSTALLED_RAM_ABI99.md) now passes after
deferring the first scene allocation to its descriptor. ABI99 is promoted;
Zeklaus13 has436 free/430 contiguous bytes. C2, width/storage, State/Save/Work and cache
capacities are retained. The ABI98 CPU failures below are historical baselines.

2026-11-23. The shared battle engine now loads the approved complete C2 Zeklaus
art without a private width override. Existing maps retain their320px projection;
Zeklaus uses352px, preserving its336px mesh and all180 source cells. The corrected ABI99 core replaces ABI97 as the reviewed reference, while
its old groups/proofs remain preserved. Current key10965478d3a2c348c22d2dea8e6ac51d
is installed-RAM checked; full Chapter I content and16KiB free remain separate.

## Storage and allocation

| Format | Width / row bytes | Maximum rows | Two color planes |
|---|---|---|---|
| R4, unchanged |320 /40|202|16160 bytes|
| R5 |352 /44|188|16544 bytes|

R5 retains R4's eight-byte header, raw colors/optional mask and compressed
ragged depth; its tag selects44-byte rows. All four views must share their
stride and descriptor height. The independent color cap is16544, so the
invalid product352x202 is rejected. Depth stays18488, actors42, nodes256,
axis18, unit slots13. R4 serialization remains byte-identical. The packager
and cold C loader enforce these same limits, including mixed-view rejection.

`sc_bytes` and the precomputed `sc_plane` are per-map renderer globals, outside
State/Save. ModCtx396 ->400 and ABI97 ->98 propagate them to every image.
State2278, Unit116, Work37790 and FSV9 Save4684 remain. All29 images are rebuilt;
zone9118, graphics4132 and cold Save reader3546 share the same4132-byte block.

The combined scene/Work block follows the active map at cold entry. Its initial
reservation retains the old320x202 capacity; RESOURCES10 grows or shrinks it
after validating the requested arena. Work follows the active color planes.
The old block is freed before replacement, avoiding two simultaneous54KiB
requests. Failure clears the slot/capacity and leaves the depth/actor allocations
available for retry. No allocation occurs in a frame or ordinary scene step.

| Map | Colors | Scene + native Work | Navigation |
|---|---:|---:|---:|
| Orbonne320x192 |15360|53150|existing node count|
| Gariland320x202 |16160|53950|3000|
| Zeklaus352x188 |16544|54334|3600|

Zeklaus adds984 scene/navigation bytes versus Gariland. This excludes program,
overlay, companion, allocator, stack, archive locks and transition residency;
it is **not** an installed free-RAM prediction. The deferred16KiB target and
the previous pilot's1504 free bytes remain historical measurements.

One shared `mod_context` address calculation is kept out of line to fit the
real packer limit. The scene-copy loop snapshots its fixed stride locally
instead of repeatedly reloading an aliased global. No new assembly or shader.

## Functional evidence

`x/wide-core/accepted/` contains PC/ASan/UBSan logs with13403 assertions,
including every source cell in all four views, exact C2 color/depth decoding,
cursor/camera/cover bounds, directional input, animated rotation endpoints,
movement, reentry,352x188 ->320x202 replacement, injected allocation failure
and successful retry. Four pictures exactly match the approved static-width
carrier. Default40-byte decoding still matches20 HEAD reference banks.

The initial ABI98 native13-actor Zeklaus board/rotation replays match PC
framebuffers and identity (`905C`, `1B90`), but failed CPU acceptance; ABI99's
passing replacements are linked above. The actors are borrowed
Gariland diagnostics; original Zeklaus encounter extraction is still separate.
The source-data package control accepts R5 and rejects a mixed-width bank;
it is explicitly a diagnostic package, not a campaign continuation.

`x/wide-core/pilot/` passes actual New Game, confirmed scenes, deployment,
ordinary fights, results, towns/Formation, Save/Continue and defeat/retry on
PC and sanitizers. A selected native Gariland fight takes8736 frames without
God/skip and produces the identical4684-byte durable save, SHA256
`c37b0544ac51bdcfa583c23c22ab35208c396ee45f85d3095173a08270cc7d4f`.
`final-pilot/` replays the final candidate groups on PC. Package ownership and
supervisor checks pass42 host +1023 C cases with sanitizers. No TiEmu run.

## ABI98 CPU baseline (superseded by the rendering follow-up)

These are datasheet cycles, not hardware FPS. Measurements use the production
100-row draw setting. Keep the normal360000-cycle assertion; `--measure-only`
retains failing profiles and never turns an overrun into acceptance.

|13-actor case|Ordinary mean|Ordinary peak|360k gate|
|---|---:|---:|---|
| HEAD Gariland reference |see reference log|371372|Fail|
| Current Gariland board |247963|372140|Fail|
| Zeklaus board |357218|453064|Fail|
| Zeklaus rotation replay, non-turn frames |349239|459644|Fail|

Zeklaus's later board frames also peak432282, so it is not only a cold-load
spike. Gariland's reference breach predates this change; the current board
adds768 cycles to that peak. The old348k/353k historical reports are not a
substitute for this source/configuration-matched reference. Do not describe
either case as passing. Cold Zeklaus loads/turns peak2.16M/4.22M separately.

This failure motivated the delivered rendering/HUD follow-up, retaining C2,
all cells and measured cache capacities. The subsequent installed-RAM milestone and promotion are delivered in
INSTALLED_RAM_ABI99.md; it also records the cold fragmentation correction. Sweegy/Fovoham depth adaptation and source
encounter authoring remain.

## Private build and handoff

### Twelve-unit comparison and profiling

Follow-up requested by the user: most scenario battles need12 units; only two
need13 (user-provided campaign planning constraint, not a new disc census).
Keep capacity13, and exercise both populations. `check_capacity_render.py`
now accepts `--actors 11|12|13` (default13) and optional `--profile-zones`.
The private descriptor retains the original eleven records and adds one or
two identical diagnostic actors on the same legal cells. The default13 output
is unchanged. These remain borrowed actors, not a source Zeklaus encounter.

| Uninstrumented case | Units | Ordinary mean | Ordinary peak |
|---|---:|---:|---:|
| Zeklaus board |12|349628|459576|
| Zeklaus rotation replay, non-turn frames |12|346978|432130|
| Zeklaus board, previous unchanged reference |13|357218|453064|
| Zeklaus rotation replay, previous unchanged reference |13|349239|459644|
| Gariland board |12|243854|368002|

Evidence: `x/wide-core/units-profile/twelve/`, `gariland12/`, `zones12/`
and `zones13/`. All PC/native final framebuffer and identity checks agree.
No emulator or production-engine changes. Instrumented builds are separate:
markers affect register allocation, so use uninstrumented numbers for gates.

Zone averages over the90-frame Zeklaus board (including the initial cold
frame): units231289/249854 cycles for12/13, screen scenery34083 for either,
cursor/HUD47101/32017. Menu construction averages76207, peaks78430; the
cursor/HUD zone peaks132908. Do not add independent zone maxima as if they
occurred in the same frame. Twelve actors permit more deferred HUD work, so
their board peak is slightly higher despite the lower mean; actor count alone
is not a monotonic whole-frame cost predictor.

Priority candidates, not promised gains or implemented optimizations:

1. Reduce the dominant unit draw/composition path: repeated human/monster
   dispatch, repeated pose/frame queries and unchanged cover composition.
   Preserve poses, masks, cache-key eviction validation and42 actor entries;
   prove equal planes and measure any proposed reuse against RAM/size limits.
2. Split and schedule HUD/menu reconstruction using its measured cost.
   The current HUD quantum is estimated at60k, while a menu chunk reaches78k;
   the forced-work rules can combine this with other expensive frame work.
   Preserve first-frame input feedback and bounded completion latency.
3. Optimize the viewport/background transfer with the existing native drawing
   primitives and fewer loop/setup costs, keeping dynamic stride and exact
   output. This is a smaller opportunity on this replay (~34k average scenery
   zone); it cannot supply the roughly100k peak reduction by itself.

The12-unit Zeklaus mean approaches360k, but the peak still needs about100k
removed/spread. None of these measurements accepts the candidate CPU gate.

```sh
rtk proxy tools/pyenv/bin/python games/fft/campaign/build_wide_core.py --out games/fft/campaign/x/wide-opt/next
rtk proxy make -C games/fft/campaign zeklaus-check
```

The builder reuses unchanged cold programs/assets from the explicit preserved
`x/chapter1-ready/package-source.89g`, rebuilding the battle entry and all29
matching modules under `x/wide-core/`. It writes source fingerprints and an
explicit `performance_accepted:false`, never touches the accepted package tree,
production contract, saves or live emulator. `build.json` records the exact
candidate key, sizes and packer headroom; `contract/candidate-contract.json`
is an interface snapshot, not a frozen release. The default packager spec now
describes the source candidate's R4/R5 support; the installed ABI97 remains
usable only with its matching old headers/contract/groups.

For bounded profiling, use `check_capacity_render.py --module-dir
x/wide-core/modules --arena-dir x/wide-core/accepted/arena --case board
--case rotations --measure-only --out x/wide-core/<new-run>`. The explicit
measurement mode still requires framebuffer/identity equality. Earlier failing
logs remain in `narrow-render/`, `reference-render100/`, `wide-render/`;
final measurements are `narrow-measure/` and `wide-measure/`.
