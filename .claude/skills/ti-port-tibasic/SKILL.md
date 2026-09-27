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
- Style references (16-bit): Chrono Trigger, Seiken Densetsu 3, Sword of Mana; the tile is the
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

- After the engine: `make bench` on the heaviest room (tile map + hero + NPCs + a text box),
  read the cycles, check the size (AMS 2 limit 24,576 bytes: data in archived files or packed
  with ZX0, `lib/unpack68k.s`, beyond that).
- End of a part: `make ti`, a run through the part with one or two screenshots read back.

## 6. Conclude, knowledge, commit

Report per part (French): rooms done, what was kept / changed, tests, cycles, size, known limits,
scenarios. Update `games/<name>/README.md`, the knowledge base with verified facts, the lessons
below, then `/ti-commit`.

## Lessons (FFA, `games/ffa/`)

- FFA room format (`murK` = room K+1, picture `decK+1`): row 1 = [encounter rate `frc`, row of
  the door table]; the grid starts at row 2, column 2 (cell of pixel (a, b) = `mur[b/9+2,
  a/9+2]`, 9-px cells, 17 × 8 on a 153 × 71 picture); values: 0 wall, > 0.9 walkable, 1/3…
  fractions = walkable with a special meaning, 3–199 door to room p, 200–299 world map, 300
  chocobo, 450+ other map, ≥ 500 story script (`scenar`), ≤ -2 text (`text⌊|p|/10⌋+1`). Door
  table (row `mur[1,2]`): door ids in columns 1..6, the key flag `clef[·]` at column id+6, the
  arrival (b, a) in the last column at rows 2k-1, 2k.
