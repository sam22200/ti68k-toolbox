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

## Behaviour and data

### The map files

- OBSERVED IN FFT: `MAP022.GNS` is a list of 20-byte records (type at +4: 0x1701 texture,
  0x2E01 primary mesh, 0x3001 alternative mesh, 0x3101 end; sector at +8, length at +12) per
  time of day and weather; the primary mesh is `MAP022.9` (35,984 bytes).
- OBSERVED: the mesh's u32 at 0x68 points to the terrain: x count (10), z count (15), then
  2 levels x 15 x 10 tiles of 8 bytes: byte 0 & 0x3F surface, byte 2 height, byte 3 slope height
  (& 0x1F) and depth (>> 5), byte 4 slope type, byte 6 flags (bit 6 can't walk), byte 7 auto
  camera. Level 1 is all zero on Gariland.
- OBSERVED: heights 0 (canals) to 10 (a chimney); streets and squares at 2, houses 6-8 with
  sloped roofs; surfaces grass, waterway, wooden floor, stone floor, roof, tree, box, chimney,
  bridge. The canals are flagged can't-walk.
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
  black with a white outline (FFT: white). EXP and JP come at milestone 19.

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
