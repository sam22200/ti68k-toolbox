# Final Fantasy Tactics: Magic City Gariland (tech demo)

An isometric battlefield on the TI-89 / Titanium in the style of Final Fantasy Tactics (PS1,
1997): the real Gariland map (10 x 15 tiles, heights 0 to 10) seen from four camera
orientations, an animated 90-degree rotation, the Gariland battle's eleven units at their
start tiles (Delita, five enemies, Ramza and four recruits, their stats drawn and computed as
FFT does), FFT's turn order (its CT clock, checked on the original's first 46 turns), depth
sorting and occlusion by the buildings, the player's units moved over the terrain with
their own Move / Jump on their turn, and FFT's Attack (its range, hit chance, damage, zodiac,
critical hits and knockback, checked on the original's previews and attacks) and the units'
abilities (what each one knows drawn as FFT does: Dash, Throw Stone, Heal, Wish, the Chemists'
items from the party's bag; checked on the original's previews and outcomes), KO and the end
(the death counter, crystals and chests, EXP, JP and level ups checked on the original,
victory, defeat and the results), and FFT's AI for the enemies and Delita (its scores, its
ranking drawn on the hit chance, its moves; 129 of 136 of the original's turns played the same,
as are its move ranges). Not a port: our own engine on the Portable Game Runtime;
the heights and the scenery (the map's own textured mesh, drawn in four greys on the PC), the
units (FFT's own battle sprites of Ramza, Delita, the Squires and the Chemists, scaled 0.6) and
the battle's numbers come from the local disc (`tools/extract.py`, `tools/units.py`,
`tools/battle.py`); the cursor, shadow, markers, hit sparks and rotation greys are ours
(`tools/art.py`). Its presentation: FFT's throw pose and hit burst (seen on the original), the
cursor and reach rings drawn on each tile's real slope, a camera that frames the action, and
every frame of the player's turns within the 360k budget (`ROADMAP.md` milestone 21). From
playing it: [2nd] on a tile drawn over by a higher one picks that one, the cursor dashed behind
the scenery, the turn order on [APPS], "Quit?" on [ESC], HP above MP (milestone 22). The
original's pace, measured on it: walks, blows, the EXP and the JP, the AI's cursor gliding to
its target, its preview and its ability's name, its camera turned when a unit is more than half
hidden (milestone 23). The reach as FFT's tiles (white, a dark edge drawing the grid), all
shown at once, [MODE] drawing them over the scenery in front (milestone 24).
Next (`ROADMAP.md` milestones 25-28): the hit flash and the weapons, an
FFTA-style interface, the water, eight orientations; then the whole battle on the calculator
(milestone 29). Study notes: `RE_NOTES.md`.

## Controls

| Key | Action |
|---|---|
| arrows | the menu's entry; the cursor (relative to the view: UP = up-right, RIGHT = down-right); at Wait, the facing |
| [F5] / [F1] | rotate the camera 90 degrees clockwise / counter-clockwise |
| [2nd] or [ENTER] | pick the menu's or the list's entry; on a reachable tile: walk there; on a unit in the weapon's or the ability's reach: use it (a tile that cannot be picked: the tile drawn on the same pixels, when that one can); at Wait: end the turn; looking around: a unit's numbers, or back to the menu |
| [shift] or [ESC] | back: from the targets to the list they came from, from a list a level up, from the reach or the facing to the menu; in the menu, undo the move (before acting) or look around; [ESC] while looking around asks "Quit?" (No first; [ESC] again: No) |
| [APPS] or [CATALOG] | the turn order (PC: A), closed by any key |
| [MODE] | the reach tiles drawn over the scenery in front of them, or hidden by it but for dashes (PC: M) |
| results | [2nd] / [ENTER] the battle again (new draws), [ESC] quits |

The turns come as FFT deals them; the cursor and the camera go to the unit whose turn it is.
While a unit walks the camera stays until the walker nears the screen's edge; while it aims or
acts, it stays when the unit and its target are both on screen, else it centres the point
between them.
Enemies and Delita (a guest) play as FFT's AI does: once the camera is on the unit the picture
stands still for ~0.2 s while it thinks, then the cursor glides to the tile it goes to, its move
range is shown, it walks; when it or its target is more than half hidden the view turns (back to
the player's on the player's turn); the cursor glides to its target, the preview shows at the
top right (0.5 s), an ability's name 1 s more, it acts and turns to face the nearest enemy; a
unit hitting from its own tile backs off after. The pace is FFT's at 30 fps: a step 0.23 s (a
climb, a drop or a jump 0.33 s), an attack's blow 0.5 s after the order, the target hit 0.8 s,
the user's EXP then its JP, the menu back 3.1 s after the order. A player's unit opens its
menu at the top right: **Move** (once; the move can be undone with [shift] until it acts, which
FFT does not allow; canals can be waded, a step from water to water costs 2), **Act** (once: a
list as FFT's, **Attack**, the unit's skill set, Basic Skill, Item or Guts, and **Defend** for a
unit whose support ability it is (drawn as FFT draws one: ~8 %; Defending until its next turn),
then the abilities it knows, an item with how many are left and
greyed at 0; the reach shown, the cursor on a unit shows FFT's preview at the top right, `Hit
95%` and `Dmg 12` (`Dmg 3-12` for a random amount, `HP +30` for a heal, `- Poison` for a
cure), [2nd] uses it: the swing (FFT's throw for Throw Stone and the items), the ability's name
at the top right, the target hit (FFT's burst: a white blob, then sparkles flying apart), the
damage, `+20` or the status removed, or `Miss`, rising above it, a critical hit or a Dash
pushing it a tile, KO at 0 HP lying down, Phoenix Down raising a KO unit; a unit with a fifth
of its HP or less kneels; then `EXP+12` rising above the user, `LvUp` for a level up, then
`JP+10`), **Wait** (four arrows around the
unit, the one it faces black: the arrows turn it, [2nd] ends the turn) and **Status** (its
level, HP, MP, Move, Jump, Speed, PA, MA, EXP, JP and job level; Brave, Faith and the zodiac
stay hidden, as decided at milestone 13). A KO unit shows its death counter (3 to 0) above it;
at its next turn after 0 it becomes a crystal or a chest, which a unit walking onto it picks
up (a crystal: HP and MP full; a chest: `Got Clothes`, one of the dead unit's items, shown
only). The battle ends when every enemy is KO (`Victory!`) or when Ramza becomes a crystal or
a chest or the whole party, Delita included, is KO (`Game Over`): the results list each of
the player's units with its level, the EXP and JP it earned. The HUD shows the height under
the cursor at the top left in FFT units (`h2`, `h7.5` on a slope); [APPS] shows the turn order
in a window (FFT's AT list: the active unit, then the next five turns; enemies in dark grey).
The cursor is dashed where the scenery is in front of it, as the hidden reach tiles. At the
bottom corners, as in Metal Gear Solid: Portable Ops, two thin panels read sideways: the
active unit at the left, the unit under the cursor at the right (light grey: an enemy), each
with its name, its HP above its MP as `52/52` and `16/16` and a gauge beside each number (HP
black, MP dark grey). Scenarios (`fft(n)`, `--scenario n`):
0 the battle's start (a new draw each run: seeded from the clock; Delita's turn), and from Ramza's
first turn (the six units before him waited): 1-4 an enemy moved behind a house seen from each
view, 5 Ramza's reach, 6 mid-rotation, 7 Ramza walking, 8 an enemy in front of him aimed
at, 9 that enemy hit (the number rising), 10 that turn drawn with seed 10 (the tests), 11 the
female Chemist's items, 12 her Potion aimed at Ramza, 13 Ramza's Wish landing on the recruit
south of him, 14 four enemies KO and the last at 1 HP in front of Ramza, 15 the recruits KO,
Delita in front of Ramza at 5 HP, 16 the AI on both sides from the start (a whole battle plays
itself: won in ~10,000 frames); above 16 Ramza's turn drawn with seed n.
Key scripts: `keys/turn.txt` (a whole turn through the menu), `keys/attack.txt` (Ramza
attacks, moves and waits) and `keys/skills.txt` (Ramza's Wish, then Wait), all from `fft(10)`;
`keys/victory.txt` (from `fft(14)`: the last enemy knocked out, the results) and
`keys/defeat.txt` (from `fft(15)`: Ramza's Wish knocks him out, the enemies knock Delita out:
Game Over).

## Build

```sh
make map.h        # once: reads MAP/MAP022 from roms/ps1/Final Fantasy Tactics (USA)/ (local):
                  # map.h and the four views fftv0.bin .. fftv3.bin, ZX0-packed (~2 min)
make units.h      # once: the units' sprites from BATTLE/*.SPR, TYPE1.SHP and TYPE2.SHP (a second):
                  # units.h and the data file fftu.bin (37.8 KB, big-endian rows, mirrored too;
                  # the crystal and the chest from BATTLE/OTHER.SPR)
make battle.h     # once: the battle's units, jobs, items and weapons, generation tables, the
                  # zodiac table, the skill sets, abilities and the bag (SCUS_942.21, ENTD,
                  # BATTLE.BIN)
make test         # headless tests (rotations, heights, the units' stats against the original's,
                  # their draw, reach, occlusion, team marker, keys, the turn order against the
                  # original's 46 turns, attacks against its previews and attacks, abilities
                  # against its cases and trials, rewards against its six actions, the death
                  # counter, pick-ups, a battle won and one lost by keys, the move ranges and
                  # the AI against the original's, a battle the AI plays both sides of, Defend);
                  # the first run makes oracle.h (the oracle below, 30 s, its next 40 turns,
                  # 20 s, its attacks, 9 min, its abilities, 10 min, its rewards, 7 s, its AI's
                  # turns, 2 min, then tools/battle.py --check); ./fft_test --ai lists each
                  # turn of the original's AI that ours plays otherwise
make pc           # SDL window: ./fft_pc   (F1 / F5 on the PC keyboard too)
make ti           # fft.89z, fftu.89y and fftv0.89y .. fftv3.89y: send all six, archive the five
                  # data files (read in place), run fft()
make xcheck       # the TI binary under ti-cycles = the PC, 16 scenarios
../../tools/pyenv/bin/python tools/oracle.py DISC.cue ../../sources/fft_ps1/gariland_t1.state trace.txt
                  # the original at Ramza's first Gariland turn (30 s, RE_NOTES.md)
../../tools/pyenv/bin/python tools/oracle.py --turns DISC.cue STATE 40 turns.txt
                  # from there, the AI playing both sides: 40 turns (unit, CTs, HPs)
../../tools/pyenv/bin/python tools/oracle.py --attack DISC.cue STATE 40 400 attacks.txt
                  # from there, Ramza's attack: 40 previews with stats poked, 400 attacks
../../tools/pyenv/bin/python tools/oracle.py --skills DISC.cue STATE 300 skills.txt
                  # from there, Ramza given every ability: 13 cases, 300 Dash and Throw Stone
../../tools/pyenv/bin/python tools/oracle.py --rewards DISC.cue STATE rewards.txt
                  # from there, six actions: every unit's EXP, level, JP before and after
../../tools/pyenv/bin/python tools/oracle.py --ai DISC.cue STATE 40 12 ai.txt
                  # from there, the move range FFT shows two units, its AI's first 40 turns (each
                  # unit's state and what the actor did) and 8 built situations replayed 12 times
../../tools/pyenv/bin/python tools/battle.py DISC.cue --check STATE TRACE [oracle.h turns.txt attacks.txt skills.txt rewards.txt ai.txt]
                  # its eleven units against our model (stats, ranges, equipment, start tiles,
                  # abilities known, support); oracle.h: the test fixture (units, the turns with
                  # their CT bonuses, attacks, abilities, rewards, the AI's turns, move ranges)
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
  orientation changes (the rows of the reach rings put back as they go). Two allocations: AMS refuses blocks above ~64 KB.
- **Reach tiles** (milestone 24: style A of a sheet of three, `x/m24_styles.png`, local: a
  white diamond, its edge 2 px of dark grey, the diamonds overlapping so the edges draw the
  grid) are drawn into that scene copy, but where the view has something in front of
  their pixels: there the view shows, but for 2-pixel dashes: a tile behind a house keeps a
  dashed edge over it (the user's pick over the whole tile and a checkerboard,
  `x/bench_cases_cachees.png`, local); no cost per frame. One pass per 16-bit word, its two
  depth bytes compared at once (diagonals < 128: `t + 127 - depth` sets bit 7 when `t > depth`),
  each row's three words computed once: ~20k a tile (+3k its shape, +3k the screen's copy).
  [MODE] (`st.over`) skips the depth: the tiles drawn over the scenery in front, ~12k. The cursor's line is 4 px per row (`x/bench_curseur.png`).
