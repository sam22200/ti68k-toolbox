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

## The battle (milestones 13-29)

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
16. **The turn and its menu** (done). Move / Act / Wait / Status, a move undone before acting, the
    facing chosen at Wait (the four directions shown on the tile), the active unit's panel
    and the cursor's target panel, only what the player uses (name, HP, MP; no CT, Brave,
    Faith or EXP, which FFT's panel shows but nobody reads), drawn in thin columns against
    the screen's left and right edges as Metal Gear Solid: Portable Ops does (its life and
    stamina are thin vertical gauges at the bottom left, the weapon and its ammo written
    turned 90 degrees along the right edge): HP / MP as vertical gauges, the name as text
    turned a quarter, pre-rendered like the turn labels, so the map keeps its full height;
    a menu readable in 160 x 100 (a small font window, one line per entry); the turn played
    by key scripts.
    Done: a whole player turn by keys in a test (`test_menu`, `keys/turn.txt`: TI = PC), the
    enemies only waiting. The panels show HP and MP as numbers too (`52/52`, `16/16`, a 3 x 5
    digit font, the user's request) beside their gauges; the menu at the top right; a window
    or panel drawn one part a frame, the scene made again only for the reach rings: a
    1300-frame battle stays under 345k a frame; program 26,445 bytes.
17. **Attack** (done). The weapon's range in tiles with FFT's height rules (vertical tolerance; bows
    and their arc where the battle has them), the target chosen on the map with its hit
    chance and predicted damage shown, FFT's evasion (class, shield, accessory; front, side
    and back), the weapon's damage formula, zodiac compatibility, the critical hit and
    knockback, the damage number rising above the target, the attack and hit poses, KO at 0
    HP. TI and emulator run (the first battle-menu frame measured on hardware paths). Done:
    hit chance and mean damage equal to the original's over repeated trials in the oracle.
    Done: FFT's formulas read in the decompilation (`RE_NOTES.md` § Attack); 40 previews of the
    original (stats, weapon, signs, sexes, facing poked) equal ours, and 400 executed attacks
    (4.75 % misses, 2.0 % criticals seen, 11.55 HP per attack) match 4000 of ours (4.2 %, 2.9 %,
    11.68). Act opens the reach (the four tiles within the height limits), the preview at the
    top right (Hit / Dmg), the swing, the hit, kneeling and KO poses (FFT's frames, `fftu`
    37.4 KB), the number rising, knockback, KO out of the turn order (`keys/attack.txt`: TI =
    PC). No bows at Gariland; the weapon sprite and FFT's Attack / skills submenu come later
    (21, 18). The rings' boxes put back instead of the whole scene copied, the rings drawn two
    a frame: an attack stays under 340k a frame, Move is no longer a stall (350k), a 1300-frame
    battle with attacks 344k at most; program 29,905 bytes. The emulator run is left for
    milestone 22 with the whole battle (the menus touch no hardware path; this session had no
    X display).
18. **The units' abilities** (done). The skills of the jobs present at Gariland (decided at 13 from
    the deployment: Squire's Basic Skill, Chemist's Items, Thief's skills, Ramza's own...):
    targeting areas, MP and charge time where they have one (a charging unit in the turn
    order, a spell landing later), status effects they cause, the items in the party's
    bag (Potion, Phoenix Down...). Reaction and support abilities the units carry
    (Counter Tackle...). Done: each ability's effect equal to the original's on one case.
    Done: what the units know read in RAM and its rule in the decompilation (JP 100..199 spent
    in the set's order on passing learn rolls): only Dash, Throw Stone, Heal, Wish (Ramza's
    only one) and the Chemists' Potion, Antidote, Eye Drop, Echo Grass, Phoenix Down can be
    known at Gariland; no charge time, MP cost, secondary set or reaction there (Accumulate,
    Yell and Counter Tackle out of reach; Equip Change no effect; Defend, the enemy Chemist's,
    with the AI). Act opens FFT's list (Attack, the skill set, its abilities; an item's count),
    the preview (`Dmg 3-12`, `HP +30`), the ability's name, `+20` or the status removed rising;
    the party's bag, the enemies' unlimited. 13 cases of the original (Ramza given every ability
    by poking his skill sets) equal ours, and 300 Dash and Throw Stone each match 4000 of ours
    (`oracle.py --skills`); our draws within 0.003 of the rule's odds; `keys/skills.txt`: TI =
    PC, 13 scenarios too. Frames 260-352k; program 33,765 bytes.
