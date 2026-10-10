# Roadmap

Each milestone is tested headless (`make test`, `make xcheck`), the emulator at the end.

1. **Terrain in four orientations** (done): the Gariland map from the disc, view rotation of
   tiles and corners, the projection, back-to-front composition of flat tiles.
2. **Heights and walls** (done): wall segments down to the front neighbour, outlines where
   heights differ, sloped tiles as polygons.
3. **Buildings and depth** (done): houses with sloped roofs, chimneys, trees, boxes; one scene
   per orientation composed once; textures and materials (`tools/art.py`).
4. **Units, occlusion, movement** (done): three units, cover masks for occlusion, cursor, reach
   (Move 4 / Jump 3), paths, walking with hops; the thief hidden from two views, seen from two.
5. **Speed and polish** (done): the rotation animation at half resolution, four scenes at the
   first frame, reach drawn locally, the program under 24 KB, TiEmu run (Titanium).
6. **The game's own scenery** (done, the user's choice): the textured mesh drawn per view on
   the PC in 4 greys with a two-layer depth per byte, four archived data files read in place,
   cover masks and reach rings from that depth; no composition on the calculator.
7. **Packed views** (done): ZX0, 209 KB of archive down to 38.9 KB, one view unpacked per
   turn (0.18 s), the same screens (xcheck checksums unchanged).
8. **Rotation greys from the views** (done): each face of the rotation frames takes the mean
   grey of its pixels in the four views, rounded (solid: checkerboards for the half greys were
   tried and declined), instead of a flat grey per material; +2 % per rotation frame (2.06 -> 2.11 M), TI = PC on the turn
   frames. A grey conversion tuned per material was tried first and declined (the user kept
   the straight luminance cut).
9. **FFT's units** (done): Ramza, Delita, Agrias and a thief from the disc's battle sprites
   (`tools/units.py` -> `units.h`, never committed), four directions (two drawn, mirrored),
   FFT's idle and walk animations, facing set by the walk; frame ~270k (units 116k for four),
   program 23.2 KB.
10. **Proportions and readability** (done): the units scaled 0.6 (FFT's height against the
    tiles, measured on a capture of the game; 16 x 26, the eyes kept), a hidden unit's contour
    drawn over the building (its mask eroded twice on the covered rows), the reach marker as a
    black line with a white one outside (the user's pick of four); frame ~305k (units 152k),
    program 24,481 bytes (95 left under the TI-89's limit).

11. **Life and teams** (done): the units' sprites in an archived data file `fftu` (program
    24,481 -> 19,205 bytes), the eyes placed on their own (1 x 2, a skin pixel between, the
    face one grey lighter), the canal's water glints sliding along it (~13k cycles), a diamond
    above enemies; frame ~320k, TI = PC on 8 scenarios.

12. **Readability** (done): the enemy marker a black diamond (the user's pick of eight shapes),
    the cursor's line 4 px per row, reach rings behind a building kept as 2-pixel dashes over it
    (computed with the scene, no cost per frame).

## The battle (milestones 13-22)

Goal: the Gariland battle playable as in the game, from the deployment to the victory
screen: the game's units, their stats, FFT's turn order, Move / Act / Wait, attacks and the
abilities those units have, the enemies acting on their own, KO, the death counter, victory
and defeat. Each milestone is playable and tested on the PC before the next (`make test`,
scenarios through `game_scenario(n)`, `make xcheck`), measured under `ti-cycles`, the
emulator only at 17 and 22.

**Fidelity rule.** The numbers (stats, CT, hit chance, damage) come from the disc and the
running original, labelled in `RE_NOTES.md`; the rules are FFT's (the community's Battle
Mechanics Guide and data maps are the reading aid, every formula checked on the oracle
before it is used). Where FFT would not fit the TI (time, RAM, screen), the TI design wins
and the difference is written down.

**The oracle.** Gariland is the second battle. A save state at its first turn (a memory card
saved by hand, `psxrun.py --memcard`, as Alundra; or a key script through the Orbonne
prologue) gives the reference: the battle units' array in RAM (address and layout found by
experiment: dumps before and after a move, an attack; the community's maps only say where
to look), traced turn by turn. Each rule is checked by replaying a situation in the original
and in our engine (same units, same RNG state when it can be set, else distributions over
many runs).

**Budgets.** Frame <= 360k cycles at any time (the battle adds a menu, numbers and poses, not
scenery); AI and other heavy work spread over frames (a thinking unit costs at most ~150k
per frame, the turn lasting a fraction of a second); RAM ~75 KB as now plus the battle
(< 4 KB); the rules data in an archived data file `fftd` (abilities, jobs, items: read in
place); sprites added to `fftu` (attack, hit, KO, crystal poses: ~6 KB per unit, read in
place). The program will grow past the TI-89's 24,576 bytes: Titanium first, the TI-89 kept
with `-pack` (decided at milestone 13).

