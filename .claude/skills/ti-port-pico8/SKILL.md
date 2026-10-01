---
name: ti-port-pico8
description: "Port a PICO-8 game (a .p8 / .p8.png cart from the Lexaloffle BBS, itch.io or GitHub, an HTML/JS export, or a port of one such as Celeste Classic) to the TI-89 Titanium through the Portable Game Runtime: get and decode the cart (shrinko8), extract code, sprite sheet, map and flags, run the original cart headless on the PC under z8lua to record reference traces, grill the user on the port decisions (/grilling: 128x128 into 160x100, 16 colours into 4 greys, 30/60 fps, 16.16 numbers, controls), then translate the Lua into fast C (bit-exact 16.16 logic diffed against the traces, native rendering), test on the PC (unit, trace, SDL), measure the TI binary under ti-cycles, the emulator last. Use it whenever the user wants to port, convert, \"porter\" or remake a PICO-8 / pico8 / fantasy-console game, mentions a .p8 file, a lexaloffle cart, Celeste Classic or another PICO-8 title for the calculator, even without saying \"PICO-8\"."
---

# ti-port-pico8 (port a PICO-8 cart to the TI-89)

Same spirit and steps as `ti-port-sdl` (read it first: upstream = specification, runtime =
target, every step ends with a check, PC first, TI last, French to the user, English in the
files). What changes with PICO-8:

1. **The source is always there.** A PICO-8 cart *is* its source: Lua code, sprite sheet, map,
   flags, sounds in one file. A `.p8.png` (the BBS download) decodes losslessly; an HTML
   export embeds the cart. "Without sources" almost never means "no cart": find the cart first.
2. **The original runs on the PC.** z8lua (Lua with PICO-8 syntax and 16.16 numbers) plus
   `scripts/p8shim.lua` runs the cart headless: `scripts/p8trace.py` prints the game state frame
   by frame for a key script. Those traces are the reference the C port is diffed against:
   the tests check the real game, not our reading of it.
3. **The machines are close but not equal** (128×128 vs 160×100, 16 colours vs 4 greys,
   30/60 fps vs 32, 16.16 Lua numbers vs a 68000 without FPU): the fitting decisions *are*
   the port. They are put to the user with `/grilling` before any C.
4. **Translate the logic, rebuild the rendering.** The Lua becomes C function by function
   (same order, same numbers); the drawing becomes native (pre-rendered rooms or TileMap,
   pre-converted sprites, palette swaps baked at build time). No Lua VM or PICO-8 API layer
   on the TI: it would cost tens of times the frame budget.

Constraints (`CLAUDE.md`): **1. performance** (the knowledge base, verified asm in `lib/`,
ExtGraph, TileMap, `experiments/` verdicts, smallest types, no float, no division or 32-bit
product in a loop), **2. visibility** (160×100, 4 greys, white outline on the main sprites).
Titanium only; the TI-89 HW2 for a release. **ASM only after asking**, on a measured hot spot.

References (read when the step says so):
- `reference/pico8-facts.md`: the PICO-8 machine and API semantics a port depends on, cart
  formats, where carts are, existing ports, pitfalls.
- `reference/grilling.md`: the decision tree to grill (with defaults and the data each
  question needs).
- `reference/lua-to-c.md`: translation patterns (state kinds, program shape, numbers,
  objects and lists, idioms, API → runtime table, the trace test).
- `assets/p8num.h`: PICO-8 numbers in C, bit-exact with z8lua (`scripts/test_p8num.sh`),
  68000-friendly (no `__mulsi3`).

## 0. Tools (once per machine)

```sh
git clone --depth 1 https://github.com/thisismypassport/shrinko8 tools/shrinko8
git clone --depth 1 https://github.com/samhocevar/z8lua tools/z8lua && make -C tools/z8lua
sh .claude/skills/ti-port-pico8/scripts/test_p8num.sh     # p8num.h = z8lua
```
Both are local, not in git (`.gitignore`). Python: `tools/pyenv/bin/python` (PIL for PNGs).

## 1. Get the cart