19. **KO, the death counter and the end** (done). A KO unit's counter (3 turns, then a crystal or a
    chest; raising it in time with a Phoenix Down), the victory condition (all enemies KO,
    crystal or chest), the defeat (Ramza or the whole party out), the rewards the units earn
    as they act (EXP and JP per action, a level up shown) and the end screen. Done: a battle
    played to victory and one to defeat by key scripts.
    Done: FFT's rules read in the decompilation (`RE_NOTES.md` § KO and the end) and six of
    Ramza's actions in the original (`oracle.py --rewards`: an attack 10 EXP and 10 JP, +2 to
    each ally of his job; a kill 20; a level up from 95 EXP, raw HP + raw / 12, HP kept) equal
    ours; the counter shown above a KO unit, 3 to 0, then a crystal (an enemy 1 in 2, a player's
    unit 3 in 4: ours 0.509, 0.748) or a chest, FFT's own images (`BATTLE/OTHER.SPR`), picked up
    by walking onto it (HP and MP full; the dead unit's item, shown only); `EXP+20` / `LvUp`
    rising above the user, EXP and JP in Status; victory (every enemy KO), defeat (Ramza a
    crystal or a chest, or the whole party KO), the results (level, EXP and JP per unit), then
    the battle again. `keys/victory.txt` and `keys/defeat.txt`: TI = PC, 15 scenarios too. A
    GCC4TI miscompilation found by `ti-cycles` (a `(u16)` product under a range test folded to
    a wrong odd constant: an address error). Frames up to 356k, one crowded pan 374k (5 KO
    counters; 362k without); program 36,579 bytes.
20. **Enemy AI** (done). FFT's AI is a search over the unit's reachable tiles x actions x targets;
    the target is a scored search of the same kind kept small: for each reachable tile (Move
    4: <= ~40), the best action (expected damage, a kill, healing a hurt ally, a status) and
    the tile's danger (enemies able to reach it), spread over frames; the enemies' habits
    measured on the oracle (whom they attack, whether they retreat or heal) and kept. Done:
    every enemy turn decided in under ~0.5 s, its choices close to the original's on the
    same situations.
    Done: FFT's decision read in the decompilation (`RE_NOTES.md` § Enemy AI) and computed on
    what an action changes: the field's worth, the 8 best ranked and drawn on their hit chance,
    the tiles by danger and steps from the focus, hit and back off, the retreat of a Critical
    unit, Delita's healer search, the Wait facing, Throw Stone's and the items' straight line;
    the enemies' Defend drawn as FFT draws a support (a command for the player's units too).
    Checked on 136 of the original's AI turns (`oracle.py --ai`: the battle and 8 built
    situations replayed 12 times): the same action 129, in our ranking 134, the same tile 125,
    the same facing 128, FFT's most frequent choice our first in every situation. Measuring its
    moves corrected milestone 11: canals waded (water to water costs 2), jumps over one tile,
    chimneys stood on, trees crossed (the map flag read as "can't walk" was not one); the two
    move ranges measured are exact. An AI turn thinks with the picture standing still (not
    drawn once two frames in a row are the same), a step a frame within 270k: 5 frames a turn
    (6 at most) after the camera's pan. `fft(16)`: the AI plays both sides to victory (~6,000
    frames), TI = PC, 16 scenarios and the key scripts too. A battle the AI plays averages
    343k a frame and peaks at 590k where rings come or go over a crowd (milestone 21); program
    45,849 bytes, RAM ~88 KB.