- **Markers on slopes.** A flat tile gets the 24 x 12 diamond at its height; a sloped one
  (25 of 150 at Gariland: ramps of 1 or 2 units, a corner raised) the outline of its four
  corners at their heights, drawn by `art.py` by the distance to the quad's edges (so a tilted
  tile's near-horizontal edge gets a line too): 13 shapes (ramps and single corners up by 1 or 2
  units, the front corner 2 up seen edge-on left flat), ring and cursor, picked per tile and view
  from the corners' lifts (`marker`).
- **Every frame**: the camera's 160 x 100
  window is copied from it at any pixel (32-bit shifted words, ~107k; once the camera stands
  still, that copy is kept aligned to the screen in 4 KB, `W->bg`, made half a frame at a time,
  patched as rings come and go, and put back by plain long moves, ~25k), the units on screen are drawn
  back to front (those off it skipped) through a **cover mask**: the pixels of their 16 x 26
  box whose depth is greater than the unit's diagonal, read from the view (both layers), so a
  building hides them exactly as it is drawn, except their contour: on the covered rows the
  sprite's mask is eroded twice (the opaque pixels whose four neighbours are opaque) and only
  that inside is taken away, so the white outline and the black line inside it stay drawn over
  the building (a hidden unit shows as an outline; no projection). The mask and its covered
  rows are kept per unit and made again only when its feet, depth or the view change (15k
  cycles each time); the frame's inside (eroded twice) is made once by `units.py` and stored
  in `fftu` with it, so only the rows that lose pixels to the cover are copied, the others
  drawn straight from the archived file (native big-endian rows, mirrored frames stored). The
  shadow is in the frames too (one sprite call fewer). Then the blinking cursor (always on top,
  as in FFT) and the HUD.
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
  the same frames), then six poses each facing the camera and away (hit, the swing's wind-up
  and blow, kneeling, lying KO, the throw: `TYPE1.SEQ`'s frames, a raised hand's top row cut,
  the lying ones drawn 4 rows higher to fit), 22 frames per sheet and 22 mirrored, each with
  its inside plane, 9.1 KB per sheet; six sheets (Ramza, Delita, Squire and Chemist, male and
  female).
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
- **Reach.** FFT's moves, measured on the original's move ranges (`spread`; `RE_NOTES.md` §
  Moving): a step costs 1, from water to water 2, never into water from land with the last
  point; Jump bounds a step's height in half units (out of water, from its surface); a jump over
  one tile (2 points, 3 from water) to a tile no lower by at most Jump half units, over lower
  ground with no enemy; enemies block, allies (and the guest Delita) can be passed but not stood
  on, nor the trees (FFT's untargetable tiles). The moves out of each tile are listed once
  (`edges`, 2.4 KB); the spread is Dial's algorithm (4 queues by cost). The reachable tiles get a marker at their standing height (a black diamond line with a white
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
- **Walking.** Tile by tile along the BFS path, FFT's pace (measured on the original's
  movement record: 14 frames a tile at 60 Hz, ~21 for a jump up): 7 frames a tile, 10 for a
  climb, a drop or a jump over a tile with a hop (tables of 64ths, no divide); while between
  two tiles the unit is sorted after both.
