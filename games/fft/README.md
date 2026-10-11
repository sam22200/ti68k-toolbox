# Final Fantasy Tactics: Magic City Gariland (tech demo)

An isometric battlefield on the TI-89 / Titanium in the style of Final Fantasy Tactics (PS1,
1997): the real Gariland map (10 x 15 tiles, heights 0 to 10) seen from four camera
orientations, an animated 90-degree rotation, the Gariland battle's eleven units at their
start tiles (Delita, five enemies, Ramza and four recruits, their stats drawn and computed as
FFT does), FFT's turn order (its CT clock, checked on the original's first 46 turns), depth
sorting and occlusion by the buildings, the player's units moved over the terrain with
their own Move / Jump on their turn, and FFT's Attack (its range, hit chance, damage, zodiac,
critical hits and knockback, checked on the original's previews and attacks). Not a port: our own engine on the Portable Game Runtime;
the heights and the scenery (the map's own textured mesh, drawn in four greys on the PC), the
units (FFT's own battle sprites of Ramza, Delita, the Squires and the Chemists, scaled 0.6) and
the battle's numbers come from the local disc (`tools/extract.py`, `tools/units.py`,
`tools/battle.py`); the cursor, shadow and rotation greys are ours (`tools/art.py`). The battle
itself (skills, KO and the end, AI) is being built: `ROADMAP.md` milestones 13-22. Study notes:
`RE_NOTES.md`.

## Controls

| Key | Action |
|---|---|
| arrows | the menu's entry; the cursor (relative to the view: UP = up-right, RIGHT = down-right); at Wait, the facing |
| [F5] / [F1] | rotate the camera 90 degrees clockwise / counter-clockwise |
| [2nd] or [ENTER] | pick the menu's entry; on a reachable tile: walk there; on a unit in the weapon's reach: attack it; at Wait: end the turn; looking around: a unit's numbers, or back to the menu |
| [shift] or [ESC] | back: from the reach, the targets or the facing to the menu; in the menu, undo the move (before acting) or look around; [ESC] while looking around quits |

The turns come as FFT deals them; the cursor and the camera go to the unit whose turn it is.
Enemies and Delita (a guest) only wait for now (half a second each). A player's unit opens its
menu at the top right: **Move** (once; the move can be undone with [shift] until it acts, which
FFT does not allow), **Act** (once: the weapon's reach shown, the cursor on a unit shows FFT's
preview at the top right, `Hit 95%` and `Dmg 12`, [2nd] attacks: the swing, the target hit,
the damage or `Miss` rising above it, a critical hit pushing it a tile, KO at 0 HP lying
down; a unit with a fifth of its HP or less kneels), **Wait** (four arrows around the
unit, the one it faces black: the arrows turn it, [2nd] ends the turn) and **Status** (its
level, HP, MP, Move, Jump, Speed, PA and MA; Brave, Faith and the zodiac stay hidden, as
decided at milestone 13). The HUD shows the turn order at the top left (FFT's AT list: the
active unit, then the next five turns; enemies on light grey, underlined), then the view (S,
W, N, E: where the camera stands) and the height under the cursor in FFT units (`h2`, `h7.5`
on a slope). At the bottom corners, as in Metal Gear Solid: Portable Ops, two thin panels
read sideways: the active unit at the left, the unit under the cursor at the right (light
grey: an enemy), each with its name, its HP and MP as `52/52` and `16/16` and a gauge beside
each number (HP black, MP dark grey). Scenarios (`fft(n)`, `--scenario n`):
0 the battle's start (a new draw each run: seeded from the clock; Delita's turn), and from Ramza's
first turn (the six units before him waited): 1-4 an enemy moved behind a house seen from each
view, 5 Ramza's reach, 6 mid-rotation, 7 Ramza walking, 8 an enemy in front of him aimed
at, 9 that enemy hit (the number rising); above 9 that turn drawn with seed n (the tests).
Key scripts: `keys/turn.txt` (a whole turn through the menu) and `keys/attack.txt` (Ramza
attacks, moves and waits), both from `fft(10)`.

## Build

