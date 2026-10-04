# Metal Gear Solid (Game Boy Color) for the TI-89 Titanium

A port "à notre sauce" of Konami's Metal Gear Solid (GBC, 2000; *Ghost Babel* in Japan), made
with the `ti-port-gb` skill's big-game track: our own engine on the Portable Game Runtime,
with the behaviour **measured on the ROM** under PyBoy (speeds, walls, patrols, vision boxes,
goal box). It is not a translation of the code. The plan is in `ROADMAP.md`.
**Status: milestone 1 done** (2026-10-04): VR Training → Sneaking → No weapon → Practice → Lv.01,
playable on the PC and on the Titanium.

```sh
make test        # extracts the data from the local ROM the first time, then the unit tests
make pc && ./mgs_pc               # SDL; --scenario N (0 start, 1 near the goal, 2 spotted, 3 hidden)
make ti          # mgs.89z + mgsdat.89y (tiles and sprites): send both, mgs() from HOME
make xcheck      # the TI binary's screen = the PC's, 4 scenarios (ti-cycles, no emulator)
make tihash      # the TI binary's state = the PC's, every frame of keys/win.txt and spotted.txt
```

Controls: arrows move (8 directions), [2nd] or [ENTER] try again after a failure or a clear,
[ESC] quits.

## Source and licence

- ROM `roms/gb/Metal_Gear_Solid_.gbc` (local, not in git): title `METALGEARGBBMGE`, **CGB only**,
  MBC5 + RAM + battery, 2 MB (128 banks), 8 KB cartridge RAM, header and global checksums valid.
  Interrupts: VBlank, STAT (raster splits: the HUD), timer, serial.
- © 1987 2000 Konami. **Commercial**: the ROM and everything generated from it (`level.h`,
  `gfx.h`, `mgsdat.*`, `x/`, the PyBoy states) stay local (`.gitignore`); the repository gets
  our code, this spec and `tools/extract.py`, which rebuilds the data from a local ROM.

## Port decisions (2026-10-04, asked with the skill's grilling)

| # | Question | Decision | Why |
|---|---|---|---|
| 1 | Fidelity | **Our own engine**, behaviour measured on the ROM; not bit-exact | 2 MB of banked SM83: a 1:1 translation would be tens of thousands of instructions; the game's feel lives in a few dozen numbers we can measure |
| 2 | First milestone | **VR Sneaking Practice Lv.01 alone**, no menus or texts | smallest end-to-end slice: movement, walls, a patrol, vision, alert, goal |
| 3 | 144 rows into 100 | **Full screen + camera**: the level is 160 wide like the TI, the camera follows Snake vertically; no GB HUD | the GB's own playfield is only 136 x 128 beside its HUD column |
| 4 | Art | **The ROM's tiles and sprites converted to 4 greys**, white outline on Snake and the guard | fastest path to a faithful look; a redraw stays possible per milestone |
| 5 | Frame rate | two GB logic frames per TI frame (30.1 fps, `RT_FRAME_TICKS2=17`) | every measured timing (frames per pixel, waits, turns) holds as it is; logic costs ~3k cycles |
| 6 | Tests | unit tests on the measured numbers, two whole-run key scripts, TI = PC by state hash and screen checksum; **no screen comparison against the ROM** | the ROM's screens differ by design (camera, greys, no HUD); one PNG for the art review |

## Measured on the ROM (milestone 1)

All found with PyBoy (`sources/mgs_gb/`, local): RAM diffs between key scripts, pokes, hooks on
the code that reads a variable, and `.claude/skills/ti-port-gb/scripts/gbdis.py` (Ghidra +
GhidraBoy only analyse bank 0 of an MBC5 ROM: 48 functions).

- **Reaching the level**: power-on, 600 frames, START x3 (logos, PRESS START, menu), DOWN x3
  (NEW GAME → VR TRAINING), A x4 (SNEAKING MODE, NO WEAPON, PRACTICE, Lv.01), A, A, then four
  briefing texts (A each, ~200 frames apart); Snake appears ~300 frames later.
  (`tools/extract.py` `boot()`.)
