# Final Fantasy Tactics (PS1): study notes

`roms/ps1/Final Fantasy Tactics (USA)/` (SCUS-94221, one MODE2/2352 track, 2,464 files, local).
Labels: **OBSERVED IN FFT** (read or measured), **LIKELY INTERPRETATION**, **TARGET
IMPLEMENTATION** (what the TI does, and why).

## Decisions (grilling, 2026-10-09)

- Goal: a rendering and terrain proof of concept of the Gariland battle map: four camera
  orientations (F1 / F5), occlusion that changes with the view, three units, one moved with
  FFT's Move / Jump rules. No combat, menus, AI.
- **Map**: the real Gariland heights, extracted from the local disc by `tools/extract.py` into
  `map.h` (never committed); the art was ours at first (`tools/art.py`). After seeing the
  map's own textures rendered in 4 greys next to our drawing (`x/real_vs_ours.png`), the user
  chose the game's textures (2026-10-09): the scenery is now the mesh drawn on the PC
  (`fftv0..3`, never committed); units, cursor and rotation frames stay ours (the frames' shades are
  measured on the views).
- **Scale**: 24 x 12 tiles, one height unit = 6 px, units ~13 x 20 plus the outline. The user
  asked for FFT Advance's tile (32 x 16) reduced by 20-30 %; three candidates (26, 24 and
  22 px) were rendered on the real map with units (PNG mock-ups) and 24 x 12 chosen. The map
  (301 x 169 px) is larger than the screen: the camera follows the cursor.
- **Rotation animated** (polygon frames between the views), not instantaneous.
- **4 greys**, Titanium first, the program kept under the TI-89's 24 KB.

## Battle decisions (grilling, 2026-10-09, milestones 13-22)

- Fidelity: stats, turn order, damage and hit chance as the game's, each number checked on the
  running original. The AI as FFT's if it can be made fast enough; if an enemy turn costs more
  than ~1 s on the TI, the numbers and a simplified version go to the user first.
- Oracle: the original at Gariland's first turn, reached by poking RAM or a patched local copy
  of the disc (the first battle loading Gariland's), else key scripts through the prologue.
- Random numbers: ours; probabilities and mean damage checked over many trials.
- Units: the disc's (ENTD 0x184, below): five enemies, Delita as an AI guest; the player's
  Ramza and four recruits drawn as the game draws them (two Squires, two Chemists), names
  from the game's lists. Random fields (Brave, Faith, zodiac, equipment, secondary skills)
  drawn each battle by the game's rules (a fixed seed in tests).
- Brave, Faith and zodiac active, shown in Status.
- Deployment: fixed tiles in the deployment area first, FFT's placement screen later.
- Dialogue: the disc's English text, optional (a setting); a box with a portrait (size picked
  on a sheet of 24 and 32 px candidates).
- EXP, JP and level ups earned during the battle, lost at the end; animations faster than the
  game's but not hurried, a key to skip.
- The current demo (Agrias, the thief) is replaced by the battle; its rotation, occlusion and
  reach tests stay with the battle's units.
- End: a results screen, then the battle again with new draws. [ESC] saves and quits, the next
  run resumes. Keys: [2nd]/[ENTER] confirm, [ESC]/[shift] cancel, F1/F5 the camera, [F2] status
  and turn order. Titanium, and the TI-89 through `-pack`.

### Abilities (grilling, 2026-10-11, milestone 18)

- Act opens a list as FFT's: Attack, then the unit's skill set, then its abilities (an item with
  none left greyed); [shift] goes back a level.
- Only what each unit knows, drawn at each battle by FFT's rule like the stats (a fixed seed in
  the tests); Accumulate and Yell cannot be learned at level 1 (300 and 200 JP for at most 199).
- Statuses only as the battle makes them (Dead, Critical, Defending); Heal, Antidote, Eye Drop
  and Echo Grass usable and missing when there is nothing to remove, as in FFT. Defending's
  effect in the formulas now, the Defend command with the AI (milestone 20); Equip Change not
  kept (no effect in battle).
- The bag as FFT's: the new game's items, used up; the enemies' do not run out.
- Done when each ability's preview and outcome equal the original's on one case, with
  distributions for the random ones (Dash, Throw Stone). Enemies and Delita still wait (the AI
  comes at milestone 20). On screen: the user's FFT pose, the number (or the status removed)
  rising, the ability's name at the top right.

### KO and the end (grilling, 2026-10-11, milestone 19)

- A KO unit's death counter shown above it (3 to 0, the panels' 3 x 5 digits); the guest's is not
  (FFT: Delita never turns).
- A crystal gives back all HP and MP (FFT's ability learning dropped: a Squire would learn
  little and it needs a choice screen); a chest one of the dead unit's items, shown by its name
  only (no inventory); both from FFT's own sheet (`BATTLE/OTHER.SPR`).
- EXP rising above the user after the target's number (`EXP+12`, `LvUp` for a level up), JP
  only in Status; FFT's rules as they are (the enemies earn too, JP shared, job levels).
- The results: `Victory!` or `Game Over`, a line per unit of the player's side (level before >
  after, EXP and JP earned), [ENTER] the battle again with new draws. Saving with [ESC] at
  milestone 29 (renumbered 2026-10-13).
- Done when a few rewards equal the original's (an attack, a kill, a level up), the crystal or
  chest odds and the counter's length as the code reads, a battle won and one lost by keys.

### Enemy AI (grilling, 2026-10-11, milestone 20)

- FFT's scoring without its simulation: FFT plays each candidate in its real engine (random
  rolls at their midpoint) and scores the whole field; we compute the same score's change on
  the units the action touches (mean damage, no critical, the KO weight).
- All of FFT's rules kept: the 8 best actions ranked and drawn on their hit chance, act then
  back off to the safest tile, a Critical unit retreats, a mostly-support unit heals and revives,
  the enemies' 1-in-7 push, Delita seeking a healer or a crystal below 45/128 HP, Accumulate and
  Yell on a coin flip, the Wait facing.
- The search as FFT's (Attack from every adjacent tile, the other abilities once per target
  from the safest tile), spread over frames: at most ~150k cycles of thinking per frame, a turn
  decided in under 0.5 s (15 frames); the numbers to the user first if the worst case is over.
- An AI turn shown as a player's without the menus: the cursor on the unit, its move range
  briefly, the walk, the ability's name at the top right, the target marked briefly, the act,
  the facing.
- Defend and the AI's item range measured on the original, then as FFT's: Defend drawn as FFT
  draws a support ability, a command for the player's units that have it.
- Checked on the original: `oracle.py --ai` logs its AI's turns (unit, tiles, ability, target,
  facing) over the 40 turns already replayed and over built situations replayed with other
  draws; our first choice must be FFT's most frequent one, our moves the same tile or one as
  safe.
- No AI for the player's side as a feature; a test scenario where the AI plays both sides to the
  end (tests, xcheck, cycles).
- Measured then: the oracle's enemy Chemist has Defend (support 0x1DF); FFT's AI throws items at
  most the unit's Move away (its exact targeting: 4) and the enemies use the items of their
  level or less (at level 1 only Potion).

## Decisions after playing milestone 21 (grilling, 2026-10-13, milestones 22-29)

From the user's play in TiEmu; the milestones themselves are in `ROADMAP.md`.
- OBSERVED (our build, headless): the "can't attack" and "farthest water tile" reports were the
  cursor on a canal tile drawn on the same pixels as the street tile 2 units higher in front
  (21-27 such spots a view); the reach, the attack and the projection were right.
- OBSERVED IN FFT: a walked tile ~0.33 s, the blow ~0.7 s after the order, an attack with its
  EXP ~3.2 s (menu back at frame ~192 of 60 Hz); ours were 2-3x faster. The weapon is its own
  sprite (`BATTLE/WEP.SPR`, 85,504 bytes, placed by `WEP1/WEP2.SHP/.SEQ`); each unit `.SPR`
  holds a ~48x32 portrait (rotated, raw rows); generic units have names (unit record +0x12C:
  Goodman, Thompson, Barton, Miria, Edgar; Greg, Doreth, Sorvez, Hampsten), drawn anew each
  game; the active-unit marker and the UI digits are in `EVENT/FRAME.BIN`; height shown as a
  large "2h" top right, "depth 1" on water; all 38 canal tiles height 0, depth 1; four
  45-degree yaws only.
- OBSERVED IN FFTA (`x/ffta_*.png`, the snowball tutorial): a see-through band bottom left with
  a bust rising above it, name, HP/MP/JP as numbers; the command menu bottom right under a
  "MENU" tab, a ball cursor; the target's band mirrored bottom right; Status full screen.
- TARGET: the original's durations at 30 fps, FFT's pauses before an AI action, the AI's camera
  turned only when the actor or the target is more than half hidden; move tiles all at once,
  style chosen on a sheet (and a test without depth); the hit flash on the sprite's inside, a
  small knockback, no sparkles; the sword and dagger from the disc during the blow; FFTA's
  layout in 4 greys (light grey bands, not dithered: text must read), FFT's portraits, names
  and marker, the turn order on APPS; water sinking as measured; eight orientations only after
  a one-view experiment.

## Tools

- `psxiso.py ... sources/fft_ps1/disc` lists the files; `--file MAP/MAP022.GNS` (and the
  `MAP022.*` resources) extracts the map. Nothing else was needed: the terrain is plain data
  in a documented format, so the game was not run headless (Gariland is the second battle,
  behind the Orbonne prologue; reaching it with key scripts would have cost far more than
  reading the file).
- Format references: the FFT modding community, `adamrt/fft_toolkit` (`src/map_record.c`,
  `src/terrain.c`, read on GitHub, not copied): map list (`MAP022` = Magic City Gariland), GNS
  records, the terrain block. Every field used was checked on this map (below).
