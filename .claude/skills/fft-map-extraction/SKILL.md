---
name: fft-map-extraction
description: Extract and adapt Final Fantasy Tactics PS1 battle maps to the TI project's four-grey scenery, using the approved C2 material and prop contours. Use for new arena views, map framing, source-color references and chapter map banks; combat rules and campaign scripting are separate work.
---

# FFT map extraction

The user approved **C2** on2026-11-23: light readable terrain, retained source
textures,1px geometric height/material boundaries and thin white silhouettes
around protruding props such as cacti. This is the default graphical direction
for subsequent FFT map extractions. Preserve this approval instead of reopening
the A..E style choice. Its palette IDs and numeric thresholds are map-specific.

Read [the C2 recipe and integration gates](references/c2-pipeline.md) when
extracting or adapting a map. Its depth-budget section decides whether a map fits
`FFT_CORE_DEPTH_MAX` at all, sorts an overrunning map into cosmetic or structural,
and gives the arbitration order: remove whole decorative props first, shrink them
only if drastic, shorten the scene last. A half-removed prop saves nothing.
`depth_budget.py` does the sorting itself, from FFT's crossed-but-not-stopped-on
tile flag. Current scripts live in `games/fft/tools/`;
resource ownership rules are `games/fft/campaign/CORE_RESOURCES.md`.

## Extract and adapt

- Reuse the existing disc/GNS/mesh decoder, archived physical sprites and
  projection helpers. Extract only the requested map; use the existing chapter
  census for selection rather than rerunning every map.
- Retain source geometry, per-pixel coverage/depth and source camera handedness
  together. Keep full views before deciding any crop. Check every stoppable
  tile center, full actor extents and marker extents in all four orientations.
  A matching PC/TI checksum cannot detect a mirror or crop error shared by both.
- Start from unquantized source luminance and explicit opacity, then quantize
  to four greys. C2 favors light sand/readable ground. Tune different materials
  using their original palette/UV/terrain identity, not noisy quantized edges.
- Draw height edges where top/sloping geometry meets a wall. Draw material
  edges only where different source materials meet; ignore same-material seams
  and triangulation diagonals. Use the existing visibility-tested1px treatment.
- Isolate protruding props using source mesh/material records. Add a1px external
  white silhouette at final output size, excluding hidden parts and nearer
  foreground geometry. Preserve opacity; white must not become transparent.
  Keep these static treatments offline in scenery colors, without a frame shader.
- Show four comparable source-color references and four TI-scale views with
  existing actors. Keep camera/scale/positions fixed across alternatives.
  Label reconstructed/unlit color references accurately; they are not PS1 captures.

## Integrate and verify

Use the actual production archive codecs/readers and shared battle renderer.
Retain the approved picture and validate projection, left/right input, rotation
endpoints, depth/cover, movement, targeting and reentry on PC. Preserve reusable
caches and set-sized buffers; allocate every required color/depth plane.

Keep bytes, stored/transfer archive sizes, scene rows/width, ragged-depth peak,
navigation/state allocations and CPU costs distinct. A format validator accepting
a cropped bank is not evidence that the entire map is playable. A wider scene
requires a coherent shared projection/stride, not individual actor offsets or
silently removed cells. Match executable overlays and declarations to the build;
never load a private wide-layout bank into an incompatible shipping core.

PC tests and sanitizers come first. Use selected68000 checks for a changed
layout/arithmetic/overlay/CPU path; reserve TiEmu for actual hardware-dependent
milestones. Reuse unchanged evidence. Publish art in ti-view when it helps the
requested review, preserving earlier verdict files and their dates.

Do not commit extracted commercial artwork or generated banks. Keep the
reproducible code, source hashes, approved style decision and remaining limits
in project docs/handover. Existing C2 approval authorizes its integration; ask
for a new visual verdict only for a materially changed picture/crop/scale.