21. **Presentation** (done). Every pose the battle uses (attack, hit, KO, crystal, chest), the
    number and the hit flash, the menu windows drawn over the scene at a fixed cost, the
    turn order and HP readable at a glance (outlined, 2-pixel details), the camera moving
    to the action. The cursor and the ground markers (reach rings, targets, areas) fit the
    tile they sit on: several inclinations drawn (flat, and each slope direction of the
    tile, as the view shows it), maybe several sizes too, instead of one flat diamond on
    every tile. Frame <= 360k in every state.
    Done (grilling: the budget first, C before asm, FFT's throw only, the hit measured on the
    original, the camera rules, markers from the real corners, the HUD left as it is): FFT's
    throw pose for Throw Stone and the items (frames 28 / 30 after the wind-up, its animations
    58-59, seen on the original run headless); its hit burst (a white blob over the target, then
    sparkles flying apart: here a black-outlined blob for 2 frames, 8 sparkles for 6); the cursor
    and the reach rings drawn on each sloped tile's four corners (13 shapes by `art.py`); while a
    unit walks the camera stays until the walker nears the edge, while it aims or acts it stays
    when both units are on screen, else it centres between them. The budget: the screen-aligned
    copy of the scene kept once the camera stands still (25k instead of 107k a frame, patched as
    rings come and go), rings drawn by 16-bit words with their depths compared two bytes at once,
    rings dropped by scene rows, the units' eroded inside and shadow made by `units.py`, the AI's
    picture check without its multiply, a picked tile reusing the reach's steps, and a frame budget
    that delays the optional work (rings, HUD parts, the copy) a frame or two. Every frame of the
    player's turns within 329k; a battle the AI plays averages 236k (343k before), 8 frames of
    6,100 at 361-383k (an AI aim or facing panning over 8-9 units), the key scripts 6 of 2,375 up
    to 405k: a 68000 view copy would close the gap (asm: ask). Program 53,069 bytes, `fftu` 55.4
    KB, RAM ~92 KB.

Milestones 22-28 come from playing milestone 21 in TiEmu (grilling 2026-10-13). Every one keeps
each frame <= 360k outside the allowed stalls, asm only after asking, `make test` and
`make xcheck` passing, and a review sheet for every visible change.

