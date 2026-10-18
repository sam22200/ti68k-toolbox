# Native affine-textured F1/F5 experiment

The later [merged-surface and visibility optimization](SURFACE_TURNS.md) retains
80x50 textured turns and measures 2.281-2.480 s in Titanium TiEmu, versus the initial
mapper's 3.312-3.535 s below. That note contains the newer builds and comparisons.

`FFT_TURN_TEXTURE` enables a genuine integer C texture mapper on the TI, replacing only
the intermediate isometric turn pictures. Three angles (22.5, 45 and 67.5 degrees)
match the low-poly fallback's sampling. Settled isometric views and direct close/wide
inspection keep their existing presentation; automatic perspective attack shots stay
disabled. Low-poly remains the default while this visual/performance experiment is reviewed.

This implements runtime texture sampling. It does not replay prepared camera images,
call the PC double-precision mapper or restore animated perspective travelling.

## Renderer and original assets

- The 479 vertices / 866 triangles of the original Gariland mesh are projected with
  fixed-point sine/cosine and the existing verified integer multiply/divide wrappers.
- Backface and screen bounds tests reject geometry before scan conversion. Affine
  8.8 UV and depth gradients are solved once per surviving triangle, then incremented
  through scanlines. Pixel loops contain no multiply, divide or floating-point operation.
- Runtime nearest-neighbour samples a 128x128 atlas with two grey/alpha nibbles per byte.
  Original palette/page artwork is filtered 2x2 **offline**, using the selected
  contrast-plus thresholds. Water retains light/dark grey; transparent texels keep their
  alpha instead of writing depth. This is a cheaper sampler than the PC filtered review.
- A reused 80x50 byte depth buffer resolves scenery. Actual outlined TI unit sprites
  are drawn at native resolution through cover masks derived from that depth. Actor
  direction follows the rotating camera; their original animation is retained.
- The 80x50 scenery planes expand to 160x100 with the existing lookup. There is no full
  160x100 Z-buffer and no persistent per-angle bitmap bank.

Texture assets generated from the local disc are four archive variables: `fftuv` (6,936 B),
`fftat0` and `fftat1` (57,344 B each), `fftat2` (16,384 B), **138,008 raw bytes total**.
They are read in place from archive. Generation hashes and provenance are retained under
`x/turn-texture/assets/texture-assets.json`; ROM-derived banks are not committed.

The 4,000-byte depth and 2,874-byte projected-vertex scratch reuse existing Status and
background work memory during a turn. There is no added bulk heap allocation on TI;
the work struct remains 43,346 bytes. Small persistent pointers/counters and transient
stack are additional; peak stack and a physical calculator's total free RAM were not
measured. Generated code reserves 304 bytes of locals plus saved registers in
`render_turn`, with nested helper use beyond that. Grayscale-driver costs remain separate.

The mapper invalidates affected background/perspective caches so a subsequent direct
inspection rebuilds its own depth and poses. It preserves working low-poly and PC
filtered renderers for comparison. No new 68000 assembly was authored.

## Build and checks

With the existing local mesh, reviewed atlas thresholds and archived camera-demo assets:

```sh
make turn-texture-bench
```

`tools/turn_texture_assets.py` prepares the quantized atlas; `turn_texture_bench.py`
builds flat and textured variants using the same eight-case input driver as low-poly.
Each covers F1/F5 from all four orientations. Both match **32 PC/TI framebuffers**,
produce three distinct intermediate images, settle to the same target views, preserve
unit records/battle RNG and never enter perspective travelling/cinematic state.

The sequential demo also matches **512 PC/TI frames**: F5 twice, F1 twice, direct
wide/close inspection and ISO return. AddressSanitizer passes the 32-frame textured PC
oracle. Ordinary game and direct-camera regression tests run separately. Generated TI
code inspection finds no `__mulsi3`, `__divsi3` or float helpers in the native mapper.

Outputs:

- `x/turn-texture/textured/fftturn.89z`: automatic actual-timer/capture suite, 62,165 B.
- `x/turn-texture/textured/fftturnc.89z`: headless datasheet-cycle companion.
- `x/turn-texture/flat/`: same detailed geometry/depth/actors without texture sampling.
- `x/turn-texture/demo/fftcut.89z`: sequential TI demo, 60,875 B.
- Each build's `data/` contains every required archive variable; send them on a clean boot.
- `headless.json`, `cycles.log`, `pc.csv`, `frames/`, `comparison.json` retain evidence.

These remain bounded native demonstrations with compact ISO UI and reviewed fixed
inspection endpoints, not a complete tactical release. Full-game packaging and stock
original-TI-89 program-size compatibility remain separate work.

## Datasheet CPU comparison at 12 MHz

All variants use three intermediate angles plus final ISO rendering. Target-view decode
is included; grayscale, display swaps, interrupts and limiter waits are excluded:

| Variant | Complete-turn CPU time |
|---|---:|
| Existing coarse terrain low-poly (earlier audit) | 0.849-0.899 s |
| Original detailed mesh, flat control | 1.929-2.037 s |
| Original detailed mesh, affine textured | 2.482-2.662 s |

On identical detailed geometry, texture sampling and gradient preparation add
**0.553-0.625 CPU seconds per full turn**. Comparing directly with coarse low-poly also
changes geometry, depth processing and actor occlusion, so that larger difference is
not all texture cost. Flat control fills opaque geometry; textured alpha can expose
what lies behind. The first textured frame costs 0.934-1.030 CPU seconds, including decode.

Profiling attributes about 7.99 million cycles per intermediate frame to triangle
scan conversion/depth/texture work, 0.48 million to projection/clearing, 0.086 million
to expansion and 0.46 million to actors. This bounded C port favors reproducibility;
front-to-back ordering, merging coplanar geometry and reducing per-triangle setup are
possible next optimizations, not measured speedups. Do not infer TiEmu or physical
hardware elapsed time from the PC GIF cadence or these CPU-only figures.

## Limits

The sampling is affine and valid for these orthographic isometric turns, not arbitrary
perspective texturing. Half-resolution geometry/depth and 8-bit depth can cause aliasing,
edge changes and coplanar ties. UV increments too steep for signed 8.8 fall back to the
face shade; degenerate projected triangles are omitted. Fine texture detail and alpha
are quantized offline. This does not reproduce the PC's 4x4 filtered appearance exactly.
New maps require regenerating/validating the asset format and its current size bounds.

## Actual TiEmu measurements, 2026-10-17

Titanium HW3 / AMS 3.10 configured at 12 MHz; active grayscale, GrayDBuf swaps and
ordinary eight-tick/32 FPS cap. The real 256 Hz timer includes the entire native turn,
destination decode and final ISO picture. Deliberate source/destination holds and
case setup are excluded. The AMS-font decoder reads the printed digits exactly.

| Source orientation | Key | Textured turn | Worst frame |
|---:|---|---:|---:|
| 0 | F5 | 3.429 s | 1.191 s |
| 0 | F1 | 3.324 s | 1.191 s |
| 1 | F5 | 3.535 s | 1.308 s |
| 1 | F1 | 3.417 s | 1.261 s |
| 2 | F5 | 3.371 s | 1.214 s |
| 2 | F1 | 3.511 s | 1.273 s |
| 3 | F5 | 3.312 s | 1.214 s |
| 3 | F1 | 3.394 s | 1.203 s |

All eight report success. Range **3.312-3.535 s**, mean about **3.412 s** per turn,
versus the coarse low-poly audit's **1.015-1.074 s**. The same three angles are used,
but the detailed mesh and depth/occlusion work also differ from coarse low-poly.
The identical-mesh flat control above isolates the smaller texture-only CPU increment;
it has not been separately timed in TiEmu. The textured motion is visibly stepped,
not a smooth real-time renderer. Native textures are now implemented, but this port
does not make them as responsive as the retained low-poly default.

Evidence: `x/turn-texture/textured/timer-results.png`, `tiemu.json`, `headless.json`
and `full-run-tiemu.gif`. The timer binary's SHA-256 is recorded in both reports.
These are emulator measurements, not physical-calculator results.

## Real-time continuous GIF