- **RAM** (WRAM bank 0 unless noted): Snake `C5C0` moving flag, `C5C2`/`C5C4` x/y (16-bit, level
  pixel + 256), `C5C6` direction (0 up, clockwise), `C5CB` walk step 0..10, `C5D8`/`C5D9` the
  position drawn (one frame late). Guard object `C700`: `C712`/`C714` x/y, `C716` direction,
  `C719` patrol state. `C6D0` mission result: FF playing, 1 spotted, 0 goal. `C0AE` camera y
  (= SCY). Metatile map: WRAM bank 5 `D000`, rows of 16 entries x 2 bytes (id, solidity in the
  high nibble: F0 solid, 00 free, A0 left half, 50 right half), 10 x 15 used. Goal: WRAM bank 2
  `D000`/`D002` (x, y + 256). Snake's goal test: `0C:4627` (box 20 x 14 from the goal point).
- **The level**: 160 x 240 (10 x 15 cells of 16 px), start (32, 192), goal box x 118..137,
  y 25..38. The BG holds the whole level (it fits the 256 x 256 map): two states (intro at
  SCY 0, play at SCY 104) give every row.
- **Snake**: 1 px per frame orthogonally; diagonals 2 px every 3 frames on both axes; along a
  wall the free axis goes at 1 px per frame. Turning: one eighth every 2 frames (a 180° turn
  goes clockwise), the first pixel 8 frames after a 90° press from standing. Collision box
  [x-6, x+5) x [y-4, y+7) on 8 x 8 quadrants: from the start the walls stop him at x 22 and 91,
  y 180 and 201. Walk cycle: 11 steps of 3 frames. (Corner sliding exists in the ROM: not kept.)
- **The guard** (x 80): waits at y 32 facing right, walks down at 1 px per 3 frames to y 120
  (264 frames, turning 2 → 3 → 4 on the way, 2 frames per eighth), waits 288 frames facing
  down, turns to the right (24 frames), walks up (turning 2 → 1 → 0) to y 32, waits 288 frames
  (turning 0 → 1 → 2), and again. When Snake appears the guard has 160 frames of top wait left.
- **Vision**: only while the guard is on screen (with the camera far below, a poked Snake in
  front of him is never seen; detection starts as he scrolls in). Boxes relative to the guard,
  Snake's feet inside = spotted, measured on a 2-pixel grid. Facing down: dy [0, 8) dx [-4, 4);
  dy [8, 24) dx [-12, 12); dy [24, 48) dx [-20, 20); dy [48, 56) dx [-12, 12). Facing up: the
  mirror. Facing right: dy [-4, 4) dx [-4, 52); dy ±[4, 12) dx [4, 52); dy ±[12, 20) dx [20, 44);
  dy [-28, -20) dx [28, 36). Walls do not block it (none was in the measured boxes' way).
- **Spotted**: the guard runs at Snake, ALERT 999, then MISSION FAILED (TRY AGAIN / EXIT); the
  port shows a "!" for 60 frames, then its MISSION FAILED box.

## The port

- `mgs.c`: the level, Snake, the guard, vision, goal, camera, drawing (~330 lines); `mgs.h`
  the state (one POD struct, PC save/load). Data: `mgsdat` (51 tiles of 16 x 16, 56 Snake images
  = 8 directions x (stand + 6 walk images), 9 guard images; 14 KB, read in place from the archive).
- `tools/extract.py`: the ROM → `level.h`, `gfx.h`, `mgsdat.bin`/`.be.bin`, review sheets in
  `x/`. Greys per CGB colour (`BG_GREY`, `SPR_GREY`): floor light grey with white grid lines,
  shadows and wall tops dark grey, outside black; Snake dark, the guard light, both outlined.
- Tests (`test_mgs.c`, `make test`): walls, speeds, turn timing, diagonal, wall sliding, the
  guard's timeline against the ROM's, vision points on both sides of every measured edge (down,
  up, right, off screen), goal, alert and try again, the hidden scenario, `keys/win.txt` (wait
  for the guard's round, follow him up along the corridor's right wall into the side room, stay
  there while he comes down, then up behind him and into the goal: clear at frame 908; every
  move ends against a wall, so it also wins with its timeline scaled by 0.9 to 1.1) and
  `keys/spotted.txt`.
- **TI**: `mgs.89z` 9 KB + `mgsdat.89y` 14 KB. Under `ti-cycles`: ~3k cycles of logic and ~67k
  of drawing per frame (TileMap + 2 sprites + text), 19 % of the 360k budget. `xcheck` (4
  scenarios) and `tihash` (2 scripts x 400 frames) TI = PC. Run on the Titanium (TiEmu):
  spotted → MISSION FAILED, and `keys/win.txt` played with `ti-play` → VR LV.01 CLEAR
  (`x/mgs_vr1_clear.gif`, local), ESC back to HOME.