- Find the cart, in this order: the Lexaloffle BBS post (the cart PNG is
  `https://www.lexaloffle.com/bbs/cposts/<id / 10000>/<id>.p8.png` for a numeric cart id, or
  `cposts/<first 2 chars>/<id>.p8.png` for a named one; the id is in the post's page), the author's GitHub/itch.io (`.p8`), an
  HTML export (`shrinko8 -F js game.js out.p8`). Other-language ports (C#, C, JS) are a second
  opinion, or a translation base when they are line by line (ccleste's `celeste.c` for
  Celeste: its floats become `fix`, the traces still come from the cart). They drift: the C#
  Celeste Classic uses floats and flag indices for the map layers, and one flag differs.
- Truly no cart (a commercial game, a video only): say so, and fall back to `ti-port-sdl`'s
  method (spec from observation) or to a C port as upstream; the trace tests are then lost.
- Keep it in `sources/<name>_p8/` (read-only, not in git), note URL, post date, author,
  licence in `games/<name>/README.md`. A BBS cart without a licence tag (Celeste) is all
  rights reserved: the cart and everything generated from it stay local; only carts tagged
  CC4-BY-NC-SA allow a non-commercial port with attribution.
- Decode and extract:
  `tools/pyenv/bin/python .claude/skills/ti-port-pico8/scripts/p8extract.py sources/<name>_p8/<cart>.p8.png sources/<name>_p8/x`
  → `code.lua`, `sheet.png`, `sheet_grey.png`, `map.png`, `sprites.bin`, `map.bin`,
  `flags.bin`, `mem.bin`, `info.json` (frame rate, callbacks, API census, features used:
  palette swaps, map layers, `peek`/`poke`, coroutines, `cartdata`…). Minified code is
  unminified automatically.
- Check: `p8trace.py` runs the cart without a Lua error for a few hundred frames with an
  empty key script and with a short play script (`--trace` of a global the game moves).

## 2. Understand

- Read `code.lua` entirely (it is short: a cart holds ≤ 8192 tokens). List: globals, object
  types and their fields, the per-frame order (`_update`, the object loop, what `_draw`
  mutates), states (title, play, death, room change, ending), rooms/levels (`map.png`).
- Read `reference/pico8-facts.md` § pitfalls against the census: every feature the cart uses
  has a line there.
- **Measure, do not derive**: jump apex, dash length, run speed, coyote frames, the max number
  of objects, which `rnd` calls touch gameplay: script them through `p8trace.py`
  (`--pre` jumps to a room, `--lib` adds helper functions for `--trace`).
- Count what the decisions need: distinct 2×2 cell blocks of the map, sprites per palette
  swap, colours carrying gameplay information, rows of a room outside a 100-pixel window.
- Write the draft spec in `games/<name>/README.md` (states, objects, constants, controls,
  rooms, measured numbers).

## 3. Grill (before any conversion)

Invoke `/grilling` (skill `grilling`) on the port plan, with `reference/grilling.md` as the
tree: rounds of numbered questions, each with the measured data and a recommended answer,
the facts looked up by you (sub-agents if needed), the decisions left to the user. Record the
answers in `games/<name>/README.md` § Port decisions. Do not start step 4 before the user
confirms the summary.

## 4. Engine and logic, against the traces (placeholder graphics)

- `games/<name>/`: `Makefile` (`include ../../runtime/rt.mk`), `<name>.h` (state struct,
  constants), `<name>.c`, `p8num.h` (copied from `assets/`), `test_<name>.c`, `keys/*.txt`,
  `traces/` (references, if the licence allows).
- Translate per `reference/lua-to-c.md`: gameplay state exact (16.16 where the cart has
  fractions), cosmetic state cheap, `_draw` mutations moved to the update, objects in a fixed
  pool with PICO-8's list order and iteration semantics, `rnd` = PICO-8's own generator (`p8_rnd`, `p8_rndi`).
- **Injection door**: `game_scenario(n)` = a room / state, matching a `p8trace.py --pre`
  line (list both in the README).
- Check: `make test`.
  - **Trace tests**: for each key script, the C trace equals the `p8trace.py` trace frame by
    frame (first divergent frame and field printed). Start with one mechanic per script, end
    with the play-through, then add **seeded random key scripts** (a generator in the game's
    `tools/`, a dozen scripts of ~1,500 frames): they reach states nobody scripts.
  - **Coverage gate**: the test fails when a mechanic never happens in any script (each dash
    direction, a death, a respawn, a room exit…; read it from the trace, e.g. the `__sfx`
    events or a state field).
  - **Mutation check**, once per mechanic: change one constant by one bit or move one update a
    frame later, and see the trace tests fail; a test that still passes proves nothing.
  - **Unit tests** for what the traces do not see (pool overflow, room transitions, the
    scenario door, the save).

## 5. PC for real

`make pc && ./<name>_pc` (SDL), and headless runs with `--shot` read back only to confirm the
placeholders. Feel check against the original in a PICO-8 web player if one exists (the BBS
post plays in the browser).

## 6. TI without UI

As `ti-port-sdl` §3: `make cycles` under `ti-cycles` on the heaviest scenarios (most objects,
effects, a room change), `make xcheck` (TI screen checksum = PC per scenario), and a **state
hash** per frame printed by the TI binary (`-DSTATE_HASH`, `BENCH_VALUE` under `ti-cycles`)
compared with the PC's: the screen does not show the 16.16 state, and GCC4TI's 16-bit `int`
breaks code that passes every PC test (Celeste: `-0xb505` became `0x4afb` on the TI only). Budget ~360k
cycles per frame at 30 fps with grayscale. Logic first (the update alone, the 16.16 code:
`p8_mul`/`p8_div` counts), then the render. Over budget: the order decided in the grilling
(redraw only what changes, fewer effects, then asm on the measured spot after asking).
`make bench` in the emulator only for the hardware paths.

## 7. Graphics (SDL first)

- Convert the sheet with a build tool (`tools/gfx.py` reading `sprites.bin`, `map.bin`,
  `flags.bin`): the decided colour → grey mapping, flipped and palette-swapped variants baked,
  the room pre-render or the metatile set, the white outline on the player / enemies /
  collectibles (`ti68k-c-patterns.md` § sprites), references in `ti-art-refs`.
- Fit the screen as decided (the 160×100 view camera, the HUD in the side bands).
- Compare 2 to 4 variants on the same headless shot (grey and `RT_MONO`), keep the winner.
- Check: the trace tests still pass (graphics never change the logic), `make pc` looks right.

## 8. TI again, then the emulator once

`make cycles` (compare with step 6) and `make xcheck`; then once `make ti`, `ti-run
<name>.89z` on the Titanium, one screenshot or printed numbers, the controls (both buttons
with two arrows held), a room change, ESC back to HOME.

## 9. Conclude, knowledge, commit

Report (French): cart and version, decisions, what was kept or changed, trace scripts and
test counts, cycles per frame (logic / render, before and after graphics), `.89z` size, known
limits, scenarios. Update `games/<name>/README.md`, the root `README.md` and `CLAUDE.md`
layout, verified platform facts in the knowledge base, a lesson below, then `/ti-commit`.

## Lessons

- **Celeste Classic** (BBS tid 2145, cart `cposts/1/15133.p8.png`, 1,429 Lua lines, 30 fps):
  decodes with shrinko8 and runs under `p8trace.py` (title, room 0, spawn, run, jump, death
  traced). The cart changes state in `_draw` (player x clamp, hair). Map layers are bit masks
  (`map(..., 4)` = flag 2). The repository's `Classic.cs` is a float port: do not test against it.
- z8lua: `cos(0)` and `sin(0.75)` return ±1.1055 (its table is read one past the end at the
  quarter turns); `p8shim.lua` clamps them. Verified 2026-10-01.
- PICO-8 decimal literals truncate (0.6 = 0x0.9999); `/` truncates toward zero while `*`
  floors (`-0x0.0001 / 2 = 0`, `-0x0.0001 * 0.5 = -0x0.0001`): `p8num.h` follows both.
- GCC4TI compiles `(s32)a * b` on 16-bit halves into `__mulsi3` calls: `p8num.h` uses inline
  `muls.w`/`mulu.w` (`p8_mul` ~380 cycles under ti-cycles instead of three library calls).
- **Celeste room 0** (`games/celeste/`, 2026-10-01): 22 scripts / 19,713 frames bit-exact, TI =
  PC by state hash, ~100-112k cycles per frame. What worked: positions as `s16` with the
  fraction kept only where the cart makes one (the fruit's bobbing y, and the "1000" text born
  there: a trace test caught it); `collide()` returning at once for a type absent from the
  room (`ntype[]`); the cosmetic `rnd` (smoke, shake) on `rt_rand` and out of the traces; the
  room pre-rendered once, 100 byte-aligned rows copied per frame with the HUD bands cleared in
  the same pass; the runtime's `RT_FRAME_TICKS2=17` (30.1 fps). A ccleste TAS converted to a
  key script (`tools/tas2keys.py`) gave the exit script nobody would write by hand. Mutations
  that no script caught pointed at missing scripts (the fruit's 40-frame bob: `fruitbob`).
- A Makefile rule that rebuilds `gfx.h` must also be a prerequisite of `NAME_pc`,
  `NAME_test` and `NAME.89z`: rt.mk's targets depend on the sources only, so a new style
  silently kept the old binary.