- **The battle oracle** (milestone 13): `tools/oracle.py DISC sources/fft_ps1/gariland_t1.state
  trace.txt` runs the game headless (psxrun's core) from a new game to Ramza's first turn at
  Gariland in 30 s and saves the core state (local, never committed); deterministic (two runs
  give the same battle, RNG included). Gariland is reached without playing Orbonne: the script
  variables `CURRENT_EVENT` (word 0x27) = 6 and `NEXT_SCENARIO` (0x64) = 1, written from frame
  2000 until the game moves on, make the scenario loader start event 7 (the academy scene
  where the recruits join), then event 9 is Gariland. Pad: CIRCLE confirms, CROSS cancels
  (USA). Deployment by pad (UP / LEFT then CIRCLE places the selected unit, R1 selects the
  next one) and by writing the deployment grid for the last two units.
- **Code reading aid**: `adamrt/fft_decomp` (cloned in `sources/fft_decomp`, local), a
  decompilation with named globals and functions: the scenario table and loader
  (`attack_load_scenario_conditionals`), the script variables, the battle unit struct
  (`include/fft/unit.h`), the deployment screen. Read to find addresses, never copied.
- **The battle's units** (milestone 14): `tools/battle.py DISC battle.h` reads the ENTD
  events, jobs, items and generation tables (`make battle.h`); `--check STATE TRACE [oracle.h]`
  compares the oracle's eleven units with that model and writes their raw stats for the C
  tests (`make oracle.h`, which runs the oracle first if its state is missing).
- **The battle's turns** (milestone 15): `tools/oracle.py --turns DISC STATE 40 turns.txt` goes
  on from the first-turn state: Ramza waits (menu DOWN DOWN CIRCLE, facing CIRCLE), then the
  player's five units lose their player-controlled flag (team byte +0x05 bit 0x08, poked every
  frame: `BATTLE_TEAM_FLAG_PLAYER_CONTROLLED`) and FFT's AI plays both sides; a turn is logged
  when `has_turn` (+0x186) rises on `g_battle_turn_unit_id` (0x8018f520), a repeat of the same
  unit with no other CT grown skipped (the AI's trial writes flicker it). 40 turns in 20 s,
  four KOs among them. `battle.py --check ... oracle.h turns.txt` turns them into the fixture:
  each turn's CT bonus is its unit's CT at the next turn less the ticks between (counted on a
  living unit that did not act).
- **The AI's turns** (milestone 20): `tools/oracle.py --ai DISC STATE 40 12 ai.txt` goes on from
  the first-turn state: the move range FFT shows Ramza and the recruit after him (tile byte 5
  bit 0x20 of `g_battle_map_tile_data` 0x8018f8cc, the menu's Move opened, a fresh load each),
  then the AI's first 40 turns as `--turns` plays them, then 8 built situations (the actor given
  CT 100, the others 0; HP, Critical or Dead poked: positions cannot be poked, the game keeps
  them elsewhere too) each replayed 12 times with Ramza's Wait delayed 7 frames more each time
  (other draws). Per turn: each unit's tile, facing, HP, CT, death counter, statuses, the
  learned bytes of its job's row, its support, and the AI's tile priorities
  (`g_battle_ai_data_base` 0x8019f3c4 + 0x5f4), at its turn's start and the next one's; what the
  actor did from its record (+0x187 moved, +0x188 acted, +0x170 last ability, +0x17a / +0x17e the
  target's tile; Defend: acted, no ability, Defending after). 2 min. A probe of the AI workspace
  frame by frame while a unit thinks (its grids, candidate tiles, ranked actions: `battle.h`'s
  `battle_ai_data_t` offsets) settled the Potion case below.
- **Attacks** (milestone 17): `tools/oracle.py --attack DISC STATE 40 400 attacks.txt` goes on from
  the first-turn state: Ramza's menu DOWN, CIRCLE (Act), CIRCLE (Attack), CIRCLE (closes the
  help box), LEFT (the recruit south of him), CIRCLE: the preview; then Execute and OK (two
  CIRCLEs). Units cannot be moved by poking x / y (the game finds a target through its own
  map of the tiles), so the cases poke the two units' stats instead (PA +0x36, Speed +0x38,
  Brave +0x24, right hand +0x1d, sign: birthday +0x09 high nibble, sex +0x06 0x80 / 0x40,
  facing +0x49 low nibble). 40 previews and 400 attacks in 9 min; `battle.py --check ...
  attacks.txt` writes them into oracle.h.
- **Abilities** (milestone 18): `tools/oracle.py --skills DISC STATE 300 skills.txt` gives Ramza
  every Gariland ability by poking (primary skill set +0x12 = 0x19, secondary +0x13 = 0x06 Item,
  learned bits +0x99 (row 0) FC 00 00 and (row 1) FF FC 00, Throw Item: support bits +0x91 |=
  0x08); FFT then shows Guts and Item in the Act menu. Each case: Act, Guts or Item, the entry,
  CIRCLE (the help box), LEFT (the recruit), CIRCLE: the preview (hit % 0x8014d058, amount
  0x8014d056), then Execute and OK; the target's HP and statuses read before and after, Ramza's
  HP and the bag (`g_main_item_quantities` 0x800596e0). A status must be poked in both the
  current set (+0x58) and `inflicted_status` (+0x1bb, what a removal checks), or a cure misses.
  13 cases and 300 executed Dash and Throw Stone each in ~10 min.
- **Rewards** (milestone 19): `tools/oracle.py --rewards DISC STATE rewards.txt` plays six actions
  of Ramza (as `--skills`, every ability given) on the recruit south of him: Attack, a kill (its
  HP poked to 1), a level up (his EXP +0x21 poked to 95), Potion, Heal missing, Dash; every
  unit's EXP (+0x21), level (+0x22), JP and total JP of career slots 0 and 1 (+0xdc, +0x104,
  u16 per job) and job levels (+0xd2) before and after, Ramza's raw stats (+0x72) and HP. 7 s.

## Behaviour and data

### The map files

- OBSERVED IN FFT: `MAP022.GNS` is a list of 20-byte records (type at +4: 0x1701 texture,
  0x2E01 primary mesh, 0x3001 alternative mesh, 0x3101 end; sector at +8, length at +12) per
  time of day and weather; the primary mesh is `MAP022.9` (35,984 bytes).