22. **Bugs and small fixes** (done). The cursor on stacked tiles: in every view 21-27 screen spots
    are shared by two tiles (a canal tile at height 0 and the street tile 2 units higher one
    tile diagonally in front draw their markers on the same pixels), so the player aimed at a
    canal tile thinking it was the Squire's ("can't attack", "the farthest water tile can't be
    clicked"; the reach and the attack were right, the "teleport" after F1 the correct turn of
    a higher unit). Fixes: [2nd] on a tile that cannot be picked tries its screen twin; the
    cursor dashed where the scenery is in front of it (worked out when it moves, ~6-15k); the
    height shown (milestone 26). The turn order leaves the top strip for a window on APPS or
    CATALOG; ESC asks "Quit? Yes / No" (No by default); HP above MP.
    Done: `twin` (the tile with the same marker position, tried when [2nd] falls on one that
    cannot be picked: in the tests the user's attack and 6 moves of Ramza's first reach);
    `cursor_mask` (the view's depth bytes compared 8 pixels at a time, the hidden rings'
    dashes; 8-19k when the cursor or the view changes, optional work, the cursor whole on AI
    turns); `K_APPS` in the runtime ([APPS] row 5 bit 0, [CATALOG] row 3 bit 6; PC A) opening
    the turn order in a window (enemies in dark grey), any key closing it; `M_QUIT`; the
    panels' HP above MP; the strip keeps the height alone (until milestone 26), the view's
    letter gone. Removing the strip moved the HUD's parts onto heavier frames: its turn-start
    wait (nothing on a turn's first frame, then a frame with room) is kept without the strip.
    Frames as before or better: the key scripts 280-319k, the AI battle 7 frames over 360k
    (360-381k) instead of 8; program 53,806 bytes. TiEmu: the build ran, but typing in the
    emulator on the desktop became unreliable before [APPS] could be checked on the keyboard
    matrix.
23. **Pace** (done). The original's durations at 30 fps (ours were 2-3x faster: a walked tile 0.33 s
    instead of 0.13, the blow ~0.7 s after the order, an attack with its EXP ~3.2 s instead of
    1.6), measured more finely on the original (the hit pose, the menu's return). Before an AI
    action, FFT's own pauses: the cursor gliding to the target, the ability's name ~1 s. The
    AI's camera: before an action, the 4 views scored on how much of the actor and the target
    the depth leaves visible; a turn only when one of them is more than half hidden, back to
    the player's view on his turn. Then decide the asm view copy on the new numbers.
    Done (measured on the original run headless: the units' movement records
    `g_battle_unit_misc_data` from fft_decomp, the HP, pictures every 2-8 frames; `RE_NOTES.md`
    § Pace): a flat tile is 14 frames at 60 Hz (0.23 s, not 0.33), a jump up ~21; an attack
    hits 31 frames after Execute, the target's hit pose ~48, the number ~48, "EXP" from +60,
    "JP" from +102, the menu at +156 after the hit; an AI turn: the camera pans to the actor,
    the cursor glides to its target (~12), the preview ~30, then the blow (Gariland's AI acted
    with Attack: the ability's name kept at ~1 s from the roadmap). Ours at 30 fps:
    `WALK_FRAMES` 7, `CLIMB_FRAMES` 10 (climbs, drops, jumps over a tile: hop tables, no
    divide), `ACT_*` (wind-up from 5, blow 13, hit 16, hit pose 24, the EXP then the JP shown,
    the menu at hit + 78, + 28 without a gain), the AI's cursor gliding a tile every 2 frames
    to its tile and its target, the preview 15 frames, an ability's name 30 more. The AI's
    camera: `view_hides` reads how many rows of a unit's sprite a view hides from the map's
    corner heights (the other views' depth not unpacked; as the depth says on 558 of 560 tiles
    and views), `ai_camera` picks the view on the plan's frame (not drawn: 6k, 93k when the
    other views are tried), the turn played before the aim, the player's view back on the
    player's turn. Frames: the key scripts as before (280-321k), the AI battle 6 frames at
    360-389k in its first 14,000 (7 at 360-381k in 6,100 before) and the AI's camera turns
    (stalls as F1 / F5); `keys/attack.txt`, `skills.txt` retimed. Program 55,419 bytes. The asm
    view copy: not needed for the player's frames; the 6 AI frames over 360k are the only case.

24. **Move tiles** (done). All shown at once (a stall added to the allowed ones) instead of ~4 a
    frame; a sheet of styles to choose from: A a white centre with a dark edge drawing the grid,
    B a light grey centre with a black edge, C the grid lines only; a test without the depth
    (no dashes: tiles drawn over the scenery in front), its cost measured, to compare.
    Done: the user picked A (`x/m24_styles.png`, local: A, B, C and A without the depth) and
    both ways of A, switched by [MODE] (a new runtime key, `K_MODE`; PC: M). The tiles all
    drawn in one frame (`ensure_scene`), the old ones going as the budget allows (8 frames at
    most); `ring_into_scene` computes each row's three words once (26k -> 18k a ring, same
    pixels). A tile: ~20k with the depth, ~12k without (+3k its shape, +3k the screen's copy);
    Ramza's Move (44 tiles) ~1.24 M / ~0.92 M, a stall as F1 / F5. Frames: the key scripts
    297-345k (defeat 371k, 6 frames over), the AI battle 359k at most in its first 14,000
    frames (6 over 360k before), 114 reach stalls. Program 56,226 bytes. Not done: pre-shifted
    flat tiles (~5k a tile without the depth, ~12k with it), left for later.
25. **Blows** (done). The hit flash on the target's inside (the 4th plane): its greys up to light
    grey then white held ~8 frames, then back, the black outline kept; the target knocked back
    a pixel or two; the sparkles of milestone 21 dropped. The sword and the dagger from
    `BATTLE/WEP.SPR` (placed by `WEP1/WEP2.SHP` and `.SEQ`), outlined white, a blade at least
    2 pixels wide, shown during the blow only as in FFT (Gariland: Broad Sword and Dagger; the
    enemy Chemist unarmed). A frame-by-frame sheet of a blow.
    Done: original armed hands/body plus WEP1/2 level-strike blades, all four facings,
    sampled/rotated offline into archived `fftw` (40,704 bytes); weapon script, body frame,
    rotation and UV records checked on the original for both weapons. Two-pixel blades with
    a white outline, their own cached cover, drawn behind the body at its depth. Visible at
    frames 13-18, then put away; unarmed/skills show no blade. Successful damage flashes
    the fourth-plane interior: light 2, white 8, light 2; black contour/shadow stay. Visual
    recoil 2/1 pixels, then 1/1, then zero; no gameplay changes. Burst/sparkles removed.
    `make test`, 25 xcheck scenarios, 87 phase/replay PC/TI screens (`make check-blows`) pass.
    Every ordinary measured frame <= 360k: blow doors 307,218, AI battle 356,180, defeat
    replay 359,070 (its six old overruns removed by deferring optional work through crowded
    pans). Review: `make review-blows`, `x/m25_blows.png`, nine blows in eleven frames.
    Program 56,297 bytes. No new hardware path; TiEmu left for milestone 29.
26. **FFTA-style interface** (done; captured on the original, `x/ffta_*.png`). Bottom left a light
    grey band with a dark rule: the unit's portrait from its `.SPR` (19x29, rising above the
    band), its name, HP above MP, a line with Lv and CT. The action menu bottom right with a
    "MENU" tab and a ball cursor, sub-menus replacing it in place; while aiming, the target's
    band mirrored bottom right. Band and menu hidden during moves and animations; the sideways
    corner panels go. A large "2h" top right for the tile under the cursor, "depth 1" under it
    on water. FFT's active-unit marker from `EVENT/FRAME.BIN` above the acting unit; the
    enemies' diamonds kept. The generic units' names drawn at each battle from FFT's own name
    lists read on the disc (deterministic in tests and scenarios); Ramza and Delita keep
    theirs. Status full screen in FFTA's layout with FFT's fields (portrait, name, job, HP and
    MP with bars, Lv/Exp, Brave, Faith, zodiac, Move/Jump/Speed, PA/MA; a second page for the
    equipment and abilities). A review sheet before applying the layout everywhere.
    Done: six original 19x29 mirrored portraits, 510 generic names and the FRAME pointer in
    archived `ffti` (40,876 bytes), with pre-shifted AMS text. FFTA-style bands / MENU tab /
    ball cursor, target preview above the mirrored target band, height and canal depth top
    right; bands and menus hidden through movement / blows. Status fills the screen, LEFT /
    RIGHT switches stats and equipment / learned abilities. Names use separate seeded draws,
    unique by gender, heroes fixed. Original check: six VRAM portrait sheets (9,216 source
    pixels), pointer texture / palette and nine names match. Review before completion:
    `make review-interface`, `x/m26_interface.png` (15 states). Tests, 40 xcheck scenarios,
    116 UI phase / key screens and 87 blow / replay screens pass. Ordinary frame peaks:
    UI doors 313,278; full AI battle 357,270; defeat replay 359,840. The expensive frame
    creating the screen cache no longer forces optional HUD / cursor work as though it had
    already used it. Program 58,749 bytes; game buffers ~105 KB. No new hardware path;
    TiEmu remains milestone 29.
    Presentation revision requested after milestone 27: transparent 80x36 character details,
    cutout portrait left, small level below, name and stacked outlined HP/MP gauges with
    right-aligned values above; CT removed from the HUD. Height/depth text also has a white
    outline over the scenery. Current review: `x/character_hud.png`, all six portraits and
    empty/999-value bars in `x/character_hud_details.png`. Tests, 52 scenario comparisons,
    116 UI and 87 blow/replay screens pass; peak 357,308 cycles, program 62,075 bytes.
    Further compact revision: level overlaid on the portrait, gauges 34x2, complete details
    within the 29-pixel portrait height. Outer portraits face inward; a central chevron and
    inline damage/heal/chance prediction replace the TARGET popup. Review:
    `x/compact_attack_hud.png` and `x/compact_hud_details.png`.
    Gauge refinement: rounded white ends, shared text/bar left anchor, values ending at
    the bar's right edge; 40x2 fill keeps `MP 999/999` readable. Review `x/rounded_hud.png`
    and `x/rounded_hud_999.png`; the 357,410-cycle peak is unchanged.
    Target/pointer revision: enemy stats meet the portrait, both names stay, and the
    centered prediction moves above the arrow. Native 16x19 ring/crossbar/blade marker
    replaces the reduced source marker; top framing fits it and facing selection keeps
    the camera still when the actor fits. Tests, 52 scenario comparisons and 203 UI/replay
    screens pass; peak 357,400 cycles. Review `x/target_hud_marker.png` / `x/active_marker.png`.
    Menu/Status refinement: the active pointer shrinks to padded 16x13; a white glove
    extends beyond FFT-style framed menus. Equipment and ability rows can be selected
    and inspected, with ESC returning to the list; the terrain HUD keeps height only.
    Original Status field/help navigation observed through pad input and its decompiled
    equipment nodes; equipment changing remains a separate action. Tests, 58 scenario
    comparisons, 177 UI screens and 87 replay screens pass; peak 356,322 cycles.
    Review `x/m26_interface.png` / `x/status_details.png`.
    Pointer/glove refinement: outlined marker 8x15 with a round upper ring; glove 15x12,
    shorter and taller. Three-pixel sideways animation ends one pixel before the text; the white menu-top
    strip is removed. Same cross-checks pass, peak 356,348 cycles; `x/hand_menu.gif`.
    Final refinement: the terrain readout is fully removed, ring diameter 6 -> 4 pixels,
    and the glove uses FFA's Game Boy title-screen silhouette and palm shading. Framing
    and animation stay; tests and the same cross-checks pass, peak 354,528 cycles.
    Review `x/adventure_menu.gif`; program 64,525 bytes, buffers 106,050 bytes.
    Responsiveness revision (the user's TiEmu play): with eleven units on screen the reckoned
    budget never left the 60k a HUD part reserves, so each part waited 8 frames: a hovered
    unit's details 13 frames, the menu after a walk 96. Now a part every frame whose reckoning
    is not negative, the menu after a walk made during it: 4 and 1 frames. Details stay during
    walks and blows (attacker left, target right), double-buffered; `Enemy` / `Guest` above
    those portraits; the menu after a move on Act; the AI's glide and pauses twice FFT's,
    counted once the HUD is drawn. Tests, 48 xcheck doors, 177 UI and 87 blow / replay
    screens pass; peak 359,242 (AI battle). Program 65,361 bytes, buffers 112,568 bytes.
    Then, the user's asks: reach tiles with 2-pixel black edges (4 px a row, `art.py`; dark grey
    first), white around the map instead of light grey (`extract.py`), and the
    prediction black in a white box framed black, made as its own HUD part (48,230 cycles at
    most; built with the name part, the medium-font try reached 77,620 and two AI frames went
    over; that font was too big). Tests, 48 xcheck doors, 177 UI and 87 blow / replay screens
    pass; peak 358,902 cycles. Program 64,739 bytes.
    Then room in the program: the sloped markers' planes (3,216 bytes) and the map's tiles
    (1,800) moved to the archived `ffti`, the markers' dark plane dropped (black and white: the
    light one). Program 63,935 → 59,753 bytes; tests, 48 xcheck doors, 177 UI, 87 blow / replay
    and 168 water screens pass.
    Then, the user's asks after a TiEmu run: the picked tile drawn as the inverse of the reach
    (a checkerboard was too faint), an attack's reach inverted (black, a white edge), an AI
    turn's unit held 1.5 s with the world moving before its plan, and the active unit's pointer
    bobbing as the glove sways. The held AI frames exposed a forced half-screen copy of the
    scene (26k) over the budget (362k): the copy now goes a quarter a frame (13k). Peaks
    348,956 (AI battle) and 358,266 (defeat); program 60,075 bytes.
    Then a battle played through on the PC (a replay tool driving `fft_pc`): 26 notes, and the
    user's picks: [2nd] held through AI turns (843 -> 446 frames to Ramza's first turn, the
    same outcome; an extra step only with 10k left in the last frame: a held battle peaks at
    350,344), the cursor blinking on a tile [2nd] cannot pick, the aimed unit drawn over the
    others every other 8 frames, "KO" and capped heals in the prediction, a smaller diamond
    above KO enemies, "Item" as the item list's title, rounds / fallen / loot left in the
    results. Tests, 48 xcheck doors, 177 UI, 88 blow / replay and 168 water screens pass;
    program 60,937 bytes.
    Then banners rendered on the calculator, the user's pick over pictures made on the PC: a
    vector font (strokes as `fill_poly` quads, ~15k each; outline by dilation), a wipe that
    waits for the strokes, drawn while the frame has room. The water's glints skip the frames
    whose view is copied (the camera panning): ~10k back, the defeat replay's walk at 360,772
    went to 348,924. Scenarios 59 / 60 show the outcome banners; tests, 50 xcheck doors, 177
    UI, 88 blow / replay and 168 water screens pass; program 62,811 bytes.
    The objective then lights up its enemies as FFT does: the camera frames them below the
    banner (moved to the top) and they pulse with the blow's white flash; start banner frames
    peak at 317,616; program 63,119 bytes.
    The stroke font looked crude (the user's word): the glyphs became Lato Black's outlines
    (Inkscape text to path, `tools/banner.py`, in `ffti`), filled even-odd with 2 x 2 samples
    a pixel (antialiased in four greys); a row of samples costs ~5k (11.5k when a pixel row is
    written), charged to the frame's budget: banner frames peak at 337,062; program 63,415
    bytes. Tests, 50 xcheck doors, 177 UI, 88 blow / replay and 168 water screens pass.
    Then FFT's darkening around the banners: the scenery a grey darker (white light, light
    dark, dark black: two ORs a long) behind a diagonal edge from the top left (16 pixels a
    frame), the words, then the shade leaving the same way ([2nd] starts it); the units stay
    bright. Its first version, masks and shifts per word, cost 144k a frame; whole longs in a
    tight loop, masks only at the edge, 45-80k. The second line had stolen the first's fill
    (both lines filled the same glyph state): fixed, "BATTLE START" shows again. Banner frames
    peak at 346,224; program 63,833 bytes; the same checks pass.