- **Turns.** FFT's clock (`ct_next`, `ct_end`): each tick every unit's CT grows by its Speed;
  the highest CT of 100 or more acts (a tie to the lower index: Delita, the enemies, Ramza,
  the recruits), losing 100; at the end it gets 20 back for each of Move and Act unused, at
  most 60. A unit at 0 HP keeps its clock, each pick lowers its death counter (3 to 0), the
  next one makes it a crystal or a chest (a player's unit 3 times in 4, an enemy 1 in 2; the
  guest never) and its clock stops. The order shown is FFT's AT list (`turn_order`:
  each living unit's next turns by its clock alone, by tick then CT reached), made at each
  turn change (11 x 5 divisions, ~10k cycles).
- **Attack.** FFT's Attack command (`attack_range`, `attack_side`, `hit_chance`,
  `attack_damage`, `attack`, `attack_step`; the rules in `RE_NOTES.md` § Attack): the four
  neighbouring tiles within 3 levels below and 2.5 above get the reach rings; from the front
  the target evades with its class evade (5, Ramza 10), from a side or the back it is hit;
  damage XA x WP with the signs' compatibility (a 16-bit step each), critical hits 4 % (XA +
  0..XA - 1, the target pushed a tile if it lands free and no higher), at most 999; KO at 0
  HP (the turn order made again). The number is AMS text written once into the HUD's spare
  rows and drawn as a 32-pixel outlined sprite. The timeline is the original's at 30 fps
  (`ACT_*`, `RE_NOTES.md` § Pace): standing 5 frames, the wind-up, the blow at 13, the hit at
  16 (hit pose 24 frames, the number 24), the EXP at +29, the JP at +51, the menu at +78 after
  the hit (+28 without a gain). The battle's draws come from our own 32-bit
  xorshift (the runtime's 16-bit wyhash tied two draws in a row: every critical added at
  least 1).
