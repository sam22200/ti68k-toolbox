# Ramza shoulder-camera exploration

The original three-mode comparison below is retained as the baseline experiment. For the
closer, native-resolution fixed view with F5 reverse and cached actor visibility, see
[Fixed shoulder refinement](FIXED_SHOULDER.md) and build `make fixed`.

Branch: `explore/fft-shoulder-perspective`. Optional inspection of the existing FFT battle,
built as `fftp`. The ordinary `fft` build retains its original renderer, animated camera
turns and saved-state layout.

`fftp()` starts immediately at Ramza's first turn, without the objective banner or initial
AI turns. All actors retain their original idle animations. Move, attacks and AI remain
available in the tactical view. The quick entry leaves everyone on their starting tiles;
it does not silently execute the opening AI actions.

## Cameras and controls

| Mode | Camera controls | Tactical changes |
|---|---|---|
| 1: fixed shoulder | Fixed behind Ramza, looking along his facing | None |
| 2: lateral shoulder | LEFT/RIGHT slide ±24 PS1 world units; ALPHA + LEFT/RIGHT look ±45 degrees | None |
| 3: orbit | LEFT/RIGHT orbit 360 degrees; UP/DOWN pitch; F1/F5 change distance | None |

The camera inspects the battlefield; Ramza does not gain a free movement action. Pitch
ranges from 28 degrees upward to 67.5 downward, with 64 quantized yaw angles. This is an
orbit camera, without camera-wall collision or a free character controller.

- In tactics, DIAMOND enters inspection (PC: C).
- MODE cycles the three modes (PC: M). ENTER resets the current camera.
- SHIFT, ESC or DIAMOND returns to tactics.
- `fftp(61)` starts on Ramza's tactical menu; `fftp(62)`, `(63)`, `(64)` enter cameras 1, 2,
  3 immediately. All four use identical seeds/equipment.
- The standard build also accepts `fft(61)` for direct Ramza control.

## Build and reproduce

With the local FFT disc and existing asset banks installed, from the repository root:

```sh
make -C games/fft test xcheck
make -C games/fft perspective perspective-check
make -C games/fft perspective-xcheck
(cd games/fft && ./fftp_pc --scenario 64)
make -C games/fft perspective-gifs
```

Run the PC game **from `games/fft`**, for example `./fftp_pc --scenario 64`, so its archived
asset counterparts can be found. The exploration build removes historical test doors
1–60 and the intermediate polygon frames of isometric 90-degree turns to fit one TI
program variable. The four textured isometric views and actor/walk/attack animations stay.
The standard build retains those test doors and animated turns; original-game oracle
tests run against that build. `test_perspective.c` tests the exploration build.

On the Titanium, send `fftp.89z` plus **archived** `fftmesh.89y`, `fftu.89y`, `fftw.89y`,
`ffti.89y`, `fftv0.89y` through `fftv3.89y`, then run `fftp()`. Use `tools/bin/ti-run` for
a clean emulator restart. `fftpc.89z` and `fftdumpc.89z` are headless measurement programs;
do not launch them on the calculator.

The plain program exceeds stock original TI-89 AMS 2.xx's 24,576-byte limit. This delivery
targets the Titanium, as does the current main FFT build; an original HW2 release needs
a packed launcher and separate RAM/launch checks. No HW2 launch or physical-calculator
FPS is claimed here.

## Shared renderer and geometry

`tools/perspective_assets.py` reuses the local-disc reader and MAP022 decoder. It exports
the **actual original mesh**, not a warped isometric image or a height-grid replacement:
479 vertices, 866 triangles and 75 supporting planes. All 300 triangles with a nonzero
vertical normal point upward in the decoded convention; the original winding permits
plane-based backface rejection. Flat shades sample the original UV texture; arbitrary
runtime texture mapping is not implemented. Surface detail is lower than in tactics.

One fixed-point renderer serves all cameras:

- Original PS1 units: tile 28, height unit 12. Actor feet use existing terrain half-heights.
  Tactical terrain, reach, positions, CT, HP and RNG are not rewritten by inspection.
- Offline `ti-table` sine/reciprocal tables; existing verified `muls16`/`divs32_16` wrappers.
  No new assembly routine.
- Vertex projection/outcode cache, world-plane backface rejection, five-plane frustum
  clipping including the near plane, scanline fills and interpolated reciprocal depth.
- 80×50 geometry doubled to 160×100. A 4,000-byte, 8-bit depth array provides terrain/sprite
  occlusion. There is no full-resolution 16-bit Z-buffer.
