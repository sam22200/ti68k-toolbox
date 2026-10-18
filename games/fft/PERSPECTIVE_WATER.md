# Fixed perspective: water, zoom and original placements

The latest [elevated wide camera and cutaway](PERSPECTIVE_CAMERA.md) supersedes the
horizontal wide camera measured here. These earlier GIFs/results remain as a baseline.

Build and check with `make fixed fixed-check fixed-xcheck` in `games/fft`.
Run `fftfix(62)` directly in shoulder inspection, or `fftfix()` at Ramza's tactical menu,
then DIAMOND. This is the `explore/fft-shoulder-perspective` experiment.

| Key | Inspection action |
|---|---|
| MODE | Toggle camera distance 40 / 80 world units (close / wide) |
| F5 | Switch to the opposite side of Ramza, retaining zoom |
| ENTER / F1 | Return to the forward direction, retaining zoom |
| SHIFT / ESC / DIAMOND | Return to tactics |

Inspection preserves positions, tactical facings, CT, HP, RNG and the active turn. Only
the idle/water clock advances. Arrows remain inactive in this fixed-view variant.

## Water

The local asset generator tags 76 horizontal water triangles using the original terrain's
surface type 14, its depth and its actual surface height (12 world units at Gariland).
The submerged standing floor is at height zero: choosing faces merely by low height
would fail to select the visible water.

During cold geometry rendering, a temporary 2,000-byte bit mask records which *winning*
pixels belong to water. Solid surfaces clear it when they overwrite those pixels.
Each direction builds four sparse lists of two-pixel white glints, with at most 128
entries per phase. The two pixels must both remain water and light gray after outlines,
so rooflines/banks retain their black contours. The current advances every eight frames,
repeats after 32, and is drawn before the cached actors.

The live path reads a packed framebuffer-byte offset plus a two-bit horizontal position:
no projection, division, scaling or depth test per glint. The two views reserve 2,056 bytes
of existing Status scratch for the lists/counters; total fixed-view metadata is 2,836
bytes, below its 7,680-byte allocation. The temporary mask aliases the existing tactical
background scratch. No extra heap allocation or assembly was introduced. At the default
Ramza anchor, close/front has 23 glints per phase, close/reverse 1–2, wide/front 9–10 and
wide/reverse 9–10.

Both directions are cached for the current zoom. MODE invalidates them to keep the
existing two-background, 12,000-byte budget rather than retaining four full backgrounds.
Changing zoom therefore causes a cold view rebuild. It does not move Ramza.

## Reproduction and measurements

```sh
make fixed-water-gifs
make check-placement
```

`fixed-water-gifs` captures 512 actual MC68000-binary frames and checks every visible
framebuffer checksum against portable C. Outputs stay local in `x/fixed-water/`:

- `perspective.gif`: full close/reverse/zoom sequence, paced by cycle estimates including
  view construction and pose-cache misses.
- `four-views.gif`, `four-views.png`: four parallel warmed excerpts, each 32 frames, at
  synchronized 32 FPS for visual review; these omit construction stalls.
- `frames.csv`, `manifest.json`: per-frame cycles/checksums, hashes and timing assumptions.

At 12 MHz, 9% grayscale CPU reserve and a 32 FPS limiter:

| Warmed view | Mean render cycles | Estimated FPS |
|---|---:|---:|
| Close / forward | 194,970 | 32, capped |
| Close / reverse | 260,616 | 32, capped |
| Wide / forward | 328,263 | 32, capped |
| Wide / reverse | 210,078 | 32, capped |

The full 512-frame demo averages 16.95 FPS including repeated zoom reconstructions and
cache misses. First close/forward construction takes 1.589 s, close/reverse 1.280 s;
the wide directions take 1.344–1.875 s. A warmed wide-direction switch still encountered
a pose miss (102.34 ms) in this sequence. Reserve tables/cache-miss frames must not be
confused with the warmed figures. Maximum observed mask usage is 1,953 bytes and pose
usage 23,505/24,000 bytes. Cache counters reset on zoom changes: the manifest reports
their maximum *per zoom*, including one bounded pose-pool reset.