27. **Water** (done). Measure on the original how deep a unit sinks in depth-1 water (all 38 canal
    tiles at Gariland), then cut the sprite's bottom by as much with a light water line; if
    the original does not sink them, ask.
    Measured: 38 depth-1 terrain records, six sheets / 60 source poses, 5,472 live submitted
    pose checks and an ordinary pad-driven canal entry. The feet stay at the floor; the
    submerged SHP removes 6..13 source rows from the lower opaque extent. Native adaptation:
    fixed cut at the projected surface (15 of 26 stored rows), light-grey edge, hidden
    shadow, wet flash/blade clipping, full hop poses until grounded, cached classification.
    Twelve doors 41-52 and `x/m27_water.png` cover four views, entry/wading/exit, blows,
    KO, Critical and a crystal. Tests, 52 xcheck doors and 371 additional phase/replay
    screens pass. Peaks: water 254,232; AI battle 355,466; defeat 356,764; UI 316,910.
    AI thinking uses the active pointer; its tile cursor returns with the plan. Reciprocal
    CT products match 300 complete clock predictions. Removed units no longer mark cells
    outside the reach grid. Program 60,451 bytes; buffers 104,880 bytes; banks unchanged.
    No new hardware path; TiEmu stays at milestone 29.
28. **Eight orientations** (not in FFT: four 45-degree yaws only). First an experiment: one
    view at 22.5 degrees, scenery only, to judge it at 160x100; only then the four in-between
    views (~45 KB more archive, the same RAM), the projection generalised (cursor, tiles,
    slopes, paths, units, camera: the 24x12 diamond assumed today), units in FFT's nearest
    facing, the rotation through the in-between angle.
