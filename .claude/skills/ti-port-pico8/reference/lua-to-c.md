# From the cart's Lua to runtime C

The cart is translated **by hand, function by function**, into C against `runtime/core/rt.h`
and `assets/p8num.h`: no Lua VM and no PICO-8 API emulation on the TI (an interpreter costs
50 to 100× the C, and a generic `spr`/`map`/`pal` layer redraws what a native renderer keeps).
The logic is translated literally (same order, same numbers); the drawing is rebuilt natively.

## Contents

1. Two kinds of state
2. Program shape
3. Numbers
4. Objects and lists
5. Lua idioms
6. Drawing: API → runtime
7. The trace test (C side)

## 1. Two kinds of state

Sort every variable of the cart before writing C:
- **Gameplay state**: whatever can change what happens next (positions, speeds, timers,
  counters, flags, room, the objects list, the rnd seed if gameplay draws from it). Exact
  16.16 when the fidelity decision is "exact" (`grilling.md` Q3), printed in the traces,
  diffed against `p8trace.py`.
- **Cosmetic state**: what only changes pixels (Celeste: hair nodes, clouds, snow particles,
  smoke drift, the shake offset). Free to approximate (8.8, integers, fewer particles, its
  own PRNG) for speed; never traced. Check that it is really cosmetic: grep every read of it.
  A cosmetic stream that draws from `rnd` shifts the gameplay stream when it is removed: give
  it its own generator in **both** the reference (`--lib`) and the port. Default to that when
  the cosmetic draws are many per frame (Celeste: clouds, snow, smoke; only the balloon offset
  and the chest position are gameplay): keeping the whole stream exact cost ~40k cycles of
  update per frame against ~7k without it, in the two Celeste test ports.

## 2. Program shape

| Cart | Port |
|---|---|
| `_init()` | `game_init()` (set **every** global: TI statics survive between runs) |
| a test state | `game_scenario(n)`, the same as `p8trace.py --pre "<lua>"` |
| `_update()` / `_update60()` | `game_update()` (one or two calls per frame, `grilling.md` Q6) |
| `_draw()` state changes | end of `game_update()`, in the cart's order |
| `_draw()` drawing | `game_render()`, pure (reads the state, writes pixels) |

Carts change state inside `_draw` (Celeste clamps the player's x in `player.draw`, moves the
hair, scrolls the clouds and particles, ticks the title flash): find every assignment
reachable from `_draw` and move it to the end of the update, keeping the order. The trace
tool runs `_draw` after `_update` by default for that reason.

All globals in one POD struct (`rt_state`, `rt_state_size`) so that `--save`/`--load` work and
`game_init` resets it with one `memset` plus the cart's initial values.

## 3. Numbers

- Every Lua number is a `fix` (16.16) until proven otherwise: a value that only ever holds
  small integers (counters, indices, frame timers, sprite numbers) becomes `u8`/`s16`; a
  value that ever gets a fraction stays `fix`. Grep its assignments; when unsure, `fix`.
- Values that are integers by construction (Celeste: `x`, `y` move by whole pixels, the
  fractions live in `rem`): `s16` plus a `fix` remainder; tile lookups and collision boxes
  then work on 16-bit integers, not on 16.16 shifts.
- **GCC4TI's `int` is 16 bits**: any literal above 32767 or negated hex needs an `L`
  (`-0xb505L`; `FIXB` adds it); `(s32)a * b` with a 16-bit `b` may still call `__mulsi3`:
  read the asm (`tigcc -S`).
- Constants: integers `FIX(n)`; decimals with the **bits PICO-8 gives them** (truncated):
  print them all with z8lua, e.g.
  `grep -oE '\b[0-9]*\.[0-9]+\b' code.lua | sort -u | while read v; do echo "__out('$v', tostr($v,true))"; done > /tmp/c.lua && tools/z8lua/z8lua /tmp/c.lua`
  then write `FIXB(0x35c2) /* 0.21 */`.
