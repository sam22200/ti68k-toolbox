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

## Tools

- `psxiso.py ... sources/fft_ps1/disc` lists the files; `--file MAP/MAP022.GNS` (and the
  `MAP022.*` resources) extracts the map. Nothing else was needed: the terrain is plain data
  in a documented format, so the game was not run headless (Gariland is the second battle,
  behind the Orbonne prologue; reaching it with key scripts would have cost far more than
  reading the file).
- Format references: the FFT modding community, `adamrt/fft_toolkit` (`src/map_record.c`,
  `src/terrain.c`, read on GitHub, not copied): map list (`MAP022` = Magic City Gariland), GNS
  records, the terrain block. Every field used was checked on this map (below).

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

## Numbers

| | FFT | TI | how |
|---|---|---|---|
| map | 10 x 15 tiles, h 0-10 | same | `extract.py` (MAP022.9 terrain) |
| tile / height unit | 28 / 12 world units | 24 x 12 px / 6 px | ratio kept (6.3 px) |
| unit height | ~0.9 tile width (capture) | ~22 px of 24 (scale 0.6) | capture, `units.py` |
| Move / Jump | 4 / 3 (squire) | 4 / 3 | game data, not measured |
| rotation | ~0.5 s, smooth | 3 frames, 0.54 s | ti-cycles |
