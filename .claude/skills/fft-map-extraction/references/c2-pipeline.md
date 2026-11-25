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

1. **Remove the fewest props that clear the cap**, not all of them —
   `minimal_props()`, and see below. Sweegy needs three of its five trees, which
   leaves 17,472 with 1,016 spare.
2. **Shrink them instead and keep every one.** Re-measured 2026-11-25 with the
   corrected prop set, scaling each prop about its own tile centre at ground level:

   | scale | Sweegy | Araguay |
   |------:|-------:|--------:|
   | 75% | 19,352 (over +864) | 18,920 (over +432) |
   | **60%** | **17,720 (768 spare)** | **18,240 (248 spare)** |
   | 50% | 16,760 (1,728 spare) | 17,480 (1,008 spare) |
   | 35% | 15,840 (2,648 spare) | 16,748 (1,740 spare) |

   **60% fits both woods with every tree kept**, and reads as a wood still. Height
   alone is nearly as good (Sweegy 60% of height: 17,928), so either axis buys
   something, but uniform is better per unit of ugliness. `--shrink` on the review
   tool renders it.

   **A flat 2D tree buys nothing.** What the stream pays for is the silhouette: the
   span is the lit byte range of each row of the *rendered view*, and the budget
   takes the worst of the four camera orientations. A billboard that still reads as
   a tree covers the same rows over the same columns in the view that costs most.
   Only making the prop *smaller* moves a span.
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

### Classifying a map automatically

`depth_budget.py` sorts every map into fits / cosmetic / structural on its own.
It starts from FFT's own terrain flag, not geometry analysis: the tile a unit may
**cross but never stop on** (`flags & 0x02`) carries the standing geometry's height
rather than the ground's. Sweegy's four trees are tiles of height 17-18 standing
over ground near 4; Slums in Dorter has no such tile at all.

**That flag alone is not enough, and taking it for "decoration" was wrong.** It
says *you cannot stop here*, and you cannot stop on a wall either, so a cathedral
wall carries it exactly like a tree. Two further conditions make a prop:

- **it rises.** Its height must exceed every ordinary neighbour's standable top. A
  flagged tile level with the ground carries nothing; Araguay flags two such tiles
  beside a trunk, and they used to hide the trunk inside a run.
- **it is free-standing.** A wall is the room's own *enclosure*: its 4-connected
  component among the rising flagged tiles runs along the map's edge. Furniture
  stands in the middle of the room. A single tile is always free-standing, because
  three of Araguay's five trees grow at the very edge of the map.