- Operators: `+ - < <= ==` as is on `fix`; `a * b` → `p8_mul` (or `p8_muli` when one side
  is an integer, or folded by hand when both are constants: `5 * 0.70710678118` is
  `FIXB(0x38914)`, the bits z8lua computes); `a / b` → `p8_div` (by `2^k`: `p8_div2k`, which
  truncates like PICO-8: `-0x0.0001 / 2 = 0` but `-0x0.0001 * 0.5 = -0x0.0001`); `a % b` →
  `p8_mod` (by a power of two: `p8_mod2k`, one AND, result always ≥ 0); `a \ b` → `p8_idiv`;
  `flr`, `ceil`, `sgn` (**sgn(0) = 1**), `abs`, `min`, `max`, `mid` → `p8_*`.
- Division by a constant that is not a power of two (Celeste hair: `/1.5`): exact needs the
  division; if the value is cosmetic, multiply by the reciprocal instead.
- Drawing coordinates: PICO-8 floors them: `FIX_INT(x)` (arithmetic shift = floor).
- `rnd(x)` → `p8_rndi(n)` for an integer n (one `divu.w`), `p8_rnd(x)` otherwise, `srand` →
  `p8_srand`: PICO-8's own generator, as `p8shim.lua`, so the C, the reference and PICO-8
  draw the same numbers (start state `p8_srand(0)` in `game_init`). `rnd(table)` = `t[flr(rnd(#t))]`.
- `sin`/`cos` (turns, **sin inverted**: `sin(0.25) = -1`), `atan2(dx, dy)` (turns, y down):
  a table generated with `ti-table`, and the **same table** loaded in the reference with
  `p8trace.py --lib` (a Lua function reading the same numbers), so that the traces stay
  equal; the error against the real PICO-8 (a 1/256 turn) does not matter, the equality does.

## 4. Objects and lists

The usual cart pattern: `types = {player, spring, ...}`, each a table with `tile`, `init`,
`update`, `draw`; `objects` a list; `init_object(type, x, y)` adds, `destroy_object` dels;
`foreach(objects, f)` or `for o in all(objects)` iterates.
- One `struct Obj` with the common fields (`type` as a `u8`, `x`, `y`, `spd`, `rem`, hit
  box, flags) plus a `union` of the per-type fields; a fixed pool `Obj objs[MAX]` and a
  count. `MAX` = the highest `#objects` over the play-through traces (trace `#objects`) plus a
  margin; overflow is an assert on the PC.
- **Order matters** (collisions and updates happen in list order): `add` appends, `del`
  removes and **shifts the rest down** (`memmove`), never swap-with-last.
- **Iteration semantics**: PICO-8's `all`/`foreach` survive a deletion: when the list got
  shorter since the last step, the index does not advance (`p8shim.lua` `all`). Reproduce it
  exactly in C (`i` advances only if `n_now >= n_before`): a removal of an *earlier* object
  then visits the next one correctly, a removal of a *later* one makes the loop visit the
  current object again, objects added during the loop are visited at the end. Write the C
  loop once (`for_all_objects`) and use it everywhere the cart uses `all`/`foreach`.
- Type dispatch: `switch (o->type)` in `obj_update` / `obj_draw`; per-type constants (`tile`,
  `if_not_fruit`) in a `const` table indexed by type.
- Methods with `this` → functions taking `Obj *`; `this.collide(type, ox, oy)` returning an
  object or nil → `Obj *` or `RT_NULL`.

## 5. Lua idioms

