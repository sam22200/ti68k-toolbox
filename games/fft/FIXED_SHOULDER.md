# Refined close shoulder view

Latest camera refinement: [elevated overview and selective cutaway](PERSPECTIVE_CAMERA.md).

The latest water animation, MODE zoom control and original-game placement audit are in
[`PERSPECTIVE_WATER.md`](PERSPECTIVE_WATER.md). The measurements below describe the earlier
close/reverse build before those additions; its local GIFs are preserved.

Build: `make fixed fixed-check fixed-xcheck` from `games/fft`. Run `fftfix(62)` on the
Titanium, or `./fftfix_pc --scenario 62` from this directory. `fftfix()` starts at Ramza's
tactical menu; DIAMOND enters inspection. The original three-mode `fftp` experiment is
retained separately. This refinement uses the same game, units, world coordinates and
original MAP022 mesh.

## Presentation

- Camera distance 40 original world units instead of 56, shoulder offset 18 instead of 12,
  pitch 5.625 degrees downward instead of 16.875. Ramza fills the lower-left foreground.
- F5 switches to the opposite side of Ramza and looks back across the map; his tactical
  facing is unchanged. F5 again returns. ENTER/F1 selects the forward view. MODE now toggles
  distance 40/80 (close/wide), rebuilding both direction caches. Arrows do not
  move the camera or character. SHIFT/ESC/DIAMOND returns to tactics.
- Native 160×100 geometry and sprites instead of an enlarged 80×50 image. Bright streets,
  light/dark directional walls and dark sloped roofs separate the major surfaces.
- 451 architectural creases/roof ledges are extracted offline from mesh adjacency.
  Coplanar triangle diagonals are omitted. Depth-tested black edges clarify rooflines,
  stairs and retaining walls. Original outlined actor art and idle poses are retained.

The new archive format is version 3, 15,194 bytes: the old indexed mesh and texture-mean
shades, plus edges and fixed-view material shades. The original three-mode renderer still
uses the old shades and accepts the extended format. Assets and framebuffer captures
remain local/ignored because they derive from the user's disc.

## Why this renderer rather than a Mode 7 floor or height raycaster?

The previous fixed-view profile spent about 6,706 cycles retaining the background versus
944,466 drawing/scaling sprites, plus 132,582 expanding the half-resolution image. Replacing
the cached background with a floor caster could not remove that dominant sprite cost.
The original geometry also already represents the sloped roofs and true terrain heights.

This implementation exploits fixed visibility instead:

1. Render each direction once with the existing fixed-point polygon pipeline, now at native
   resolution, using a temporary 16,000-byte reciprocal-depth array.
2. Turn that depth into byte-aligned visibility masks for the actors' screen rectangles.
   The measured two-view masks occupy only 1,582 bytes. Billboard depth remains the feet
   approximation used by the previous prototype.
3. Lazily scale each original idle pose, combine its transparency with visibility once,
   and cache packed mask/light/dark bytes. The bounded 24,000-byte pool resets safely when
   full; no growing allocations or generated executable code are used.
4. Each animation frame copies the cached background and composites cached masked bytes.
   There is no per-pixel depth test, projection or runtime scaling on a pose-cache hit.

This uses precalculated Z visibility; it does **not** incorporate the separately licensed
FAT SDK, and no measured claim is made for an unimplemented Mode 7 or FAT replacement.
The warmed geometry lookup is 290 cycles per frame; further floor optimization would
have negligible effect on that path. Cold view construction is the remaining visible stall.

Both backgrounds are temporary RAM images computed on entry, not archived per-room
background assets. They reuse 12,000 bytes of the tactical scene allocation. The unpacked
view's existing 52,320 bytes hold the 24,000-byte pose pool, 16,000-byte temporary depth
and 12,320-byte mask pool. Metadata reuses the Status buffer, with a compile-time size
check. No additional heap block is allocated. The tactical scene/view caches are marked
stale before returning, and rebuilt through the existing path.

## Reproduce the GIFs and timings

```sh
make -C games/fft fixed fixed-check fixed-xcheck
make -C games/fft fixed-gifs
```

`fixed-gifs` builds `fftfcapc.89z` for `ti-cycles`, captures 320 frames with four F5 changes,
and compares every framebuffer checksum to portable C. Outputs:

- `x/fixed-shoulder/close.gif`: actual TI-binary framebuffers at their estimated cadence.
- `x/fixed-shoulder/frames.csv`, `manifest.json`: stage cycles, checksums, cache occupancy,
  binary hashes and timing assumptions.
- `before-after.gif` / `.png`: optional comparison with the preserved local prototype
  (`x/fixed-before/fftdumpc.89z` and its old mesh bank). Without that local baseline, the
  script still produces the current demo. The comparison uses synchronized 8 FPS for
  visual assessment only; the separate GIFs retain their cycle-based cadence.

Results at 12 MHz, with 9% reserved for grayscale and a 32 FPS limiter:

| Measurement | Previous fixed view | Refined fixed view |
|---|---:|---:|
| Resolution | 80×50 enlarged | 160×100 native |
| Warmed animation | about 9.05 FPS | 32 FPS (limiter) |
| Demo including first construction / cache misses | 8.27 FPS, 64 frames | 23.54 FPS, 320 frames |
| First forward frame | 772 ms | 1,434 ms |
| First reverse frame | unavailable | 1,137 ms |
| Later reverse/forward switches, warmed | unavailable | 31.25 ms |
| Warmed render cycles | 1,205,576 | 194,608 |

The two whole-demo averages use different sequences and lengths; they are not a matched
continuous-motion benchmark. The cache reached 23,730/24,000 pose bytes, 80 pose misses and
zero pool resets. The first return to the forward direction needed another pose (95.82 ms);
subsequent switches met the frame limiter. Program: 64,607 transfer bytes (64,517 payload).
Stock original TI-89 AMS's 24,576-byte limit still needs a separate packed-release effort.

These are MC68000 datasheet-emulator cycle estimates with estimated AMS calls, not physical
hardware FPS or TiEmu wall-clock timing. GIF delays round to centiseconds. Startup and bank
transfer precede the captured frames. Do not run `fftfixc` or `fftfcapc` on the calculator.

## Checks

`test_fixed.c` checks closer framing, both directions, ignored arrows, tactical state/RNG
conservation, cached versus rebuilt images in 24 cases over four facings and street/roof
anchors, and returning to Move. Address/undefined-behavior sanitizers pass (alignment
instrumentation excluded for the pre-existing isometric word-aligned framebuffer casts).
The 110-frame PC/TI replay checks flipping, reset, returning to tactics and a three-tile
walk. The original three-camera tests and their three return replays also still pass.

384 comparison/demo source frames match PC/TI checksums. Profiling detail is recorded in
`FIXED_BENCHMARK.csv`. No new assembly or hardware I/O path was added.

The normal `fftfix.89z` was also launched on a private TiEmu Titanium/AMS 3.10 profile
with all eight banks archived. Forward rendering, F5 in both directions and SHIFT were
exercised. The single LCD review is `x/fixed-shoulder/titanium.png`; the private emulator
and Xvfb display were stopped afterward.
