# Retired experiment

The user rejected free tactical play in perspective (2026-10-16). Its active input route
and build targets were removed. Previous source snapshots are local under `x/retired-wide/`;
previous measurements/GIFs remain historical evidence only. See `ATTACK_CAMERA.md` for the
replacement: isometric selection and a target-centered attack shot.

# Playable wide perspective exploration

`FFT_PLAY` extends the fixed shoulder renderer with actual tactical input in its widest
view. Move, undo, Attack, abilities/items, Wait/facing, CT turn changes and AI use the same
update functions and unit records as the isometric game. This is an experimental build on
`explore/fft-shoulder-perspective`; the ordinary game and inspection-only builds are retained.

## Try it

From `games/fft`, with the local disc-derived assets already extracted:

```sh
make fixed-texture-contrast    # only if the reviewed scenery caches are missing
make wide-play-review         # prepares fftbg.bin and the two actual-input GIFs
make wide-play                # fftplay_pc and fftplay.89z
./fftplay_pc --scenario 65
make wide-play-check          # rules/renderer tests, PC/TI image checks and cycle results
```

`fftplay(65)` opens the widest perspective directly on Ramza's turn, with the original
eleven-unit deployment and deterministic scenario-61 equipment/stat draw. `fftplay()` starts
at Ramza's isometric menu; DIAMOND+MODE enters wide directly. DIAMOND otherwise remembers
the last zoom. The close perspective remains a paused inspection view with animated poses.

| Key | Wide perspective |
|---|---|
| Arrows | Menu selection, tile cursor, or Wait facing; tile movement follows this camera |
| 2nd / ENTER | Confirm the ordinary game command |
| SHIFT / ESC | Cancel/back; SHIFT undoes an uncommitted Move |
| DIAMOND | Smooth return to the isometric view |
| MODE | Smooth close/wide change |
| F5 / F1 | Opposite direction / original direction |
| ALPHA | Auto / Solid / Dotted scenery |
| APPS | Turn order |

On PC: Ctrl/Space/Z = 2nd, Shift/X = SHIFT, C = DIAMOND, M = MODE, V = ALPHA,
A = APPS. Ordinary arrow-repeat behavior is retained. Perspective picking uses the explicit
world tile; it deliberately bypasses the isometric shortcut for two tiles sharing pixels.

The TI output under `x/wide-play/` includes `fftplay.89z` and nine banks in `banks/` with their
archive attribute set. Transfer those archived banks with the program on a clean boot.
The interactive program is 63,607 transfer bytes in this revision: intended for Titanium,
not a stock-AMS original TI-89 release (its 24,576-byte execution limit needs a packed build).
No new TiEmu session or physical-calculator test was performed for this gameplay revision.

## Actual replays

- `x/wide-play/walk.gif`: original deployment, choose Move, walk three tiles, undo,
  then smoothly return to the isometric map. Keys: `keys/wide-walk.txt`, scenario 65.
- `x/wide-play/attack.gif`: choose Move and walk to `(5,5)`, Attack the original enemy
  at `(6,5)`, show its hit/HP loss, choose Wait/facing and hand the turn to the next recruit.
  **Scenario 66 starts Ramza at `(4,8)` instead of `(4,11)` to reach combat immediately.**
  Enemy positions and stats are unchanged; this is not an original opening-turn recording.
  Keys: `keys/wide-attack.txt`. The underlying replay continues for 600 frames; the GIF
  shows the first 300 frames.

Both GIFs contain the C game's real inputs and framebuffers, enlarged with nearest-neighbor
sampling. Playback is 20 FPS for review and does not claim measured calculator cadence.

## Rendering and memory

The camera anchor is captured on entry and stays fixed while units act. Walk/Jump uses the
existing path, interpolation and hop tables in world coordinates. A unit's new projected
rectangle invalidates only that unit's scaled pose slots and visibility mask. Unchanged
units retain their masks and poses. Actors are re-sorted by reciprocal depth, with the
original armed-body/blade atlas and interior hit flash. Separate bank/offset/flash keys keep
armed frames from aliasing ordinary poses; the cache is bounded and recycled when full.

