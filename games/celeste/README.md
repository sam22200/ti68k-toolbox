# Celeste Classic for the TI-89 Titanium

Port of the PICO-8 Celeste Classic, made with the `ti-port-pico8` skill. **Slice 1: room 0
(100 m)**: spawn, run, jump, wall jump, dash, spikes, death and respawn, the fake wall and
its fruit, the room title; the exit at the top opens an "end of demo" screen. The logic is
the cart's Lua translated function by function, **bit-exact** with the cart (16.16, `p8num.h`):
every frame of 22 key scripts equals the original cart run under z8lua.

Run on the calculator: `celeste()` ([2nd] jump, [shift] dash, arrows, [ESC] quit; on the end
screen [ENTER] plays again). `celeste(1)`: the player already standing on the spawn tile.

## Build and test

Tools: `tools/shrinko8`, `tools/z8lua` (`ti-port-pico8` SKILL.md §0).

```sh
make test       # extract cart/ into x/, reference traces (z8lua), trace + unit tests
make pc         # SDL window (STYLE=A|B|C: the graphics variants, B by default)
make cycles     # celestec.89z for ti-cycles;  make xcheck XCHECK=1 FRAMES=160 KEYS=keys/fakewall.txt
make tihash     # the TI binary's per-frame gameplay state hash = the PC's, every script
make ti         # celeste.89z
```

