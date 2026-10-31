# Shared scenery and orientation-cache experiment

An isolated C/68000 implementation, not a replacement for the campaign's reviewed
art. Production defaults, F1/F5 animation, gameplay and perspective are unchanged.
All disc-derived assets and captures remain local under ignored `x/`.

## What was built

`build.py` reads MAP022 (Gariland) and MAP056 (Orbonne) from the local disc. It
creates **one common 27,956-byte bank with 143 projected material pieces**, plus
1,216/1,136-byte map descriptors. Wood, paving, grass, water and other source
material patterns are sampled offline; Gariland supplies common patterns first.
The manifest records the actual source tile/palette/page and explicit aliases.
Surface labels are descriptive hints, not a promise that a terrain surface ID
identifies one unique original texture.

Maps store four corner heights, material/wall selection and small flags per cell.
The C renderer generates tops, slope caps and exposed wall strips in all four
source-handed camera orientations. Slopes support a normalized corner difference
of at most three height units. A complete seen plane avoids color overdraw while
two depth/mask layers per byte retain the existing coverage format. There is no
full pixel Z buffer on the calculator. Shared sprites are read directly from an
archived variable, with no 28 KB decompression allocation.

**The heightfield omits decorative mesh geometry and changes the source look.**
Buildings become terrain volumes; windows, doors, original roof details and some
overhanging architecture are absent. The new images differ in 28–33% of Gariland's
full-map pixels and 34–40% of Orbonne's. The comparison uses the same full-map
extent, heights, scale and orientation, without actors:

![Source at left; shared terrain pieces at right](x/comparison.png)

The source-looking result is separately tested with exact XOR residuals: short
independently compressed 256-byte patches restore **all six planes byte for byte**,
including the existing occlusion/silhouette data. This control requires only a
256-byte decoding scratch, but costs more Flash and CPU. It is a fidelity/cost
control, not the proposed production format.

## Measured storage

All figures below are payload bytes for these two maps together, excluding actor,
UI, title, tactical metadata already stored elsewhere, variable envelopes and new
executable code. The existing first-view contour tails are retained independently
and excluded equally from both sides.

| Representation | Bytes | Change from current scenery |
|---|---:|---:|
| Current eight compressed six-plane views | 77,529 | reference |
| Shared material bank + both map descriptors | 30,308 | −47,221 (60.9%) |
| Same bank/descriptors + exact residuals | 132,081 | +54,552 (70.4%) |

37 top-piece records are referenced by both maps; 28 additional wall records are
common wall types. These counts do not imply that every visible source surface
has been preserved. Adding more maps can amortize a bank; these two-map savings
must not be extrapolated to untested maps or called a full-game storage saving.

`blocks.py` also tests exact shared 8×1/2/4/8 projected pixel blocks, compressed
placement lists and the original separately compressed coverage planes. Its local
`x/blocks.json` accounts for every part and checks reconstructed color pixels.
Those are exact pixel dictionaries, distinct from reusable semantic materials.

## Native timing and RAM

`bench.c` is compiled by GCC4TI and executed by `ti-cycles`. Measurements use
MC68000 datasheet cycles at nominal **12 MHz**, not PC wall time, TiEmu or measured
calculator FPS. RAM below is allocated scenery cache + temporary scratch only.
Full-game actor/work buffers, executable, AMS, grayscale and saved LCD are extra.
Zone timings include estimated AMS memset/memcpy charges; raw logs retain those
estimates separately. All runs use orientation zero initially, then 1,2,3,0.

| Map / representation | Cache | Cold preparation | New-view preparation | Scenery RAM |
|---|---|---:|---:|---:|
| Gariland / current exact | one full view | 175 ms | 175–194 ms | 52,320 B |
| Gariland / current exact | four full views | 730 ms | cache hit | 209,280 B |
| Orbonne / current exact | one full view | 178 ms | 170–192 ms | 46,080 B |
| Orbonne / current exact | four full views | 721 ms | cache hit | 184,320 B |
| Gariland / shared approximation | one full view | 853 ms | 853–875 ms | 61,040 B |
| Gariland / shared approximation | four full views | 3,450 ms | cache hit | 218,000 B |
| Gariland / shared approximation | four colors + current coverage | 3,450 ms | 771–788 ms | 113,360 B |
| Orbonne / shared approximation | one full view | 1,038 ms | 973–1,043 ms | 53,760 B |
| Orbonne / shared approximation | four full views | 4,042 ms | cache hit | 192,000 B |
| Orbonne / shared approximation | four colors + current coverage | 4,042 ms | 874–940 ms | 99,840 B |

