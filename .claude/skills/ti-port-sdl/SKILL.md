---
name: ti-port-sdl
description: "Port an open-source SDL/PC game (Open Flappy Bird, etc.) to the TI-89 through the Portable Game Runtime: pick the best upstream version on the web, rebuild the engine and game design first (unit and integration tests on the PC), run it on SDL, benchmark then run it on the TI, only then do the graphics (several visual variants compared on SDL screenshots), run on the TI again, conclude, update the knowledge base and commit with ti-commit. Use when the user asks to port, \"porter\", adapt or remake an existing PC/SDL game for the calculator."
---

# ti-port-sdl (port an open SDL game to the TI-89)

The port is a **rewrite against `runtime/core/rt.h`**, not a line-by-line translation: the upstream
code is the specification (rules, constants, feel), the runtime is the target. Every step below
ends with a check; do not start the next step on a failing one. Report progress to the user in
French, write the project files in English.

Constraints, in this order (`CLAUDE.md`): **1. performance** (reuse what is measured: knowledge
base, verified asm in `lib/`, ExtGraph, TileMap, `experiments/` verdicts; no float, no division
in a loop, smallest types), **2. visibility** (160×100, 4 greys, outlined main sprites).
Titanium only; the TI-89 HW2 (`TI_CALC=89`) only for a release. ASM only after asking.

## 0. Choose the upstream version

- Search the web (GitHub, ticalc.org, itch.io…) for the candidates. Rank them: **pure C** first
  (then C++ with little OO), SDL1/SDL2, small and readable game logic, a clear licence, complete
  game (menus, score, difficulty). Prefer the one closest to the original game's feel.
- Tell the user the pick in one line with the runner-up and why; clone it into
  `sources/<name>/` (read-only, not in git).
- Build and run it on the host if possible (reference for the feel, and screenshots).
- Write the **spec** from the code, into `games/<name>/README.md`: states (title, play, game
  over), physics constants (gravity, jump, speeds, spawn intervals), collision rules, scoring,
  difficulty curve, controls. Rescale to 160×100 and 32 fps (`RT_FRAME_TICKS`): convert every
  float into fixed point (`s16` 8.8 or `s32` 16.16 when needed), per-frame values into integer
  deltas, `rand()` into `rt_rand()`. Note the upstream URL, commit and licence.

## 1. Engine and game design (placeholder graphics)

- `games/<name>/`: `Makefile` (`NAME`, `SRC`, `TESTS`, `include ../../runtime/rt.mk`),
  `<name>.h` (state struct and constants, shared with the tests), `<name>.c`, `test_<name>.c`.
  Reference: `games/life/`, `runtime/demo/`.
- The whole game state in one struct pointed to by `rt_state` (enables `--save`/`--load`).
- Separate the logic (`game_update`, pure functions such as `physics_step`, `collide`) from the
  drawing; `game_render` uses only `draw_rect`/`draw_text` placeholders at this step.
- **Injection door**: `game_scenario(n)` jumps to useful states (0 = normal start, then e.g.
  just before a collision, high score, max difficulty, game over). List them in the README.
- Check: `make test`.
  - **Unit tests** (`sw_init(n)` + `sw_step(keys)` + asserts, no window): physics over N frames,
    collisions at the edges, scoring, state transitions, difficulty, determinism with a fixed seed.
  - **Integration tests**: scripted runs, `./<name>_pc --headless --scenario N --keys <script>
    --frames F` and a pinned final checksum or state.

## 2. Run it for real on the PC

`make pc && ./<name>_pc` (SDL window), and scripted headless runs with `--shot` read back to
confirm the placeholders behave. Fix the feel (jump height, speed) against the upstream game.

## 3. TI, benchmark without UI

Without the emulator first: run the TI binary under `tools/bin/ti-cycles` (datasheet cycles per
marked zone, the screen from memory): `make cycles` (`-DRT_CYCLES`, zones update and render) on
the worst-case scenarios (most objects on screen), and `make xcheck` (same screen checksum as the
PC per scenario, `KEYS=` script, `TI_FILES=` data files). Read the printed cycles per frame and
checksums, not a drawing; `--png` only when a checksum differs. `make bench` (`<name>b.89z`,
`TI_ARGS=N ti-run`) only for what needs the real hardware path (grayscale driver cost).
Budget: ~360k cycles per frame at 30 fps on hardware (grayscale driver included); TiEmu
under-counts `movem` and shifts, so keep a margin (`ti68k-performance.md` §1). Over budget:
optimise from the knowledge base (tables, redraw only what changes, TileMap, pre-shifted
sprites), measure again. Program size: note it (TI-89 AMS 2 limit 24,576 bytes).