- OBSERVED: the mesh's u32 at 0x68 points to the terrain: x count (10), z count (15), then
  2 levels x 15 x 10 tiles of 8 bytes: byte 0 & 0x3F surface, byte 2 height, byte 3 slope height
  (& 0x1F) and depth (>> 5), byte 4 slope type, byte 5 ceiling depth and the transient marks
  (0x20: a move destination shown), byte 6 flags (bit 0 blocked, bit 1 untargetable, bit 7
  can't stop: `map_tile_t` in fft_decomp), byte 7 auto camera. Level 1 is all zero on Gariland.
- OBSERVED: heights 0 (canals) to 10 (a chimney); streets and squares at 2, houses 6-8 with
  sloped roofs; surfaces grass, waterway, wooden floor, stone floor, roof, tree, box, chimney,
  bridge. Correction (milestone 20): bit 6 (set on the canals) was read as "can't walk" until
  the move ranges were measured on the original; it has no established meaning. No tile is
  blocked at Gariland: the canals (depth 1) are waded, the chimneys stood on, the trees (bit 1,
  untargetable) crossed but not stopped on.
- OBSERVED: the slope byte is four 2-bit edges, from the top bits N S W E (0 low, 1 half,
  2 high); N is +z and E is +x: every incline's high side meets the neighbour at h + slope
  height (the 25 sloped tiles checked once against their neighbours). A corner is high when an
  adjacent edge is 2, or, with no edge at 2 (convex), when both adjacent edges are 1.
- LIKELY INTERPRETATION: the standing height of a sloped tile is its middle, h + slope / 2
  (FFT shows heights like 7.5 on stairs).
- TARGET: `map.h` holds per tile the four corner heights, top and side materials, walkable
  (not can't-walk, not tree or chimney), standing height in half units.

### Camera and drawing

- OBSERVED (known play, not measured here): FFT turns the battlefield between four diagonal
  directions with a smooth rotation of about half a second, and has two elevation angles;
  units and the cursor are never hidden by a dedicated roof-fading system: the player turns
  the camera to see behind buildings.
- OBSERVED: the primary mesh (pointer at 0x40): counts of textured triangles, textured
  quads, untextured triangles and quads (Gariland: 24, 361, 18, 51), their vertices (s16 x,
  y, z; x 0-280, z 0-420, y 0 to -120: a tile is 28, a height unit 12, y up is negative), the
  textured ones' normals, then per textured triangle 10 bytes (u, v, palette, -, u, v, page,
  -, u, v) and per quad 12 (a fourth u, v). Texture MAP022.8: 256 x 1024, 4 bits, the page
  adds 256 to v; 16 palettes of 16 BGR555 colours at the u32 0x44, colour 0 transparent. The
  untextured polygons are the map's black skirt. Quads are ABCD in the PS1's order (ABC,
  BDC). The mesh's heights per tile match the terrain grid (mean difference ~1 unit, roofs).
- LIKELY INTERPRETATION: a 3D renderer (textured polygons, ordering table) with the map as a
  mesh; the terrain grid only drives rules and the cursor.
- TARGET: no 3D at play time. Each view is a 2:1 isometric picture of the textured mesh,
  drawn on the PC with the depth of each pixel (the tile it belongs to) and read in place;
  the rules turn the terrain grid by renumbering tiles and corners; the rotation between two
  views is three flat-shaded polygon frames from the grid; occlusion is FFT's: things behind
  a building are hidden, turning reveals them (cover masks from the views' depth, README).

### Units

- OBSERVED (game data, known): a squire has Move 4, Jump 3; units cannot stop on another
  unit; enemies block the path.
- TARGET: BFS over walkable tiles, a step allowed when the standing heights differ by at most
  Jump (both ways), allies passable but not a destination; walking 4 frames per tile with a hop
  on height changes. Units were our drawings at first; since milestone 9 they are FFT's own
  sprites (below).
- OBSERVED (`BATTLE/*.SPR`, `TYPE1.SHP`, `TYPE1.SEQ`; formats from FFTPatcher's
  ShishiSpriteEditor and TacticsTemplateG, read on GitHub, the numbers checked on this disc): a
  sprite file is 16 palettes then a 256-wide 4-bit sheet (288 raw rows, then a compressed
  part); a frame is up to 8 tiles of the sheet placed from the unit's anchor (near the feet);
  frames 9-13 face the camera looking down-left, 14-18 face away looking up-left, 3 tiles each
  (body and two arms). The battle idle marches in place (frames 11 10 9 10 11 12 13 12, 6 8 10
  8 ticks), the walk uses the same frames faster (2 4 6 4). Ramza (RAMUZA, chapter 1), Delita
  (DILY), Agrias (AGURI) and the thief (THIEF_M) all use TYPE1. The other two directions are
  the same frames mirrored (exmateria-gambit-tactics' reading of BATTLE.BIN, not verified
  here); the poses are ~14-23 x 37 pixels.
- OBSERVED (a capture of Gariland sent by the user): a standing unit (hair to feet) is ~235 px
  where a canal one tile wide spans ~130 px of height, i.e. ~0.9 of a tile's width; the faces
  show two eyes drawn in the outline's colour (colour 1) inside the skin.
- OBSERVED: the canal tiles have surface type 0x0E (MAP022.9); FFT animates its water.
  TARGET: the views stay still; glints slide on the visible water pixels (README).
- TARGET: the same 10 frames per unit scaled 0.6 to 16 x 26 (0.5, the first choice, made a
  unit 0.75 of our 24-px tile), each eye placed on its own (1 x 2 black at the centre of its
  group of colour-1 pixels in the face, a pixel apart) on a face one grey lighter, FFT's idle and walk timings at ~30 frames per second, the mirroring at draw
  time (a 256-byte table), a world facing per unit, turned with the camera.

### The battle (milestone 13, the oracle)

- OBSERVED (disc, `EVENT/ATTACK.OUT` at 0x801cf938: 480 records of 0x18 bytes): the scenario
  table. Events 1-2 Orbonne's chapel (map 62), 3-6 the Orbonne battle (map 56, ENTD 0x183),
  7-8 the Military Academy (map 24, ENTD 0x188), **9-12 Gariland** (map 22, ENTD 0x184, squad
  0x100, Ramza mandatory). Event 9 runs the battle's condition script 3.
- OBSERVED (disc, `BATTLE/ENTD4.ENT` entry 4, event 0x184 "Gariland Fight"): six units.
  Delita (sprite 04, his own Squire job 04, level 1, Sagittarius, guest: team byte 0x84, AI
  controlled) at (8, 12); five enemies (team 0x90), level 1, Brave / Faith / zodiac /
  names random (0xFE): Squire M with a Broad Sword at (6, 5), Squire M with a Dagger at
  (1, 1), Squire M with a Dagger at (5, 4), Chemist M (Brave fixed 38) at (7, 4), Squire F
  with a Dagger at (3, 3). War trophies: Mythril Knife, Phoenix Down, Potion. Squire skills
  (set 05): Accumulate, Dash, Throw Stone, Heal; Chemist: Item (Potion, Phoenix Down...);
  Delita's set 1C adds Wish, Ramza's 19 adds Yell. The male Chemist's sprite (ITEM_M.SPR)
  uses `TYPE2.SHP` / `TYPE2.SEQ`, not TYPE1.
- OBSERVED (run): a new game creates Ramza only; the academy scene adds six recruits (two
  male and two female Squires, a male and a female Chemist, level 1, Brave and Faith 48-70)
  and Delita in party slot 16. Gariland's deployment area (5 x 5 grid, row by row, 1 =
  valid): `00000 / 00011 / 11111 / 10000 / 00000`, unit limit 5, Ramza mandatory.
- OBSERVED (run, RAM `0x801908cc`, 21 units of 0x1C0 bytes, offsets from fft_decomp and all
  checked against the screen and the party data): job +3, team +5, level +0x22, Brave
  +0x24, Faith +0x26, HP / max +0x28 / +0x2A, MP / max +0x2C / +0x2E, PA +0x36, MA +0x37,
  Speed +0x38, CT +0x39, Move +0x3A, Jump +0x3B, x +0x47, y +0x48. Enemies are units 0-5
  (Delita 0), the player's 16-20. At level 1: Squires HP 34-44, MP 10-11, PA 3-4, MA 3-4,
  Speed 6, Move 4; Chemists HP 35-38, Move 3; Ramza HP 52, MP 16, PA 5, MA 5, Move 5; Delita
  HP 53, PA 6.
- OBSERVED (run, `sources/fft_ps1/gariland_t1.txt`): every unit starts at CT 0 and gains its
  Speed per clock tick, several ticks per frame; all have Speed 6, so all reach 102 on the
  same tick and act in unit order (Delita, the five enemies, then Ramza: ties go to the lower
  unit index). An acting unit's CT drops by 100 when its turn starts and gets 20 back for each
  of Move and Act it did not use: Delita moved only (102 -> 22), enemy 1 moved and acted
  (-> 2). LIKELY INTERPRETATION: FFT's known rule (CT - 100, - 80, - 60).
- OBSERVED: the AI writes trial positions and HP into the real unit records while it thinks
  (a trace shows units jumping between tiles for a frame): read the records between turns.
- TARGET: milestone 14 rebuilds these units and stats from the disc and compares them with
  this table; milestone 15 the CT clock against this trace.

### The battle's units (milestone 14)

- OBSERVED (fft_decomp, read: `main_unit_init_for_battle`, `main_unit_calculate_entd_data`,
  `main_unit_generate_base_raw_stats`, `main_unit_calculate_actual_stats`,
  `main_unit_set_equipment_attributes`, `main_unit_calculate_random_equipment`): a unit has five
  24-bit raw stats (HP, MP, SP, PA, MA; battle unit +0x72, 3 bytes each) and its job's growth
  and multiplier per stat (+0x81). A generated unit draws raw = base << 14 + rand() * variance
  / 2 (rand() 0..32767) from one of four rows (`0x8005e90c` base and starting equipment,
  `0x8005e93c` variance): generic male (base HP 30, MP 14, SP 6, PA 5, MA 4, variance 2 1 0 0
  0), generic female (28 15 6 4 5), Ramza (30 15 6 5 5), monster. Each level above 1 adds
  raw / (growth + level - 1). A stat is raw * multiplier / 100 >> 14 (at least 1; HP, MP <=
  999, Speed <= 50, PA, MA <= 99), then the equipment adds the helmet's and armour's HP / MP
  (`0x80063ed8`) and each item's attributes (`0x800642c4`: PA, MA, SP, Move, Jump; Jump <= 7);
  Move and Jump start from the job (`0x800610b8`, 48 bytes per job). ENTD value 254 means
  random: Brave and Faith 45 + rand() * 30 / 32768, birthday 1..365 (zodiac from the day limits
  at `0x800661e8`), an item one of those of the highest required level not above the unit's
  that its job can equip (not rare). At level 1 for Gariland: Leather Hat (+8 HP) and Clothes
  (+5 HP) on every unit whose ENTD says random, Delita's weapon Dagger or Broad Sword.
- OBSERVED (run, `tools/battle.py --check`): the eleven units at Ramza's first turn read from
  the oracle's RAM: their HP, MP, Speed, PA, MA, Move and Jump are exactly this model applied
  to their raw stats, jobs and equipment; their raw stats lie in their type's range, their
  equipment among the candidates, random Brave / Faith in 45-74, fixed values as the ENTD
  says, start tiles as the trace's first line. Ramza was generated by the new game (type
  Ramza: Brave and Faith 70, Leather Hat, Clothes, Battle Boots: Move 5, Broad Sword); the
  recruits are the Military Academy's ENTD 0x188 units (generated there, then kept: the
  deployed ones are its slots 3, 7, 4 and 2).
- OBSERVED: FFT's x / y are our x / z (every start tile is a walkable tile at the expected
  height; one enemy Squire starts on a roof, h 8); facing 0 -y, 1 -x, 2 +y, 3 +x
  (`battle_move_direction_e`). Both Chemists' sprites (ITEM_M, ITEM_W) use `TYPE2.SHP`, whose
  frames 9-18 are the same poses as TYPE1's.
- TARGET (`tools/battle.py` -> `battle.h`, `battle_units()` / `unit_stats()` in `fft.c`): the
  same model and numbers, drawn with our RNG (`rt_rand() >> 1` for rand()) at each battle
  (scenario 0 seeds from the clock; test scenarios use their number). The units in FFT's
  order (Delita, the five enemies, Ramza, the four recruits: CT ties follow it); the player's
  units at the oracle's deployment tiles facing -y (decision Q11: a fixed placement); Delita a
  guest (passable, not selectable). Ramza's birthday, chosen by the player in FFT, is drawn.
  A random item with two candidates is one of the two (FFT: one of n). Stats checked in C on
  the oracle's raw values (`oracle.h`, `make test`).

### Turn order (milestone 15)

- OBSERVED (fft_decomp, read: `battle_action_run_between_turn_control`, `battle_action_end_turn`,
  `battle_action_calculate_at_list`, `battle_action_sort_at_list`,
  `battle_status_build_upkeep_action`): the clock is a state machine run between turns. ADD_CT:
  every unit whose CT is not frozen (Stop, Sleep, crystal...) gets CT += Speed (halved under
  Slow, x1.5 under Haste), at most 254. PICK_UNIT: the unit with the highest CT above 99, a tie
  to the lowest index (strict comparison in index order); its CT becomes CT % 100; no unit:
  charged abilities tick, statuses expire, then ADD_CT again. After a turn the machine resumes
  at PICK_UNIT, so several units at 100+ act in turn before the next tick. End of turn: +20 if
  the unit did not move, +20 if it did not act (Don't Move / Don't Act count as used), then at
  most 60. A dead unit is picked like the others (its CT ticks): each pick lowers its death
  counter (+0x07, 3 in battle); picked at 0 it becomes a crystal or a chest (rand() & 0x10, a
  wider chance for the player's units) and its clock stops. Every pick draws one rand().
- OBSERVED (fft_decomp): the AT list the HUD shows: the active unit first, then for each living
  unit its next turns, its clock run alone from its CT (no end bonus assumed), keyed tick << 8
  | (100 - CT % 100), inserted in unit order before the first larger key (stable), 40 entries.
- OBSERVED (run, `oracle.py --turns`, 46 turns from the battle's start, the player's side played
  by the AI): every turn's unit and every unit's CT at each pick equal the rule above fed with
  the original's bonuses (0, 20 or 40 per turn) and KOs; Speed 6 everywhere, so the order is
  index order with the bonuses deciding who reaches 100 a tick earlier (a unit that waited,
  42 + 60 = 102, goes before one that moved and acted, 2 + 60 = 62). A KO unit's CT keeps
  growing; the first enemy KO turned into a crystal or chest at its fourth pick, its CT frozen.
- TARGET (`ct_next`, `ct_end`, `turn_order` in `fft.c`): the same rule (Slow, Haste, Stop and
  charged abilities when abilities need them, milestone 18); the death counter kept (a crystal
  or chest at milestone 19); our RNG is not drawn at a pick (decision Q3: no RNG fidelity). The
  AT list of the next ORDER_N (6) turns. Until the menus (milestone 16): enemies and the guest
  wait (+40), a player's unit moves (+20, the turn ends) or waits with [diamond].

### The turn menu (milestone 16)

- OBSERVED (run, `oracle.py --turns`): Ramza's turn opens a menu whose third entry is Wait
  (DOWN DOWN CIRCLE from the first, Move, then Act); Wait asks for the facing, confirmed with
  CIRCLE, and the turn ends with the +40 bonus.
- TARGET (`menu_keys`, `make_window`, `make_panel`, `draw_arrows` in `fft.c`): Move / Act /
  Wait / Status at the top right, one line each in AMS's small font; Move once (greyed after),
  Act greyed until milestone 17; Wait shows four arrows on the neighbouring tiles' sides, the
  facing in black, turned by the arrows as the cursor moves. Differences from FFT, for the TI:
  a move can be undone ([shift] in the menu) until the unit acts, which FFT does not allow;
  Status shows only what the battle uses (level, HP, MP, Move, Jump, Speed, PA, MA: Brave,
  Faith and the zodiac stay hidden numbers, decision of milestone 13; no EXP or JP until
  milestone 19); the units' panels follow Metal Gear Solid: Portable Ops (the user's
  reference) instead of FFT's bottom windows: two thin columns at the bottom corners, read
  sideways, with the name, HP and MP as numbers and gauges, so the map keeps its height.