```sh
make map.h        # once: reads MAP/MAP022 from roms/ps1/Final Fantasy Tactics (USA)/ (local):
                  # map.h and the four views fftv0.bin .. fftv3.bin, ZX0-packed (~2 min)
make units.h      # once: the units' sprites from BATTLE/*.SPR, TYPE1.SHP and TYPE2.SHP (a second):
                  # units.h and the data file fftu.bin (37.4 KB, big-endian rows, mirrored too)
make battle.h     # once: the battle's units, jobs, items and weapons, generation tables, the
                  # zodiac table (SCUS_942.21, ENTD, BATTLE.BIN)
make test         # headless tests (rotations, heights, the units' stats against the original's,
                  # their draw, reach, occlusion, team marker, keys, the turn order against the
                  # original's 46 turns, attacks against its previews and attacks); the first
                  # run makes oracle.h (the oracle below, 30 s, its next 40 turns, 20 s, its
                  # attacks, 9 min, then tools/battle.py --check)
make pc           # SDL window: ./fft_pc   (F1 / F5 on the PC keyboard too)
make ti           # fft.89z, fftu.89y and fftv0.89y .. fftv3.89y: send all six, archive the five
                  # data files (read in place), run fft()
make xcheck       # the TI binary under ti-cycles = the PC, 10 scenarios
../../tools/pyenv/bin/python tools/oracle.py DISC.cue ../../sources/fft_ps1/gariland_t1.state trace.txt
                  # the original at Ramza's first Gariland turn (30 s, RE_NOTES.md)
../../tools/pyenv/bin/python tools/oracle.py --turns DISC.cue STATE 40 turns.txt
                  # from there, the AI playing both sides: 40 turns (unit, CTs, HPs)
../../tools/pyenv/bin/python tools/oracle.py --attack DISC.cue STATE 40 400 attacks.txt
                  # from there, Ramza's attack: 40 previews with stats poked, 400 attacks
../../tools/pyenv/bin/python tools/battle.py DISC.cue --check STATE TRACE [oracle.h turns.txt attacks.txt]
                  # its eleven units against our model (stats, ranges, equipment, start tiles);
                  # oracle.h: the test fixture (units, the turns with their CT bonuses, attacks)
```

`map.h`, `units.h`, `battle.h`, `oracle.h`, `fftu.bin` and the views are generated from the disc
and never committed: without the disc the game does not build. `art.h` is ours and committed (`make art.h` regenerates it from
`tools/art.py`). Without its data files the game says `missing fftu, fftv0-fftv3`.

## Architecture

- **One map, four views.** World tiles `(x, z)` are turned into view tiles `(u, v)` per
  orientation (0: `(x, z)`, 1: `(H-1-z, x)`, 2: `(W-1-x, H-1-z)`, 3: `(z, W-1-x)`) and a tile's
  four corner heights are renumbered with it (`(c - rot) & 3`); `build_view()` keeps the
  current orientation's tiles in view order. Units, reach and paths live in world coordinates
  only: turning never moves anything.
- **Projection.** A tile is a 24 x 12 diamond, one FFT height unit is 6 px:
  `x = (u - v + H') * 12`, `y = 64 + (u + v) * 6 - h * 6` in the scene. FFT's own ratio
  (tile 28 units wide, h = 12 units) gives 6.3 px at this tile size.
- **The views are the game's map, drawn on the PC.** `tools/extract.py` reads the map's
  textured mesh (MAP022.9: 866 triangles once the quads are split; texture MAP022.8, 16
  palettes) and draws each orientation with a depth buffer in the projection above, 4x
  supersampled, averaged to the TI's pixels and cut into 4 greys by luminance (the 25 / 55 /
  82 % percentiles of the whole map: the same greys in every view). The mesh matches the
  terrain grid (x / 28 and z / 28 are the tile, -y / 12 the height). A view is one data file
  (`fftvN`, 52,320 bytes packed with ZX0 into 9.1-10.3 KB): the two planes of the 320 x 218 scene, then the **depth**: every
  pixel belongs to a tile (its face's centre pushed a little into the solid, so a wall
  belongs to the tile it bounds), and per image byte the two frontmost view diagonals `u + v`
  with the mask of their pixels (a third depth in one byte is dropped: 2.5 % of the bytes).
