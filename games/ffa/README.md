# Final Fantasy Alternative, remake (Portable Game Runtime)

Remake of **Final Fantasy Alternative v1.07** (David Coz, 2002, TI-Basic, ~500 KB: an FF7-like
RPG, one hero, random battles, materias, limits) as a 16-bit style C game: Chrono Trigger /
Seiken Densetsu 3 / Sword of Mana look in 4 greys, continuous movement, scrolling rooms.
Original data: `ffa_en/` (local only). Skill: `ti-port-tibasic`. Progress: `PROGRESS.md`.

```sh
make test                  # unit + integration tests (no window)
make pc && ./ffa_pc        # arrows move, shift runs, 2nd/ENTER examine, ESC quits
make ti                    # ffa.89z (60 KB: Titanium; too big for an AMS 2 TI-89)
make bench BENCH=40        # ffab.89z -> ffab(N): cycles per frame of scenario N
```
`rooms.h` is generated from the local `ffa_en/` data by `tools/rooms.py` (make does it).

## Base unit (benchmark, `experiments/scale_mockup.py`)

One original 9-px cell = one base unit N; the hero is N wide, 1.5 N tall. Room 8 mock-ups:

| N | room 8 | hero on screen | 16x16 tiles per room | verdict |
|---|---|---|---|---|
| 8 | 136×63, fits the screen: no scrolling | 8×12, 12 % of the height | 36 | the original's size: too small for detail |
| **16** | 272×126 = 1.7 × 1.3 screens | 16×24, 24 % | 144 | **chosen**: CT/SD3 proportions, light scrolling, 1 cell = 1 TileMap tile |
| 32 | 544×252 = 3.4 × 2.5 screens | 32×48, 48 % | 560 | the hero hides the room, 4× the tiles |

Cost on the TI is nearly the same for all three (the TileMap engine draws the whole screen, 81k
TiEmu cycles, runtime README): the choice is readability, tile memory and art effort.

## Engine

- Rooms: `tools/rooms.py` turns each original matrix into a collision grid (walls, floor,
  doors, triggers), a door table (destination, key flag, arrival cell) and a tile map; small
  rooms are padded to the 11 × 7 screen and centred.
- Movement: 1.5 px/frame walking (48 px/s = 3 tiles/s), 3 px/frame running; 8 directions;
  10×8 feet hitbox; axis-separated collision; **corner sliding** (up to 6 px of overlap is
  nudged around a corner, the Chrono Trigger feel).
- Doors: bumping into a door cell opens it (key flag checked) with a fade to white in 4 levels
  (3 frames each; `fade_planes` lowers each pixel's grey level on the planes).
- Triggers: texts are solid objects examined with 2nd; story scripts run when stepped on.
- Encounters: the original counter (`mc += frc` per step of one tile, fight above 15+rand(5)).

## Art (style chosen on room 8, `experiments/style_mockup.py`)

Compared on the same view (grey and mono): planks (busy: the hero sinks in), white stone
(readable but washed out), the upscaled original, and **slabs** (chosen): light grey floor,
dark brick wall faces two tiles tall with a cornice, black wall tops. The hero is dark with a
white outline, so he stands out on the light floor. Furniture comes from the CC0 Ninja Adventure
tileset (Pixel-boy) converted to 4 greys with per-object luminance quantiles, plus own ASCII
objects. Walls are auto-tiled from the collision grid (`tools/art.py`), so the art always
matches the logic.

## Text and events

`dialog.c`: speaker name tag, 3 lines of 24 characters (6×8 font), 2 characters per frame,
2nd/ENTER to skip or turn the page, Yes/No choices. `story.c`: each event is a C coroutine
(`SAY`, `ASK`, `WAIT`, `WALK`, `FADE`), its resume point is one state field.

## Measured (Titanium, TiEmu cycles per frame, budget 375k; 2026-09-27)

| screen (scenario) | render |
|---|---|
| castle courtyard, 121-tile set, 2 NPCs (105) | 117k |
| throne hall, Edouard (106) | 111k |
| dungeon hall (110) | 105k |
| battle vs the boss, menu open (52) | 186k |
| throne hall with a full dialogue box (53) | 281k (the 72 characters of text cost ~125k) |

Tiles are shared per area (castle 87, outside 121, dungeon 31 tiles: 15.3 KB, was 24.8 KB
per room). `ffa.89z` is 60 KB. Verified running on the Titanium (story1 scene, ESC to HOME).

## Scenarios (`--scenario N` / `ffa(N)`)

| N | state |
|---|---|
| 0 | new game: room 8 (hero's bedroom), first-level stats |
| 1 | story1 and story2 done, room 6 (throne hall) |
| 2 | + Dungeon Key (Olen), room 7 under the dungeon door |
| 3 | + little key, room 12 |
| 4 | + riddle solved, room 14 (prison hall) |
| 5 | + switch, Fire materia, Power Wrist, room 11 under the weapon room door |
| 6 | + boss beaten (Cell 2 Key), room 16 |
| 7 | + Buster Sword, Bronze Bangle, room 5 in front of the throne hall door (ceremony next) |
| 8 | + knighted, room 18 (Olen gives Cure next) |
| 9 | + Cure materia, room 4: the exit south ends part I |
| 50, 51, 52 | a fight against monster 1, 2, 3 (the boss), command menu open |
| 53 | room 6 with a full dialogue box open (bench) |
| 100 + r | room r (original number), hero on the free cell nearest the centre |
