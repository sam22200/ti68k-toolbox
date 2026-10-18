# Textured turns at 40x100

`FFT_TURN_TEXTURE_QUARTER_X` is an opt-in moving-scenery experiment: render **40x100**
then repeat each column four times to fill 160x100. It keeps full vertical resolution,
the original textures, merged geometry, three intermediate angles and native-resolution
outlined actors. Settled isometric views remain identical. It does not enable animated
perspective travelling or automatic combat shots, or change the default turn strategy.

## Implementation and memory

The existing two 500-byte half-resolution color planes fit 40x100 exactly, as does the
4,000-byte byte-depth buffer reused from Status scratch. Projection uses `x >> 2`
and the original vertical coordinate. Actor cover samples depth at `(x >> 2, y)`;
actors are never stretched. Expanding each color byte with two applications of the
existing bit-doubling LUT produces 32 output bits. No new assembly or bulk allocation
is used. The game work allocation remains **43,346 B**.

The original edge arrays have only 58 rows and cannot hold this view's 100 scanlines.
The new mode therefore uses bytes 3000-3399 of the existing 4,000-byte background
scratch for two 100-entry signed-short edge arrays. Projected vertices occupy at most
3000 bytes (the mesh loader's 500-vertex bound; this mesh has 479). These scratch
ranges are disjoint and even-aligned. The normal background cache is invalidated
after a turn, as in the previous texture mapper. Full free RAM and nested peak stack
on a physical calculator have not been measured.

Conservative visibility is regenerated using the exact new rounding: all 150 tile
pivots, 16 angles and four horizontal anchor phases. The `fftsurfx` bank is **21,306 B**,
read in place from archive. It retains the existing 32-entry directory layout but
aliases unused second-mode entries instead of duplicating visibility-list payloads.
The original `fftsurf` bank and old shader remain available. Original UV/atlas banks
are unchanged. Generated ROM-derived assets remain local under `x/`.

## Reproduce and validate

```sh
make turn-horizontal-bench
```

This builds 80x50 and 40x100 with the same optimized surface renderer and scene/camera
cases. Each matches 32 PC/native pictures. Settled pictures match between variants;
the regenerated 80x50 binary/pictures also match the previous optimized build exactly.
Both cover eight full turns including final destination decode/render.

A broader 40x100 check covers six pivots (four map corners, street and roof), all four
horizontal phases and all directions/angles: **576 PC/native pictures match**. All
576 also match a PC control that visits every surface, verifying that the new angle
lists omit no rendered geometry. Before that check, an independent pixel oracle
checks **all 32,000 output-plane bits** of the horizontal expansion on both targets.
The sequential demo matches another **512 PC/native frames**, including four rotations,
direct wide/close inspection and return. Ordinary game/camera-cut tests pass.

Generated `native-turn.s` contains no float, `__mulsi3` or `__divsi3` helpers in the
mapper, rasterizer, gradient solver or horizontal expander. The 40x100 timer executable
is **62,921 B**; the bounded sequential demo is 61,655 B. Complete-game single-variable
packaging and original-TI-89 stock-AMS size limits remain separate release work.

## CPU measurements

Datasheet CPU only at 12 MHz, excluding grayscale, display waits and limiter:

| Moving scenery | Mean full-turn CPU | Range |
|---|---:|---:|
| Optimized 80x50 | 1.995 s | 1.914-2.074 s |
| **40x100** | **2.291 s** | **2.186-2.394 s** |

The new view uses **14.86% more CPU** in these cases. Both contain 4,000 scenery pixels;
40x100 doubles the scanline count. Per-row gradient starts, clipping and span work
cost more despite shorter horizontal runs. Profiling confirms raster work rises from
5.71M to 6.81M cycles per intermediate picture; expansion rises from 0.086M to 0.192M.
Projection and actors remain approximately unchanged. This is a different sampling
shape, not a pixel-count reduction.

The image has finer vertical edges/heights and visibly wider horizontal texture steps.
`x/turn-horizontal/comparison.png` compares the same angles, using PC pictures checked
against native rendering. Initial results do not justify choosing it for speed; its
visual tradeoff can be reviewed separately.

Outputs: `x/turn-horizontal/{80x50,40x100}/`, `comparison.json`, `comparison.png`,
the generated asset banks and `demo/fftcut.89z` with all archived files in `demo/data/`.
The experiment remains optional.

## Actual TiEmu results

Titanium HW3 / AMS 3.10, TiEmu 3.04 at configured 12 MHz, grayscale, GrayDBuf swaps
and ordinary frame limiter active. One clean isolated-profile boot; the desktop
emulator was untouched. The 256 Hz timer includes the whole turn, destination decode
and settled picture, excluding independent case setup and deliberate holds:

| Source orientation | Key | Full turn | Worst frame |
|---:|---|---:|---:|
| 0 | F5 | 2.832 s | 1.027 s |
| 0 | F1 | 2.656 s | 1.015 s |
| 1 | F5 | 2.902 s | 1.109 s |
| 1 | F1 | 2.820 s | 1.062 s |
| 2 | F5 | 2.726 s | 1.039 s |
| 2 | F1 | 2.878 s | 1.074 s |
| 3 | F5 | 2.632 s | 1.003 s |
| 3 | F1 | 2.738 s | 0.992 s |

All eight cases pass. Mean **2.773 s**, range **2.632-2.902 s**, versus the optimized
80x50's **2.379 s mean / 2.281-2.480 s range**. This is about **16.6% more elapsed
time**, agreeing with the CPU comparison's direction. The 80x50 executable rebuilt
byte-for-byte identically, so its earlier measured native baseline remains applicable.

`40x100/tiemu.json`, `timer-results.png` and `full-run-tiemu.gif` retain the evidence;
`tiemu-comparison.json` includes both timer reports. Timer binary SHA-256:
`3f550e8faac1aa00b80f6b70e5829b9c98b3bdf40ee58b287ef0b3292d27007e`.
These are emulator durations, not physical-calculator timing or GIF-derived FPS.
The experiment is useful for comparing image readability, and does not improve speed
over the retained 80x50 renderer.

`40x100/horizontal-turn-tiemu-4x.gif` shows the first independent F5 case from the
actual LCD recording. Its 5.2-second clip includes the source/destination holds;
the timer measures 2.832 seconds for that turn. Only launch/idle tails were trimmed;
no interior cuts, inserted holds or speed changes. `capture.json` records the original
recording hash and trim boundaries. `full-run-tiemu.gif` includes all eight cases and
their setup resets, rather than a continuous playthrough.

The PC camera/expansion oracle also passes AddressSanitizer (leak detection disabled
for runtime allocations kept until process exit).