- **Views in memory**: one view at a time is unpacked into a 52 KB buffer (`zx0_asm` from
  `lib/unpack68k.s` on the TI, 2.2 M cycles = 0.18 s; a C copy on the PC), during the first
  frame of the turn that reaches it, and copied into the 17 KB scene buffer when the
  orientation or the reach changes. Two allocations: AMS refuses blocks above ~64 KB.
- **Reach rings** are drawn into that scene copy, then the pixels in front of their tile are
  restored from the view but for 2-pixel dashes: a tile behind a house keeps a dashed ring
  over it (the user's pick over the whole ring and a checkerboard, `x/bench_cases_cachees.png`,
  local); no cost per frame. The cursor's line is 4 px per row (`x/bench_curseur.png`).
- **Every frame**: the camera's 160 x 100
  window is copied from it at any pixel (32-bit shifted words), the units on screen are drawn
  back to front (those off it skipped) through a **cover mask**: the pixels of their 16 x 26
  box whose depth is greater than the unit's diagonal, read from the view (both layers), so a
  building hides them exactly as it is drawn, except their contour: on the covered rows the
  sprite's mask is eroded twice (the opaque pixels whose four neighbours are opaque) and only
  that inside is taken away, so the white outline and the black line inside it stay drawn over
  the building (a hidden unit shows as an outline; no projection). The mask and its covered
  rows are kept per unit and made again only when its feet, depth or the view change (15k
  cycles each time); the rows above the covered ones are drawn straight from the archived
  `fftu` (native big-endian rows, mirrored frames stored), only the covered rows are copied
  and eroded. Then the blinking cursor (always on top, as in FFT) and the HUD.
- **Units.** FFT's sprites (`tools/units.py`): its tile-made frames (`TYPE1.SHP`) composed,
  scaled 0.6 (each TI pixel takes the source pixels whose centre falls in it: opaque when half
  of them are, its grey their vote), the face's skin one grey lighter and each eye placed on its own (FFT's eye pixels found as 1-2
  groups in the face, each drawn 1 x 2 black at its centre, a skin pixel between the two:
  scaling alone merged them into a bar),
  cut to 16 columns (a hand tip in 4 frames of 40), a black line on the silhouette and a white
  outline: 16 x 26, the feet at row 21. The scale is FFT's proportions: on a capture of the
  game, a standing unit is ~0.9 of a tile's width (a canal one tile wide as the ruler); 0.5
  made them 0.75 (`x/proportions.png`, local).
  FFT draws two directions, the front looking down-left and the back up-left, and mirrors them
  for the other two (stored mirrored in `fftu`: drawn as they are); each unit has a world
  facing, turned into the view each frame, set by each step of a walk. The battle idle (FFT
  marches in place) and the walk are FFT's sequences (`TYPE1.SEQ`; the Chemists' `TYPE2` has
  the same frames), then five poses each facing the camera and away (hit, the swing's wind-up
  and blow, kneeling, lying KO: `TYPE1.SEQ`'s frames, a raised hand's top row cut, the lying
  ones drawn 4 rows higher to fit), 20 frames per sheet and 20 mirrored, 6.2 KB per sheet; six
  sheets (Ramza, Delita, Squire and Chemist, male and female).