With the exact residuals, one-view preparation is 1,451 ms Gariland / 1,678 ms
Orbonne; preparing all four takes 5,823 / 6,352 ms. One extra 256-byte scratch is
charged. The mixed policy still takes 1,260–1,306 / 1,314–1,488 ms on a turn.
Exact residuals are reapplied to rebuilt coverage only, preserving already patched
color planes; tests explicitly check this rather than XORing colors twice.

A cache hit here measures pointer selection (~0.008 ms), **not a complete visible
rotation**. It excludes the retained low-poly animation, viewport rendering,
actor rendering, input and display waits. Cold preparation excludes executable
unpacking, chapter loading, VAT discovery and allocation. It measures constructing
or decoding cached scenery once, never background work on every idle game frame.

All policies copy the same 160×100 viewport into runtime planes in about **5.33 ms**
in this deliberately simple row-memcpy control (32 repetitions). This is only a
scenery-copy stage; it is not a new measurement of game FPS. No actor, UI or tactical
stage is benchmarked here. Four cached views do not make a warmed stationary view
faster: it already has everything it needs with one cached orientation.

### Why four views are blocked in the complete game

Orbonne already allocates 35,540 B Work + 15,360 B scene planes + 46,080 B current
view = 96,980 B, plus 11,528 B saved LCD/grayscale and roughly a 65 KB unpacked
native program. State lives in that executable; do not count it again.

Replacing the current view with four exact full views raises the buffers alone to
235,220 B, **before** grayscale and executable. That exceeds physical RAM once
those are included, even before AMS. Gariland's four full views are larger still.
Allocating successfully in `ti-cycles` does not prove calculator feasibility: its
synthetic heap deliberately exceeds physical calculator limits.

Four color images plus current coverage need 92,160 B on Orbonne before a temporary
seen mask. Even borrowing the existing scene plane for that scratch, Work + scene
+ cache + grayscale + unpacked program exceed 219 KB, beyond the normal AMS user
budget. This mixed policy is not a demonstrated full-game solution either.
The single-view compositor can borrow one invalidated scene plane as its seen
scratch during loading/rotation, avoiding its standalone 7,680/8,720-byte extra
allocation. This reuse is a budget observation, not an integrated game change.

## Verdict

Keep **one complete orientation** in the production game for now. All four fully
prepared views save only occasional destination preparation at an unaffordable RAM
cost. They do not improve ordinary warmed player-frame responsiveness. Do not
precompute them while the user is navigating the battle menu.

The common material bank is a working foundation for a more compact art pipeline,
but the current heightfield-only reconstruction fails both the visual-fidelity and
CPU comparisons. A production shared bank needs reusable floor/wall/roof pieces
**and compact decorative placement/geometry metadata**, rather than assuming terrain
corner heights describe the original church or houses. Reuse source materials
visually, then validate coverage, handedness and art on both maps. Adding a large
residual picture to disguise missing geometry defeats this prototype's savings.
These results do not establish that a carefully authored piece bank cannot win.

## Reproduction

From the repository root, with the local FFT USA disc and existing Orbonne and
campaign scenery banks prepared:

```sh
make -C games/fft/scenery assets
make -C games/fft/scenery benchmark
make -C games/fft/scenery blocks
```

`assets` writes the shared bank, map descriptors, exact controls, eight reference
views, comparison PNG and archived `.89y` variables. `test` uses ASan/UBSan and an
independent Python per-pixel depth oracle. It compares every output byte, exercises
screen clipping and guard bytes, verifies exact residuals and coverage-only rebuilds.
`benchmark` runs 16 native cases; all 80 per-view color/coverage fingerprint pairs
must match the oracle before a result is accepted. Native code uses the existing
verified `lib/unpack68k.s`, with no new assembly. `x/bank.s` can be inspected via
GCC4TI `-S`: the multiplication helper in descriptor validation is outside rendering;
the compositor contains no floating-point or 32-bit division/multiply helper.

