# Flappy Bird (Portable Game Runtime port)

Port of **sdlbird** by Wei Mingzhi (https://github.com/CecilHarvey/sdlbird, commit `36744bb`,
BSD licence; C-style code in `.cpp` files, closest clone of the original's rules and feel). The
upstream code is the specification; the game is rewritten against `runtime/core/rt.h`. Upstream
sources: `sources/sdlbird/` (local only). Runner-up: `paintenzero/cflappy` (ANSI C, Apache,
time-step physics, further from the original).

```sh
make test          # unit + integration tests (no window)
make pc            # ./flappy_pc [--scenario N]  (2nd = Space/Ctrl/Z, up, Enter; Esc quits)
make ti            # flappy.89z → flappy(N) on the calculator
make bench BENCH=8 # flappyb.89z → flappyb(2): render cost of a frame with 3 pipes
python3 gfx.py > gfx.h   # sprites from the ASCII art (make does it when gfx.py changes)
```

## Spec (from `BirdGame.cpp`) and scaling

Upstream: 287×511 portrait, 60 fps, playfield 401 px above the land. Here: 160×100 landscape,
32 fps, playfield 88 px (land strip 12 px). One frame here = 1.875 upstream frames.

- **Vertical scale 0.22** (88 / 401); velocities × 0.22 × 1.875, accelerations × 0.22 × 1.875².
- **Horizontal: time-preserving**, not 0.22: scroll 1.5 px/frame (48 px/s), so distances keep
  their durations: pipe spacing 60 px (1.25 s), pipe width 16 px, bird hitbox 8 px (0.5 s in a
  pipe, upstream 0.62 s). The landscape screen shows ~2.8 s ahead (upstream ~1.9 s).

| | upstream | here (8.8 fixed point where noted) |
|---|---|---|
| gravity | 0.32 px/f² | 63 (0.246 px/f²) |
| flap | velocity 5.2, angle -45° | -532 (rise 9.8 px, apex ~9 frames = 0.28 s) |
| rotation | +2.7°/f up to 85° | +5°/f up to 85° |
| pipes | 3, spacing 150, width 50, speed 2 px/f | 4, spacing 60, width 16, 1.5 px/f (2,1,2,1…) |
| opening | 91 px, top 60 + rand()%200 | 20 px, top 13 + [0, 45) (one `mulu`, no division) |
| bird hitbox | 24×24 inside a 48×48 cell | 8×6, sprite 13×10 drawn at hitbox - (4, 5) |
| ceiling | height clamped at -50 | -11 px (the bird may leave the top, pipes still hit) |
| death drop | 8 px/f, angle 85 | 845 (3.3 px/f) |

Rules kept: only `pipe[0]` (leftmost) is tested for collision and scoring; a point when the pipe
centre passes the hitbox left edge; the ground kills; states title → get ready → play → hit
flash (2 frames) → drop → game over ("game over" after 16 frames, panel after 29, restart
allowed after 37, score counting up); medals bronze 10, silver 20, gold 30, platinum 40; day or
night background drawn at random on each round.

Changes: controls are keys (2nd, up, ENTER) instead of touch buttons; no fades, no sound, no
bird colours (4 greys); the best score lives for the session only (upstream saves a file).

## Scenarios (injection door, `--scenario N` / `flappy(N)`)

| N | state |
|---|---|
| 0 | title (normal start) |
| 1 | get ready |
| 2 | playing, pipes 24 px ahead, bird centred in the opening (bench: `BENCH=8`) |
| 3 | playing, bird far above the opening: hits `pipe[0]` |
| 4 | playing, score 39 = best: next pipe gives platinum and a new best |
| 5 | game over screen, score 25, best 30 (silver) |
| 6 | playing, about to touch the ground |
| 7 | autopilot, endless (restarts itself): demo and long runs |

## Tests (`test_flappy.c`)

States and presses (held keys do nothing), flap arc height and apex time, free fall → dead →
over, hit flash, 1.5 px/frame scroll, ceiling clamp, pipe and ground collisions, scoring,
medal thresholds, over → ready keeps the best; integration: 3000 autopilot frames with pipe
invariants every frame (spacing, opening range), bit-identical replay with the same seed, and a
600-frame scripted run from the title (3 rounds, final state pinned).

## Graphics and visibility

Style chosen from three compared on screenshots (grey, mono, day and night):
white sky with a light-grey city (night: light-grey sky, dark-grey city), light pipes with a
white highlight, dark shade and black outline, bird with a **white outline** (mask dilated by one
pixel), score in bold white digits outlined in black (readable over pipes and sky), panels
white with a black border. Rejected: light-grey sky (the bird's light-grey body vanished) and
flat dark pipes (legible but dull). Bird tilts: -45°, 0°, 45°, 90° made with RotSprite
(Scale2x ×3, rotate, sample); 20-30° tilts were unreadable at 13×10 px.

## Measured (Titanium, TiEmu cycles per frame, budget 375k)

| | update | render |
|---|---|---|
| placeholders (rects), scenario 7 | 1.5k | 100k (no pipe on screen) |
| playing, 3 pipes (`BENCH=8`, scenario 2) | – | 158k |
| game over screen (scenario 5) | 0.5k | 244k (431k with pipes drawn as 28 `draw_rect` stripes each) |

`flappy.89z`: 9.5 KB. TiEmu under-counts sprite shifts (performance §1): check on hardware
before a release.
