# Native textured rotation: merged surfaces and angle visibility

This follow-up keeps the three F1/F5 intermediate angles, the original textures,
the outlined native unit sprites and the settled isometric views. It optimizes the
native integer mapper; it does not use PC texture code, prerecorded angle images
or new 68000 assembly. Default low-poly turns and direct fixed-perspective cuts
keep their existing policy. The optimized texture build is available for review.

## Geometry and visibility

`tools/turn_surface_assets.py` processes the existing local MAP022 mesh and quantized
UV bank. Consecutive triangles merge only when they share a reversed edge, have the
same shade and texture group (including water treatment), form an exact coplanar
parallelogram and have matching shared UVs and affine fourth-corner UVs. No building,
ledge, actor or texture is removed. There are **357 compatible pairs**, giving
**509 candidate surfaces instead of 866 triangles**. Screen rounding that collapses
or breaks convexity falls back to the original pair at runtime.

The generated `fftsurf` bank also carries conservative facing lists. Generation
enumerates the exact native projection over every one of the map's **150 tile
pivots**, all 16 quarter-table angles and every screen-anchor rounding phase for
80x50 and 80x25. A surface is retained if either original triangle can face the
camera in any enumerated case. This is facing rejection, not a precomputed occlusion
solution. Screen bounds and depth tests still run; primitive order is preserved to
retain alpha and equal-depth ownership in the visibility-only control.

Convex contour direction selects the left or right scanline edge directly, avoiding
row clearing and per-edge min/max updates. Triangles are convex; merged quads must
pass all four projected corner tests before this path runs. Native UV/depth gradients,
alpha sampling and the existing byte depth buffer remain. Inner pixel loops still
contain no multiply, division or floating-point operation.

The bank is **31,270 bytes**: 144-byte header/directories, 509 records of 22 bytes
and ordered face lists. It is archived and read in place on TI. The 43,346-byte game
work allocation, 4,000-byte reused depth and 2,874-byte reused projection buffers
remain unchanged. A persistent bank pointer and transient stack are additional;
total physical RAM and peak nested stack are not measured. Current loader bounds
and counts deliberately target this MAP022 bank; regenerate and validate for new maps.

## Reproduction and checks

```sh
make turn-surface-bench
```

This generates local-only ROM-derived assets and builds four controls:

- `reference`: current pointer-based 80x50 triangle renderer.
- `visibility`: original triangles, conservative angle lists only.
- `merged-spans`: merged surfaces, angle lists and directed spans at 80x50.
- `merged-half-y`: the same renderer with optional 80x25 moving scenery.

Each has eight full F1/F5 cases and 32 PC/native screen comparisons. Visibility is
pixel-identical to the reference. Settled views, units, battle RNG and camera policy
are unchanged. A separate driver covers six pivots (four map corners, street and
roof), four anchor phases and all directions/angles: **576 pictures per optimized
variant match the native binary**. All 576 visibility pictures also match the PC
reference. The sequential demo matches another **512 PC/native frames**, covering
four consecutive rotations, direct wide/close inspection and return.

The merged mapper shares one affine plane across each compatible quad. Rounding in
the original triangle interpolation means it is not pixel-identical: the 24 central
moving pictures differ by 16-499 pixels out of 16,000 (at most 3.12%). Differences
include texture seams, quantized depth and actor cover. Final settled pictures are
identical. The optional half-height variant differs by up to 3,783 pixels (23.65%);
its coarser vertical geometry/cover is a visible concession and is not enabled by
the primary optimized build.

`x/turn-surface/comparison.png` presents the reference, optimized 80x50 and optional
80x25 pictures. These are PC pictures whose checksums match native TI rendering;
they are not emulator captures. Ordinary game and camera-cut regressions pass.
Generated `optimized-turn.s` was inspected: the mapper contains no float or
`__mulsi3`/`__divsi3` helpers. A small direct-mapped gradient-cache experiment was
slower and removed; an earlier `noinline` experiment was also rejected.

## Datasheet CPU results at 12 MHz

Eight complete turns, three intermediate pictures plus destination decode/render.
Grayscale interrupts, display synchronization and limiter waits are excluded:

| Variant | Mean CPU | Range | Reduction from initial texture port |
|---|---:|---:|---:|
| Initial texture port (historical frozen measurement) | 2.564 s | 2.482-2.662 s | reference |
| Current pointer-based reference | 2.437 s | 2.360-2.530 s | 4.98% |
| Conservative angle lists only | 2.399 s | 2.316-2.493 s | 6.45% |
| Merged surfaces + directed spans, 80x50 | **1.995 s** | **1.914-2.074 s** | **22.21%** |
| Optional 80x25 moving scenery | **1.571 s** | **1.532-1.620 s** | **38.72%** |