29. **The whole battle on the calculator.** Titanium then TI-89 HW2: one full battle in TiEmu
    (a GIF), the cycles per state, the program and data sizes, the free RAM.

Engine, any time:
- the TI-89 HW2 (AMS 2.09) run and its free RAM (single-buffer fallback);
- a faster rotation (fewer, larger polygons per column, or a 68000 span filler: ask first);
- FFT's second camera elevation;


Presentation follow-up (completed 2026-10-17; [details and checks](EVENTS.md)):

- [x] Reveal banner letters twice as fast (12 instead of 6 pixels/tick).
- [x] Slow the darkening animation to half speed (8 instead of 16 pixels/tick).
- [x] Show the complete victory message, not only `VIC`; hold every complete message.
- [x] Animate KO-to-crystal/chest conversions, framing and queuing each event.
- [x] Animate crystals using the eight original source images.

Storage follow-up (completed 2026-10-17; [details and checks](STORAGE_CLEANUP.md)):

- [x] Retire textured rotations, their configuration option and169,278 bytes of banks.
- [x] Remove disabled attack-fixture dependencies and12,028 bytes of images.
- [x] Compact font/names, remove unused marker and deduplicate exact blade frames.
- [x] Compress fixed inspection colours into existing scene RAM; no new output allocation.
- [x] Explore per-frame sprite compression and bounded/lazy mirror caches; record size,
  sampled native decode CPU and RAM tradeoffs without enabling an unmeasured game cache.