- `a and b or c` is a ternary **only if b is never false/nil** (else it yields c).
- `x ~= nil`, `if x then` (only nil and false are false: `0` is true in Lua!).
- 1-based tables (`t[1]`, `#t`), inclusive `for i = a, b` loops: off-by-one at every bound.
- Tables used as sets (`got_fruit[1 + level_index()] = true`) → bit sets.
- String building (`"x" .. score`) → a small integer-to-text routine (no `sprintf` per frame).
- Closures stored in tables, `setmetatable` OOP → explicit structs and functions.
- Coroutines (`cocreate`/`yield`) → an explicit state machine or the protothread macros of
  FFA's story scripts (`ti-port-tibasic` lessons): the resume point is in the state.
- `btn(i)` → `input_held(KEY)`, `btnp(i)` → `input_pressed(KEY)` plus PICO-8's repeat
  (15 frames, then every 4) if the game relies on it (menus); edge detection the cart does by
  hand (`jump = btn(k_jump) and not p_jump`) is translated literally.
- `t()`/`time()` → frames / 30 (or 60) in `fix`, from a frame counter in the state.
- `peek`/`poke` on the draw state (0x5f00..) → explicit variables (palette, camera);
  elsewhere → the data array it addresses (`mset` → the room's cell array, which also
  invalidates the pre-rendered room or the tile map: `tilemap_dirty()`).

## 6. Drawing: API → runtime

| PICO-8 | Port |
|---|---|
| 16 colours | 4 greys via the decided mapping (`grilling.md` Q9), applied **at build time** |
| `spr(n, x, y, w, h, fx, fy)` | `draw_sprite` of a pre-converted `RtSprite` (8 wide; w×h groups as 16/32 wide); flipped variants generated at build time (only the ones the cart uses) |
| `pal(a, b)` swaps for a sprite | a pre-baked variant per (sprite, swap) used: never per pixel at run time |
| `pal()` whole-screen flashes | a variant set or a cheap global effect (invert, fill), decided per effect |
| `palt` | the mask plane (colour 0 transparent by default) |
| `map(cx, cy, sx, sy, w, h, layer)` | the room pre-rendered once (static rooms) or an `RtTilemap` of 16×16 metatiles; `layer` = a flag mask (**bit mask**, not a flag index) chosen at build time |
| `rectfill` | `draw_rect` (inclusive corners in PICO-8: w = x2 − x1 + 1) |
| `circfill(r <= 3)` | small disc sprites |
| `line`, `pset` | `draw_rect` for horizontal/vertical lines; anything else: a runtime extension (measure) |
| `print` | `draw_text(F_SMALL)` (AMS 4×6, variable width: recentre the texts) or a 3×5 sprite font for the PICO-8 look |
| `camera(x, y)` | an offset the game adds to its draw calls (shake), plus the view camera of the 160×100 fit |
| `clip` | not in the runtime: avoid, or extend it |
| `fillp` | dithered sprites or rectangles |
| `cls(c)` | `draw_clear` or the room background copy |
| `sfx`, `music` | nothing (kept in the trace as events) |
| `pget`, `sget` | never read the screen; `sget` reads the const sprite data |

Build-time conversion: a `tools/gfx.py` (as `games/flappy/gfx.py`) reads `sprites.bin`,
`map.bin`, `flags.bin` from `p8extract.py` and writes the C arrays (`gfx.h`) or a data file
for `rt_file` when the program grows past the AMS 2 limit (24,576 bytes).

## 7. The trace test (C side)

- `test_<name>.c` runs the same key script as the reference (`sw_load_script`,
  `sw_script_keys`, the runtime's format), maps the keys like `p8trace.py --map`, and prints
  one line per frame: the frame number, then the same expressions in `p8_hex` format.
- `make trace` (or a test) diffs it with the reference produced by `p8trace.py ... --trace`
  for each script; on a difference, print the **first divergent frame and field**: that frame
  is where the translation is wrong.
- Scripts: one per object type and mechanic (a jump, a wall jump, a dash per direction, a
  spring, a fall floor, a death and respawn, a room change), and the play-through. Keep the
  reference outputs in `games/<name>/traces/` if the licence allows it, else regenerate them
  from `sources/` in the Makefile.