Against the current pointer reference, the optimized full-quality variant saves
18.14% CPU, and the optional half-height variant saves 35.51%. This does not halve
the total rotation latency: preprocessing/final view rendering, sprite work and
display waits also remain. Do not convert these CPU ratios into claimed TiEmu times.

Timer executables are 62,715 B (80x50) and 62,863 B (80x25). The sequential demo is
61,459 B. Complete-game packaging beyond the single-variable limit remains separate;
these are bounded native demonstrations, not an original-TI-89 stock-AMS release.

## Build outputs and feature flags

Under `x/turn-surface/`:

- `assets/`: atlas, UVs, surface bank and generation provenance.
- `{reference,visibility,merged-spans,merged-half-y}/`: `fftturn.89z` timer suite,
  `fftturnc.89z` datasheet companion, `data/`, per-case evidence and 32 pictures.
- Optimized directories also contain `fftscheck.89z` and 576-picture camera parity logs.
- `demo/fftcut.89z` and `demo/data/`: continuous four-turn/direct-inspection sequence.
- `comparison.json`: measured costs, image differences, binary/source hashes.

All data variables must be sent archived on a clean boot. The texture mapper requires
`FFT_TURN_TEXTURE`; the optimization bundle is `FFT_TURN_TEXTURE_SURFACES`,
`FFT_TURN_TEXTURE_MERGE` and `FFT_TURN_TEXTURE_SPANS`. `FFT_TURN_TEXTURE_HALF_Y` is
independent and optional. Omitting the surface/merge flags retains the original
triangle baseline; visibility alone is a separate lossless control. Surface assets
are required by the optimized build, and the harness rejects a missing bank rather
than accepting a silent low-poly fallback.

## Actual TiEmu 80x50 timing

Titanium HW3, AMS 3.10, TiEmu 3.04 configured at 12 MHz, grayscale/GrayDBuf and
the ordinary frame limiter enabled. The 256 Hz timer includes the three intermediate
pictures, destination decode and final settled picture. Setup and explicit holds
remain outside the interval, as in the initial native audit.

| Source orientation | Key | Full turn | Worst frame |
|---:|---|---:|---:|
| 0 | F5 | 2.410 s | 0.863 s |
| 0 | F1 | 2.292 s | 0.863 s |
| 1 | F5 | 2.480 s | 0.945 s |
| 1 | F1 | 2.398 s | 0.910 s |
| 2 | F5 | 2.351 s | 0.886 s |
| 2 | F1 | 2.457 s | 0.921 s |
| 3 | F5 | 2.281 s | 0.863 s |
| 3 | F1 | 2.363 s | 0.851 s |

All cases pass. Mean **2.379 s**, range **2.281-2.480 s**, versus the first native
texture port's 3.412 s mean / 3.312-3.535 s range: **30.27% less elapsed time**.
Worst individual frames remain 0.851-0.945 s, so three angles still look stepped.
This is actual emulator timing, not physical calculator timing or measured real-time FPS.

Evidence: `merged-spans/tiemu.json`, `timer-results.png`, `full-run-tiemu.gif`,
binary SHA-256 `da8b7b3ca33069ec52a34a0973dd358bed14eb313ce74eea206e419f64c9c972`.
The emulator used a private Xvfb/profile; the live desktop instance was untouched.

The optional 80x25 timer suite also passes all eight cases on a separate clean boot:

| Source orientation | Key | Full turn | Worst frame |
|---:|---|---:|---:|
| 0 | F5 | 1.906 s | 0.699 s |
| 0 | F1 | 1.859 s | 0.699 s |
| 1 | F5 | 1.964 s | 0.757 s |
| 1 | F1 | 1.894 s | 0.722 s |
| 2 | F5 | 1.871 s | 0.710 s |
| 2 | F1 | 1.941 s | 0.746 s |
| 3 | F5 | 1.847 s | 0.699 s |
| 3 | F1 | 1.882 s | 0.699 s |

Mean **1.896 s**, range **1.847-1.964 s**, about **44.44% less elapsed time** than
the initial texture port. Evidence is in `merged-half-y/tiemu.json`,
`timer-results.png` and `full-run-tiemu.gif`; timer binary SHA-256
`0ed5c3dda1e6aead76bc7096a45525e69ad3e5caecb4769c2ba7f9bc955c3f43`.
This trades visible vertical detail for speed. The recommended review build remains
80x50, keeping the stronger image quality with an approximately one-second turn saving.

`merged-spans/optimized-turn-tiemu-4x.gif` shows the first F5 benchmark case at its
actual cadence (one turn, fourfold nearest-neighbor enlargement). Its 4.1-second
clip includes source/destination holds; the measured turn itself is 2.410 seconds.
`capture.json` records source hash and trim boundaries. Only launch/idle tails were
removed; there are no interior cuts, added holds or speed changes. The full recordings
contain independent benchmark cases with setup resets, not a continuous playthrough.