Evidence: `x/storage.json`, `x/measurements.json`, `x/*.log`, `x/comparison.png` and
the checked-in `RESULTS.json` snapshot. The full scene images and exact source data
remain local. The benchmark executable is **RT_CYCLES only**, not a launchable game
or an emulator test. No TiEmu interaction was performed for this experiment.

## Follow-up: one pattern per face, projected at run time

The material bank above gives every face of a material the same 8×8 swatch, and it
has only two wall pieces. `faces.py` gives **each top and each wall face its own
world-space 8×8 four-grey pattern**: an upper wall unit and slope cap, then lower
units. Patterns are sampled back from the reviewed production views, and all four
orientations vote. A wall face's texture u axis is world-consistent between the two
orientations that show it. Shared patterns are k-medoids chosen among **Gariland's
patterns only**. Orbonne takes the nearest, optionally with a few greedy extras of
its own.

Wrong scenery pixels against production, mean of four orientations (same geometry):

| Patterns | Gariland | Orbonne |
|---|---:|---:|
| material bank above | 71% | 60% |
| own pattern per face, no sharing | 21% | 27% |
| 64 tops + 128 walls from Gariland | 23% | 35% |
| 32 tops + 64 walls from Gariland + 16/16 Orbonne extras | 27% | 31% |

The remaining error is mostly geometry: repeated lower wall patterns on tall walls,
Orbonne's church as a heightfield column, deep map skirts and absent off-grid decor.

Pre-projecting every (pattern, shape, rotation) piece would cost 68 KB for 32+64
patterns. `proj.py` + `proj.c` therefore project at run time. Its common bank holds
86 piece shapes, each row as a start/length plus one texel index per pixel. The
textures are stored expanded, 64 bytes each. Maps store 16 bytes per tile: 4
heights, top pattern and 4 edges × 2 wall patterns.

| Payload, both maps | Bytes |
|---|---:|
| current eight compressed views | 77,529 |
| common bank (15,485 texel maps + 8,192 textures + tables) | 24,058 |
| Gariland + Orbonne tile data | 2,416 + 2,256 |
| total | **28,730** (−63%) |

Textures stored at 2 bpp would save 6 KB more. `make proj-test` checks the C
compositor byte for byte against the Python oracle: all six planes for 4 orientations
× 2 maps, plus a coverage-only rebuild and guard bytes. `make proj-bench` runs the
GCC4TI build under `ti-cycles`. Its checksums equal the oracle's.

| One cached orientation | Load | Turn |
|---|---:|---:|
| current ZX0 views (above) | 175–178 ms | 170–194 ms |
| projected faces, C only, Gariland | 936 ms | 935–978 ms |
| projected faces, C only, Orbonne | 1,132 ms | 1,100–1,177 ms |
| projected faces + `proj68k.s`, Gariland | **576 ms** | 563–587 ms |
| projected faces + `proj68k.s`, Orbonne | **673 ms** | 661–692 ms |

RAM is the one-view layout (2 colour + 4 coverage + 1 seen plane), with no
decompression buffer. In the C-only build, `-DSP_PROFILE` splits one Gariland view:
texel unpacking costs ~4.5 M cycles (~150 cycles/pixel, 30k visible-row pixels),
byte/coverage writes ~2.7 M, and row setup plus visibility tests ~3.9 M. Rows
already hidden by nearer pieces skip their texels.

`proj68k.s` (`sp_rows_asm`, asked for and approved) draws all rows of one piece. It
keeps all seven plane pointers in registers. An unrolled computed-jump texel loop
costs 46 cycles/pixel: texture bit 7 → X → `addx` into the light plane, bit 6 into
the dark plane. Byte emission is unrolled, with the same coverage rule as the C code.
Pieces needing horizontal clipping, and coverage-only rebuilds, keep the C path. The
native checksums are unchanged, so it is exact. One Gariland view now spends ~5.3 M
cycles in the routine (~1,000 cycles/row) and ~1.6 M in compose, memset and C edges.
That is still 3.3–3.8× the current decode. A turn would add ~0.4–0.5 s CPU unless
the projection is spread over the turn animation's frames (not yet measured).