Measured on the maps that taught the rule: Chapel of Orbonne is one component of
33 tiles spanning 13x10 of a 14x10 map, on the border — the perimeter wall, and
the map has no prop at all. Underground Book Storage has a 10x2 component across
the full width — the back wall — and **three 2x1 components in the middle of the
room**, which are its two free-standing bookcases and a table. Sweegy's five trees
are five components of one tile. Roof of Riovanes keeps its tower (a 3x3 component
on the border, and the map's subject) and gives up two small roof turrets.

A third fault sat underneath both: **a tile with no second terrain level carries
eight zero bytes**, and reading that as an ordinary tile of height 0 collapsed the
ground under everything. Map 3 has 117 absent level-1 entries of 117, Chapel of
Orbonne 116 of 140. Skip an all-zero entry.

Then remove every polygon over a prop tile and re-measure: clearing the cap means
cosmetic, still over means structural.

Chapter I classifies 9 fits, 1 cosmetic (Sweegy, 5 prop tiles, 240 polygons,
21,640 -> 15,288) and 1 structural (Slums in Dorter, no prop tile), matching every
figure established by hand.

**The whole game, 2026-11-24.** 109 maps read; **15 are left out as side quests**
and `optional_map()` names them: the ten Deep Dungeon floors (NOGIAS, TERMINATE,
DELTA, VALKYRIES, MLAPAN, TIGER, BRIDGE, VOYAGE, HORROR, END), the three Colliery
Underground floors with the Underground Passage in Goland, and Nelveska Temple.
Those three groups appear in the battle compendium under "5) Optional Battles"
and nowhere else, so nothing but the side quest reaches them. **Zarghidas Trade
City is not among them** although Cloud's battle is listed as optional, because
the same area also takes random battles. Pass `--include-optional` to count them.

Of the **94 that remain: 63 fit, 4 cosmetic, 27 structural, none unreadable.**
Every over-budget map has a workable remedy, which is the point of the census:

- the 4 cosmetic fit once their props are gone: Roof of Riovanes Castle (5, two
  small roof turrets, +400 -> -1,096), St. Murond Temple (50, +48), Sweegy Woods
  (74, five trees, +3,152 -> -3,200) and Araguay Woods (80, five trunks);
- the 27 structural need an authored crop, and all keep at least 69% of their
  rows, the worst being Public Cemetery at 159 of 228. None is unsalvageable.

These counts are the **corrected** ones. A first pass read 8 cosmetic and 23
structural, because the prop rule took walls for decoration; the section below
has the faults and the pictures that found them.

One caution. The fitting group is **not** comfortable: Orbonne Monastery fits by
**8 bytes**, Beoulve residence and Poeskas Lake by 16, and 15 maps sit under 1 KB
of margin, which is expected since `FFT_CORE_DEPTH_MAX` was derived from Orbonne
in the first place.

### Reviewing the prop removal by eye, and the three faults it found

`tools/cosmetic_prop_review.py` renders each cosmetic map's dearest view three
times side by side -- the source in its own colours, the same with every prop
pixel tinted, and the map without them -- so the removal is approved or refused
by eye. Colour comes from the proven rasterizer run once per RGB555 field, never
a second renderer; the tinted pixels are exactly where the two depth buffers
differ, so a prop hidden behind geometry is not counted as a loss.

**The first run was a demolition**, and that is how the three faults above were
found. Chapel of Orbonne lost its walls, pillars and stained glass -- 73.6% of
the scene, leaving a floating slab; Roof of Riovanes lost the tower that is the
map's subject; the Book Storage lost its back wall. Run the review before
trusting any arithmetic verdict about geometry.

With the rule corrected (rises, free-standing, no phantom level), 2026-11-25:

| map | name | props | removed | lost px | share | result |
|----:|------|------:|--------:|--------:|------:|--------|
|  74 | Sweegy Woods | 5 | 240 | 5,952 | 20.1% | cosmetic — the five trees, terrain untouched |
|  80 | Araguay Woods | 5 | 441 | 6,111 | 19.6% | cosmetic — the five trunks, terrain untouched |
|   5 | Roof of Riovanes Castle | 5 | 96 | 3,198 | 9.9% | cosmetic — two small roof turrets; the tower stays |
|  50 | St. Murond Temple | 8 | 148 | 4,862 | 15.2% | cosmetic, but **a judgement call**: both bell towers and the spire, and a hole opens in the roof |
|  58 | Underground Book Storage 2 | 6 | 136 | 3,448 | 9.1% | structural — see below |
|  51 | St. Murond Temple | 6 | 136 | — | — | structural |
|   3 | Hall of St. Murond | 2 | 82 | 242 | 0.7% | structural |
|  62 | Chapel of Orbonne | 0 | 0 | 0 | 0% | structural — it has no prop at all |

**Underground Book Storage is the trap seen from the other side.** Its two
bookcases and its table are correctly found and removing them is visually clean,
and it saves **zero bytes**: 20,952 before, 20,952 after. They stand entirely
inside the room's silhouette, so no row span moves. Finding a prop is not the
same as saving anything, and the budget, not the prop count, decides.

Of the four that are cosmetic, Sweegy and Araguay lose the trees they are named
for, and St. Murond's "props" are free-standing single-tile *towers* that are
geometrically indistinguishable from a tree and visually not one. Each is a call
to make per map, not a default. Roof of Riovanes is the one clean win: two small
turrets for 1,496 bytes, the tower untouched.

**The ground under a prop.** FFT gives the prop tile the prop's own height, so
the ground under it has to be guessed from around it, and the *lowest* neighbour
guesses too low: it took the floor out from under Sweegy's and Araguay's trees
and left white holes. Use the **highest** standable neighbour; it leaves no stump
on either. A hole that survives that is in the source — the Book Storage's floor
under a bookcase has no polygon of its own, the object's base draws it.

There is no "keep it drawn, drop it from the depth" middle road either: the
stream's span term is measured on the **rendered view**, not on the depth, so a
prop still in the picture still costs its bytes.

### Removing every prop is the test, not the remedy

The classifier removes them all to ask "would that clear the cap?". The authoring
pass should remove the **fewest** that do, and `depth_budget.minimal_props()`
measures it. What a prop costs is not its pixels but whether it widens a row's
span, so props inside the silhouette are free and the ones on its edge carry
everything:

| map | over by | props | fewest that clear it | result |
|----:|--------:|------:|---------------------:|--------|
|  50 | +48 | 8 | **1** — one corner turret, worth 480 alone | 18,056, 432 spare. The other tower, the spire and the small trees stay |
|   5 | +400 | 5 | 3 of 5 | 17,976, 512 spare. The big tower stays |
|  74 | +3,152 | 5 | 3 of 5 | 17,472, 1,016 spare. **Two trees stay** |
|  80 | +2,744 | 5 | 4 of 5 | 18,304, 184 spare |

St. Murond Temple is the lesson: the map is 48 bytes over and four of its eight
props save **nothing at all** — they stand inside the roof's silhouette. Removing
all eight disfigured the building for 976 bytes when 480 were wanted.

`cosmetic_prop_review.py --minimal` renders this version, and `--shrink S` the
one that keeps every prop at scale S.

**The four decisions are applied.** `extract.PROPS_DROPPED` carries them as the
tiles the props stand on, and `extract.views()` — the one place a mesh becomes
four views — drops their polygons before rendering, so every reader downstream
sees the shipped geometry:

| map | dropped | ships at | spare |
|----:|---------|---------:|------:|
|   5 | (9,4) (9,5) (9,6) | 17,976 | 512 |
|  50 | (0,11) | 18,056 | 432 |
|  74 | (9,10) (1,1) (9,2) | 17,472 | 1,016 |
|  80 | (0,0) (10,8) (0,7) (5,6) | 18,304 | 184 |

**Sweegy Woods no longer gates chapter I.** Gariland's `map.h`, `tiles.bin` and
`fftv*.bin` re-extract byte for byte, because no map outside the table is touched.
A tile in the table that carries no prop raises rather than silently shipping the
map over budget, and `test_prop_rule.py` guards that.

### Which maps are fought on, and which are only scenes

The arbitration differs completely. On a **battle** map the camera is tactical and
every unit must stay visible, so a wall that hides the field is itself a candidate
for removal. On a **scene** map nothing is played: the framing is the author's, and
geometry that costs nothing on the dearest view can stay as it is.

`tools/battle_maps.py` answers it from the disc. The join is on an id both sides
agree about: the GameFAQs compendium's "Debug Number" is the **ENTD**, not the
event — checked against this project's own extraction, where Orbonne Monastery is
387 and Gariland is 0x184 = 388 — and `EVENT/ATTACK.OUT`'s scenario table gives
every event its map and its ENTD. 57 maps host a battle the compendium details.

**The positive list is sound, the negative one is not.** This compendium prints no
Debug Number at all for Sweegy Woods (ENTD 384), and the entry it labels "Mandalia
Plains / 388" is really Gariland. So a map's absence means "no battle confirmed",
never "this is a scene". For one map, read the ENTDs its scenarios use.

Settled that way, for the maps the prop review raised:

| map | scenarios | ENTD | verdict |
|----:|-----------|-----:|---------|
|   3 | Hall of St. Murond Temple | 461 | battle (two Divine Knights and a Sorcerer) |
|   5 | Roof of Riovanes Castle | 433, 434 | battle (Riovanes Rooftop) |
|  50 | St. Murond Temple | 460 | battle (Murond Holy Place) |
|  51 | St. Murond Temple | 326, 327 | **scene** — no battle claims those ENTDs |
|  52 | Chapel of St. Murond Temple | 462 | battle (Arc Knight and two Archaic Demons) |
|  58 | Underground Book Storage 2F | 422 | battle |
|  62 | Chapel of Orbonne Monastery | 256 | **scene** — events 1 and 2, the opening |

Two maps are named "St. Murond Temple"; only map 50 is fought on. That is why the
join has to be on an id and not on a name.

**The interiors are the open problem.** A room's four walls are exactly what the
enclosure rule protects, and on a battle map they are also what hides the field
from a tactical camera. The Book Storage, St. Murond's hall and the Limberry
interiors are fought in, and there the authored pass has to choose which walls to
drop **per camera orientation**, not per map: a wall behind the camera hides
nothing, the same wall in front hides everything. No rule for that yet, and it is
work for the semi-supervised extractor, not for a heuristic.

**A hypothesis to test there** (observed, not measured): the game appears to box a
scene in with walls too, and those almost always have to go as well — so "scene"
may not mean "keep the enclosure" in practice, only "the reason to drop it is
framing rather than visibility". Chapel of Orbonne and map 51 are the two scenes
to try it on. Do not fold this into the classifier before the extractor has shown
it on several maps.

### One mesh, several records (map 53)

Map 53, **Entrance to Death City**, used to read as nothing at all (`IndexError`,
no lit row). The cause was ours, not the disc's: **a map's mesh is spread over its
GNS records.** The primary record (0x2E01, or the first 0x2F01 for the 16 maps that
carry no 0x2E01) holds the day/clear state, and each later state record fills only
the header pointers it overrides, inheriting the rest. Map 53 is the only one of the
121 that uses this for its geometry: it keeps the texture palettes (0x44) and the
terrain (0x68) on the primary but leaves the polygon pointer (0x40) at 0, and its
1,062 polygons live in the following 0x2F01. Decoding the primary alone therefore
drew an empty scene.

`extract.mesh_blob()` is now the single place that decides what a map's mesh is: it
appends each donor record and rebases the pointer, so every reader keeps working on
one blob. For the 120 other maps it returns the primary record's bytes unchanged,
byte for byte — Gariland's shipped `map.h`, `tiles.bin` and `fftv*.bin` were
re-extracted and are identical. Map 53 measures 17,328 worst over 164 rows and
**fits with 1,160 bytes to spare**. `tools/test_mesh_blob.py` guards the assembly
on synthetic records.

Never select a mesh record by type alone. `core_audit.py` was already right
(`MESH_TYPES` reads all three); the renderer was not.

**Why a geometric rule does not work.** Five variants were tried and all failed:
asking whether a polygon floats above the standable top of its tile, or rises more
than a margin above it, or seeding on floating polygons and growing the cluster.
They fail because a prop tile's standable top *is the prop*, so nothing floats
above it, and because 96 of Sweegy's 144 tree polygons have their centre over a
neighbouring hill whose top is higher than the leaves. Use the flag.
