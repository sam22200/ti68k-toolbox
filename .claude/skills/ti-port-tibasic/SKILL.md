---
name: ti-port-tibasic
description: "Remake a TI-Basic game (first target: Final Fantasy Alternative, ffa_en/) as a 16-bit style C game on the Portable Game Runtime: extract the sources, matrices, pictures and texts to text/PNG, understand them (with the official guide), then rebuild part by part (one room at a time) with a new engine (tile map, scrolling, continuous movement), new art (Chrono Trigger / Seiken Densetsu 3 style in 4 greys), tests and state injection on the PC first, the TI last, a /ti-commit per room and a PROGRESS.md checkpoint. Use when the user asks to port, upgrade, remake or \"passer en 16 bits\" a TI-Basic game."
---

# ti-port-tibasic (remake a TI-Basic game in 16-bit style)

Same spirit and steps as `ti-port-sdl` (read it: upstream = specification, runtime = target,
every step ends with a check, PC first, TI last, French to the user, English in the files), with
three differences:

1. The upstream is **TI-Basic plus data variables** (programs, matrices, pictures, strings,
   lists), packed in `.89g` groups: it must be **extracted to text and PNG** first.
2. It is **not a port but a remake**: keep the *technical* existence (walls, walkable cells,
   doors, triggers, secrets, story flags, stats, formulas, texts) and redo the *visual* existence
   (tiles, sprites, menus, effects) at a larger scale, 16-bit style.
3. The game is too big for one pass: work **part by part, then room by room**. The user fixes the
   end of each part (FFA part I: stop after the prison and the castle, before the world map).

Constraints (`CLAUDE.md`): **1. performance** (TileMap engine for maps, pre-shifted / masked
ExtGraph sprites, tables, no float, no division in a loop, the measured costs of
`ti68k-performance.md` §7 and `runtime/README.md`), **2. visibility** (160×100, 4 greys, white
outline on the hero, NPCs and monsters). Titanium only; ASM only after asking.

## Session rules

- **Usage limit**: watch the remaining usage. Before it runs out: stop, make every test pass or
  note the failures, commit (`/ti-commit`), and update `games/<name>/PROGRESS.md` with done /
  remaining / current bugs and failing tests / the exact next steps. Never start a new room
  near the limit. A new session starts by reading `PROGRESS.md`.
- **One part, then one room at a time**. A room is done when its tests pass, it runs on the PC,
  and it is committed (`/ti-commit` after each room). The TI only at the milestones (engine
  benchmark, end of part), when nearly everything is right on the PC.
- **No UI until the end** (`CLAUDE.md`, development flow): unit tests and the play-through test
  on the PC, headless shots read back, `ti-cycles` for the TI binary (`make xcheck`: every
  scenario's screen equal to the PC's, the data variable and the save files included); the TI
  emulator (slow, screenshots) only at the milestones below.
- **Injection everywhere**: story flags, discoveries (chests, keys), hero stats, inventory,
  room and position are all settable from a scenario number, a PC state file or a `--set`
  script, so that any room or fight can be tested alone.

## 0. Extract (once per game, skip what exists)