### Attack (milestone 17)

- OBSERVED (fft_decomp, read: `battle_action_run_pre_formula_setup`, `battle_formula_weapon_damage`,
  `battle_formula_calculate_weapon_damage`, `..._physical_evade`, `..._facing_evade`,
  `..._final_hit_percent`, `..._zodiac_compatibility`, `..._critical_hit`, `..._knockback`,
  `battle_move_check_knockback_destination`, `battle_move_apply_knockback`,
  `battle_target_calculate_strike_lunge_range`, `battle_target_calculate_aoe_vertical_tolerance`):
  Attack runs the weapon's formula; the battle's weapons (Dagger 0x01: knife, WP 3; Broad
  Sword 0x13: sword, WP 4; none: item 0) all use formula 1 and strike (flag 0x80). XA = PA
  (sword, bare hands) or (PA + Speed) / 2 (knife), then the zodiac (below), then a critical
  hit (rand() * 100 / 32768 < 4) adds rand() * XA / 32768 to XA and calls the knockback;
  damage XA * YA, YA = WP (bare hands: PA * Brave / 100, at least 1), at most 999. Each step
  is a 16-bit multiply then a truncated division. Zodiac: the signs' distance indexes
  BATTLE.BIN 0x8018f600 (0 0 0 1 2 0 3 0 2 1 0 0: 1 bad, 2 good, 3 opposite: worst for the
  same sex, best else); p = XA * 25: good +p / 100, best +p / 50, bad -p / 100, worst -p / 50,
  XA at least 1. Hit: evades are the target's class evade (job byte 0x19: Ramza's Squire 10,
  the other Squires and Chemists 5), shield, accessory, weapon (only with Weapon Guard); the
  attack's dominant axis against the target's facing: facing the attacker is the front (all
  evades), the back drops class and shields, a side the class evade; accessory evade always
  counts; each source rolled rand() * 100 / 32768 >= evade. The preview draws the same rolls
  (it advances the RNG) and shows no critical (its random value is 0x4000). Range: the four
  neighbouring tiles whose height (2 h + half, half units) is within [h - 6, h + 5] of the
  attacker's. Knockback (a critical only, not when it kills): one tile away along the
  dominant axis (a tie: y), onto a tile in the map, free, not higher (facing edges compared),
  without water or lava; a fall of more than Jump levels costs (levels - Jump) * max HP / 10.
  At 0 HP the unit is Dead, its death counter 3. HP <= max / 5: Critical (the kneeling pose).
- OBSERVED (`BATTLE/TYPE1.SEQ`, animation records from 0x406, (frame, ticks) pairs and 0xFF
  opcodes): hit 19 / 20 (animations 66-69), the swing 11, 27 (wind-up), 31 (blow), 27 facing
  the camera and 29, 33 facing away (52-57), kneeling 21 / 22, lying 23 / 24 (102-105). The
  weapon is a separate sprite in FFT (WEP*.SPR), not in these frames.
- OBSERVED (run, `oracle.py --attack`: Ramza's first turn, Act, Attack, the recruit south of him
  (a target can be an ally), the preview read at 0x8014d056 / 0x8014d058): the game's own case
  previews 95 % and 12 (Broad Sword, PA 5; the target faces him, opposite signs, both male:
  worst); 40 cases with the attacker's PA, Speed, Brave, weapon, sign and sex and the target's
  sign, sex and facing poked: every preview equals the model. 400 attacks executed in the
  game's case (each after other cursor moves, which draw rand()): 19 misses (4.75 %), 8
  criticals seen (damage above 12, 2.0 %; with XA 3 a critical adds 0 a third of the time), no
  push (the tile behind the target is higher), 11.55 HP per attack.
- TARGET (`attack_range`, `attack_side`, `hit_chance`, `attack_damage`, `attack`, `attack_step`
  in `fft.c`): the same rules and numbers with our RNG (`make test`: the 40 previews equal;
  4000 of our attacks: misses 4.2 %, criticals seen 2.9 %, 11.68 HP per attack). Our draws use
  a 32-bit xorshift: the runtime's wyhash16 (16-bit state) tied two draws in a row, every
  critical then adding at least 1 (criticals seen 3.8 %). Differences, for the TI: Act goes straight
  to the weapon's targets (FFT's submenu of Attack and the skills comes with them, milestone 18);
  only a tile with a living unit can be confirmed (FFT lets an empty tile be attacked); the
  knockback compares standing heights, and a fall's damage is added to the number shown; the
  preview is a small window at the top right (Hit 95% / Dmg 12) instead of FFT's two portraits;
  the poses are FFT's, scaled 0.6, without the weapon (milestone 21); the attack takes 34
  frames (~1.1 s: wind-up 6, blow 4, the target hit 8, the number rising 8 px), the number in
  black with a white outline (FFT: white). EXP and JP: milestone 19.

### Abilities (milestone 18)

- OBSERVED (fft_decomp, read: `main_unit_calculate_abilities`, `main_unit_learn_job_abilities`,
  `main_util_roll_pass_fail`; the JP table 0x80066182 read in the oracle's RAM: 0, 100, 200,
  350...): a generated unit's JP per job are the JP its job level needs plus rand() * 100 /
  32768 (level 0: 100 + that), so 100..199 at level 1 for every Gariland unit; then each
  ability of the job's skill set, in the set's order (`g_main_ability_human_skillset_abilities`
  0x80064a94: 16 actions, then reactions, supports, movements), is learned if rand() * 100 /
  32768 < its learn rate and the JP left pay its cost. Ramza, made by the new game, knows Wish
  only (`learned_abilities[0] = 4`). Only the primary set's abilities can be used (no unit has a
  secondary set at Gariland). Learnable with 199 JP: Squire's Dash (80 JP, 90 %), Throw Stone
  (90, 80 %), Heal (150, 70 %); Chemist's Potion (30, 100 %), Antidote (70, 65 %), Eye Drop (80,
  35 %), Echo Grass (120, 35 %), Phoenix Down (90, 60 %); Delita's Wish (0, 100 %).
- OBSERVED (run, RAM at Ramza's first turn): the eleven units' known abilities (+0x99, a row of
  3 bytes per job) are all outcomes of that rule: Delita Dash and Wish, the enemy Squires Dash
  (two of them Throw Stone too), the enemy Chemist Potion and Antidote, the recruits Dash, the
  Chemists Potion with Antidote and Eye Drop or Eye Drop only. The party's bag
  (`main_item_init_new_game_inventory`): Potion 5, Hi-Potion 2, Antidote 1, Eye Drop 2, Echo
  Grass 1, Phoenix Down 2 and one of each other item; an enemy's item use is not counted
  (`battle_action_decrement_player_item_quantity`: the player's team only), and an item is
  used up when the action starts, whether it lands or not (`battle_action_prepare_attack`).
- OBSERVED (fft_decomp, read: the ability range records 0x8005fbf0, `battle_target_set_ability_panels`,
  `battle_target_set_panels_for_action`, `battle_target_apply_vertical_tolerance`, the formulas
  `damage_random_y_times_pa`, `apply_damage_and_knockback`, `3c_..._heal_target_two_fifths`,
  `heal_z_times_ten`, `heal_random_one_to_z_add_status`, `apply_status_to_action`,
  `battle_status_modify_inflictions`, `calculate_final_hit_percent`,
  `apply_evasion_changes_due_to_statuses`): an ability's range spreads `range` steps over the
  map whatever the heights, then a vertical tolerance (where flagged) keeps the tiles within 2 x
  tolerance half units of the user's height; items reach 1 tile, 4 with Throw Item (every
  Chemist has it innately), heights aside. Dash: 1 step, not the user, within 1 level; damage
  (rand() * 4 / 32768 + 1) * XA, XA = PA with the zodiac (no critical), never evaded; at
  execution only, a knockback (Attack's) when rand() * target's Brave / 32768 < rand() *
  user's Brave / 32768. Throw Stone: 4 steps, not the user, any height, (rand() * 2 / 32768 + 1)
  * XA, evaded as Attack (flags 4 bit 0x02), the same knockback. Heal: 1 step or the user,
  within 2 levels, removes Darkness, Silence, Poison; Antidote Poison, Eye Drop Darkness, Echo
  Grass Silence; a removal keeps only the statuses the target has (`inflicted_status`), none:
  a miss. Wish: 1 step, not the user, within 3 levels; the user loses max HP / 5, the target
  heals twice that. Potion: 30 HP (Z 3 x 10). Phoenix Down: needs Dead, revives with rand() *
  20 / 32768 + 1 HP. Defending halves the base hit (100 -> 50) of evadable actions: the chance
  (50 - evade) * 100 / 50, the roll rand() * 50 / 32768 >= evade. No area, charge time or MP
  cost at Gariland.
- OBSERVED (run, `oracle.py --skills`, Ramza given every ability on the recruit south of him,
  the attack's game case: PA 5, the worst signs, XA 3): previews Dash 100 % and 9 (FFT's preview
  draws its own roll: the amount executed differs), Throw Stone 95 % and 6, Wish 20, Potion 30,
  Phoenix Down 11 on a KO target (2 executed), Heal / Antidote / Echo Grass 100 % on a target
  with the status (removed), 0 % without (a miss; Eye Drop and Phoenix Down used up anyway),
  Throw Stone and Attack 90 % on a Defending target, Defending kept after the hit; Wish's cost to Ramza (52 -> 42) lands ~5 s after the heal.
  300 Dash executed: 1, 2, 3, 4 times XA 25.0, 25.7, 24.0, 25.3 %; 300 Throw Stone: 5.7 %
  misses, once and twice XA 46.3 and 48.0 %; no push (the tile behind the target is higher).
- TARGET (`known`, `can_use`, `skill_range`, `skill_hit`, `skill_amount`, `skill`, the menus
  `acts_keys`, `make_window` in `fft.c`; `battle.py` reads the sets, abilities and statuses):
  the same rules and numbers with our RNG (`make test`: the 13 cases; 4000 Dash of ours 24.7,
  25.3, 24.9, 25.0 %, 4000 Throw Stone 5.1 % misses, 47.3 and 47.6 %; 3000 battles' draws within
  0.003 of the rule's exact odds; Dash on the flat pushed 534 of 1000 times, Brave 70 against
  60). Differences, for the TI: the preview
  shows the range of a random amount (Dmg 3-12, HP +1-20) instead of FFT's one roll; Wish shows
  only the target's heal (the user's HP drops in its panel); a cure shows the first status
  removed; the abilities use the attack's swing pose (FFT has throw and item poses, milestone
  21); a KO unit's tile can be chosen only with Phoenix Down; the Defending status ends at the
  unit's next turn (INTERPRETATION: FFT's own Defend lasts until then).