- **The battle's units** (`tools/battle.py` -> `battle.h`, `battle_units()`, `unit_stats()`):
  Gariland's ENTD (Delita, five enemies), the new game's Ramza and four of the Military
  Academy's recruits at the oracle's deployment tiles. At each battle every random value is
  drawn as FFT does (raw stats from the unit type's base and variance, Brave and Faith 45-74,
  birthday and zodiac, the random items among those of the unit's level), then the stats
  computed from the raw ones, the job's multipliers and the equipment, in 32-bit integers
  (once per battle). Checked against the original: `RE_NOTES.md` § The battle's units.
- **Rotation.** Three frames between two views, drawn at half resolution (80 x 50, doubled)
  as polygons projected at 22.5, 45 and 67.5 degrees around the cursor tile, sorted by
  depth (counting sort), only the faces towards the camera, units on top; the cursor tile stays
  where it was on screen and the camera eases back afterwards. The 45-degree entries of the
  sine table equal the cardinal projection exactly (6 x 256), so the last frame meets the
  composed view. Each face (a tile's top, each of its four walls) is one solid grey: the mean
  grey of its pixels in the four views, rounded (`extract.py`, `Tile.look` in `map.h`), one
  grey darker when facing right, so the frames keep the views' tones (light streets, dark
  roofs) and the volumes. Checkerboards between two greys (7 shades) were tried and declined
  by the user (`x/turn8_damiers.gif` against `x/turn8_sans_damiers.gif`, local).
- **Reach.** FFT's rules, each unit's own Move and Jump (height difference in half units, both
  ways), canals, trees and chimneys block, enemies block, allies (and the guest Delita) can be
  passed but not stood on (`bfs`). The reachable tiles get a marker at their standing height (a black diamond line with a white
  one outside it, readable on every floor; chosen among four, `x/choix_marqueurs.png`), drawn into the
  scene buffer after a fresh copy of the view; the pixels in front of the tile (the view's
  depth) are put back over it.
- **Water.** The canal's tiles (FFT surface 0x0E, a `water` flag in `map.h`) get white
  2-pixel glints on a lattice of the scene (every other row, one even column in 4, staggered)
  sliding 2 px right every 8 frames: a current. Only the positions the view shows (its depth:
  not under a bank or a roof) are kept, once per orientation (`make_glints`, ~100 per phase),
  and drawn on the screen after the view copy (~13k cycles). The views themselves stay still.
- **Team at a glance.** Enemies have a black diamond with a white border above the head
  (`foe_gfx`, `art.py`), never covered (the user's pick of eight shapes,
  `x/bench_marqueurs_ennemis.png`, local): a hidden enemy shows its contour and its diamond over
  the building. Chosen over an inverted outline (black outside, white inside) and a dotted
  one, which need a second look (`x/choix_camps.png`, local); FFT itself tells the teams by
  palettes, which 4 greys cannot.
- **Walking.** Tile by tile along the BFS path, 4 frames per tile, a hop on height changes;
  while between two tiles the unit is sorted after both.
- **Turns.** FFT's clock (`ct_next`, `ct_end`): each tick every unit's CT grows by its Speed;
  the highest CT of 100 or more acts (a tie to the lower index: Delita, the enemies, Ramza,
  the recruits), losing 100; at the end it gets 20 back for each of Move and Act unused, at
  most 60. A unit at 0 HP keeps its clock, each pick lowers its death counter (3), then it is
  a crystal or a chest and its clock stops. The order shown is FFT's AT list (`turn_order`:
  each living unit's next turns by its clock alone, by tick then CT reached), made at each
  turn change (11 x 5 divisions, ~10k cycles).
- **Attack.** FFT's Attack command (`attack_range`, `attack_side`, `hit_chance`,
  `attack_damage`, `attack`, `attack_step`; the rules in `RE_NOTES.md` § Attack): the four
  neighbouring tiles within 3 levels below and 2.5 above get the reach rings; from the front
  the target evades with its class evade (5, Ramza 10), from a side or the back it is hit;
  damage XA x WP with the signs' compatibility (a 16-bit step each), critical hits 4 % (XA +
  0..XA - 1, the target pushed a tile if it lands free and no higher), at most 999; KO at 0
  HP (the turn order made again). The number is AMS text written once into the HUD's spare
  rows and drawn as a 32-pixel outlined sprite. The battle's draws come from our own 32-bit
  xorshift (the runtime's 16-bit wyhash tied two draws in a row: every critical added at
  least 1).
- **Rings without stalls.** Old rings go by putting back their boxes from the view when they
  are few (an attack's four; a Move's dozens: the whole copy, 130k), new ones are drawn two a
  frame (`uncover` with three pointers and constant plane offsets: 18k a ring before). An
  attack's first frame was 437k, Move a 0.74 M stall.