Portrait/cache follow-up (completed 2026-10-17; [details and checks](BLADE_CACHE.md)):

- [x] Keep one portrait facing, saving2,088 archive bytes without a RAM mirror.
- [x] Compress blade frames and keep three fixed phase slots, prepared during wind-up;
  save14,304 additional archive bytes with1,300 bytes of tactical Work.
- [x] Check cold/warm replacement, crowded attacks, fast-forward and the original
  full-battle frame budget; retain the current single-facing HUD images as intended.

## Auto-battle follow-up — 2026-10-18

- [x] Expose the existing AI for this player unit or all allies; retain manual control by default.
- [x] Stop automation safely with Shift/ESC, keeping committed movement, damage and rewards.
- [x] Retain partial-turn Move/Act limits, per-unit ownership and inventory rules.
- [x] Check menu/cancellation and a full automatic battle on PC and native68000.

See `AUTO_BATTLE.md`. Original specialized orders and their target selectors remain unimplemented.

## Management/font exploration — 2026-10-18

- [x] Extract the original proportional font; compare native-height/compact variants.
- [x] Build three portable Formation prototypes with shared actors, portraits and cursor.
- [x] Reuse original context/equipment/ability hierarchy and original JP costs.
- [x] Export PC tours; cross-check native68000 screens and measure cache costs.
- [x] Keep smaller menu fonts/five visible slots; remove all text shadows and add
  FFA's dark gradient and grey-to-white dialogue progression/page continuation.