13. **Decisions and the oracle** (done: `RE_NOTES.md` § Battle decisions, `tools/oracle.py`). `/grilling` on the scope (defaults: Gariland only, its own
    units and positions, no story text but one line before and after, no job system or
    shops, the game's RNG replaced by ours, Brave / Faith / zodiac kept as hidden numbers,
    the TI-89 by `-pack`). The save state at Gariland's first turn; the battle unit struct
    found and documented (position, HP, MP, CT, speed, PA, MA, Brave, Faith, job, equipment,
    status, facing, team). Done: a trace of the original's first turns read back from RAM.
14. **The battle's units** (done). Gariland's ENTD, the new game's Ramza and the academy's
    recruits (`tools/battle.py` -> `battle.h`: a generated header while the data is small,
    ~0.3 KB; the `fftd` file when the abilities come), their sprites (six sheets, the Chemists
    from `TYPE2`, mirrored frames stored: `fftu` 18.7 KB), FFT's draw and stat formulas (raw
    stats, growth, multipliers, equipment) at each battle with our RNG. All eleven units' HP,
    MP, Speed, PA, MA, Move and Jump equal the original's at Ramza's first turn (`battle.py
    --check`, and in C on its raw values: `make test`); per-unit Move / Jump in the reach.
    Units off screen skipped, cover masks kept per unit, uncovered rows drawn straight from
    the data file: the frame stays at 190-345k with eleven units (program 21,171 bytes).
15. **Turn order** (done). FFT's clock read in the decompilation (every tick CT += Speed, the
    highest CT >= 100 acts, a tie to the lower index, -100, +20 per unused Move / Act, at most
    60; a KO unit's clock runs its death counter down) and checked on the original's first 46
    turns (`oracle.py --turns`: the player's units handed to FFT's AI; every turn's unit and
    every unit's CT equal, four KOs and a crystal included); the order in the HUD (FFT's AT
    list, the next six turns, enemies on light grey); the cursor and the camera to the active
    unit; enemies wait, a player's unit moves or waits ([diamond]). The HUD strips cached (a
    40-turn battle stays under 356k a frame; program 22,935 bytes).
16. **The turn and its menu.** Move / Act / Wait / Status, a move undone before acting, the
    facing chosen at Wait (the four directions shown on the tile), the active unit's panel
    (name, job, HP, MP, CT) and the cursor's target panel; a menu readable in 160 x 100 (a
    small font window at the bottom, one line per entry); the turn played by key scripts.
    Done: a whole player turn by keys in a test, the enemies only waiting.
17. **Attack.** The weapon's range in tiles with FFT's height rules (vertical tolerance; bows
    and their arc where the battle has them), the target chosen on the map with its hit
    chance and predicted damage shown, FFT's evasion (class, shield, accessory; front, side
    and back), the weapon's damage formula, zodiac compatibility, the critical hit and
    knockback, the damage number rising above the target, the attack and hit poses, KO at 0
    HP. TI and emulator run (the first battle-menu frame measured on hardware paths). Done:
    hit chance and mean damage equal to the original's over repeated trials in the oracle.
18. **The units' abilities.** The skills of the jobs present at Gariland (decided at 13 from
    the deployment: Squire's Basic Skill, Chemist's Items, Thief's skills, Ramza's own...):
    targeting areas, MP and charge time where they have one (a charging unit in the turn
    order, a spell landing later), status effects they cause, the items in the party's
    bag (Potion, Phoenix Down...). Reaction and support abilities the units carry
    (Counter Tackle...). Done: each ability's effect equal to the original's on one case.
19. **KO, the death counter and the end.** A KO unit's counter (3 turns, then a crystal or a
    chest; raising it in time with a Phoenix Down), the victory condition (all enemies KO,
    crystal or chest), the defeat (Ramza or the whole party out), the rewards the units earn
    as they act (EXP and JP per action, a level up shown) and the end screen. Done: a battle
    played to victory and one to defeat by key scripts.
20. **Enemy AI.** FFT's AI is a search over the unit's reachable tiles x actions x targets;
    the target is a scored search of the same kind kept small: for each reachable tile (Move
    4: <= ~40), the best action (expected damage, a kill, healing a hurt ally, a status) and
    the tile's danger (enemies able to reach it), spread over frames; the enemies' habits
    measured on the oracle (whom they attack, whether they retreat or heal) and kept. Done:
    every enemy turn decided in under ~0.5 s, its choices close to the original's on the
    same situations.
21. **Presentation.** Every pose the battle uses (attack, hit, KO, crystal, chest), the
    number and the hit flash, the menu windows drawn over the scene at a fixed cost, the
    turn order and HP readable at a glance (outlined, 2-pixel details), the camera moving
    to the action. Frame <= 360k in every state.
22. **The whole battle on the calculator.** Titanium then TI-89 HW2: one full battle in TiEmu
    (a GIF), the cycles per state, the program and data sizes, the free RAM.

Engine, any time:
- the TI-89 HW2 (AMS 2.09) run and its free RAM (single-buffer fallback);
- a faster rotation (fewer, larger polygons per column, or a 68000 span filler: ask first);
- FFT's second camera elevation;
