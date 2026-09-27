# Case study: Puzzle Bobble v0.3 (David Coz, 2002)

Source: `games/puzzle_bobble/` (from https://www.ticalc.org/archives/files/fileinfo/245/24579.html).
About 1,000 lines of C: a complete NOSTUB grayscale game using ExtGraph, a custom timer interrupt
and direct keyboard reading. It runs on the TI-89 HW2 and on the Titanium once recompiled with
GCC4TI and ExtGraph 2, after one fix. General lessons are in `ti68k-c-patterns.md`.

## Files

| File | Content |
|---|---|
| `bobble.c` | the whole game |
| `sprites_bob.h` | `long sprites[16][20]` (7 used): 10 light-plane rows then 10 dark-plane rows, 32-bit wide with the 11-pixel bubble in the top bits; `short bob_mask[10]` (inverted disc mask); `short spe_mask[4][10]` (4 erase-fade masks); `char border[52]` (wall tile: 26 light + 26 dark rows) |
| `trig_tables.h` | `int cos255[256]`, `int sin255[256]` scaled by 128 |
| `sprite_test.h` | `test_sprite()`: pixel collision against one plane |
| `bobble.tpr` | TIGCC IDE project: `-Os -Wall -W -Wwrite-strings`, links `extgraph.a` |

Build: `tools/bin/ti-cc -o bobble bobble.c`. Run: `ti-run games/puzzle_bobble/bobble.89z`, press a
key on the credits line, 2ND to shoot, arrows to aim, ESC to quit (CLEAR switches the calc off).

## Program flow (`_main`)

1. `LCD_save(screen)`, print credits on the status line, **`ngetchx()`** (AMS keyboard, still
   available: interrupts are not touched yet).
2. Reset game state, `malloc` two 3840-byte virtual planes `light`/`dark`, `calloc` the
   erase/fall work arrays (no NULL checks).
3. `ClearGrayScreen2B`, `randomize()`, pick the first two colours.
4. **Interrupt setup**, in this order:
   - `pokeIO(0x600017, 0xFC)`: auto-int 5 start value → period 257 − 0xFC = 5 counts of the
     ~1024 Hz base, **205 Hz** instead of AMS's 19.3 Hz (0xCC) (**verified**, patterns §4). This
     is the game's clock resolution.
   - save auto-int 1, redirect it to `DUMMY_HANDLER` (no AMS keyboard scan, no status-line
     indicators, and grayscale will chain to "nothing");
   - save auto-int 5, install `myint5handler`.
5. `calc()`: once, with `sqrt`/`atan2` (floats are fine outside the loop), precompute the
   polar coordinates of the aiming cursor's points.
6. `ClrScr(); GrayOn();` then draw the current and next bubble, the walls (`draw_border`), the
   initial 5 rows (`init()` → `refresh()`), the score panel (`draw_bubbles`).
7. Main loop: `while (display() != ESC);`.
8. Teardown: `free` everything, **`GrayOff()`**, restore the int-5 start value (originally 0xCC,
   then 0xB2 if `GetHardwareVersion() != 2`, which is wrong on the Titanium; our port saves and
   restores `PRG_getStart()`), restore int 1 and int 5, `LCD_restore(screen)`,
   `ST_showHelp(EXTGRAPH_VERSION_PWDSTR)` ("powered by ExtGraph…" on the status line).

The order matters: grayscale is switched off before int 1 is restored, and the vectors are
restored last. Every allocation and vector is undone on the single exit path.

## The clock (auto-int 5)

```c
volatile long int mseconds = 0, mseconds50 = 0;
volatile int seconds = 0, minutes = 0, resettime = 1, running = 1;
INT_HANDLER oldint5 = NULL;
DEFINE_INT_HANDLER(myint5handler) {
    if (resettime) { seconds = minutes = resettime = 0; }
    else if (running) {
        mseconds++; mseconds50++;
        if (mseconds == 19) { seconds++; ... }   // bug: mseconds is never reset, so this fires once
    }
    ExecuteHandler(oldint5);                     // keep AMS timers alive
}
```

- Every shared variable is `volatile`: required, or the optimiser keeps them in registers.
- Only `mseconds50` is really used, as a monotonically increasing tick counter.
- It chains to the AMS handler, so `OSRegisterTimer` still works (used by `wait()`), but it runs
  faster because of the 0xFC start value.

## The frame loop (`display()`)

```c
if (mseconds50 - frame < speed1) return 0;   // speed1 = 3 ticks per frame
frame = mseconds50;
if (ball.on) draw_ball();                    // move and collide the flying bubble
result = key();                              // read the keyboard
if (erasing.erasing) erase_ball();           // fading group
if (score_refresh) draw_bubbles();           // score panel only when changed
FastCopyScreen(light, GetPlane(0));          // publish the virtual planes
FastCopyScreen(dark,  GetPlane(1));
draw_cursor(cursor_angle, A_NORMAL, GetPlane(1));   // cursor drawn directly on the real dark plane
if (falling.erasing) fall_ball();            // falling bubbles also drawn directly on real planes
```

- A frame limiter on the interrupt tick, not a busy delay.
- Layers: the static scene lives in `light`/`dark`; transient things (cursor, falling bubbles) are
  drawn on the real planes after the copy, so they vanish at the next copy without any erase code.

## Keyboard (`key()`)

| Code | Key (TI-89 matrix) | Action |
|---|---|---|
| `_rowread(~((short)(1<<6))) & 1` | row 6 b0 = ESC | quit |
| `_rowread(0x7D) & 0x40` | row 1 b6 = CLEAR | `off()` (power off, resumes on ON) |
| `_rowread(0x7E) & 0x08` | row 0 b3 = RIGHT (comment says "Left") | `cursor_angle -= 1`, clamped |
| `_rowread(0x7E) & 0x02` | row 0 b1 = LEFT (comment says "Right") | `cursor_angle += 1`, clamped |
| `_rowread(~((short)(1<<0))) & (1<<4)` | row 0 b4 = 2ND | fire, if no ball flying, no animation running, and `mseconds50 - key_repeat > 50` |

- Held keys are read every frame: holding an arrow rotates continuously (speed = frame rate).
- Fire uses a **cooldown**, not edge detection: holding 2ND fires again after 50 ticks.
- The game-over screen waits with `while (!(_rowread(~((short)(1<<6))) & 1));`, i.e. a busy wait
  on ESC, because `ngetchx()` is unusable with int 1 redirected.
- In TiEmu, a key has to stay down across at least one frame: send it with `ti-key --hold 0.3`.

## Physics and maths

- Positions are fixed point `<< 4`: `ball.x = (LEFT_BORD+43) << 4`, drawn at `ball.x >> 4`.
- Direction from an 8-bit angle: `dx = -(BALL_SPEED * sin255[a]) >> 7`, `dy = -(BALL_SPEED *
  cos255[a]) >> 7` with `BALL_SPEED 30`, which gives about 1.9 px per frame.
- Walls: if `x` leaves `[LEFT_BORD+1, RIGHT_BORD-11]`, `dx = -dx` (+50 points per bounce).
- Collision: after each axis move, `test_sprite(x, y, sprite, 10, dark)` ANDs the bubble with
  the **dark virtual plane**. On a hit, step back and stop.
- Snapping: `search_pos()` converts the pixel position into a cell of the hexagonal grid
  `storage[12][15]` (odd rows shifted by half a bubble, `top_offset` flips the parity when a new
  row is pushed), then calls `three()`.

## Game logic on the grid

- `storage[row][col]` (0 = empty, 1–7 = colour) is the source of truth; `refresh()` redraws the
  whole field from it (AND with `bob_mask` then XOR the sprite), and also recomputes
  `allowed_color[]` (only colours still on the board can come next) and `game_over` (a bubble on
  row 10).
- `three(x, y)`: breadth-first flood fill over the 6 hex neighbours (`next_x/next_y` tables) to
  collect the same-colour group. If more than 2, move them to `erasing` and call `fall()`.
- `fall()`: flood fill from every top-row bubble; whatever is not reached is floating and moves to
  `falling`.
- Score: `30 * 2^(n-3)` for n removed bubbles, +50 per wall bounce.
- When `shots > 10` (i.e. on the 11th shot), the grid shifts down one row and a random row is added at the top.

## Animations driven by elapsed time

- Erase fade: `mask = ((mseconds50 - erasing.time[0]) >> 4) * 3 / 4`, which picks one of the 4
  `spe_mask` patterns (dithered disc → full disc) ANDed over each bubble. When `mask > 3`, the
  bubbles are gone and the field is redrawn.
- Fall: `dy` grows quadratically with `mseconds50 - falling.time[i]` (each bubble starts 10 ticks
  after the previous one), drawn with AND mask + OR sprite straight onto the real planes.

## Bugs and fragile spots found

| Issue | Effect | Fix / lesson |
|---|---|---|
| `FastDrawGrayHLine2B(..., 82, 2, light, dark)`: colour hard-coded as `2` | With ExtGraph 2 the limit line lands in the **dark** plane, which `test_sprite` checks, so every shot stops on the line straight away. | **Fixed**: `COLOR_LIGHTGRAY`. Never hard-code colour values. |
| Original 2002 `.89z` | Crashes on the Titanium (blank screen). | Recompile old sources with GCC4TI. |
| `mseconds == 19` without a reset | `seconds`/`minutes` are wrong (unused, harmless). | Use `% 19` or reset. |
| No `malloc`/`calloc` NULL checks | Crash if RAM is full. | Check and bail out cleanly. |
| ~1.2 KB of locals in both `three()` and `fall()`, nested | Stack pressure. | `static` work arrays. |
| Int-5 start value restored from a hard-coded table (0xCC for HW2, else 0xB2), with a ROM-base `GetHardwareVersion()` that answers "HW1" on the Titanium | **Wrong on the Titanium** (**verified**): the AMS default is 0xCC on both models, the game left the Titanium at 0xB2 (AMS timers at 13 Hz instead of 19.3 Hz). **Fixed in our port.** | `PRG_getStart()` before, `PRG_setStart()` after (done in `bobble.c`). |
| `GetHardwareVersion()` reads the ROM base by hand | Works, but is obscure. | `HW_VERSION`. |
| Swapped LEFT/RIGHT comments in `key()` | Confusing for readers. | Use `_keytest(RR_LEFT)`-style constants. |
| `off()` on CLEAR inside the game | Surprising: a stray CLEAR switches the calc off (it resumes on ON). | Avoid power-off in games, or document it. |
| AMS keyboard handler still active in int 5 while `_rowread` polls | The docs warn it can interfere. | Also redirect int 5's keyboard work, or accept it. |

## What to reuse

The setup/teardown order, the int-5 tick with a frame limiter, virtual planes plus
`FastCopyScreen`, transient layers drawn after the copy, fixed-point motion with 8-bit angles and
trig tables, a logical grid as source of truth with BFS flood fills, and time-driven animations.
