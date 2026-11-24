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

## Depth budget: deciding whether a map fits, and what to give up

Measured 2026-11-24 on Chapter I's eleven maps with the production encoder
(`games/fft/tools/depth_budget.py`), confirmed against the shipped Gariland bank
whose R4 header stores depth 18,304. Full figures: `campaign/PACK1_INVENTORY.md`.

`fft.c` unpacks one view's ragged depth into `vbuf`, capped at
`FFT_CORE_DEPTH_MAX` = 18,488. The stream is

    4 * sc_h          one DepthRow per scene row
  + 4 * sum(span)     four planes over each row's even byte span

Only the worst of the four views matters, and `sc_h` is per map
(`chapter_files.inc` reads it from the descriptor). Nine of eleven Chapter I maps
fit with 184 to 5,760 bytes to spare. Orbonne is not an overrun: 4 * 192 + 17,720
= 18,488 exactly, so the cap was sized on it.

**Do not use `map_budget.ragged()` for this gate.** It is a second implementation
that keeps different rows; it agrees with the encoder only where a map has no
framing freedom. It understated Sweegy by a factor of four (19,304 against the
real 21,640).

### Three categories

A map over the cap is over for one of two reasons, and they need opposite fixes.

- **Fits.** Nothing to do.
- **Cosmetic.** A few decorative props carry the excess. Sweegy Woods: four trees,
  36 polygons each on one tile apiece, rising from height 6 to 17-18, plus a fifth
  24-polygon cluster reaching 20. Removing the four trees takes 21,640 to 17,216
  and **keeps all 208 content rows** — the ground framing is untouched.
- **Structural.** The height is terrain. Slums in Dorter: 104 of its 144 tall
  polygons form one mass over x 7-10, z 10-16, from -1 to 17, and its level 1
  carries real rooftops up to height 15. Removing geometry down to height 7 — 295
  polygons, 37% of the mesh — is what it would take. Only a shorter scene works.

### Arbitration order, measured on Sweegy

1. **Remove whole decorative props.** 17,216, 1,272 spare, every row kept.
2. **Shrink them instead.** Only works if drastic: uniformly at 35% gives 18,312
   (176 spare); 50% still fails at 19,312. Height alone is worse — even at 35% of
   their height the trees leave 18,736, over by 248. Both axes matter roughly
   equally; it is the canvas area the prop covers, not its height as such.
3. **Shorten the scene.** The last resort, and the only route for a structural map.
   `sc_h` 202 -> 165 for Sweegy (43 rows off the top), 202 -> 172 for Slums in
   Dorter (54 rows off the bottom). They cut from opposite ends because their row
   costs differ: Sweegy 41/136/44 bytes top/middle/bottom, Slums 4/151/81.
   Cropping changes no tile, no tactical rule and no camera orientation — only how
   many pixel rows the view bank stores. Check that a unit standing on the highest
   tile still has its 26 sprite rows inside the kept window.
4. **Nothing else is available.** Raising the cap is closed: 436 free bytes
   measured on the installed 13-unit Zeklaus. Three pixel-exact re-encodings (a
   span per depth/mask pair, exact byte bounds, two runs per row) give at best
   18,326 on Sweegy and still fail on Slums, and each costs either the word
   alignment `depth_rows()` needs or a test in the per-actor-row cover path,
   against 3,168 cycles of frame margin.

### The trap: a half-removed prop is worth zero bytes

Removing part of a prop does not shrink a single span, because what is left draws
the same silhouette. Measured on Sweegy: dropping the 48 polygons that float clear
of the ground changed the stream by **zero bytes**; dropping all 144 of the four
trees took it from 21,640 to 17,216. Always remove a prop whole.

The reason a naive rule gets this wrong is attribution. 96 of those 144 polygons
have their centre over a *neighbouring* tile — a wide canopy overhangs the hill
next door, whose standable top is higher than the leaves — so a test that compares
a polygon against the tile its centre falls on calls them terrain. Compare against
the prop's own tile.

### Classifying a map automatically: not solved

An automatic cosmetic/structural classifier is **not working yet** and must not be
trusted. Seeding on geometry that floats above the standable surface, growing to
whole props and re-measuring still labels Sweegy structural, because the seed
tiles are themselves found by centre attribution and so miss the canopies. Reading
both terrain levels is necessary but not sufficient. Until this is fixed, classify
by hand: measure with `depth_budget.py`, then test removing the candidate props
and re-measure.
