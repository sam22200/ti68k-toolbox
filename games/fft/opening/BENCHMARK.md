## Configurable reach and tactical facing (2026-10-19)

CATALOG now switches filled movement tiles and the exterior contour in both
active chapters, default filled. State is1,090 bytes (+4); Work remains35,540.
Sixteen exact PC/native pixel/state checkpoints pass, including settings open,
contour on screen, rotation and restored fills, plus320 focused scene/cache
comparisons across both maps. Existing animation, source-pose and combat checks
still pass. Mask/render constants are archived to respect TTPack's64,936-byte
input ceiling; see `../REACH_CONTOUR.md`.

Ramza's original tactical face is read from unit+0x49, never a transient scene
actor angle. The initial source actor was still facing3072 while his tactical
face was0 (-z). The importer now uses the latter for every actor. Ramza's four
projected facings are checked. `opening_assets.py --facings-only DISC` refreshes
existing unit metadata without regenerating scenery or duplicating sprites.

Settled player-camera workloads and their compression cost are documented in
`../MAP_COSTS.md`; no TiEmu/FPS claim follows from these CPU measurements.

## Reach-cache follow-up (2026-10-19)

The reach-cache change adds 20 Work bytes (now 35,540), with no new asset bank.
Historical follow-up: thirteen PC/68000 checkpoints matched, including range/rotation/cancel,
launch-key and complete-title replays;
the previous ten-case figures below remain a historical checkpoint. The player
control changes from 238,602/284,862 mean/peak cycles to 238,606/284,866.
Large-range entry measurements, tradeoffs and reproduction: `../REACH_CACHE.md`.
The four-camera Orbonne control confirms a6.9–9.4% reduction in entry-frame CPU
time. The experimental exterior contour saves another16.6–19.0% compared with
optimized filled tiles, with no tactical reach change; see `../REACH_CONTOUR.md`.

## Opening review checkpoint (2026-10-18)

This section supersedes the initial-slice numbers below. Reproduce with
`make -C games/fft/opening check`; full details: `x/check/checked.json`.
All ten PC/68000 pictures and explicit tactical/pending-action/handoff summaries
match. Datasheet CPU only: excludes grayscale interrupts, frame waits and packed
startup. Cold pose/scene preparation and multi-actor entrances can exceed the
ordinary frame budget; no claim of sustained hardware 30 FPS is made.

| Replay | Mean ordinary cycles | Peak ordinary cycles | Peak load/turn cycles |
|---|---:|---:|---:|
| player | 238602 | 284862 | 2585154 |
| rotations | 255422 | 357838 | 4488348 |
| fight-first | 289903 | 427696 | 2812344 |
| ending | 273222 | 502458 | 2890310 |

State: 1,086 bytes; Work: 35,520 bytes; the two scene/work allocations total
96,960 bytes, with another 11,528 bytes for saved LCD/grayscale buffers.
The eight archived banks total 112,241 bytes. Script poses and source title
art account for much of the increase over the first slice; all pose records
still use one shared canonical/mirror dictionary and one decoded slot per actor.
The checked native container is 64,413 bytes at this checkpoint. Packed chapter
launch and the Academy/Gariland handoff are documented in `../campaign/README.md`.

The previous "45-frame portraits" regression remains fixed: actual decode/copy
charges and fair animation opportunities give a measured 5–12 PC update frames
in the cold-panel test. This is not an elapsed TiEmu stopwatch result.

---

# Orbonne opening: native audit

Reproduce with `make check` in this directory. The full report and per-run logs
are local in `x/check/`; `check_native.c` executes the actual game loop and
exports an explicit combat-state signature after the run. These are **MC68000
datasheet cycle measurements**, not TiEmu timing and not PC renderer FPS.
ROM calls have estimated costs. Grayscale interrupts and packed startup are
outside these zones. A 12 MHz conversion below is CPU time only.

Ten PC/TI checkpoints match exact framebuffer checksums and tactical state,
including a 9,500-frame replay through victory and the conclusion. Existing
Gariland tests and its native build also pass; its .89z size remains 65,585 bytes.

| Door / run | Average ordinary cycles | Peak combat cycles | Peak scripted cycles |
| --- | ---: | ---: | ---: |
| Immediate Ramza, 60 frames | 213,343 | 252,732 | — |
| First 1,200 fast battle frames | 279,591 | 395,384 | — |
| Complete encounter + ending, 9,500 frames | 335,943 | 404,696 | 428,618 |
| Chapel, 420 frames | 231,793 | — | 437,032 |
| Ending/restart review, 250 frames | 322,671 | — | 530,026 |

The terrain-column projectile blockers also use 16-bit heights: 31 x 12 exceeds a byte.