- `scripts.txt` lists the key scripts (`keys/*.txt`, the runtime's format): one per mechanic,
  `exit` (the first 100 frames of ccleste's TAS, `tools/tas2keys.py`), `play`, and 12 seeded
  random ones (`tools/fuzz.py 12 1500 keys`). Their references (`traces/`, 6.5 MB) are
  regenerated from the cart, not committed.
- Traced state (`trace.lua` = `trace_line` in `test_celeste.c`): globals, the gameplay objects
  (smoke is cosmetic: out), the player or the spawn's fields. Smoke, shake, hair, dead
  particles and the view camera are cosmetic: own random numbers (`rt_rand`), not traced.
- Coverage gate: the test fails when no script shows one of 19 mechanics (8 dash directions,
  dash without direction or without dash left, death, respawn, fake wall, fruit, refill,
  screen edge clamp, exit...).
- Mutation check (2026-10-01): gravity +1 bit (33 failures), coyote 6 → 5 (31), dash time
  4 → 3 (27), respawn delay 15 → 14 (16), left clamp −1 → −2 (14), spawn landing delay (18),
  fruit bob period 40 → 39 (1, `fruitbob`); the right clamp cannot happen in room 0.
- `tools/measure.py`, `tools/mapstats.py`: the measures below (`measures/`, from `x/`).

## Measured (ti-cycles, datasheet cycles per frame, averages)

| Script | update | render |
|---|---|---|
| play | 26.7k | 67.2k |
| fakewall | 32.2k | 74.3k |
| dash | 34.2k | 78.3k |
| fuzz_03 (1,500 frames) | 30.1k | 70.0k |

About 100-112k of the ~360k budget at 30 fps (grayscale included). `celeste.89z` 20.5 KB.
TI = PC: same screen checksum (`xcheck`, 4 scripts) and same gameplay state every frame
(`tihash`, 22 scripts, 19,713 frames). Emulator (Titanium, once): greys, [2nd] jump, the view
camera.

## Scenarios

| n | State | `p8trace.py --pre` |
|---|---|---|
| 0 | game start, room 0, spawn animation | `begin_game()` (PRE0 in the Makefile) |
| 1 | the player standing on the spawn tile | PRE1 in the Makefile |

## Graphics (decision Q9: variants compared)

`gfx.py` converts the cart's sheet per group (terrain, bg layer, player body, hair, fruit,
smoke). Variants on the same shots: A dark (the original look: black background), **B light
(kept)**: white background, black terrain, the bg rocks the only light grey, the player black
with a white outline, hair black with a dash and light grey (dark ring) without; C light with
dark grey terrain (less contrast). B reads best on the LCD and follows the black and white rule.
The player is a 16-wide 10×10 sprite (outline included), the room is pre-rendered once into
two 128×128 planes and 100 byte-aligned rows are copied per frame (the HUD bands cleared in
the same pass).

## Source

| Item | Value |
|---|---|
| Game | Celeste Classic (PICO-8), Maddy Thorson + Noel Berry, 2015 |
| Reference cart | Lexaloffle BBS tid 2145, cart id 15133: `https://www.lexaloffle.com/bbs/cposts/1/15133.p8.png` (downloaded 2026-10-01, 160×205 PNG), copied in `cart/` |
| Cart hash | sha256 `b8c78f53bc18cd9c8830c794093a3b724d6208de9eae290f0b84403225740188`, identical to the local `sources/celeste_p8/celeste.p8.png` |
| Cart licence | BBS post without a CC tag: all rights reserved (code, art, map); committed by the owner's decision (Q2) |
| Repository given by the user | `https://github.com/NoelFB/Celeste` @ `1b0ce45` (= remote HEAD, local clone `sources/celeste/`). It does **not** contain the cart: only `Source/PICO-8/Classic.cs` (1,782 lines, the C# port shipped inside the 2018 Celeste) and `Emulator.cs` (its PICO-8 API over XNA), plus `atlas.png`/`font.png`. MIT for the code only, "does not include the actual commercial Celeste game or assets" (its README). |
| Line-by-line C port | ccleste (`github.com/lemon32767/ccleste`, archived, no licence file): not cloned |
| Decoding | `p8extract.py` (shrinko8): `code.lua` 1,429 lines, 30 fps (`_update`, `_draw`), 28 API functions, 98 sprites on the map; outputs in `x/` |

`Classic.cs` differs from the cart (second opinion only, never a test reference): floats
everywhere (`E.rnd`, `Math.Sin`), `map()` layers passed as flag *indices* 2/1/3 instead of
the cart's bit masks 4/2/8, **33 snow particles** (`i <= 32`) against the cart's 25
(`for i=0,24`), `(int)(x - last)` truncation in the platform carry. It keeps the cart's
`move_x` loop (`i = start .. abs(amount)` inclusive, see below).

## Program

- Load: top-level code builds `clouds` (17, 4 `rnd` each) and `particles` (25, 5 `rnd`
  each) **before** `_init`: 168 draws of the PRNG before the title.
- `_init` → `title_screen()`: `got_fruit[1..30] = false`, `frames = deaths = 0`,
  `max_djump = 1`, `load_room(7,3)` (the title is room 31).
- `_update` (30 fps): `frames = (frames+1) % 30`, seconds/minutes while `level_index() < 30`;
  `music_timer`, `sfx_timer`; **`freeze > 0` returns** (and `_draw` returns too: the frame
  is not redrawn); shake (`camera(-2+rnd(5), -2+rnd(5))`); delayed restart
  (`delay_restart` 15 frames); then `foreach(objects)`: `obj.move(spd)` then
  `type.update(obj)`; title: O or X starts the game (`start_game_flash` 50 → −30, then
  `begin_game()` = room 0).
- `_draw` **changes gameplay state** (to move to the end of `game_update`, cart order):
  player x clamp to −1..121 (`spd.x = 0`), hair positions, clouds and snow (both call `rnd`
  when they wrap), dead particles (`t -= 1`, `del`), `message` (text index), `big_chest`
  (state 0 → 1 → 2, `pause_player`, `flash_bg`, `new_bg`, the orb spawn), `orb` (speed,
  pickup: `max_djump = 2`, `freeze = 10`), `flag` (score, `show`), `room_title` (`delay`,
  self-destroy), `lifeup.flash`.
- Draw order: background `rectfill` (colour 0, `frames/5` during the big chest flash, 2
  after it), clouds (not on the title), `map(..., 4)` (bg layer), platforms and big chest,
  `map(..., 2)` (terrain; x −4 on the title), the other objects, `map(..., 8)` (**empty**:
  no tile has flag 3), snow, dead particles, the 4 black borders (for the shake), title
  credits, the summit's black side bars (room 30).
- Room change: player `y < -4` and `level_index() < 30` → `next_room()` (x+1, or the next
  row after x = 7). Death: spikes (tiles 17/27/43/59 with speed-direction tests), `y > 128`.
  Respawn = `load_room` again after 15 frames.

## Objects

One list `objects`, update and draw in list order, PICO-8 `foreach` semantics.

| Type | Tile | Gameplay | Notes |
|---|---|---|---|
| player_spawn | 1 | entry animation from y = 128 (`spd.y = -4`, 3 states) | player exists 27 frames after `load_room` |
| player | – | movement, jump, wall jump, dash, hair colour = dashes left | hitbox (1,3,6,5) |
| spring | 18 | `spd.y = -3`, `spd.x *= 0.2`, refills dash; hides 60 frames when its fall floor breaks | |
| balloon | 22 | refills dash, back after 60 frames; y = start + sin(offset)·2, **offset = rnd(1)** at init | hitbox (−1,−1,10,10) |
| fall_floor | 23 | breaks 15 frames after a touch, gone 60 frames | solid |
| fruit | 26 | +1 berry, `y = start + sin(off/40)·2.5` | `if_not_fruit` |
| fly_fruit | 28 | flies away once the player dashed | |
| fake_wall | 64 | broken by a dashing player (16×16), drops a fruit | |
| key / chest | 8 / 20 | key opens the chest (x jitter `rnd(3)`, 20 frames) → fruit | |
| platform | 11 / 12 | moves ±0.65 px/frame, wraps, carries the player | drawn under the terrain |
| message | 86 | memorial text (room 11) | logic in `draw` |
| big_chest / orb | 96 | room 21: cutscene, then the orb gives 2 dashes | logic in `draw` |
| flag | 118 | room 30: score box (berries, time, deaths) | logic in `draw` |
| smoke, lifeup, room_title | – | cosmetic (smoke: 5 `rnd` at init) | |

Objects per room at load (`measures/rooms_objects.txt`): 0 (title) to 15 (room 3, 12 fall
floors). Peak during play (`measures/collide_work.txt`, smoke included): **21**.

## Numbers (measured, `measures/measures.txt`)

- **`move_x`/`move_y` move `|amount| + 1` pixels** when `amount != 0` (inclusive loop from 0;
  same in `Classic.cs`): max run speed 1.0 px/frame shows as **2 px per frame**: +58 px in 30
  frames held. Speed 1 reached in 2 frames (accel 0.6), stop in 2 frames.
- Jump: `spd.y = -2`; apex **19 px**, 11 frames up, 23 frames of air time; holding the button
  changes nothing (no variable jump). Running jump: 46 px forward.
- Gravity 0.21 (halved when `|spd.y| <= 0.15`), terminal speed 2 (0.4 on a wall slide),
  reached after 10 frames of fall.
- Dash: `freeze = 2` (two frozen frames), 4 frames of `dash_time`, speed 5 (3.54 diagonal),
  then `appr` to 2·sign at 1.5/frame: **25 px** horizontal or vertical, 23 + 22 px diagonal
  (10 frames after the press). Dash without a direction: speed 1 then 2 (20 px).
- Coyote time 6 frames (`grace`), jump buffer 4 frames (`jbuffer`) *code*.
- Timings: title → game 80 frames after the press; spawn animation 27 frames; death →
  player again 42 frames.
- Decimal constants and their z8lua bits (`measures/constants.txt`): 0.01 `0x028f`, 0.05
  `0x0ccc`, 0.1 `0x1999`, 0.15 `0x2666`, 0.2 `0x3333`, 0.21 `0x35c2`, 0.25 `0x4000`,
  0.3 `0x4ccc`, 0.4 `0x6666`, 0.5 `0x8000`, 0.6 `0x9999`, 0.65 `0xa666`, .75 `0xc000`,
  0.70710678118 `0xb504`, 5·0.70710678118 `0x3.8914`, 1.5, 2.5, 3.5.
- Products: every gameplay product has a constant or an integer operand (no 16.16 × 16.16 of
  two variables). Divisions in gameplay: `x/8`, `y/8` (power of two, `p8_div2k`), `off/40`
  (fruit bob: `off` is an integer, so the fraction is a 40-entry table, exact). Cosmetic
  divisions: hair `/1.5`, `/3`, `/5`, `/30`, `/32`, `/64`.
- `sin` in gameplay: balloon, fruit, fly_fruit (y positions that collide). The trace tests
  need the same sine table in the C and in the reference (`--lib`).

## Random numbers (one PICO-8 stream, `srand` never called)

| Use | Calls | Kind |
|---|---|---|
| clouds, snow at load; their respawn in `_draw` | 168 at load, then 1 per wrap | cosmetic |
| smoke init (`rnd(0.2)`, 2 × `rnd(2)`, 2 × `maybe()`) | 5 per smoke | cosmetic |
| wall-slide smoke `rnd(10) < 2` | 1 per frame of slide | cosmetic |
| shake `camera` | 2 per frame of shake | cosmetic |
| big chest particles | 3 per particle | cosmetic |
| **balloon `offset = rnd(1)`** | 1 per balloon per room load | gameplay (balloon height) |
| **chest x jitter `rnd(3)`** | 1 per frame for 20 frames | gameplay (where the fruit appears) |

Every cosmetic draw shifts the two gameplay ones: exact traces need the cosmetic stream too,
or a separate generator on both sides (question Q13, round 4).

## Map and graphics facts (`measures/mapstats.txt`)

- 32 rooms of 16×16 cells (128×128 px), rooms 0–30 played, 31 = title. The map uses the
  shared lower half (rows 32–63 = sprites 128–255): only 128 real sprites.
- Flags: 0 solid, 1 terrain layer, 2 bg layer, 4 ice. Layers used: bg (11 tiles) and terrain
  (45 tiles); **the fg layer is empty**, and no tile is in both layers: one tile per cell, so
  a room is one 2-layer picture with only the platforms and the big chest between the layers.
- Distinct 16×16 blocks (2×2 cells) over rooms 0–30: 293 (bg), 371 (terrain), **791**
  (both): too many for a metatile set; a room pre-render is the natural fit.
- Screen: 128 px wide fits 160 (16 px bands each side); 128 px tall loses 28 rows in 100. The
  spawn is at the bottom (y 80–112 in 29 rooms; room 20 at y = 56, room 27 at y = 8), the exit
  at the top. Spikes lie in the top four rows (y 0–31) of 17 rooms and in the bottom four
  rows (y 96–127) of 15 rooms; spikes or objects in the top band of 18 rooms (per room in
  `measures/mapstats.txt`).
- Texts placed outside a 100-row window: title credits at y 96 and 102, the memorial text box
  at y 94–110, the summit box at y 2–31, the room title at y 58–70, the time at (4,4).
- Colours (PICO-8 indices, pixel counts): player = 8 red (145, hat/hair, swapped by `pal`),
  15 peach (85), 3 dark green (31), 1 dark blue, 7 white; terrain = 7 white (1,179), 12 blue
  (723), 5 dark grey (423, the bg rocks); background 0 black, clouds 1, snow 6/7.
  Luma: red 85 ≈ dark grey 88 ≈ dark green 88: **the player's red is the grey of the bg
  rocks**. Hair = dashes left: 1 → red (8), 0 → blue (12), 2 → white/green blink (7/11).
  Whole-screen palette effects: title start flash (`pal` of 6 colours), big chest flash
  (background colour cycling), `new_bg` (background 2, pink clouds) after the orb.

## Cost picture (estimates before step 6, kept for the later rooms)

- Collision work (upper bound: objects scanned by `collide`, `measures/collide_work.txt`):
  room 3 up to **2,205 scans and 111 `collide` calls per frame** (avg 1,453); other rooms
  300–640 on average. Every fall floor checks the player 3 times per frame.
- Render, first guess from `runtime/README.md` costs: room pre-render copy 25–35k (real),
  up to 21 masked 8×8 sprites, 17 cloud rectangles, 25 snow dots, 5 hair discs, 8 dead
  particles; budget ~360k per frame at 30 fps with grayscale.

## Scenario door (proposal for the later rooms)

`game_scenario(n)`: 0 = title; 1..31 = room n−1 with the player at its spawn, no animation
(`p8trace.py --pre "begin_game() load_room(rx,ry) <replace the spawn by a player>"`, as in
`scripts/measure.py`); later: 40 = big chest cutscene, 41 = two dashes (after the orb),
42 = summit.

## Port decisions (grilling, 2026-10-01)

| # | Decision |
|---|---|
| Q1 source | The BBS cart is the reference; the Lua is translated function by function; `Classic.cs` is a second opinion only |
| Q2 licence / git | The user's choice: everything in git, cart data and generated art included |
| Q3 fidelity | Exact: same gameplay state as the cart every frame (16.16 bits), proven by trace diffs; cosmetic state approximated |
| Q4 scope | Room 0 only: spawn animation, run, jump, wall jump, dash, spikes, death, respawn, fake wall + fruit, "100 m" title, deaths/berries HUD in a side band; no title screen, clouds or snow. Exit at the top → "end of demo" screen (time, deaths, berries): [ENTER] restarts room 0, [ESC] quits |
| Q5 screen | 1:1 pixels, room centred at x = 16, vertical camera following the player (dead zone ±8 px, lead 1.5 × vertical speed, clamped to the room; at the bottom during the spawn) |
| Q6 frame rate | One cart update per frame, frames alternating 8 and 9 ticks of 256 Hz (30.1 fps): an opt-in runtime extension |
| Q7 numbers | Mixed: x, y `s16`; `rem`, `spd`, dash speeds 16.16 (`p8num.h`); timers and counters `u8`/`s8` |
| Q8 controls | [2nd] = jump (O), [shift] = dash (X), [ESC] = quit, no pause |
| Q9 greys | Variants compared on shots at the graphics step; very high contrast: assets and characters mostly pure black and white, the player outlined in white; hair: 1 dash = dark, 0 dash = light, 2 = blink |
| Q10 room render | The room pre-rendered once into two 128×128 planes, a byte-aligned 100-row window copied each frame; the fake wall cells redrawn once when it breaks |
| Q11 effects | All kept: hair, smoke, death particles, "1000" flash (small pre-made sprites); the shake is vertical only (keeps the byte-aligned room copy) |
| Q12 sound | None (events kept in the traces as `__sfx`) |
| Q13 random | Room 0 has no gameplay `rnd`: the cosmetic draws use their own cheap generator, outside the traces (traces = player, gameplay objects, globals); the exact PICO-8 stream comes back with balloons and chests |
| Q14 save | None |
| Q15 budget | Estimate ~100k of ~360k; fallbacks: redraw only what changes, fewer effects, then asm on the measured hot spot (asked first) |
| Q16 model | Titanium |