- [x] Extract original equipment icons, verify source crops/palettes against PS1 VRAM,
  and show their compact native conversions beside each equipped item.
- [x] Audit referenced original dialogue pages (largest measured103 characters), use
  smaller dialogue glyphs, and fit width/height to actual text without reserved rows.
- [x] Add configurable10x16 animated actors on three named Formation rows (4 or5 columns);
  align header values and pad dialogue speaker names. Check dense12/15-slot visual fixtures.
- [x] Add animated job cards and reversible session-local Squire/Chemist confirmation;
  preserve the original fixture's Knight/Archer requirements.
- [x] Extract original Orbonne/Gariland title strips and adapt their feathered reveal/erase.
- [x] Add configurable inverse-white dialogue bodies/name badges, retaining the gradient,
  typewriter frontier and session preference; check exact palette inversion and PC/native replay.
- [x] Fit speaker badges to proportional name widths and trim the extra bottom row.
- [ ] Select a layout, then implement persistent party/inventory, equipment, learning and saves.

See `MANAGEMENT.md`; menus are separate from the battle executable. Job previews change
only the current menu session; campaign management is not connected yet.

## Opening content — Orbonne (branch feat/fft-opening)

- [x] Observe the original chapel → battle → abduction flow from cold boot.
- [x] Generate map 62's single-camera scene and map 56's four tactical views.
- [x] Extract eleven source battle units and validate their level-grown stats.
- [x] Share the Gariland battle engine; manual Ramza, AI guests/enemies, victory/loss.
- [x] Add bounded native bow/sword effects and usable opening techniques.
- [x] Play the chapel conversations, outside confrontation and conclusion.
- [x] Deduplicate/compress actors, compute mirrors on misses, trim one-off poses.
- [x] Reuse dialogue/Status buffers, read water-effect points in archive, trim empty rows.
- [x] Restore Gariland's geometric contours and animate visible source-palette water.
- [x] Fair pose decoding, walker priority, portrait-mask indexing and bounded HUD latency.
- [x] Compare complete PC/TI framebuffer/state checkpoints and record native cycles.
- [x] Produce local PC presentation GIFs and packed TI launcher/data files.
- [x] User-requested Titanium TiEmu packed launch and initial grayscale review (soft-map build).
- [ ] Interactive keyboard/playthrough/exit review; launch the corrected presentation when requested.
- [ ] Full original special-skill/status/Charge behavior and abduction choreography.
- [x] Academy bridge and an integrated campaign handoff into Gariland (see review below).

Build, controls, local assets and scope: `opening/README.md`.
Measured tradeoffs and remaining stalls: `opening/BENCHMARK.md`.


### Opening review corrections and chapter handoff (2026-10-18)

- [x] Remove damage intervals; source Potion 30 HP and CT-delayed Archer Charge.
- [x] Correct source camera handedness in Orbonne and the campaign Gariland variant.
- [x] Static source scene poses, prayer/kneeling/alarm, depth order and white dialogue borders.
- [x] Source location title, black fades, non-repeating abduction and adapted Chocobo escape.
- [x] Reuse Gariland wood/stone artwork offline; static management roster/job actors.
- [x] Academy bridge and Gariland continuation instead of restarting the prologue.
- [x] PC/68000 scene/combat/continuation checks; native archive metadata is endian-safe.
- [ ] Connect speaker portraits to opening/Academy dialogue; the management prototype
  already shows portraits, but these scenes currently draw only the fitted name badge.
- [ ] Remaining jobs: distinct Jump, magic targeting and periodic song/dance semantics.

The user's twenty-job action requirements and their implementation boundary are
preserved in `.claude/skills/ti-port-ps1/reference/fft-job-actions.md`.

The native supervisor's heap-lifetime check is documented separately in
`campaign/README.md`; this milestone is not a full campaign or PS1 movie port.
