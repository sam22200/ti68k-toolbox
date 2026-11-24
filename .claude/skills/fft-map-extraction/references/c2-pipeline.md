# C2 recipe and measured Zeklaus control

## Approved control

User verdict2026-11-23: "je prefere le C2". C2 is the original light C palette,
the existing geometric ledges, white external cactus outlines and sand/stone
material boundaries. It is not the assistant's F0/F1/F2 texture mixture.
Approved pictures are local:

- `games/fft/campaign/x/zeklaus-materials/C2-four-views.png`
- `games/fft/campaign/x/zeklaus-materials/C2/view-0.png` through `view-3.png`
- `games/fft/campaign/x/zeklaus-materials/C2/view-0.planes` through `view-3.planes`

Do not regenerate or retune this control to integrate it. Use the saved planes
and assert identical pixels when only framing/storage changes.

## Existing offline code

Paths below are relative to `games/fft/tools/`:

1. `extract.py`: disc/GNS records, mesh polygons, four-view rasterization and
   packed R4 color/depth codecs. The original terrain's upper layer has a fixed
   256-cell stride, not width*height. Keep this in every decoder.
2. `isometric_style.py`: projection, real ledge adjacency, visibility-tested
   contour masks. It follows `extract.SC_W` on wider review canvases.
3. `map_art_review.py`: hash-keyed source rasters, original five proposals,
   borrowed actor fixtures/cameras. C quantizes per-view10/34/65 percentiles of
   non-background luminance, with geometric ledges and white exterior.
4. `map_preferred_review.py`: source prop isolation, depth-clipped1px white ring,
   shared comparison/full-view sheets. Its F color recipe is not approved C2.
5. `map_material_review.py`: applies the material boundaries to C plus the prop
   ring, yielding C2. Reuse its source adjacency and the shared outline renderer.
6. `map_color_reference.py`: original-palette RGB at the same projection. Three
   palette-channel passes preserve source coverage/depth and recover source
   luminance; explicitly unlit/reconstructed, with illustrative TI actor markers.

Commands use `rtk proxy tools/pyenv/bin/python games/fft/tools/<script>.py`.
These initial review helpers are deliberately tied to MAP076. When extracting
another map, factor the common recipe and supply a source-checked material/prop
classification; do not copy a second renderer or pretend palette numbers are
universal. The terrain surface values sand1/rocky cliff6 help cross-check the
classification, but visual mesh boundaries determine the drawn material edge.

MAP076-specific classification: sand top palettes0/1, stone top palettes2/3/4;
palette9/page2 vertical cards are the cacti (64 triangles118..181), palette15
prop/tree bases are excluded from stone/sand contours. There are52 top/top
material-interface edges. White cactus rings take priority where masks overlap.
None of these numbers is a rule for other maps.

## Framing and memory lesson

Current integration follow-up: the shared source candidate is ABI99, with
per-map stride/plane, old R4(40 bytes) and R5(44 bytes), and a separate16544-byte
two-color-plane cap. All four views must agree;352x202 is invalid. Cold
RESOURCES10 resizes the scene/Work block and supports shrink/failure/retry.
State/Save/Work and actor42/depth18488 remain; ModCtx is400. No private width
override is needed now. See `campaign/WIDE_SCENES.md`,
`campaign/RENDER_OPTIMIZATION.md` and `campaign/x/wide-opt/`.

The corrected ABI99 core is promoted (2026-11-24), key
`10965478d3a2c348c22d2dea8e6ac51d`, groups
`campaign/x/core-abi99-release/packages/`. See `campaign/INSTALLED_RAM_ABI99.md`.
Actual installed Zeklaus13 has436 free/430 contiguous bytes including logging,
passes both rotation directions and releases all battle allocations. Bootstrap
must defer scene/Work allocation until the first descriptor: a speculative
narrow scene followed by pinned actors fragmented the actual AMS heap.
RESOURCES10 still handles exact size, shrink/failure/retry.16KiB remains
unpassed; do not shrink caches or change C2 to hide the tight margin.
Selected ordinary CPU gates pass: Zeklaus12 peaks331842/327594 and13 peaks
339962/356832; PC/native pictures agree. Retain earlier ABI98 failures and
ABI97 proofs. `--measure-only` remains profiling, not acceptance. This core
checkpoint does not author Zeklaus's source encounter or certify every future
map/action. The earlier private carrier is a pixel control, not the active engine.

Zeklaus has10x18 lower nodes,176 stoppable cells, no unblocked upper floor,
716 triangles and336px projected width. Its approved complete review lives on
a352px canvas with188 rows (source top54). The old320px crop excludes one
stoppable corner center per orientation: (9,0)/(0,0)/(0,17)/(9,17). Therefore a
"crop16px" planning flag never proved that it was safe. Do not discard those
corners, move individual actors, rescale scenery alone or certify RAM from a PC.

The approved color planes contain two packed bitplanes; source depth is separate.
The old320px candidates need16656..17056 ragged-depth bytes against18488, but
this is not the full-width peak. Recompute depth from full source coverage for
the actual integrated width. Preserve all188 rows and all four views.

The subsequent private real-engine carrier (`campaign/check_zeklaus.py`,
`make zeklaus-check`) validates352x188 on PC/ASan/UBSan. All source cells,
176 stoppable, cursor/input/rotations/movement/cover and reentry pass; exact
full-width depths are16880/17280/17088/17056 under18488. Four raw R4 banks
total83342 bytes. `ZEKLAUS_FLAGS=--native-layout` compiles the actual native
entry through generated include overlays: State2278/Save4684/Work37790,
scene colors16544, stride44. The384 additional color bytes plus600 navigation
bytes are a buffer delta only; they do not certify installed RAM or CPU.
The initial dry fixture's WATER=0 was invalid: supply a real empty phase table.
Legacy view0 contour tables must remain available once in shared ffcu.

This carrier keeps the approved pixels, but borrows diagnostic actors; it is
not an encounter extraction or production core. Its188-row ceiling cannot
load existing202-row maps. Blindly widening the global maximum to352x202 adds
1616 bytes, exceeding the pilot's measured1504 free. A production solution
must adapt shared width/stride/allocation and measure actual native frame/RAM
costs; never ship the private banks under the unchanged320px interface.

Current shipping ABI97 is320px/40-byte rows,42 actor frames and18488 depth bytes;
do not substitute wider banks into it. Save FSV9 remains independent of scenery.
Future-map/native RAM is not certified by the pilot's1504 free bytes;16KiB is a
deferred soft target and does not authorize shrinking measured caches.

Background/window/cover code must agree on projection origin, row stride and
camera bounds. Native row copies assume word/long alignment, so inspect their
loops before choosing a42-byte stride. Buffer capacity, active plane length,
Work placement and cleanup must stay coherent across map reentry and failure.

The optional `adamrt/fft-software-renderer` is a second PC reference for mesh,
palettes, lights and camera. It is not an exact PS1 oracle; its own lighting and
painter-sorting caveats are documented in `campaign/ZEKLAUS_REVIEW.md`.