- `ti89decode.py` decodes the `.89g` groups: programs → `programs/*.txt` and
  `ffa_decoded.txt` (all variables in one file, `==== group : folder\name (type) ====`
  headers), pictures → `pictures/*.png` (1-bit, the picture's own size). Matrices and strings
  are inside `ffa_decoded.txt`. Screenshots shipped as `.gif` may be BMP: convert with PIL.
- Write parsers in `games/<name>/tools/` (Python, `tools/pyenv`): matrix → Python/JSON, text
  programs → dialogue tables. Keep the original data **local** (`ffa_en/` is not in git: the
  generated C data and the tools are; ask the user before committing extracted original text or
  art).

## 1. Understand (per part)

- Read the official guide / readme for the walk-through of the part: it gives the order of the
  rooms, the puzzles, the bosses and the expected level.
- Map the part: start room, door graph (walk the doors from the start room, stop at the part's
  frontier), per room its matrix (walls, walkable cells, doors, triggers), its picture and its
  scripts. A contact sheet of the pictures with the matrix values drawn on top is the fastest way
  to read it.
- Write `games/<name>/docs/part<N>.md`: rooms (id, name, size, exits, triggers, chests, NPCs),
  story flags (original variable → new name), texts, monsters of the part (stats, formulas),
  hero stats and levels, shops. This is the specification the tests check.

## 2. Scale and style (per game, once)

- **Benchmark the base unit** (the hero's footprint = one original cell): build the same room at
  each candidate (e.g. 8, 16, 32 px) as mock-ups on the PC (screenshots, grey and `RT_MONO`) and
  as a TI bench (cost of the tile map + hero + NPCs per frame). Pick with reasons (readability,
  screen fraction of the hero, scrolling, map memory, art effort); record it in the README.
- Style references (16-bit): Chrono Trigger, Seiken Densetsu 3, Sword of Mana; the pick per
  category (scenery, characters, UI, portraits) in the `ti-art-refs` skill; the tile is the
  grid unit, characters are taller than one tile (16×24 on 16×16 tiles); perspective: top-down
  3/4 with walls showing their face.
- Compare 2 to 4 art variants on the same screenshot before drawing all the rooms.

## 3. Engine (once, then extended)

- Map = 16×16 tile map (`RtTilemap`, drawn by the TileMap engine) + a collision grid kept from
  the original matrix (scaled: one original cell = N×N pixels) + a trigger list (doors, texts,
  chests, scripts) generated from the matrix by the tools.
- Movement: continuous (sub-pixel 8.8 or pixel steps), 8 directions, axis-separated collision
  with **corner sliding** (the Chrono Trigger feel), camera following the hero, clamped to the
  room. Loading zones (fade) between rooms; one room = one map.
- Effects the original did not have (fades, battle animations, bigger battle sprites), but only
  within the frame budget: measure them.

## 4. Room by room (the loop)

For each room of the part, in the guide's order:
1. Tools: matrix + picture → tile map, collision grid, triggers (generated C), tests of the
   generator (walls where the original has walls, doors where it has doors).
2. Game logic of the room: its triggers, texts, chests, NPCs, story flags, the scripts it runs
   (`scenar` in FFA), fights. Unit tests with the state injected (`sw_init(scenario)` + the
   flags), integration tests with a key script walking through the room.
3. Art: tiles and sprites of the room, visibility checked on screenshots (grey and mono).
4. PC run (`make pc`, headless shots read back), then `/ti-commit`, then `PROGRESS.md`.

## 5. Milestones on the TI

- After the engine: `make cycles` under `ti-cycles` on the heaviest room (tile map + hero + NPCs
  + a text box), read the cycles, check the size (AMS 2 limit 24,576 bytes: data in archived files or packed
  with ZX0, `lib/unpack68k.s`, beyond that).
- End of a part: `make xcheck` with the play-through key script (every scenario, data and saves),
  then once `make ti`, a run through the part with one or two screenshots read back.

## 6. Conclude, knowledge, commit

Report per part (French): rooms done, what was kept / changed, tests, cycles, size, known limits,
scenarios. Update `games/<name>/README.md`, the knowledge base with verified facts, the lessons
below, then `/ti-commit`.

## Lessons (FFA, `games/ffa/`)

- **FFA room format** (`murK` = room K+1, picture `decK+1`, parser `tools/murparse.py`):
  row 1 = [encounter rate `frc`, row of the door table] and is also the top exit row; column 1
  is the left exit column; the door-table row doubles as the bottom exit row; C = `mur[2,1]`
  holds the arrival column. Logic cell (x, y) = (a/9 + 1, b/9 + 1). Values: 0 wall, > 0.9
  walkable, 3..199 door, 200+ other maps, >= 500 story script (walked on), -100 < p < -1 text
  (solid, read with ENTER). Door table: ids in columns 1.., key flag at column i+6 (0 none,
  -1 on foot), arrival (b, a) in column C, read from the room being **left**, -1 = keep.
  Metadata sits on unreachable border cells: sanitize (a non-wall cell with no walkable
  neighbour is a wall; the door-table row keeps only exits). Keys no program sets = locked.
- **Walls auto-tiled from the collision grid** (3/4 view: face above a floor cell, upper face
  two cells up, dark top elsewhere) + a per-room layout (floor rectangles under furniture,
  objects, fills): the art cannot disagree with the logic. One tile set per area, not per room
  (24.8 -> 15.3 KB).
- **Story scripts as C coroutines** (protothread macros SAY/ASK/WAIT/WALK/FADE/BATTLE, the
  resume line in the state): readable, saveable, testable; one macro per line (`__LINE__`).
- **Scenarios = story checkpoints** (flags and items of each walkthrough step) + fights and
  dialogue states for benches: every room and fight is testable alone.
- **A play-through test** (BFS over the collision grid with the real keys, fights fought,
  dialogues read) caught what unit tests did not (the examine probe missing a chest).
- **Size**: 64 KB per TI variable: big data goes to a data variable read in place (`rt_file`),
  saves go through `rt_save` (written at exit). Text: draw a dialogue page once into a
  plane-format buffer, copy it byte-aligned (281k -> 218k per frame).
- **The TI run must change rooms**: the PC backend redraws the map every frame, the TI one caches
  the TileMap plane; a stale cache (new room's logic under the old room's picture) passed every
  PC test. Walk through a door (and back, a key still held) at each TI milestone.
- Colour CC0 assets (Ninja Adventure) convert well to 4 greys with per-object luminance
  quantiles; 1-bit original sprites become battle sprites with 2x EPX + automatic shading
  (highlight top-left, shade bottom-right) + a white outline.
- OR-mode text cannot be light on dark: dialogue boxes are light with black text; greyed text
  on black is invisible (put a light band behind it).
