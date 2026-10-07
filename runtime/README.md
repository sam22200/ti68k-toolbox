# Portable Game Runtime (roadmap phase 1)

One C game engine, two targets: the PC (SDL2) to iterate and test, the TI-89 / Titanium to ship.
A game includes only `core/rt.h` and implements four hooks; everything else is the runtime's.

```
runtime/
  core/rt.h, rt_core.c        API, shared state, PRNG, rectangle clip, camera clamp
  platform-sw/rt_sw.c         PC software renderer (ExtGraph plane format, bit-exact), PNG, state
                              files, input scripts; used by the SDL backend and by unit tests
  platform-sdl/rt_sdl.c       PC window, keyboard, options (below)
  platform-ti68k/rt_ti.c      calculator: NOSTUB _main, GrayDBuf, ExtGraph sprites, TileMap
                              engine, AMS fonts read in place, 256 Hz tick, keyboard matrix
  rt.mk                       make rules for every game: pc, test, ti, bench
  tests/                      runtime self-tests: test_runtime.c (PC), xcheck.c (PC = TI),
                              rbench.c (cost of each primitive on the TI)
  demo/                       minimal game using every piece, with its unit tests
  tools/amsfont.py            extracts the AMS fonts for the PC build (platform-sw/amsfont.h,
                              made by make from tools/rom/TI89Titanium_OS.89u; not in git)
```

## The game side

```c
#include "../../runtime/core/rt.h"
void game_init(void);        // set EVERY global here: on the TI statics survive between runs
void game_scenario(u16 n);   // injection door: n = 0 normal start, n > 0 = a precise test state
u8   game_update(void);      // one fixed step (256 / RT_FRAME_TICKS Hz, 32 fps); 0 = quit
void game_render(void);      // draw the whole frame
```

- Frame rate: `RT_FRAME_TICKS` ticks of 256 Hz per frame (8 = 32 fps). `-DRT_FRAME_TICKS2=17`
  (in `CFLAGS_GAME`, for every file) alternates frames of 8 and 9 ticks: 30.1 fps, a PICO-8
  cart's 30 fps (`games/celeste`); the PC's virtual clock follows it.
- Input: `input_held(K_UP)`, `input_pressed(K_A)`, `input_released(...)`, one snapshot per
  frame. Keys: arrows, `K_A` [2nd], `K_B` [shift], `K_C` [◆], `K_D` [alpha], `K_ENTER`, `K_ESC`,
  `K_DIGIT(1..9)` (keypad grid, for gestures). PC: arrows, Ctrl/Space/Z, Shift/X, C, V, Enter,
  Esc, keypad or number row. The macros return the `u32` key bits: `K_ENTER`, `K_ESC` and the
  digits are above bit 7, so `u8 go = input_pressed(K_A | K_ENTER)` loses them; write `!= 0`.
- Drawing (hidden planes, double-buffered): `draw_clear`, `draw_rect(x, y, w, h, C_*)`,
  `draw_sprite(x, y, &RtSprite)` (8/16/32 wide, masked or opaque, clipped), `draw_text(x, y, s,
  F_SMALL|F_MEDIUM, C_*)`, `draw_tilemap(&RtTilemap, camx, camy)` + `tilemap_dirty()`
  (the TI rebuilds its plane when `map`, `tiles` or `w` change, so one struct refilled per room works).
  Data formats are ExtGraph's (see `rt.h`): the same arrays work on both targets.
- `rt_rand()` (wyhash16, deterministic from `rt_seed`), `rt_ticks()` (256 Hz; virtual on the PC).
- `rt_file(name, &size)`: big read-only data outside the program (64 KB per variable): on the TI
  the variable NAME (OTH, `ttbin2oth -89 dat NAME.be.bin NAME`, read in place, archived or
  locked in RAM), on the PC the file `NAME.bin` in the host byte order. Returns RT_NULL when
  missing. Every `draw_*` writes through `rt_light`/`rt_dark` on both targets, so a game can
  point them at its own plane-format buffer (`RT_PSIZE` bytes each, even address) to render
  once (FFA's dialogue box, Desolate's room background).