## 4. TI for real

The last step, once (`CLAUDE.md`, development flow: the emulator is slow, avoid it until
everything passes without UI). `make ti`, `ti-run <name>.89z` (Titanium). Keys for a `_rowread` game: `ti-key --hold 0.4 …`.
Stuck modifier or a command that does not arrive: `ti-emu restart`. Check with printed numbers
or one screenshot read back (`ti-shot x.png --lcd`), and that ESC returns to HOME.

## 5. Graphics (SDL first)

- Draw the sprites, tiles and fonts in 4 greys (`RtSprite` light/dark/mask, `RtTilemap`), sized
  for 160×100. Upstream art can be a starting point (check the licence, keep copyrighted art out of
  git or the repo private), usually redrawn at this resolution.
- **Visibility**: white outline on the main character and the obstacles/enemies (mask dilated by
  one pixel, `ti68k-c-patterns.md` § sprites); contrast the playfield against the background
  (plain or light-grey scenery behind black/dark sprites); nothing that matters under 2 pixels;
  the score readable over everything.
- **Compare variants**: build 2 to 4 styles (e.g. `-DSTYLE=n`, or one scenario per style),
  take the same headless screenshot of each (`--headless --scenario N --frames F --shot
  shot-<style>.png`, also with `-DRT_MONO`), read them all, pick the best, give the user the
  reasons in a few words, keep only the winner in the code.
- Check: tests still pass (graphics must not change the logic), `make pc` looks right.

## 6. TI again

Without UI first: `make cycles` (the graphics cost: same scenarios, compare with step 3) and
`make xcheck` (a differing checksum = the TI render differs from the SDL one: fix it there, with
`ti-cycles --png` to see it). Then, once, `make ti` and a real run with one screenshot read back.

## 7. Conclude

Short report to the user: upstream version, what was kept or changed from the original, test
count, cycles per frame (before/after graphics), `.89z` size, known limits, scenario list.

## 8. Update the knowledge

- `games/<name>/README.md`: spec, controls, scenarios, build, measured numbers.
- Root `README.md` and `CLAUDE.md` layout: the new game.
- New **verified** platform facts (a cost, a bug, a trick) in `ti68k-performance.md` or
  `ti68k-c-patterns.md`; a porting lesson in this `SKILL.md`; a runtime gap fixed in
  `runtime/README.md`.

## Lessons (Flappy Bird, `games/flappy/`)

- **Scaling**: scale the vertical to the playfield (Flappy 0.22) but keep the horizontal in
  *time* (pick an integer-friendly scroll speed, then distances = upstream durations × speed):
  a uniform 0.22 made the pipes 11 px wide and the lookahead 5 s. Convert per-frame values
  with the frame ratio (60 → 32 fps: velocities × 1.875, accelerations × 1.875²), then check
  the discrete result against upstream's discrete one in a test (the flap apex came out 10.4 px
  instead of 9.9 until the impulse was retuned).
- **Autopilot scenario**: a scenario that plays by itself (`auto_pilot` in the state) gives the
  bench a live state, a long-run integration test (invariants every frame, replay identity) and
  a demo. It may still die: the bench must not rely on it (next lesson).
- **Bench the worst state explicitly**: `RT_BENCH` renders the final state; choose scenario +
  `BENCH` count so that it is the heaviest screen, check that state on the PC with `--frames`.
- **Graphics cost is per call**: stripes of `draw_rect` cost more than a whole sprite; draw
  repeated shapes as sprites (pipe bodies as a column of identical rows, `h` per call).
- **Pixel-art tilts**: RotSprite offline, 45° steps only at this size (c-patterns § sprites).
- `make pc NAME=x` builds `x_pc`: handy to keep one binary per style while comparing.
- The emulator keeps a stuck modifier between runs (garbled typing, wrong toolbar):
  `ti-emu restart` before sending.

## 9. Commit

Invoke `/ti-commit` (engine, tests, graphics, docs as separate commits when they stay coherent).
