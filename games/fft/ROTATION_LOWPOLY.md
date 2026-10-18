# Native low-poly F1/F5 rotation

Current follow-up: [live height contours](TURN_EDGES.md) improve relief readability
in the native 160x100 path, adding 125.269 ms mean datasheet CPU per turn (+11.55%),
552 program bytes and no RAM/assets. The historical timings below predate this change.

This remains the default. A later opt-in [native affine-textured turn port](NATIVE_TEXTURED_TURNS.md)
now exists and measures 3.312-3.535 s in TiEmu; the baseline below predates that mapper.
A later [comparison against selected 80x25 textures](TURN_COMPARISON.md) adds an
explicit `FFT_TURN_LOWPOLY_NATIVE` variant rasterized directly at 160x100. Its
1.273-1.355 s TiEmu results are distinct from the historical 80x50 figures below.

The user clarified that removing perspective travelling must **not** remove isometric
turning animation. The fixed exploration build now restores the existing native terrain
rotation, including the bounded TI review executable. The earlier hold/decode/switch
GIF showed a real implementation gap, not a GIF capture error.

F5 rotates right, F1 left. Each turn renders three intermediate angles (22.5, 45 and
67.5 degrees) at 80x50, doubles them onto the LCD and then displays the textured target
orientation. It reuses `draw_turn`, its fixed sine table, counting sort, word-span fills
and actual outlined TI unit sprites. No new assembly or texture mapper was added.

This is intentionally coarse transition geometry: the tactical height grid supplies
flat tops and side faces with mean material shades, rather than the detailed textured
room mesh. Actors appear above the terrain without exact scenery occlusion during
turning. The settled view restores the original scenery, contrast/height contours and
depth-aware actors. Collision, target selection and tactical rules do not change.

Close/wide inspection still enters, switches preset and exits directly. Automatic
perspective attack shots stay disabled. The separate `FFT_CAMERA_MOTION` flag enables
historical perspective experiments; it is not needed for native F1/F5 animation.
`FFT_TURN_HOLD` exists only to reproduce the rejected old hold/switch benchmarks.

## Reproduce

```sh
make camera-cuts-check
make turn-audit
make camera-cuts-demo
```

The common turn driver feeds actual F1/F5 inputs through the original game update and
renderer. Its eight cases cover both keys from all four orientations. All **32 native
TI framebuffers match the PC oracle**, every case produces three distinct intermediate
pictures, ends at the correct orientation, preserves units/battle RNG and stays out of
perspective/cinematic state. The 512-frame inspection demo also matches PC/TI, including
F5 twice, F1 twice, direct wide/close and ISO return. Ordinary game and camera-cut tests
pass; the old polygon rotation of the standard ISO game is unchanged.

`x/turn-audit/fftturn.89z` is an automatic timer/capture program with the bounded compact
ISO UI, not the complete tactical release. Its 59,235-byte executable uses existing
archived banks from `x/turn-audit/data/`. Run `fftturn()` after a completed clean transfer.
Grayscale, GrayDBuf and the ordinary 32 FPS cap remain active. A one-second source hold
and an animated destination hold surround each case outside the measured interval.
The final table shows total and worst-frame milliseconds from the 256 Hz timer.

`x/camera-cuts-poly/fftcut.89z` is the 57,991-byte sequential demonstration. The old
hold/switch binary and GIFs under `x/camera-cuts/` are preserved separately.

## CPU counts and memory

The datasheet 68000 audit at 12 MHz counts **0.849-0.899 CPU seconds per complete turn**,
including target-view decode and final ISO rendering. Worst frame: 0.374-0.408 CPU
seconds; the first frame also pays for decoding the destination. These exclude display,
interrupts and limiter waits; they are not physical hardware FPS or GIF timing.

Restoring the existing buffers adds 2,748 bytes of persistent fixed-view work RAM:
232 bytes of span bounds, 1,000 of half-resolution planes, 512 of expansion lookup and
1,004 of grid/sort scratch. They live in the existing heap work allocation. The work
struct is 43,346 bytes on TI. No new asset bank, per-angle image or depth buffer is used.
The sequential demo executable grows by 2,794 bytes versus the old cut-only demo.
Transient stack usage has not been profiled. Full-game packaging remains a separate
existing blocker; this is not a stock original-TI-89 release claim.

CPU evidence and binary provenance: `x/turn-audit/headless.json`, `cycles.log`, `pc.csv`
and `frames/`. `tools/turn_audit.py --read-tiemu` decodes the actual printed digits
against the local AMS font after capturing `x/turn-audit/timer-results.png`.

## Actual TiEmu timings, 2026-10-16

Titanium HW3 / AMS 3.10 configured at 12 MHz. The 256 Hz timer runs with grayscale,
GrayDBuf swaps and the ordinary 32 FPS cap. All eight cases pass on the actual emulator:

| Source orientation | Key | Complete turn | Worst frame |
|---:|---|---:|---:|
| 0 | F5 | 1.027 s | 0.417 s |
| 0 | F1 | 1.062 s | 0.417 s |
| 1 | F5 | 1.074 s | 0.441 s |
| 1 | F1 | 1.027 s | 0.417 s |
| 2 | F5 | 1.015 s | 0.406 s |
| 2 | F1 | 1.062 s | 0.433 s |
| 3 | F5 | 1.050 s | 0.429 s |
| 3 | F1 | 1.027 s | 0.417 s |

The mean is about 1.043 s per full turn. Three coarse images are rendered before the
textured destination; this is a stepped animation, not a claim of 30 FPS motion. The
earlier ~0.4-second result measured a **different, rejected hold/switch path without
intermediate pictures**. Native moving textures are still unimplemented, so no textured
rotation timing comparison is claimed.

Evidence: `x/turn-audit/timer-results.png`, `tiemu.json`, `full-run-tiemu.gif` and
`headless.json`. Digits are matched exactly to the AMS font, not guessed from OCR.
Setup and deliberate source/destination holds are outside the timed interaction;
target decode and final rendering are inside. These are emulator results, not physical
calculator measurements. TiEmu can underestimate some MC68000 instruction timings;
the separate datasheet audit above remains necessary.

## Continuous demonstration GIF

`x/camera-cuts-poly/f1-f5-lowpoly-tiemu.gif` shows two F5 turns followed by two F1
turns in one uninterrupted native sequence; `-4x.gif` enlarges those same LCD frames.
`rotation-and-direct-views-tiemu.gif` also retains direct wide/close inspection and ISO
return. These clips come from a separate clean TiEmu run of the sequential demo, using
the same native turn code as the timer audit. They do not splice together the eight
independent benchmark cases.

The full 40-second capture is `full-run-tiemu.gif`. Only launch and idle tails are
trimmed; there are no interior cuts, acceleration or inserted holds. `capture.json`
records binary/evidence hashes and trim boundaries. The LCD was sampled at 20 Hz,
which is a capture setting rather than a rendering performance claim.