The longest measured combat frame is **33.7 ms of CPU** at 12 MHz. The ordinary
combat average is compatible with the existing 30 Hz limiter, but occasional
frames exceed the 360k budget; grayscale overhead is additional. This is not a
promise of a constant hardware 30 FPS. The most expensive scripted frame in
this audit is **44.2 ms of CPU**. The scene-view decode/animated-turn category
peaks at **4,419,170 cycles (~368 ms)** in a single frame. A three-frame turn
therefore remains visibly slower than normal play. It is not included in
ordinary-frame averages. No perspective renderer or perspective transition is
part of this opening build.

## Measured optimizations

The first implemented chapel used 16 per-pixel depth tests per actor row. Its
recorded average at the 420-frame door was 732,404 cycles, peak 1,394,232. Replacing
that with three-byte sampling and cached coverage, preparing initial covers
before the title, caching the window, spreading text preparation and applying
the top gradient with word masks brought the same door to 229,880 / 434,660.
The output shader still uses the existing eroded-interior silhouette rule.

The initial native Work allocation was 47,040 bytes. Omitting unused immersion code,
aliasing Status/dialogue storage with idle portrait panels, and discarding 20
provably empty scene rows reduce total game buffer RAM from **114,880 to
96,960 bytes**. Native State is 1,012 bytes (already part of executable statics).
Gray runtime allocations add 11,528 bytes. Every individual game allocation is
below AMS's ~64 KB limit (46,080 and 50,880 bytes).

The eight required archive payloads total **101,757 bytes**. Logical actor frames
shrink from 139,568 raw bytes to 39,171 bytes including lookup/stream directories.
Only 1,071 bytes of canonical compressed streams belong to Ovelia/Simon. The
packed native program is `ffocore.89y` (44,651 file bytes), with a 1,095-byte
`fftopen.89z` launcher. Container headers are included in these last two sizes,
not in the archive-payload total. No full-screen image is stored per dialogue.

## Battlefield and presentation corrections

The reviewed Orbonne palette now uses per-view thresholds and mild offline
texture filtering (`tools/opening_assets.py:battle_greys`). In the initial
camera, pure-white lit scenery fell from 71.1% to 4.0% before the subsequent
geometric contour pass. All four views' coverage depths/masks, terrain heights
and actor/weapon/chapel/story banks remain byte-identical. Flat rotation shades
follow the new scenery tones. Twenty source surface-0x11/depth-four cells are
now blocked for the human party; their heights and rendered coverage are unchanged.

The softened views alone totaled 34,950 bytes. Adding Gariland's exact geometric
edge method and the water tone clamp brings them to 37,423 (+2,473). Archived
water-effect points add 1,948 bytes. Total data growth for these corrections is
4,421 bytes; Work grows by only two bytes for animation scheduling. No runtime
contour/filter pass or decoded glint array is allocated. `x/fixes/verified.json`
records unchanged depths and intentional collision changes.

The old render-order decode quota left some visible actors permanently frozen.
The same one-miss quota now prioritizes the walker/action and rotates the idle
queue; identical already-decoded poses can be shared. CPU cost increases versus
the broken version because the missing animations and water now actually draw.
The complete-run mean also mixes more ending frames: fixing HUD scheduling allows
held-key fast-forward to complete combat earlier. The final tactical/RNG signature
still matches the preceding complete-encounter control. Do not compare those
whole-run means as identical battle workloads.

Cold enemy-panel browsing takes **5–12 frames**, versus 45 for most targets in
the preceding PC control (about 0.17–0.40 s versus 1.5 s at the target 30 Hz;
these are frame counts, not TiEmu stopwatch measurements). Reserving a possible
decode for every visible actor caused the delay. The budget now charges actual
decodes/copies. The portrait opacity mask also uses the same trimmed index as
the artwork. `visual_checks.c` tests all 11 idle actors, 648 exact walking
poses, both panel masks and four visible water phases; `make check` runs it.
The native ten-case replay also passes exact PC/TI pixels and tactical state.

There is no `pidle_tick`/perspective prefetch in this opening build: its
`FFT_CONFIG` and `FFT_EXPLORATION` code is excluded. No idle perspective CPU or
RAM can be reclaimed here. `x/review/orbonne-animation.gif` is the ordinary
PC AI without fast-forward input, suitable for inspecting the animations.

## Remaining checks / limits

At the user's request, the preceding softened-map packed build and eight banks were
cold-launched in Titanium TiEmu on 2026-10-18. The corrected map, actors and
Ramza's turn menu display successfully (`x/orbonne-tiemu.png`). The first typed
invocation arrived before the boot transfer finished; repeating the invocation
after transfer completed launched the game. No files were resent from HOME.
The emulator was handed back at that turn and was not replaced for these
presentation corrections. Full interactive play, return to
HOME, heap margin and original TI-89 HW2 entry remain unchecked. Native CPU
results do not validate those hardware paths. The presentation GIFs in
`x/review/` remain PC framebuffer replays at nominal 30 updates/s, not TiEmu
captures or calculator performance measurements.

Original AI and skill fidelity limits are listed in `README.md`. The complete
opening reaches the ending without forcing HP or victory; it does not yet
continue into the Academy/Gariland campaign flow.