### KO and the end (milestone 19)

- OBSERVED (fft_decomp, read: `battle_unit_update_attacker_earned_experience`,
  `battle_unit_apply_action_rewards`, `main_unit_check_level_up`, `main_unit_apply_level_growth`,
  `battle_unit_store_jp_and_calculate_unlocked_jobs`, `main_job_calculate_level`): an action
  whose outcome did something earns its user 10 EXP, a kill 20 / the target's KO count (counted
  first), plus the target's level minus the user's, at least 1, at most 99 (Gained EXP Up
  doubles); several targets: the highest award; a miss nothing; reactions nothing. Everybody
  earns, enemies and the guest too, but a Dead, Crystal, Petrify or Treasure user. At 100 EXP
  (level < 99): EXP 0 (the rest lost), level + 1, each raw stat += raw / (growth + the old
  level) (growth 0 counts as 1), the stats made again, HP and MP only clamped to the new
  maxima. JP: 8 + 2 x the job level + the old level / 4 (none without EXP), Gained JP Up x 1.5;
  every other unit whose initial team is the user's gets a quarter, in the user's job slot;
  9999 at most. Job levels: JP totals 100, 200, 350, 550, 800, 1150, 1550, 2100
  (`g_main_job_level_jp_requirements` 0x80066184, read on the disc).
- OBSERVED (run, `oracle.py --rewards`): Attack 10 EXP and 10 JP for Ramza (job level 1 at 173
  total JP: 8 + 2 + 0), +2 JP in slot 0 for Delita and each living recruit (the Chemists too:
  their Squire slot), none for the enemies; a kill 20 EXP and no share for the KO target; from
  95 EXP a level up to 2 with EXP 0, raw HP 518253 -> 561440 (+ raw / 12: Squire growth 11 +
  level 1), max HP 52 -> 55, HP kept at 52; Potion 10 EXP; Heal missing nothing; Dash 10.
  Initial EXP: an ENTD unit's EXP is rand() * 100 / 32768 when its ENTD says random
  (`main_unit_calculate_entd_data`; Delita 78, the enemies 11-98); the recruits (made from the
  academy scene's ENTD: INTERPRETATION) 4-91; Ramza 0 (`main_unit_generate_out_of_battle`).
  Every unit's total JP in its job 100-199 at the start (Ramza 173).
- OBSERVED (fft_decomp, read: `main_status_set_ct`, `battle_status_build_upkeep_action`,
  `battle_unit_generate_crystal_or_treasure`, `battle_unit_generate_treasure`,
  `battle_unit_learn_from_crystal`): the death counter is set to 3 when Dead is set or removed
  (0xFF for an IMMORTAL unit: Delita's ENTD flags 0x84); each dead upkeep (a pick of the unit)
  lowers it 3 -> 0, the next one turns the unit (an Undead one may revive): a crystal when
  rand() & 0x10 (an enemy, 1 in 2) or & 0x110 (a player-controlled unit, 3 in 4), else a
  treasure; it stays on the map as a unit. A unit moving onto it: a chest gives one of the dead
  unit's non-empty equipment slots at random (none: one of the 13 consumables 0xF0-0xFD but
  Elixir); a crystal offers the abilities the mover can learn (each by its learn rate) or
  restores HP and MP, only the restore when nothing is learnable.
- OBSERVED (fft_decomp, read: `battle_action_check_battle_outcome`,
  `battle_menu_dispatch_system_function`): defeat (checked first) when a unit with importance
  3 (Ramza, team 0x0B) is a crystal, a chest or gone, or when every blue unit present, the guest
  included, is in the KO set (Crystal, Dead, Petrify, Invite, Blood Suck, Treasure); victory when
  every other unit is: a plain KO counts. Checked after each action's rewards, at Wait and
  between turns. Gariland's own condition script was not read (the generic path is "defeat all
  enemies"). Its war trophies (ENTD): Mythril Knife, Phoenix Down, Potion.
- TARGET (`rewards`, `job_level`, `outcome`, `pick_up`, `end_battle`, `draw_end`, `ct_next` in
  `fft.c`; the crystal and the chest from `BATTLE/OTHER.SPR`, `units.py`): the same rules and
  numbers with our RNG (`make test`: the six actions equal, crystals 0.509 of an enemy's and 0.748
  of a player's over 4000 each; the counter 3, 2, 1, 0 then the object; a battle won and one
  lost by keys). Differences, for the TI: a crystal only restores HP and MP; a chest's item is
  shown, not kept; JP shared to a unit of another job are dropped (we keep each unit's own job
  only); a single target, so no highest-award rule; the results have no war trophies; the outcome
  is checked after each action and at each turn's start (which covers Wait). The crystal is one
  of FFT's eight turning images, white with a black line, floating; the chest is drawn at 0.55
  (24 px wide) with its greys picked per material (by luminance it was a mush).

### Moving (milestone 20)

- OBSERVED IN FFT (oracle: the move ranges shown to Ramza, Move 5, and to the recruit after him,
  Move 4, Jump 3 both): the canal tiles (surface 0x0E, depth 1) are in them, a chimney too; the
  occupied tiles are not; a tree tile is never a destination.
- OBSERVED (fft_decomp, read: `battle_move_set_tile_flags_for_pathfinding`,
  `battle_move_calculate_spread`, `battle_move_update_candidate`, `battle_move_check_horizontal_
  jump`, `battle_move_init_source_geometry` / `_destination_geometry`): a tile is enterable unless
  blocked (untargetable ones too for a walking unit); a step's remaining range loses the terrain
  cost of the tile it spreads from (`g_main_terrain_movement_cost_tables[0]`, 0x8005ea50: 2 for
  surface 0x0E-0x11, 255 for 0x12, 0x19, 0x1C, 0x3F, 1 for the others at Gariland); its height
  is the exit side's (+ 2 x depth out of water) against the entry side's, at most 2 x Jump; a
  unit spreads by jumps too, over up to Jump / 2 tiles (1 here), to a tile no lower, the tiles
  jumped over below the unit's body; enemies' tiles are not enterable, allies' are.
- LIKELY INTERPRETATION (fitted to the two ranges and the AI's turns, where the code read above
  did not match): a step from water to water costs 2, any other step 1; a step into water from
  land cannot use the last point; a jump costs 2 (3 from water) and climbs at most Jump half
  units (3: a 1.5-level rise), not 2 x Jump. With these the two ranges are exact (0 tiles off of
  150 each) and the AI's turns end on the original's tile 125 times in 136 (73 with the old
  rule: canals, trees and chimneys blocked).
- TARGET (`edges`, `spread_start`, `spread_go`, `compute_reach`, `make_path`): those rules for the
  player's units and the AI; the AI's spread from its focus uses the code's cost (the tile it
  spreads from: from the focus outwards, so a step out of water costs 2), which matched more of
  its moves. A jump's path is one step of two tiles (the walk slides over the gap). Units in
  water stand on its bottom; milestone 27 adds depth-1 sprite immersion below.

### Enemy AI (milestone 20)