- `rt_load(name, data, size)` / `rt_save(name, data, size)`: a game's save. PC: `NAME.sav` at
  once. TI: the OTH variable NAME ("sav"), written **when the program exits**, after the
  grayscale teardown, then archived (c-patterns §10: SymAdd and Flash writes may need AMS
  dialogs); keep `data` valid until then. Verified on the Titanium (FFA: save, quit, Continue).
- Mono build: `-DRT_MONO` (sprites and tiles draw their dark plane in black).
- State: set `rt_state`/`rt_state_size` to a POD struct in `game_init` for PC save/load.

## Game Makefile and workflow

```make
NAME := flappy            # calculator name, <= 8 chars (bench builds NAMEb: keep <= 7)
SRC := game.c
TESTS := test_game.c      # defines main(), drives frames with sw_step()
include ../../runtime/rt.mk
```

1. **Unit tests first** (`make test`): no window. `sw_init(scenario)`, then `sw_step(keys)` per
   frame, assert on the game state; `sw_level(x, y)` / `sw_checksum()` for pixels.
2. **PC window** (`make pc && ./NAME_pc`). Options: `--scenario N`, `--load F` / `--save F`
   (raw state), `--keys F` (input script, lines `<frame> <keys...>` held until the next line,
   e.g. `0 RIGHT` / `30 RIGHT A` / `60`), `--frames N`, `--headless`, `--shot F.png`, `--seed N`,
   `--scale N` (scripts up to 32,768 lines). In the window: F2 save state, F3 load, F12 screenshot, Tab 8× speed, P pause,
   O single step. `--headless --keys k.txt --frames 90 --shot s.png` renders without a window.
3. **Calculator last of all** (`make ti`, then `ti-run NAME.89z` or type `NAME(3)` for scenario 3),
   on the Titanium, once everything passes without UI (the emulator is slow: screenshots); the
   TI-89 HW2 only for a release.
4. **The TI binary without the emulator**: `make cycles` builds `NAMEc.89z` (`-DRT_CYCLES`: no
   grayscale, interrupts or keyboard). Its `rt_ticks()` advances deterministically
   with the same virtual256Hz clock as the PC, including16-bit wrap.
   `tools/bin/ti-cycles` runs the binary on the PC with datasheet
   cycles per update and render, `--keys`/`--frames` as the PC, data files with `--file`, saves
   written with `--save-dir`. `make xcheck XCHECK="0 1 2" FRAMES=100 KEYS=k.txt` compares its
   screen checksum with the PC `--headless` run, scenario by scenario. In the emulator, `make bench` builds `NAMEb.89z`; `NAMEb(S)` runs
   RT_BENCH updates then renders from scenario S and prints cycles per frame (TiEmu counts:
   optimistic for ExtGraph code, performance §1). `make bench BENCH=8` sets the count; the renders
   draw the *final* state, so pick S and BENCH so that it is the heaviest screen.

## Verified (Titanium, TiEmu, 2026-09-27)

- `tests/xcheck.c`: 8 scenes (every primitive, colour, clip edge, both fonts, tile map with
  clamped camera) give the same Fletcher-16 on the PC and on the calculator, grey and mono. The
  sums are pinned in `test_runtime.c`, so `make -C runtime/tests test` guards the PC renderer.
- Costs (`rbench`, TiEmu cycles): full tile map 81k; 10 masked 16×16 sprites 46k; 10 masked 8×8
  27k; 160×100 rectangle 37k; 6 chars of text 10–11k; clear 3k (TiEmu; ~17k real). Demo frame:
  update 0.4k, render 95k of a 375k budget.
- Found on the way: ExtGraph 2 `ClipSprite8_MASK_R` draws garbage when `x & 15` is 9..15 (worked
  around in `rt_ti.c`); AMS `free(NULL)` crashes (Address Error); `DrawStr` is ~15k cycles per
  character on two planes, hence the in-place font; drawing tiles one by one cost 254k per screen,
  hence the TileMap engine.