Each actor owns a fixed 768-byte visibility-mask slot: even a clipped 104-pixel-tall sprite
needs at most `9*85 = 765` bytes. Eleven slots use 8,448 bytes of the existing 12,320-byte
mask scratch. The 24 KB pose pool and 16 KB scenery depth image are retained. Metadata uses
the existing status scratch, not another allocation. The compact Status/AT/menu UI avoids
overwriting that scratch. The normal build retains its complete original presentation.

One background and its depth remain active. The other existing 6 KB background slot caches
the reachable-tile outlines. It is regenerated when the range/mode changes, then copied;
the cursor is projected onto the terrain's actual sloped corners every frame. Hidden cursor
fragments are dashed. F5 invalidates the direction rather than reusing a stale depth image.
Cutaway probes remain tied to the captured camera anchor, so walking cannot silently change
the geometry behind the cached image.

`fftbg` is a 24,000-byte archived bank containing the four previously reviewed textured
backgrounds. Texturing still happens offline. Those images are used only at the matching
original Ramza anchor/facing in Auto mode. Another anchor/facing or cutaway style uses the
actual flat geometry; it never replays a mismatched textured image. No room-image series is
generated for every unit position.

## Validation and measured limits

`make wide-play-test` passes 615 input/update/render frames, including 555 differential
tactical-state and **both RNG** comparisons against the original isometric update for the
same world commands. Cases cover Move/undo, Attack, Wait, a forced enemy turn's real planner
and action, Potion consumption/healing, Status/AT, return/zoom, removed/offscreen actors and
all 150 terrain positions. ASan+UBSan also pass. Normal-game tests, fixed inspection tests
and the actual-pixel cutaway tests pass separately.

`make wide-play-check` checks **810 frame hashes** from the real 68000 binary against PC:
210 walking/undo/return frames and 600 combat/next-turn frames. `x/wide-play/validated-ti.json`
contains source/binary hashes; each replay has `cycles.log`, `cycles.csv`, `frames.csv` and
the final TI framebuffer PNG. Timing uses `ti-cycles` MC68000 datasheet instruction counts
and estimated AMS calls; neither interrupts nor synchronized LCD cadence are measured here.

| Stage / replay | Mean cycles | Worst frame | CPU-only FPS estimate |
|---|---:|---:|---:|
| Walking near the starting position | 463,892 | 539,194 | 23.5 |
| Walking farther away in the combat fixture | 341,297 | 372,696 | 32.0 |
| Attack animation | 299,886 | 422,152 | 32.0 |
| Attack selection | 382,863 | 696,344 | 28.5 |

Estimates reserve 9% of a nominal 12 MHz CPU for grayscale and cap at 32 FPS. Each measured
frame includes the game update and render. Cache warming and misses are included; camera
transitions and the cold first frame are excluded from this table. The earlier actual
[TiEmu idle benchmark](SCENERY_FPS.md) does not measure these gameplay paths.

Two visible stalls remain. Initial native-view construction takes about 42.6–42.9 million
cycles, roughly 3.9 seconds under that CPU-reserve model. First selection of Move builds
the reachable-tile overlay in one frame, peaking at 3.24 million cycles (about 297 ms);
the later selection frames use its cache. No background/depth reconstruction occurs during
either replay's settled gameplay. Camera changes still rebuild it and retain the existing
transition cost; the GIF's uniform playback does not model those pauses.

The fixed camera can let units/cursor leave its frame or hide behind architecture; automatic
tracking, framing an entire battle, incremental reach-overlay construction, full equipment/
ability Status pages, projectile effects and the complete results presentation remain
outside this review. SHIFT/DIAMOND allow ordinary command cancellation and return to the
isometric map. This demonstrates viable actions with a cached perspective background,
without claiming that the whole battle is already comfortably playable from one fixed shot.
