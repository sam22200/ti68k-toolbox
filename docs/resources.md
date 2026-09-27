# TI-89 resources (C & 68000 ASM)

## Tools

- Compiler: https://github.com/debrouxl/gcc4ti — Emulator: https://github.com/debrouxl/tiemu
- Graphics: https://github.com/debrouxl/ExtGraph — OS patcher: https://github.com/debrouxl/tiosmod
- Patches: HW3Patch / h220xTSR — https://www.tigen.org/kevin.kofler/ti89prog.htm
- 68k OSes (TI-89 1.00→2.09, Titanium 3.00→3.10, AMSpatch): https://tiplanet.org/forum/archives_list.php?order=hit&cat=OS+68k&multi_chaine_search=OS#files
  (command-line download: `https://tiplanet.org/modules/archives/download.php?id=<id>`, without `/forum`)
- 68000 reference (optional): https://www.nxp.com/docs/en/reference-manual/MC68000UM.pdf
- Sites: https://www.ticalc.org, https://www.yaronet.com/forums/2-ti, https://tiplanet.org

## Important sources for learning C (ticalc.org)

All of the sources below are downloaded and extracted in `sources/` (index and what each is good
for: `sources/README.md`; they are all under `https://www.ticalc.org/pub/89/asm/source/<name>.zip`).


- TICT Tutorial Series 1 (1p1/s1p1 … s1p8): uncompressed images; an interrupt handler as a
  clock; TSR programming in C (v1.05); generating sprites; grayscale basics; grayscale text
  scroller over a background image; fast pixel-access macros and line drawing (s1p7_89 v1.10);
  fast scrolling up/down/left/right (s1p8_89). Series 2 (advanced): s2p1 texturing vertical strips,
  s2p2.
- tictex_src_89.zip — TICT-Explorer v1.20 (NOSTUB file explorer: folders, archiving, eBooks,
  PIC/TEXT, hex viewer, crash protection, 6 languages…).
- tichess_src.zip — TI-Chess v4.00 (full chess game for the TI-89/92+/V200).
- All the sources by the user's brother (sources / src / 3D tutorial):
  https://www.ticalc.org/archives/files/authors/60/6027.html

## Nice C sources

- https://github.com/debrouxl/ti89_authenticator, https://github.com/debrouxl/ti89bb
- tankers.zip (Tankers 68k v6.0.1: tanks, single player vs AI + link-cable multiplayer), star.zip
  (grayscale starfield), snow.zip (snow effect), mode7engine.zip (Mode7 pseudo-3D engine,
  projected sprites, interrupt-based speed regulation), keyreleased.zip (`_rowread`: wait until
  all keys are released), zelda_source.zip (Link's Awakening), xchange_src.zip (Xchange v1.24:
  RLE, interrupt handler, saving high scores), world_map_source.zip, life89.zip (Game of Life,
  NOSTUB), fzero.zip (F-ZERO), fatsdk.zip (FAT-Engine: textured raycasting), doom_source.zip
  (Doom89, built on FAT), crystal_engine_source.zip (Zelda engine), crogue_source.zip (CalcRogue).
- Puzzle Bobble (David Coz, 2002, ExtGraph): https://www.ticalc.org/archives/files/fileinfo/245/24579.html
  → ported in `games/puzzle_bobble/`, analysed in
  `.claude/skills/ti89-c-dev/reference/puzzle-bobble-analysis.md`.

## Nice ASM sources

- time.asm (timer), hello.asm, graytest.zip (grayscale), smasrc89.zip (SMA v0.38, Sonic),
  megacar.zip (MegaCar v1.2 + level editor), chronosrc.zip (Chrono Fantasy, RPG), bomber.zip
  (BomberBoy v.45 beta).

## Graphics assets (sprites, tilesets, maps)

Ripped from commercial games: fine as references or for personal ports, **copyrighted** (do not
redistribute them in a published game without checking).

- The Spriters Resource, https://www.spriters-resource.com/ : the largest library of sprite
  sheets (characters, animations, tilesets, backgrounds) for NES, SNES, GB/GBC/GBA, Genesis, etc.
  First place to look.
- Sprite Database, https://spritedatabase.net/ : an alternative archive of retro sprites and
  sheets, useful when a game is missing from The Spriters Resource.
- VideoGameSprites, https://www.videogamesprites.net/ : classic sprites, strongest on SNES-era
  JRPGs (Final Fantasy…).
- The chosen sheets per category (scenery, characters, UI, portraits): the `ti-art-refs` skill.
- VGMaps, https://vgmaps.com/ : complete maps and reconstructed levels; the reference for
  rebuilding a level layout or a tile map (Zelda-like overworlds, platformer stages).

