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
- Disc readers written for milestone 13 (ENTD events, jobs, skill sets, sprite tables,
  deployment records) are in the session scratchpad; milestone 14 moves what it needs into
  `tools/`.

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

## Numbers

| | FFT | TI | how |
|---|---|---|---|
| map | 10 x 15 tiles, h 0-10 | same | `extract.py` (MAP022.9 terrain) |
| tile / height unit | 28 / 12 world units | 24 x 12 px / 6 px | ratio kept (6.3 px) |
| unit height | ~0.9 tile width (capture) | ~22 px of 24 (scale 0.6) | capture, `units.py` |
| Move / Jump | 4 / 3 (squire) | 4 / 3 | game data, not measured |
| rotation | ~0.5 s, smooth | 3 frames, 0.54 s | ti-cycles |