- **Abilities.** (`known`, `can_use`, `skill_range`, `skill_hit`, `skill_amount`, `skill`;
  the rules in `RE_NOTES.md` § Abilities) `battle.py` reads the skill sets, the abilities
  (kind, Y, range, vertical tolerance, flags) and the status records from the disc, keeping
  only what a level 1 unit can learn (199 JP at most: no Accumulate or Yell); each battle draws
  what every unit knows by FFT's rule (JP 100..199 spent in the set's order on passing learn
  rolls; Ramza knows Wish). A range spreads its steps over the map, heights aside, then a
  vertical tolerance where the ability has one; Dash and Throw Stone deal 1..Y times PA with
  the zodiac and push by a Brave roll, Throw Stone is evaded as Attack, Defending halves the
  base hit; cures miss without their status, Phoenix Down needs a KO target; the party's
  items run out, the enemies' do not, an item is spent even when it misses.
- **KO and the end.** (`rewards`, `job_level`, `outcome`, `pick_up`; the rules in `RE_NOTES.md`
  § KO and the end) An action that does something earns its user 10 EXP (a kill 20 / the
  target's KO count) + the target's level - its own, and 8 + 2 x its job level + level / 4 JP,
  a quarter to each living unit of its side in the same job; 100 EXP is a level up (the stats
  made again from the raw ones grown, HP and MP kept). A crystal or a chest stays a unit with
  `dc` DC_CRYSTAL / DC_CHEST (`unit_at` ignores it: it is walked onto, never targeted), picked
  up at the end of a walk, then DC_GONE and off the map. The outcome is checked after each
  action and at each turn; the results are drawn into the scene buffer one line a frame (AMS
  text) and copied each frame. The death counter is a 3 x 5 digit sprite made once.
- **Enemy AI.** (`ai_setup`, `ai_ability`, `ai_choose`, `ai_facing`; the rules in `RE_NOTES.md`
  § Enemy AI) FFT's own decision without its simulation: each usable ability's best (origin,
  target) by the change of the field's worth (a unit: HP x 128 / max HP, -32 Critical, -192 KO,
  its side + and the other -; damage at the middle roll), kept if it beats standing on the
  fallback tile; the best eight drawn in order on their hit chance; the tiles by their danger
  (10 - steps, summed over the enemies acting before the unit's next turn) and their steps from
  the focus (the nearest enemy; the nearest healer or crystal for a Critical unit retreating or a
  guest under 45/128 HP); Throw Stone and thrown items need a clear line (FFT's projectile, every
  2 units through the terrain's columns and the units' boxes); Attack tried from every tile, the
  others once per target from the safest tile. Spread over frames: the picture stands still
  first (the animation clock stops; once two frames in a row have the same checksum the frame is
  no longer drawn), then a step a frame within ~270k cycles (the setup's halves, 90 tiles of the
  spread from the focus, ~50 candidates): 5 frames a turn on average, 6 at most. Shown at FFT's
  pace: the cursor glides a tile every 2 frames to where the unit goes and to its target, the
  preview 15 frames, an ability's name 30 more. Its camera (`view_hides`, `ai_camera`): on the
  plan's frame (not drawn) each view's hiding of the actor (on the tile it acts from) and of its
  target is read from the map's corner heights at four columns of the sprite (the other views'
  depth is not unpacked; as the view's depth on 558 of 560 tiles and views); more than half of
  either hidden: the view turns to the one hiding the least before it aims, and back to the
  player's view on the player's turn.
- **Reach tiles at once.** Old tiles go by putting back the scene rows they are on from the
  view (~0.5k a row, and into the screen's copy), as many rows a frame as the budget allows
  (8 frames at most); the new ones come after, all in one frame: a stall, as F1 / F5 (Ramza's
  Move, 44 tiles, ~1.24 M, ~0.92 M over the scenery; an attack's 4 tiles ~0.1 M; before,
  ~4 a frame: Move's 44 took 13 frames). The whole scene copied in one frame was 130k. Picking a
  tile reuses the steps of the reach just shown (`make_path`: a spread less, ~90k).
- **A frame's budget** (`FRAME_K`, thousands of cycles): before drawing, what the frame costs
  without its optional work is reckoned (19k a unit on screen, 16k a cover mask to make, the
  scene's copy 25k or the view 107k, the update's known heavy work: a turn's start, an aim, a
  reach, an AI facing); the optional work (new rings, a HUD part, the screen's copy kept) waits
  for a frame with room for it, 8 frames at most (the copy 2), then is done anyway. Things
  appear a frame or two later; the frames keep within the budget.
- **HUD.** Each part (the height, the menu, status, turn order or "Quit?" window, the two panels) is drawn
  into a work buffer when what it shows changes (as the frame's budget allows) and copied each
  frame by words (~5k cycles for all): drawn every frame with `draw_rect` and AMS text they cost ~40k and more (ExtGraph's
  fill and the ROM's `memcpy` have a high fixed cost per call, under `ti-cycles` at least). At
  most one part is rebuilt per frame, none on a turn's first frame (the camera starts its pan
  there, the heaviest frames), and a window one part a frame (its box, then each line: a
  status line of AMS text costs up to ~38k, the box ~30k); the menu's pointer is added at the copy, so the arrows
  redraw nothing. What does not change is made once at the start: the names turned a quarter
  for the panels, the facing arrows;
  the panels' numbers come from a 3 x 5 digit table already turned. A panel costs ~21k to make
  (78k when its text was turned pixel by pixel). The scene is made again only when the reach
  rings appear or go, not at every change of mode.
- **Stacked tiles** (milestone 22). In every view 21-27 screen spots are shared by two tiles: a
  canal tile at height 0 and the street tile 2 units higher one tile diagonally in front draw
  their markers on the same pixels (the user's "can't attack" and "teleport" reports: Ramza in
  the canal, the Squire on the street beside him). [2nd] on a tile that cannot be picked
  (`pickable`) tries the tile with the same marker position (`twin`). The cursor's mask is
  dashed where the view's depth is in front of it (`cursor_mask`: the depth bytes compared
  8 pixels at a time, the hidden rings' dashes), made again when the cursor or the view
  changes (8-19k), as optional work; on an AI turn the cursor stays whole (its frames are the
  fullest).

## Numbers (ti-cycles, Titanium, datasheet timings)

| | cycles | at 12 MHz |
|---|---|---|
| a player's turn (menus, the reach, a tile picked, walks, attacks, abilities, Wait, the results; every frame of the five key scripts and of 15 scenarios) | 160-250k on average a state, 329k at most | 32 fps (frame-limited) |
| a battle the AI plays both sides of (fft(16), its first 14,000 frames: pans, reach tiles, walks, blows, up to 11 units on screen) | 359k at most, its reach tiles (114 stalls, ~0.67 M median, 2.5 M at most with a view change) and its camera turns (allowed stalls, as F1 / F5) | 32 fps |
| an AI unit's view picked (its plan's frame, not drawn) | 6k, 93k when the other views are tried | |
| its parts: the view copied (the camera moving) / its screen-aligned copy put back (still) | 107k / 25k | |
| a unit on screen (its cover mask made again: +16k) | ~19k | |
| a reach tile drawn into the scene (all in one frame) | ~26k, ~18k over the scenery ([MODE]) | |
| an AI unit thinking (the picture still: the frame not drawn; a step a frame) | 94k on average, 270k at most | 5 frames a turn (6 at most), after the camera's pan |
| one rotation frame (half resolution) | 2.1-2.3 M | 3 frames + the next view unpacked (2.2 M): 0.75 s per turn |
| start (the first view unpacked) | 2.5 M | 0.2 s |

Program: `fft.89z` 56,226 bytes (over the TI-89's 24,576 since the menu: Titanium first, the
TI-89 by `-pack` later, as decided at milestone 13), plus the units' sprites `fftu` (55.4 KB, read in place) and four view files of 9.1-10.3 KB (38.9 KB of archive; 209 KB
unpacked, see the history below). RAM: the unpacked view 52 KB, the scene buffer 17 KB, 22 KB
of work (the AI's and the moves' tables 7 KB, the screen's copy of the scene 4 KB; ~92 KB in
all). Checked in TiEmu (Titanium, `ti-run`, the four views archived
at the transfer: attribute byte 3, see the `ti89-emulator` skill): the views, F5, a
selection with its rings (`x/emu_real.png`; packed views: `x/emu_zx0.png`, S, W, N and a
selection, local; milestone 21's build: a Chemist's turn with its rings, `x/emu_m21.png`). The first version drew
its own scenery on the calculator (24 KB program, 4 x 17 KB of RAM): start 1.6 s, walking
frame 370k, selection 3.8 M.

## Limits

- The rotation is 0.54 s and the selection 0.31 s, not instantaneous: drawing polygon rows in
  C costs ~550 cycles per row (both planes), and a rotation frame has ~200 polygons.
- The TI-89 HW2 (AMS 2.09) was not run; the program is over its limit (45.8 KB: `-pack`
  planned), the game needs ~92 KB of free RAM (else it says so).
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
- The AI plays the enemies and the guest only (FFT's auto-battle for the player's units is a
  test scenario); FFT's mostly-support policy is not coded (no Gariland unit can reach its
  77/128 helpful abilities: an Attack is always there, an enemy Chemist only uses Potion at
  level 1); FFT's tile priorities are never reset between turns (the unit's own tile can then
  rank first): left out, it changed none of the 136 turns checked. Of the original's 136
  turns, 7 take another action (5 of them one of our ranking that its draws on the hit chance
  picked), 11 end on another tile and 8 face another way. The attack has no weapon sprite (FFT draws it apart); Dash, Heal and Wish
  use the swing pose, Throw Stone and the items FFT's throw (FFT has an item pose too); a random amount is previewed as its range (FFT
  shows one roll); Wish shows the target's heal only (the user's cost drops in its panel).
- A crystal only restores HP and MP (FFT also offers its abilities); a chest's item is shown,
  not kept (no inventory); JP shared to a unit of another job are not kept (FFT stores them
  in that job); the results have no war trophies (FFT's Mythril Knife, Phoenix Down, Potion);
  [ESC] does not save the battle yet (milestone 29).
- The reach tiles appear in one stall (Ramza's Move ~0.1 s; a view change with them up to
  0.2 s); pre-shifted flat tiles (the x of a tile is a multiple of 12) would roughly halve it.
  A battle lost by keys has 6 frames over the budget (up to 371k); the budget delays other
  optional work by a frame or two (the HUD's parts follow the camera's pan).
- The hit burst is drawn for damage only (FFT has other effects for heals and statuses).
