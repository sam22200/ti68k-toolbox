# Final Fantasy Alternative, remake (Portable Game Runtime)

Remake of **Final Fantasy Alternative v1.07** (David Coz, 2002, TI-Basic, ~500 KB: an FF7-like
RPG, one hero, random battles, materias, limits) as a 16-bit style C game: Chrono Trigger /
Seiken Densetsu 3 / Sword of Mana look in 4 greys, continuous movement, scrolling rooms.
Original data: `ffa_en/` (local only). Skill: `ti-port-tibasic`. Progress: `PROGRESS.md`.

```sh
make test                  # unit + integration tests (no window)
make pc && ./ffa_pc        # arrows move, shift runs, 2nd/ENTER examine, ESC quits
make ti                    # ffa.89z (not yet: stand-in tiles too big, see PROGRESS.md)
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

## Scenarios (`--scenario N` / `ffa(N)`)

| N | state |
|---|---|
| 0 | new game: room 8 (hero's bedroom), first-level stats |
| 100 + r | room r (original number), hero on the free cell nearest the centre |