`x/turn-texture/demo/f1-f5-textured-tiemu.gif` shows two F5 rotations followed by two
F1 rotations in the same native run, with no resets between turns. The `-4x.gif` version
enlarges the same 160x100 LCD frames. `textured-turns-and-inspection-tiemu.gif` also keeps
the subsequent direct wide/close inspection and ISO return.

The original 65-second recording is `full-run-tiemu.gif`; `capture.json` records trim
boundaries, executable/source provenance and evidence hashes. Only launch/idle tails are
removed: there are no interior cuts, inserted holds or playback acceleration. The LCD
capture rate is 20 Hz, independent of the measured rotation latency. The ordinary live
desktop emulator was untouched; recording used a private clean Titanium profile/display.

## Optimization follow-up, 2026-10-17

The TiEmu table above measures the initial mapper. The following changes have **only
datasheet CPU measurements** so far; do not multiply the old TiEmu duration by their
CPU ratios and present that as a new measured interaction time. Low-poly remains the
default, and perspective transitions remain disabled.

The first optimization accesses projected triangle vertices and edge endpoints through
pointers instead of copying structs. GCC4TI emitted calls to AMS `memcpy` for the original
corner copies. All 32 pictures remain identical to the frozen pre-optimization PC/TI
checksums. An attempted `noinline` triangle function was about 2.6% slower and was rejected.

`FFT_TURN_TEXTURE_HALF_Y` is a separate, opt-in concession. It rasterizes moving scenery
at **80x25**, then duplicates rows to the existing 80x50 color/depth buffers before
expansion and actor occlusion. Backwards expansion preserves the source rows and requires
no new bulk allocation. Units still use full-resolution sprites; scenery edges and
actor cover boundaries become coarser vertically. All textures and the detailed mesh
remain present. Final settled isometric pictures remain identical, at their usual quality.

| Native variant | Full-turn CPU range at 12 MHz | Mean CPU | Reduction from initial mapper |
|---|---:|---:|---:|
| Initial 80x50 textured mapper | 2.482-2.662 s | 2.564 s | reference |
| Pointer access, same 80x50 image | 2.361-2.531 s | 2.438 s | 4.93% |
| Pointer access + opt-in 80x25 scenery | 1.859-1.944 s | 1.890 s | 26.31% |

Each new variant matches 32 PC/native frames over eight full F1/F5 turns, keeps three
intermediate angles, preserves units/RNG and ends at the expected view. Work memory
remains 43,346 B. Timer executables are 62,185 B and 62,327 B respectively. These tests
exercise the benchmark's action-centered camera; broader camera placement, hardware
timing and interactive visual review remain before accepting the reduced-resolution option.

Reproduce with `make turn-texture-opt-bench`. The frozen initial CPU counts, picture
checksums and binary/source hashes are in `tools/turn_texture_reference.json`.
Outputs are `x/turn-opt/{pointers,half-y}/`, `comparison.json` and
`resolution-comparison.png`. The comparison sheet uses PC pictures checked against
the native TI binary, not new emulator screenshots. Neither optional reduced resolution
nor texture mapping is silently enabled in the default game.

Subsequent investigation:

- Compatible coplanar merging, conservative angle visibility and directed spans are
  now implemented and timed in [SURFACE_TURNS.md](SURFACE_TURNS.md), without removing
  decorative geometry. Primitive ordering remains unchanged. Front-to-back ordering
  and occlusion preprocessing remain untested candidates.
- Precompute affine attribute gradients for selected angles. Camera translation,
  half-pixel rounding and screen clipping require care; compare RAM/Flash cost and
  image differences before replacing runtime solves. This stores geometry commands,
  not full-screen backgrounds.
- Use fewer intermediate angles. Two instead of three reduces rendered work but
  makes larger angular jumps; no measured complete-turn duration is claimed yet.
- Expand atlas texels to bytes to remove nibble selection, trading additional Flash
  for a cheaper fetch. Texture-only CPU cost is about 0.6 s in the initial identical-mesh
  comparison, so this cannot eliminate the much larger geometry/rasterization cost.

The surface follow-up retains the lossless pointer optimization and simplifies
compatible planar geometry before sacrificing more resolution or intermediate frames.