- Original masked, outlined FFT sprites and idle selection; bounded per-unit scale caches;
  far-to-near billboards. Depth is approximated at their feet, so steep pitch/contact cases
  can show occlusion errors. Perspective water immersion is not implemented.
- Geometry cached while the camera stands still; the final framebuffer cached when the
  selected animation frames also stay unchanged. Camera changes still redraw the view.

Scratch aliases reuse the existing unpacked-view, Status and scene-cache buffers: **no
additional heap block**. The unpacked-view cache is invalidated before returning to
tactics. Scale-cache pointers are also 8-byte aligned on LP64 PC builds. The new mesh bank
is 12,522 bytes, including 2,112 table bytes, read in place from archive. No extra room
backgrounds are stored. Original-disc banks and generated images remain local/ignored.

The TI compiler reports 43,346 bytes for the existing Work block: scene+Work 60,786 bytes
and unpacked-view scratch 52,320 bytes, 113,106 game heap bytes in both renderers (runtime
planes, stack and program are separate). A per-unit scale-cache slot is 630 TI bytes,
stored inside that reused scratch block. Generated `-Os` perspective assembly contains
no 32-bit multiply/divide helpers or floating-point helpers; the older tactical/stat code
still has its existing wide arithmetic.

## Measurements and GIFs

`make perspective-gifs` creates `x/perspective/`:

- `fixed.gif`, `lateral.gif`, `orbit.gif`: actual 68000 framebuffer dumps, each paced from
  its own measured render cycles.
- `comparison.gif`: identical input frames side by side, **uniform 8 FPS for visual
  comparison only**, not a speed demonstration.
- `frames.csv`: per-frame stage cycles, checksum and display delay.
- `manifest.json`: binary hash, timing assumptions and aggregates.

The script has 312 frames per mode: idle, lateral movement/yaw, pitch and a complete orbit.
Every one of the 936 TI source frames is checked against portable C. Individual timing is
`max(1/32, (render_cycles + 600)/(12 MHz × 0.91))`, allowing 9% CPU for grayscale and 600
cycles for update. Startup is excluded. First-view construction is included in the GIF,
but reported separately from steady averages. These are **datasheet MC68000 emulation
results**, with estimated AMS calls, not TiEmu timing or physical hardware. GIF delays
round to centiseconds. Slow rendering also slows the animation clock in this prototype.

Final stage costs and aggregates: `PERSPECTIVE_BENCHMARK.csv`.

Two optimizations were measured:

1. Plane backface rejection reduced first fixed-view construction from about 1.12 s to
   0.77 s, retaining the original mesh and camera path.
2. A conservative 8×8 minimum-depth grid and hidden-sprite rejection preserved all 936
   checksums but slowed this path: fixed 9.06 → 8.74 FPS, lateral-moving 1.34 → 1.25,
   orbit-moving 1.50 → 1.39. It was removed: block construction/bookkeeping was not repaid.

**Recommendation: fixed shoulder inspection** for the current TI implementation. Lateral
inspection can provide occasional discrete viewpoints; continuous orbit remains a geometry
research tool at this detail level. Reduce/merge the original mesh and restrict visible
regions before adding effects. A Mode 7 floor cannot replace the buildings and sloped roofs.

## Validation

Tests cover direct Ramza entry, live idle poses, tactical state/RNG conservation, lateral
and yaw limits, a full orbit, pitch/zoom limits, cached versus rebuilt framebuffers, and
return to the normal Move command. Standard FFT tests and the 60 existing 40-frame PC/TI
cross-check scenarios remain required.
`perspective-xcheck` additionally checks all three starting cameras through mode switches,
return to tactics and an animated three-tile Move using `keys/perspective-return.txt`.

AddressSanitizer and UndefinedBehaviorSanitizer pass the perspective tests with alignment
instrumentation disabled: existing isometric routines use 32-bit loads from 16-bit-aligned
plane rows. A stack-lifetime bug exposed on returning to Move was fixed: temporary inverted
cursor rows now live through the subsequent blit. PC/TI checks cover that return path.

A final private TiEmu Titanium / AMS 3.10 run loaded the normal exploration executable
with all eight archived banks. Mode switching, lateral/orbit/pitch inputs and returning
to tactics were exercised. Review: `x/perspective/emulator/titanium.png`. The private
emulator/display were stopped afterward; the user's desktop was not driven.