These are MC68000 datasheet-emulator cycle estimates with estimated AMS calls, **not
physical-hardware FPS or TiEmu wall-clock timings**. Startup is excluded; GIF delays round
to centiseconds. The normal executable is 65,161 transfer bytes (65,071 on-calculator
payload), within the 65,518-byte variable limit. An original TI-89 with stock AMS still
requires the separate packed-release work because of its 24,576-byte launch limit.

Checks: 48 cache/rebuild comparisons across four facings, street/roof positions, both
distances and directions; four-phase water tests freeze actor poses and check all changed
pixels are white/light-gray canal pixels; tactical/RNG conservation and return to Move;
110-frame PC/TI zoom/flip/return replay; 512 PC/TI GIF source frames; address/undefined
sanitizers (the pre-existing word-aligned isometric casts exclude alignment checks).
The original three-camera experiment's tests and three PC/TI return replays also pass.
The normal `fftfix.89z` was launched from a clean private TiEmu Titanium/AMS 3.10 profile
with all eight banks archived. MODE and F5 produced the wide reverse view; the one LCD
review is `x/fixed-water/titanium.png`. The private emulator/display were stopped afterward.

## Enemy placement audit

`tools/check_placement.py` compiles an independent export of the actual exploration state,
reads ENTD 0x184 from the user's local disc, and runs the original game afresh through
the existing documented boot/deployment door. No battle-position pokes are used. The
eleven initial positions match both the disc/deployment definitions and the original's
live RAM, with every CT still zero. Then the original plays through Ramza's first turn.

There is **no X reflection or X/Z transposition in unit placement**. PS1 X remains X,
PS1 Y becomes world Z; mesh centers are `(28*x+14, 28*z+14)` in both renderers.
The five enemies really do move before Ramza receives control:

| Enemy, in encounter order | Initial X/Z (quick start) | Original at Ramza's first turn |
|---|---|---|
| Male Squire with sword | 6,5 | 6,8 |
| Male Squire initially on the roof | 1,1 | 0,4 |
| Other male Squire | 5,4 | 7,6 |
| Chemist | 7,4 | 8,6 |
| Female Squire | 3,3 | 3,7 |

Delita also moves from 8,12 to 7,9. Our quick-start door deliberately gives Ramza control
at the *initial deployment*, skipping all those AI turns. It is not a save of the
original first Ramza turn. This audit leaves that behavior intact, rather than silently
moving enemies while changing only the camera. Player deployment remains the documented
fixed choice; the original lets the player choose their own deployment.

Evidence: ignored `x/placement/placements.csv`, `report.json`, `trace.txt`, original
screenshots and two original save states. The existing oracle uses HLE BIOS and documented
script/deployment shortcuts; this is a controlled Gariland run, not a clean manual
playthrough of the introduction.

## Sprite discussion

The current perspective already reuses exactly the isometric bank's 16-pixel-wide,
outlined, animated actors. They are camera-oriented upright billboards, scaled with
distance and occluded by the real map. No second perspective character set is stored.

Keeping every actor at its unscaled isometric size would reduce the sense of depth and
make nearby/far units look equally large. Rotating the whole 2D sprite with the terrain
would flatten characters against the floor. The useful reuse is therefore the existing
billboard method. Its limitation is visual: near magnification exposes the small source
pixels and a three-quarter drawing does not become a true rear/eye-level view. Wide mode
reduces this magnification, although nearby allies can still occupy the foreground.

Recommendation: retain the reused sprites for ordinary units, use close view for Ramza's
shoulder and wide view to read the scene. A dedicated larger rear sprite for Ramza could
be a later art experiment; it is not implemented or benchmarked here.