- OBSERVED (fft_decomp, read: `battle_ai_build_command`, `select_initial_action`,
  `init_target_consideration`, `find_nearest_target`, `build_movement_scenarios`,
  `select_destination`, `fill_foe_proximity_grid`, `simulate_action_and_score`,
  `classify_ability_effect`, `evaluate_ability_outcome`, `evaluate_movement_origins`,
  `take_next_movement_origin`, `insert_ranked_action`, `select_ranked_action`,
  `choose_wait_facing`, `select_retreat_action`; a sub-agent's report, every point re-read):
  FFT plays each candidate (ability, target tile, origin tile) in its battle engine with the
  random rolls at their middle (`get_random_0_7fff` returns 0x4000: no critical, Dash's mean)
  and scores the field: each unit HP x 128 / max HP plus a signed weight per status (Dead -192,
  Critical -32, Poison -26, Darkness -64 x the share of its abilities it blocks...) plus 357 for
  its kept equipment, the actor's side + and the other -. The best candidate of each ability
  enters a ranking of 8 if it beats the score of moving to the fallback tile (by score, then hit
  chance, then the origin's danger); the ranking is drawn in order, entry i taken when rand() %
  100 < its hit chance, else the first. Tiles: danger = sum over the enemies acting before the
  unit's next turn of max(0, 10 - steps, heights aside); steps from the focus by the unit's moves
  (125 where the spread does not reach); the focus is the nearest enemy (Manhattan, the lower
  index on a tie), for a Critical unit with support left (two of its side alive, one not
  Critical) or a guest under 45/128 HP the nearest unit that can give HP (Potion, Wish) or a
  crystal. Within reach (Move + the shortest range on an enemy, items counting half on an ally,
  at least the steps from the focus) the fallback is the safest tile one step nearer (3 steps
  with a ranged ability against an enemy), else the safest tile from which it strikes next turn
  (that reach halved 1 time in 7 for the enemies); acting from its own tile it acts, then runs the
  decision again with no ability: the safest tile (hit and back off). Attack is tried from every
  tile (its facing changes the hit chance), the others once per target from the safest tile
  reaching it (a target used up only once really tried); an ability that can reach the user is
  also tried on itself from its own tile then the fallback tile (ties: from its own tile first).
  Throw Stone and items thrown 2 tiles or more need a clear straight line (from 24 above the
  feet to 24 above the target's, every 2 units: inside a tile's column up to its height x 12, or
  a standing unit's 16 x 16 x 36 box, blocks it). Enemies use the items of their level or less
  (Potion only, level 1; Phoenix Down is level 13), items at the unit's Move at most. With none
  ranked it goes to the fallback tile, Defending if Defend is its support. At Wait it faces the
  nearest enemy acting before its next turn, by quadrant, unless the neighbour that way is off
  the map, blocked or 3 higher.
- OBSERVED (oracle `--ai`): of the original's 136 turns (40 of the battle, 96 in 8 built
  situations), the same action and target as our first choice 129 times, in our ranking 134,
  the same tile at the end 125, the same facing 128; in each built situation FFT's most
  frequent choice is our first (a Squire next to a recruit attacks then backs off; Critical:
  Throw Stone from its tile then away; out of reach: closer; the Chemist: Potion on the hurt
  ally from the tile its throw reaches, then away; a recruit at 4 HP: Throw Stone 9 times in 12,
  Dash 3; the Chemist with nobody in reach: moves and Defends; Delita under 45/128: still
  attacks). FFT's support abilities: drawn among those the unit's jobs' JP bought
  (`main_unit_calculate_rsm`): at 199 JP at most only Equip Change (Chemist, 0 JP, always),
  Equip Axe or Defend (Squire, 170 or 50 JP); the enemy Chemist of the oracle has Defend (0x1DF),
  the others Equip Change (0x1E0), Ramza none.
- OBSERVED (probe): FFT's tile priorities are never reset between turns (`score_reachable_tiles`
  writes only the tiles it scores); with the unit's own tile left unscored they can make it rank
  first among the origins. LIKELY INTERPRETATION: the case is rare; emulated (the field's
  absolute score, the grid kept across turns, FFT's own grid loaded in the tests) it changed none
  of the 136 turns, so it was left out.
- LIKELY INTERPRETATION: FFT's mostly-support policy (a unit with 77/128 of its usable
  abilities on allies heals and revives first) cannot occur at Gariland (Attack is always
  usable, an enemy Chemist only Potion), nor statuses but KO, Critical and Defending.
- TARGET (`ai_setup`, `ai_setup2`, `ai_ability`, `ai_ability_go`, `ai_score`, `ai_clear`,
  `ai_choose`, `ai_facing`, `ai_think`, `ai_next`, `ai_step`): the same rules computed on what an
  action changes (the units hit, the user for Wish, a crystal's or a chest's pickup), the same
  ranking and draw with our RNG; Defend drawn as FFT draws a support (Defend learned with the
  Squire job's JP after the reactions and supports listed before it, then one of the two
  supports), a command of the Act list for the player's units that have it. The turn shown as a
  player's without the menus. Cost on the TI: the picture stands still once two frames in a
  row have the same checksum (the animation clock stops while thinking), the frame is then not
  drawn and the thinking runs a step a frame within ~270k cycles: 5 frames a turn on average,
  6 at most, after the camera's pan (~13 frames, as before).

### Presentation (milestone 21)

- Decisions (grilling): every frame of the game within 360k (the start, the rotation and a
  picked tile's path allowed as stalls); C before asm (asked first); FFT's throw pose only (no
  weapon sprite: 1-2 px at 0.6; no item pose); the hit as FFT draws it, measured; the camera:
  while walking it follows the walker past a margin, while aiming or acting it stays when both
  units are on screen, else centres between them; the markers drawn from each tile's real
  corners; the HUD as it is unless the user flags something.
- OBSERVED (`BATTLE/TYPE1.SEQ` animations 58 / 59: frames 11, 27, then 28 held facing the camera
  and 16, 29, 30 facing away; run headless, `oracle.py`'s skills state, Ramza's Throw Stone on
  the recruit, a picture every 3 frames): the thrower raises the arm behind the head then holds
  it forward over the head: the same wind-up as the swing (27 / 29), then 28 / 30 instead of the
  blow 31 / 33.
- OBSERVED (same run, the target every 2 frames): at the impact a white burst about twice the
  unit's size covers the target for ~4 frames (60 Hz), then breaks into white sparkles and a
  blue glow flying apart for ~12 frames; the number rises over it; the target's sprite itself
  keeps its colours (the hit pose).
- TARGET: the throw pose for Throw Stone and the items (`unit_pose`, `units.py` frames 28 and
  30); the burst as a white blob outlined black for 2 frames and 8 outlined sparkles flying
  apart for 6 (`art.py` `spark`, `draw_spark`), for damage that hits: white alone vanishes on the
  light floors, FFT's glow cannot be drawn in 4 greys.
- OBSERVED (`map.h`): 25 of the 150 tiles are sloped: ramps of 1 or 2 units (two adjacent
  corners up) in the four directions, and two tiles with one corner up.
- TARGET: the cursor and the rings drawn on the tile's four corners at their heights (`art.py`
  `slope_marker`, 13 shapes, picked by the corners' lifts in the view, `marker`).

### Pace (milestone 23)

- OBSERVED (run headless from `gariland_t1.state`, Ramza's first turn; each unit's movement
  record `g_battle_unit_misc_data` 0x800b7308, 16 x 0x440 bytes, found by its backlink +0x134
  to the unit's stats: `real` +0x18 x / height / depth in 1/4096 world units, `step_phase`
  +0x7f; read in fft_decomp): Move, a tile picked, the unit walks at once (6 frames after
  [O]) and FFT asks "Are you sure you want to move here?" once it stands there. A flat tile
  is two phases of 7 frames at 60 Hz (to the edge, to the centre: 8190 units a frame, a tile
  28 x 4096): 14 frames, 0.23 s; a step up 2 units: the edge, a jump phase rising 10 frames
  and falling 2, the centre: ~21 frames. (Our first estimate, 0.33 s, counted the cursor's
  glide.)
- OBSERVED (Ramza's Attack on the recruit, Broad Sword; pictures every 2-6 frames): Execute
  at 0; the panels close (8); the wind-up from ~10, the blade from ~26; the slash and the hit
  at ~31 (the HP lowered in RAM at +10, ahead of the picture); the target's hit pose ~48
  frames; the number rises ~12 frames and stays to ~+84; "10 EXP." +92-+128, "10 JP" +134-+164;
  the menu back at ~+188 (3.1 s).
- OBSERVED (the first AI turns after Ramza waits, a Chemist's Attack): the turn's unit at 0,
  the camera pans to it (~30 frames), its panel ~36, the move range then the walk; after the
  walk the camera turned (~16 frames) where the actor was behind a house; its panel again
  (~36), the cursor glides to the target (~12) and its panel (~12), the preview with the hit
  chance ~30, the blow ~30 later; a level up message ~40; the next turn ~200 after the hit.
  No ability was used in these turns: the name box was not measured (~1 s kept from the
  grilling).
- TARGET (30 fps, half the frames): a tile 7 frames, a climb, drop or jump 10 (a hop table);
  the attack: standing 5, wind-up to 13, blow to 16, the hit at 16 (hit pose 24, the number
  24), the EXP at +29, the JP at +51 (18 frames each), the menu at +78 (+28 when nothing is
  earned); the AI: the cursor gliding a tile every 2 frames to its tile and its target, the
  preview 15 frames, an ability's name 30 more. The panels' slides are not reproduced (our
  panels are drawn when they change).
- TARGET (the AI's camera): FFT's rule is not read; ours turns only when the actor (on the tile
  it acts from) or its target is more than half hidden, to the view hiding the least of the
  worse of them, and the player's view comes back on the player's turn. Hidden rows from the
  map's corner heights (the top faces' upper edges at four columns of the sprite): as the
  view's depth says half hidden or not on 558 of 560 tiles and views (`make test`).

## Blows (milestone 25)

- OBSERVED IN FFT (disc plus `tools/weapons.py --check DISC STATE`): the level strike uses
  unit SEQ 130/131, body wind-up 86/135, blades with body 87/136, 89/138, 90/139. Weapon
  scripts 2/3 in WEP1 (TYPE1 units) and WEP2 (Chemists) select their own shapes. WEP SHP
  has 32 weapon-type offsets at +4, 512 frame offsets at +0x44, data at +0x846; its header
  rotation indexes `BATTLE.BIN`'s table at 0x80094508. This is a screen rotation, applied by
  `battle_gfx_construct_polygon_data_for_units`. WEP.SPR's first palette block and sheet at
  +0x200 supply the blade; `g_battle_gfx_item_graphic_data` at 0x800943e4 supplies its palette
  and row. Dagger type 1, Broad Sword type 3. On the original's back-facing level swings,
  frames 34/35/36 and 74/75/76 match our body frames, weapon animation, rotation and part
  position/size/UV exactly. The fixture updates the initialized display weapon as well as
  battle stats; a stats-only poke leaves the original's graphics stale.
- TARGET: existing 0.6 scale and original palettes, a blade thickened horizontally and
  vertically to at least two pixels, white outline. Rotation and sampling are offline;
  separate archived `fftw` (40,704 bytes), no RAM sprite copies except the covered blade.
  Armed hands/body use the original level-strike frames; low/high swings remain adapted to
  the level swing. Blade drawn behind the body (the ordering table prepends the original's
  weapon layer), hidden during wind-up, visible at 13-18, put away at 19.
- TARGET (user's explicit choice): the earlier burst/sparkles removed. Successful damage
  flashes only the sprite's fourth-plane interior: light grey 2 frames, white 8, light 2,
  then normal; black contour and dark shadow stay. Rendered recoil away from the attacker:
  horizontal 2 px/vertical 1 for 6 frames, then 1/1 for 4, then 0. The logical tile, HP,
  hit/miss, abilities and critical gameplay knockback are unchanged.
- CHECKED: 91 interior pixels light/white, 32 black contour pixels unchanged, misses/heals/
  Defend unflashed; `make test`; 25 default xcheck scenarios, 87 additional PC/TI screens at
  blow phases and replay ends (`make check-blows`). Every measured ordinary frame <= 360k:
  blow doors 307,218 max, 14,000-frame AI run 356,180, defeat replay 359,070. Initial/view
  loads, rotations and all-at-once reach creation are the allowed stalls. Optional HUD,
  cursor mask and ring removal no longer force themselves into a crowded camera pan.
  Review sheet: `make review-blows` -> `x/m25_blows.png` (local).

## Numbers

| | FFT | TI | how |
|---|---|---|---|
| map | 10 x 15 tiles, h 0-10 | same | `extract.py` (MAP022.9 terrain) |
| tile / height unit | 28 / 12 world units | 24 x 12 px / 6 px | ratio kept (6.3 px) |
| unit height | ~0.9 tile width (capture) | ~22 px of 24 (scale 0.6) | capture, `units.py` |
| Move / Jump | Squire 4 / 3, Chemist 3 / 3, Ramza 5 / 3 (Battle Boots) | same | job and item data, oracle RAM |
| HP at level 1 | Squires 34-44, Chemists 35-38, Ramza 52, Delita 53 | same model, drawn | `battle.py --check`, `test_draw` |
| rotation | ~0.5 s, smooth | 3 frames, 0.54 s | ti-cycles |
| CT | +Speed per tick, act at 100+, -100, +20 per unused Move / Act, max 60 | same | decomp, 46 turns of `oracle.py --turns` |
| Attack | XA (PA, knife (PA + SP) / 2) x WP, zodiac, critical 4 %, hit 100 - class evade from the front | same | decomp, 40 previews and 400 attacks of `oracle.py --attack` |
| abilities | learned: JP 100..199 spent in order on passing learn rolls; Dash 1..4 x XA, Throw Stone 1..2 x XA, Wish 2 x max / 5, Potion 30, Phoenix Down 1..20 | same | decomp, 13 cases and 600 trials of `oracle.py --skills` |
| rewards | 10 EXP (kill 20 / KOs) + level difference, 100 a level; JP 8 + 2 x job level + level / 4, a quarter shared; death counter 3, then crystal (enemy 1/2, player 3/4) or chest | same (the share to the same job only) | decomp, six actions of `oracle.py --rewards` |
| moving | canals waded (water to water 2), jumps over one tile, trees crossed | same, the two measured ranges exact | 2 ranges of `oracle.py --ai`, decomp |
| AI | scores the field (HP x 128 / max, KO -192, Critical -32), ranks 8, draws on hit chance; danger 10 - steps; hit and back off | same, 129 of 136 turns the same action, 125 the same tile | decomp, 136 turns of `oracle.py --ai` |
| a hit | a white burst ~4 frames, sparkles ~12 (60 Hz) | interior flash 2 light / 8 white / 2 light, small visual recoil, no sparkles | user choice, milestone 25 pixel checks |
| pace (60 Hz / ours 30) | a tile 14, a jump ~21; Execute to hit 31, menu +188; AI glide ~12, preview ~30 | 7, 10; 16, +78 after the hit; 2 a tile, 15 | movement records and pictures, § Pace |

## FFTA-style interface (milestone 26, 2026-10-13)

- OBSERVED IN FFTA: the local original captures `x/ffta_*.png` show bottom portrait bands,
  bottom-right MENU and ball cursor, target band mirrored, full-screen Status with a second
  equipment page. These are layout references; the numbers and rules remain FFT's.
- OBSERVED IN FFT: portrait raw pixels are `.SPR +0x8200..0x9200`, a 256x32 texture.
  The original quad maps UV `(128,0),(128,31),(80,0),(80,31)` to the portrait's four corners:
  the 48x32 patch is rotated into a 32x48 bust. Portrait palette 8 matches player CLUTs in
  the oracle's VRAM; enemy recolour palette 11 is adapted to the same four-greys portrait.
  `battle_gfx_load_spritesheet_into_vram_slot` and
  `battle_menu_build_unit_portrait_poly` establish the texture and UVs.
- OBSERVED IN FFT: the floating selection pointer uses FRAME.BIN UV `(64,128)..(80,152)`,
  CLUT `(64,0x1E2)`. Its palette is `BATTLE.BIN` at loaded address `0x80094B64`. Texture
  pixels start at FRAME.BIN `0x1000`. The original points at its cursor; the target uses
  this shape over the active unit, as requested, keeping native tile targeting rings.
- OBSERVED IN FFT: `EVENT/SMALL.OUT` loaded at `0x801D7000` binds name sections through
  offsets at `+0x70` and string base `+0xEC`; the names start at `+0xED`. Entries
  `0x100..0x1FE` are 255 male names; `0x200..0x2FE` are 255 female names. Name records use
  digits `0..9`, capitals `0x0A..0x23`, lowercase `0x24..0x3D`; `FA` is a space,
  `D9 C1` an apostrophe, `D9 B6` a period, `FE/FF` terminate a record. Names are at most
  11 characters. `main_unit_store_character_names` excludes used names of the same gender.
- TARGET IMPLEMENTATION: `tools/interface.py` creates archived `ffti` (40,876 bytes),
  generated `interface.h`, six mirrored 19x29 busts with white outlines, a 16x14 pointer,
  two fixed-width name tables and AMS small glyphs pre-shifted offline. Portraits use
  nearest-neighbour scaling, four greys by the existing palette-luminance thresholds.
  Generic names are unique by gender, drawn once per battle by an independent 16-bit
  generator seeded from that battle; existing combat/stat RNG streams stay unchanged.
- TARGET IMPLEMENTATION: 80-pixel horizontal bands, light grey with a dark rule, portrait
  rising above the band, name / HP / MP / Lv-CT. Menus replace one another bottom right
  under a tab, ball cursor; a valid target gets its mirrored band and the hit/amount
  preview above it. Motion and action animations hide bands and menu. Height `2h` / `0h`
  and canal `depth 1` appear top right. Status pauses simulation: stats on page 1, equipment
  and learned skills on page 2; LEFT / RIGHT pages, ESC / shift returns.
- CHECKED: `interface.py --check DISC gariland_t1.state` matches all six portrait sheets
  against the original's VRAM (9,216 source portrait pixels), pointer texture/palette and
  all nine generic oracle names. Native tests cover seeded name stability/uniqueness,
  HP/CT updates, page reversal with unchanged units, depth label and hidden motion bands.
  40 xcheck scenarios, 116 additional UI partial-build/page/input screens and 87 existing
  blow/replay screens agree between PC and TI. Fifteen review states are in local
  `x/m26_interface.png`. No new hardware path; no emulator run for this milestone.
- CHECKED: all ordinary measured frames <=360,000 cycles: UI doors 313,278, the 14,000-frame
  AI replay 357,270, defeat replay 359,840. Profiling found a just-completed background cache
  was permitting forced HUD work in the expensive frame that built it: `bg_drawn` records
  whether the frame actually used the cheap cache. Budget also adjusts when scene edits
  invalidate a previously usable copy. Aligned long copies and cached preshifted text keep
  the native UI small; Status builds a part per frame, peaking at 173,440 cycles.

### Character HUD revision (2026-10-14)

- TARGET (user reference): transparent character details, with a cutout portrait on the
  left, name to its right, HP and MP stacked with small current/maximum values above
  their gauges, and only a small level under the portrait. No zodiac badge or CT field.
  Both active and target panels use this layout; Status keeps its full statistical pages.
- IMPLEMENTATION: the original portrait's palette-zero mask already removes its background;
  the previous grey band hid that transparency. Reuse the same six outlined 19x29 assets,
  with no new bank or extraction. Five cached parts compose an 80x36 panel, including an
  opacity plane for the portrait, outlined glyphs and 56-pixel gauges. HP is black, MP dark
  grey, empty gauge interiors light grey. Values align to the right above each gauge.
  CT no longer invalidates the panel cache. Motion and blows still hide the details.
- CHECKED: existing native tests, 52 scenario PC/TI screens, 116 UI phase/input screens and
  87 blow/replay screens pass. Every ordinary measured frame remains <=360k: UI doors
  317,458, 14,000-frame AI replay 356,014, defeat replay 357,308. The generated panel text
  and HUD loops have no software multiply/divide or floating-point helpers. Program 62,075
  bytes; allocations 52,320 and 55,556 bytes, both below 64 KB. Actual framebuffer review
  covers all six portraits, empty HP/MP, 500/999 HP, 999/999 MP and level 99, in local
  `x/character_hud_details.png`; active/target scenes are in `x/character_hud.png`.
  No hardware path changes or emulator run.

### Compact attack HUD revision (2026-10-14)

- TARGET (second user reference): level over the portrait, shorter/thinner gauges, two
  opposing character panels during an attack, with the portrait defining the HUD's height.
- IMPLEMENTATION: complete 80x29 panels, level over the lower portrait corner, gauges
  34x2 with a one-pixel white border. Left portrait mirrors inward, right portrait sits
  at the outer right edge and looks inward. Name/forecast, HP and MP fit in the portrait's
  height, with no panel background. Palette-zero portrait cutouts and archived data stay
  unchanged. The valid target's cached inline damage/heal/status and chance replace the
  actor's name, with an outlined central chevron; the old TARGET window is removed.
  The forecast calls the existing combat formulas, including skill amount ranges, and
  caches its target/ability together with the actor's stats. Target name remains visible.
- CHECKED: native tests, 52 PC/TI scenario screens, 116 UI partial/input screens and 87
  blow/replay screens pass. Every ordinary measured frame stays <=360k: UI doors 317,566,
  14,000-frame AI replay 356,116, defeat replay 357,410. Native HUD/text loops have no
  software multiply/divide or floating-point helpers. Program 62,109 bytes; allocations
  52,320 and 54,300 bytes, saving 1,256 RAM bytes from the preceding panel layout.
  Actual framebuffer review: `x/compact_attack_hud.png` (menu and attack),
  `x/compact_hud_details.png` (six portraits, empty bars, level 99 and three-digit values).
  Potion aiming also shows the inline positive prediction. No hardware path changes.

### Gauge alignment refinement

- TARGET / IMPLEMENTATION: align the gauge's visible left edge with the text anchor,
  round its four outer corners through the opacity mask, and align the values' right
  outline with the gauge's right outline. The two-row fill stays thin; width 40 leaves
  a gap between the ten-pixel `MP` label and the 28-pixel `999/999` string.
- CHECKED: native tests, 52 scenario comparisons and 203 UI/blow/replay PC/TI screens
  pass; ordinary peak remains 357,410 cycles. Program 62,171 bytes, allocations unchanged.
  Actual PC framebuffer review includes three-digit HP/MP on both panels at once;
  rounded outer corner pixels remain transparent. Local review: `x/rounded_hud.png`
  and `x/rounded_hud_999.png`.

### Target attachment and active-marker revision

- TARGET / IMPLEMENTATION: enemy gauges and values now end against the portrait at
  panel column 61; the target name aligns to the same edge. Both character names remain,
  while the centered prediction uses a separate seven-row outlined cache above the
  comparison arrow. Cache matching suppresses a stale prediction when target/ability
  changes. All 510 source names fit: the longest measured string is 43 pixels, leaving
  the target name's first pixel inside its panel. Three-digit HP/MP still fit both sides.
- TARGET / IMPLEMENTATION: a native 16x19 active pointer replaces the unreadable reduced
  source marker. `tools/art.py` draws a ring, wide crossbar and shaded pointed blade,
  dilates the silhouette for a white outline, and emits three native word-row planes in
  `art.h`. The original archived pointer stays as unused reference data, preserving the
  existing bank layout. Its full body/marker top margin is 39 pixels. Facing selection
  holds the camera when the actor fits, avoiding unnecessary repeated full-view copies.
- CHECKED: tests, 52 PC/TI scenario screens, 116 UI partial/input screens and 87 blow/replay
  screens pass. The AI finishes the same 80-turn native battle in 10,086 frames. Every
  ordinary measured frame stays <=360k: the 14,000-frame AI replay peaks at 350,712, defeat
  replay at 357,400. A 369,558-cycle facing frame during the initial camera-margin change
  was mandatory work (budget already negative): profiling isolated the moving view copy,
  eleven actors and the facing arrows. Holding the fitting camera removes that overrun;
  no extra stall exemption or larger budget. Generated outlined-text/HUD loops have no
  software arithmetic helpers. Program 62,733 bytes; allocations 52,320 and 54,930 bytes.
  Review: `x/target_hud_marker.png`, `x/target_hud_999.png`, `x/active_marker.png`.

### FFT command hand and Status inspection refinement

- OBSERVED IN FFT (ordinary pad input, `gariland_t1.state`, no RAM writes):
  `DOWN:24 DOWN:24 DOWN:24 CIRCLE:120` opens Status; `SELECT:60` activates its
  field cursor. Four `DOWN:30` presses followed by `CIRCLE:60` open the Move help
  panel. The original uses a white glove outside the command frame and beside the
  selected Status field. Local original captures: `x/ps1_status_overview.png`,
  `x/ps1_status_select.png`, `x/ps1_status_down.png`, `x/ps1_status_help.png`.
- OBSERVED (read in `fft_decomp`): `bunit_menu_update_node_cursor` handles directional
  selection and confirmation/cancel for field explanations, including equipment and
  ability nodes 18..27. `bunit_menu_equipment_and_ability_panel_thread` draws five
  equipment slots and ability/skill-set entries. Actual gear swapping is a separate
  Change Equipment action (`battle_menu_run_equip_thread`), outside native Gariland's
  current action set. The pad capture above confirms field help, not a gear change.
- TARGET / IMPLEMENTATION: a native padded 16x9 outlined white glove replaces the
  internal ball, extending outside light-grey framed menus with black title tabs.
  Status's second page selects each of its five equipment slots and learned abilities,
  including learned Defend. Confirmation opens existing item bonuses or ability
  range/height, JP, effect, stock and evasion; ESC returns to the list. Browsing never
  changes equipment, HP, learned skills or inventory. Cursor movement reuses the cache;
  detail opening/closing keeps the nine-part per-frame build.
- TARGET / IMPLEMENTATION: active pointer reduced from 16x19 to padded 16x13, retaining
  the ring/crossbar/blade and white silhouette outline. Camera framing stays at 39
  pixels, now including six pixels of headroom. Reducing that margin with the sprite
  caused a 363,120-cycle defeat frame; keeping established framing removes the overrun.
  The terrain HUD draws height alone; removing the depth line saves 600 cache bytes.
- CHECKED: unit tests (slot/ability navigation, detail/back, empty entries and unchanged
  unit data), 58 PC/TI scenario screens, 177 partial/input UI screens and 87 blow/replay
  screens pass. Ordinary peaks: UI 295,764, Status 202,426, 14,000-frame AI battle
  349,634, defeat 356,322 cycles; no budget exemption added. Program 64,937 bytes;
  allocations 52,320 and 54,332 bytes. Review: `x/m26_interface.png` and
  `x/status_details.png`. No hardware path changed or TiEmu interaction.

### Animated glove and narrower pointer refinement

- TARGET / IMPLEMENTATION (user's marked-up native screenshot): active marker outline
  width 10 -> 8 pixels, keeping its 13-row height and existing camera framing. Glove
  outline width 15 -> 18 pixels, padded to 32 for the existing ExtGraph sprite primitive.
  An eight-byte lookup gives a three-pixel sideways motion in 32 native frames; the
  maximum-right tip leaves a one-pixel gap before menu and Status text. This cadence is
  a native visual adaptation, not an original FFT timing measurement.
- TARGET / IMPLEMENTATION: remove the horizontal white highlight and draw only the
  title tab above the frame. Its cached first row masks the three upper rows, preserving
  scenery in the rest of that strip without another cache or background redraw.
- CHECKED: tests, 58 PC/TI scenarios, 177 interface screens and 87 blow/replay screens
  pass. Peaks: UI 295,790, AI battle 349,660, defeat 356,348 cycles; all <=360,000.
  Program 65,133 bytes; RAM allocations unchanged. `make review-interface` now writes
  `x/hand_menu.gif` from eight real framebuffer captures over one full animation cycle.

### Shorter, taller glove and round active ring

- TARGET / IMPLEMENTATION (user correction): glove width 18 -> 15 pixels, height 9 -> 12,
  increasing its palm vertically instead of lengthening its index finger. Restore the
  16-wide sprite primitive; reposition it so the existing three-pixel sway still leaves
  one pixel before the text and the full glove fits even the bottom menu row.
- TARGET / IMPLEMENTATION: upper marker ring uses a six-by-six round contour and rounded
  four-by-four opening. Outlined width stays eight pixels; padded height becomes 15.
  Established camera framing stays unchanged, with four pixels of headroom.
- CHECKED: tests, 58 PC/TI scenarios, 177 interface screens and 87 blow/replay screens
  pass. Peaks: UI 296,338, AI battle 349,660, defeat 356,348 cycles. Program 65,109 bytes;
  allocations unchanged. Updated actual-framebuffer animation: `x/hand_menu.gif`.

### Canal immersion (milestone 27)

- OBSERVED IN FFT (`make check-water-original`, `tools/water.py`, the reproducible
  `gariland_t1.state`): all 38 canal records have surface 0x0E, floor height 0, depth 1.
  Humans remain at that floor (`misc.screen.vy = 0`), but use the submerged SHP set.
  TYPE1/TYPE2.SHP's first word points to its second 256-offset table; each table's frame
  data starts 0x402 bytes after it. Across six sheets and ten idle/walk poses each, the
  head stays at the same source row and the lower opaque extent loses 6..13 rows:
  Ramza 8..12, Delita 6..11, male Squire 7..13, female Squire 8..11, male Chemist 7..10,
  female Chemist 6..8. The submerged bottom is 4..8 source rows above the anchor.
- OBSERVED IN FFT (live GPU packets): 5,472 complete submitted UV-part sets match the
  disc's normal/submerged pose sets. Each of the 38 actual terrain records is copied
  beneath a visible test unit, paired with depth cleared, for all six sheets; each pair
  uses a fresh state, healthy idle, 24 settling frames and a 26-video-frame probe window.
  This is a controlled display experiment, not 38 walked paths. Off-camera units are
  culled; the shared display scratch can be overwritten by another unit; submitted GPU
  packets lag the current animation record. The checker reads submitted CLUT/page packets
  and matches the whole pose set, requiring a nonempty result for every pair.
- OBSERVED IN FFT (ordinary pad-driven movement, no coordinate/terrain pokes): from
  Ramza's first menu, `CIRCLE:60 CIRCLE:60 UP:30 CIRCLE:100` moves (4,11) -> (5,11).
  His height falls -24 -> 0; the submitted submerged pose first appears after the floor
  landing (video frame 185 of this sequence). Local evidence: `x/m27_original.txt`,
  `x/m27_original_walk.txt`, `x/m27_original_water.png`, `x/m27_original_poses.png`.
- OBSERVED (read in fft_decomp): `battle_gfx_select_unit_shp_frame` switches an ordinary
  human to its submerged set when the surface is at least 12 world units above its feet;
  a raised hop uses the normal set. `battle_gfx_calculate_screen_z_from_input_coords`
  keeps ordinary humans on the floor; Float, Walk on Water, crystals and treasure have
  their own exceptions. Gariland has no deeper canal case to reproduce here.
- TARGET (`unit_in_water`, `draw_unit_water`): fixed-height native cropping at the projected
  depth-1 surface, six pixels above the anchor: draw 15 of the 26 stored rows, hiding the
  lower legs and shadow, with a light-grey edge following the existing contour/cover rule.
  Wet flashes and blades stop at the same surface. Wading stays cropped; hopping into or
  out of water uses the full pose until grounded. Crystals/chests retain their full images.
  The original's separate submerged art and variable limb shapes are adapted to a fixed
  cut; no new sprite bank or per-pixel operation is added. Terrain classification is cached
  with each unit's cover; the dry shader keeps its existing fast path (22 extra RAM bytes).
- TARGET (performance): the active-unit pointer identifies an AI actor while it thinks;
  the tile cursor returns when the plan is shown. This avoids combining its drawing with
  a new turn's clock/order update, uncached camera copy and eleven unit covers. CT order
  uses `ti-table recip --n 51` products instead of repeated divides (numerator <=149,
  table exact through 815); 300 predictions at Speed 1..50 equal full clock advances.
  `compute_reach` skips removed units before marking occupied cells: the new traversal
  fixtures exposed an out-of-range write that the TI binary caught as code corruption.
- CHECKED: unit tests, 52 xcheck doors, 168 water phase screens, 116 UI screens and 87
  blow/replay screens pass. Every ordinary measured frame remains <=360,000 cycles:
  water doors 254,232, AI battle 355,466, defeat replay 356,764, UI doors 316,910.
  Review: `x/m27_water.png` (twelve actual native scenes). Program 60,451 bytes, game
  buffers 104,880 bytes; archived banks unchanged. No new hardware path or emulator run.