Adapting to the TI-89: screen 160×100 (TI-89 and Titanium), 4 grey
levels (2 planes, ExtGraph `COLOR_*`), no colour. The best sources are **Game Boy / GBC** sheets
(4 shades, 8×8 tiles, 16×16 sprites, 160×144 screen: almost 1:1); NES/SNES assets need their
palette reduced to 4 levels by luminance and often a downscale (a 16×16 SNES character stays
16×16 but looks huge on 160×100). Store sprites as ExtGraph plane data (`unsigned short` rows for
16-pixel-wide sprites, light and dark planes, plus a mask when transparency is needed) and tile
maps as `unsigned char` indices (`ti68k-game-techniques.md` §9 and §11 for compression).

Converting a captured scene (first real case: `games/campfire/`, Chrono Trigger's camp fire):
- **Recover the native resolution first**: captures are often scaled non-uniformly (this GIF was
  256×224 × 2.484 horizontally and × 2.246 vertically); box-downscale back, then take the
  **per-pixel median of the animation frames** to kill GIF dithering (only truly moving parts differ).
- **Map and sprites separated**: sprite sheets (Spriters Resource) hold field/battle poses, not
  cut-scene poses (template matching found none of the camp poses): segment from the scene instead.
  A hand-traced polygon per character plus "remove ground-coloured regions connected to the
  polygon edge" (night ground = saturated blue) gives clean masks; interior blue parts (clothes)
  survive because they are not connected to the edge. Rebuild the ground under each character from
  a shifted patch of the same rows; it is hidden by the sprite anyway.
- **Grey levels**: a dark scene needs thresholds from its own luminance quantiles (40/75/93 % here),
  and each sprite its own quantiles (22/50/78 %), or the characters come out pale and flat.
- Budget on the TI-89: 217 unique 16×16 grey tiles = 13.9 KB; the whole program fitted in 24,365
  bytes, just under AMS 2.xx's 24,576-byte limit.

Readability: consider a **1-pixel white outline** around the main sprites (hero first, then NPCs
and enemies) so they stand out from the background in 4 greys; it only changes the mask
(`ti68k-c-patterns.md` §3, masked sprites). Not mandatory for every sprite, but ask the question
for the main character.

## Techniques from other platforms and research (web research, 2026-09)

Digest in `ti68k-game-techniques.md` §11–12 and `ti68k-performance.md` §4.

- Hardware reference: J89hw.txt, TI-89 ports (sync bit, timers, link port, keyboard):
  http://tict.ticalc.org/docs/J89hw.txt
- GCC4TI grayscale internals: `tools/gcc4ti/trunk/tigcc/archive/gray.s` and
  https://debrouxl.github.io/gcc4ti/gray.html
- 68000 decompressors: ZX0 https://github.com/emmanuel-marty/unzx0_68000, LZ4
  https://github.com/arnaud-carre/lz4-68k, LZSA https://github.com/emmanuel-marty/lzsa, exact cycle
  counts https://github.com/going-digital/execram, aPLib on Mega Drive
  https://gendev.spritesmind.net/forum/viewtopic.php?t=703
- Level and map compression (metatiles, object-coded levels): https://www.nesdev.org/wiki/Level_compression
- Pseudo-3D roads: http://www.extentofthejam.com/pseudo/ ; Commander Keen tile refresh:
  https://fabiensanglard.net/ega/ ; Another World VM: https://fabiensanglard.net/anotherWorld_code_review/
- Game AI: https://www.chessprogramming.org, https://www.gameaipro.com (flow fields, utility, influence
  maps), The Pac-Man Dossier https://www.gamedeveloper.com/design/the-pac-man-dossier, Jump Point
  Search https://users.cecs.anu.edu.au/~dharabor/data/papers/harabor-grastien-icaps14.pdf
- Integer tricks: Lemire, fastrange (https://arxiv.org/abs/1805.10941) and fast remainder
  (https://arxiv.org/abs/1902.01961); multipartite tables (de Dinechin & Tisserand 2005); Hacker's Delight.
- PRNGs: http://www.retroprogramming.com/2017/07/xorshift-pseudorandom-numbers-in-z80.html,
  https://lemire.me/blog/2019/07/03/a-fast-16-bit-random-number-generator/
- C structure: https://gameprogrammingpatterns.com, https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html,
  https://gafferongames.com/post/fix_your_timestep/
- 32 KB contests: https://demozoo.org/competitions/1848/ (Mekka & Symposium 32K game), Micro Mages
  https://morphcat.de/micromages/
