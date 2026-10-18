# Scenery textures on perspective geometry: PC review

Update: [textured grey water and smooth camera modes](PERSPECTIVE_MOTION.md) supersede
the original canal conversion below. Current water retains its texture in light/dark grey
and the C white glints; dry-surface contrast does not affect it. Counts below are historical.

This experiment applies the actual Gariland ground, wall and roof artwork used by the
existing isometric TI asset pipeline. Character sprites remain the current TI-style bank;
the preceding original-character experiment was not what the user meant by scenery sprites.

The isometric scenery is stored as composed, compressed views (`fftv0..3`), rather than a
collection of independent reusable tile sprites. Its source is MAP022's texture atlas,
palettes and polygon UV coordinates. This tool reads that same local-disc source, verifies
all 866 triangles against the indexed perspective mesh, and reproduces the isometric
extractor's four-grey luminance thresholds. It does not warp an isometric screenshot.

## Reproduction and output

Run `make fixed-scenery-textures` in `games/fft`. Local disc and existing extracted mesh
bank are required. Outputs stay ignored under `x/scenery-textures/`:

- `flat-textured.gif` and `.png`: current flat scene on the left, actual scenery textures
  on the right, with close/front, close/back, wide/front and wide/back camera rows.
- `nearest-filtered.gif` and `.png`: one texture sample per pixel versus 4x4 texture
  supersampling. Filtering averages source luminance before four-grey quantization.
- `<view>-textures.gif`, `<view>-nearest.gif`: individual native 160x100 animations.
- `manifest-pc.json`, per-view metadata, exact winning face labels, depth and frame dumps.

Playback at 10 FPS is for review, not a rendering speed measurement.

## Rendering and limits

The `FFT_TEXTURE_REVIEW` diagnostic records actual winning face IDs and architectural edge
pixels during the C renderer's cold geometry pass. It is explicitly rejected on 68000
builds. The offline Python mapper intersects camera rays with those winning triangles,
then interpolates original UVs at the world-space intersection, giving perspective-correct
sampling. It retains the exact C coverage, including automatic cutaway gaps and dotted edges.

The mapped four-grey background is replayed through the C renderer's existing background
cache. Original TI-style actors, cached pose composition, terrain depth masks, white
outlines, HUD and canal glint animation are consequently drawn by the same C code.
Textures cover water as part of the original scenery; glints retain the old cached positions.

Opaque geometry and its depth are retained. Fully transparent source texels keep the flat
surface underneath rather than exposing geometry behind it. This affects 41 close/front
and 32 wide/front pixels with filtering, and zero in the opposite views. The one-sample
variant falls back on 135/117 pixels respectively. This review does not add alpha-tested
scenery visibility to the calculator renderer.

All texture mapping/filtering happens offline on PC. No runtime texture path, new production
asset bank, TI build, ti-cycles, xcheck or TiEmu test is introduced. Calculator time, storage
and memory costs remain unmeasured; these GIFs establish the visual choice only.

## Checks and visual assessment

All 866 source triangle/UV associations match the current indexed bank. The diagnostic's
128 flat camera frames exactly match the previous PC visibility-review frames. Another
256 textured C replay frames cover both sampling modes in four cameras. Four-grey plane
packing round-trips exactly; unfilled sky/cutaway pixels remain unchanged, architectural
edges stay black and exported camera transforms remain identical on replay. Existing fixed
camera and cutaway PC tests also pass without the diagnostic flag.

The original façades, roof tiles and paving make the environment more recognizable, with
substantially more texture contrast than the flat materials. The wide view becomes busy;
filtering reduces distant speckling while preserving nearby details. Both variants are
provided for user review before deciding whether to integrate calculator-side texturing.

## Contrast variants (PC only)

Run `make fixed-texture-contrast` for `x/scenery-contrast/three-contrasts.gif` (current,
contrast-plus and contrast-strong columns) and `before-after.gif` (current / contrast-plus).
The four camera rows and native individual-view GIFs retain the same actors and geometry.
These are rendered from original filtered luminance, not from recolouring the four-tone
screenshots. Each texture's conversion compresses the middle-grey interval around the
existing middle threshold, making more highlights white and more shadows black:

| Profile | Middle-grey interval width | Purpose |
|---|---:|---|
| current | 100% | Preserve the previous texture review |
| contrast-plus | 65% | More separation while retaining some texture detail |
| contrast-strong | 35% | Show the limit: large black regions lose roof/wall detail |

The luminance-to-four-grey choice is made offline. Keeping one chosen variant requires
the same two-bit format and the same 6,000-byte cached background as before. The replay
uses the identical C binary and drawing path; no runtime contrast pass is introduced.
This does not establish the cost of calculator-side texture mapping, which is still
unimplemented and unmeasured. No TI validation was run.

Checks cover the exact unchanged baseline, byte-for-byte equal background sizes, plane
packing round trips, preservation of sky/cutaway gaps and edges, unchanged camera transforms,
and a nondecreasing number of black/white texture pixels. The two new profiles have 256
PC replay frames. GIF playback remains a uniform 10 FPS for review only. Visually, the
65% profile is the better compromise; the 35% profile makes close-view roofs too dark.