- **HUD.** Each part (the top strip, the menu or status window, the two panels) is drawn
  into a work buffer when what it shows changes and copied each frame by words (~5k cycles
  for all): drawn every frame with `draw_rect` and AMS text they cost ~40k and more (ExtGraph's
  fill and the ROM's `memcpy` have a high fixed cost per call, under `ti-cycles` at least). At
  most one part is rebuilt per frame, none on a turn's first frame (the camera starts its pan
  there, the heaviest frames), and a window one part a frame (its box, then each line: a
  status line of AMS text costs up to ~38k, the box ~30k); the menu's pointer is added at the copy, so the arrows
  redraw nothing. What does not change is made once at the start: the turn labels (Ra, De, Sq,
  Ch) ORed in at a bit offset, the names turned a quarter for the panels, the facing arrows;
  the panels' numbers come from a 3 x 5 digit table already turned. A panel costs ~21k to make
  (78k when its text was turned pixel by pixel). The scene is made again only when the reach
  rings appear or go, not at every change of mode.

## Numbers (ti-cycles, Titanium, datasheet timings)

| | cycles | at 12 MHz |
|---|---|---|
| ordinary frame (view copy 66-73k, with water glints up to 152k; the units on screen 93-185k: 6 to 10 of the 11; cursor + HUD 15-46k) | 176-345k (a 1300-frame battle, every player turn attacking through the menu: 344k at most) | 32 fps (frame-limited) |
| a turn change (the clock, the AT list) | +10-45k, one frame | |
| walking frame (the walker's cover mask made again each frame) | 290-350k | 32 fps |
| an attack (Act: 4 rings, two a frame; the swing, the number; its first frame drops the rings) | 290-340k | 32 fps |
| Move (the reach, its rings drawn two a frame) | 350k, rings complete in ~0.5 s | 32 fps |
| a tile picked (the path, the scene without rings) | 0.60 M | 0.05 s |
| one rotation frame (half resolution) | 2.1-2.3 M | 3 frames + the next view unpacked (2.2 M): 0.75 s per turn |
| start (the first view unpacked) | 2.5 M | 0.2 s |

Program: `fft.89z` 29,905 bytes (over the TI-89's 24,576 since the menu: Titanium first, the
TI-89 by `-pack` later, as decided at milestone 13), plus the units' sprites `fftu` (37.4 KB, read in place) and four view files of 9.1-10.3 KB (38.9 KB of archive; 209 KB
unpacked, see the history below). RAM: the unpacked view 52 KB, the scene buffer 17 KB, 11 KB
of work (~81 KB in all). Checked in TiEmu (Titanium, `ti-run`, the four views archived
at the transfer: attribute byte 3, see the `ti89-emulator` skill): the views, F5, a
selection with its rings (`x/emu_real.png`; packed views: `x/emu_zx0.png`, S, W, N and a
selection, local). The first version drew
its own scenery on the calculator (24 KB program, 4 x 17 KB of RAM): start 1.6 s, walking
frame 370k, selection 3.8 M.

## Limits

- The rotation is 0.54 s and the selection 0.31 s, not instantaneous: drawing polygon rows in
  C costs ~550 cycles per row (both planes), and a rotation frame has ~200 polygons.
- The TI-89 HW2 (AMS 2.09) was not run; the program is over its limit (29.9 KB: `-pack`
  planned), the game needs ~81 KB of free RAM (else it says so).
- Smaller data was measured and not kept: tile sprites deduplicated (FFT paints each face of
  each house: 686 sprites, 616 unique, 601 with 180-degree and mirror flips, 439 at 20 % of
  pixels tolerated: 102 KB), smoothing the speckles before packing (-10 %, windows lost);
  dropping the depth data (the old cover masks from the grid) would give ~22 KB but an
  approximate occlusion and a 370k walking frame.
- The four greys are a straight luminance cut: the textures read as speckles on roofs and
  paving (the user's choice, against our flat drawn version; a conversion tuned per material
  and lit faces was tried and declined, `x/greys_view0.png`). The rotation frames are one
  grey per face, not the textures.
- The camera has one elevation (FFT has two), no zoom; the map has only its first level (FFT's
  second level, bridges over passages, is empty on Gariland).
- Roofs are walkable as in the terrain data.
- No skills, items or AI yet (milestones 18-22): enemies only wait; the attack has no weapon
  sprite (FFT draws it apart) and Act opens the weapon's targets directly (no submenu).
- The ordinary frame's margin is thin with ten units on screen (345k of 360k). Picking a tile
  (0.60 M: the path) is a one-frame stall; a Move's rings take ~0.5 s to appear (two a frame).
- The cursor and the reach rings are flat diamonds on sloped tiles too (milestone 21).
