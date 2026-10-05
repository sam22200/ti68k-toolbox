---
name: ti-port-ps1
description: "Study a PlayStation 1 (PS1 / PSX) game from its disc image and rebuild a small, TI-89-sized interpretation of one of its mechanics through the Portable Game Runtime (first target: Alundra's movement, jumps and terrain heights in one room): read the .cue/.bin (ISO 9660, SYSTEM.CNF, PS-X EXE header), decompile the executable or a RAM dump with Ghidra's built-in MIPS processor (headless), run the disc headless on the PC (pcsx_rearmed libretro core driven from Python, HLE BIOS, no Sony BIOS) for RAM traces, RAM diffs, save states and screenshots, grill the user on the experiment first (/grilling), keep RE_NOTES.md with every finding labelled OBSERVED / INTERPRETATION / TARGET, derive a behavioural model (never translate MIPS functions), then build it in milestones: simulation on the PC with unit tests, a purpose-built test room, 16x16 presentation, TI under ti-cycles, the emulator last. Use it whenever the user wants to port, remake, study, reverse-engineer, decompile or \"porter\" a PlayStation / PS1 / PSX game or a mechanic from one for the calculator, mentions a .cue/.bin/.iso PS1 image, roms/ps1/, SLUS/SCUS/SCES executables, PCSX, DuckStation or Alundra, even without saying \"PlayStation\"."
---

# ti-port-ps1 (study a PS1 game, rebuild one of its mechanics on the TI-89)

Same spirit as `ti-port-gb`'s big-game track (read `ti-port-gb/reference/big-game.md` once:
measure on the running original, our own engine, a roadmap of small milestones), pushed one
step further. **A PS1 game is never ported**: 33 MHz MIPS, 2 MB RAM, a GPU drawing textured
polygons, hundreds of MB of data on the disc, against a 12 MHz 68000 and 160×100 in 4 greys.
What we take from it is **behaviour and design**: how a mechanic feels and which rules make it
work, measured on the running game, then rebuilt as the smallest system that gives the same
play on the TI. The original guides the design; it never dictates the architecture.

1. **A disc, not a ROM.** `scripts/psxiso.py` reads the `.cue/.bin` (MODE2/2352), lists the
   ISO 9660 files, extracts the boot executable named by `SYSTEM.CNF` and decodes its PS-X EXE
   header (load address, entry). Room data, graphics and often code overlays sit in big
   archive files (`DATA/DATAS.BIN`): leave them alone unless a milestone needs them.
2. **Ghidra reads MIPS out of the box** (no extension): `scripts/psxdecomp.sh` decompiles the
   executable, or a 2 MB RAM dump taken in play (code loaded from the disc at run time is
   only there). A reading aid to answer precise questions, never a source to translate.
3. **The game runs headless on the PC.** `scripts/psxrun.py` drives the pcsx_rearmed libretro
   core from Python with its HLE BIOS (no Sony BIOS): ~2,000 frames per second, key scripts,
   RAM traces per frame, RAM dumps, save states (the injection door), PNG/GIF.
   `scripts/ramdiff.py` finds variables by comparing dumps, `scripts/psxgrid.py` measures a
   rule on a grid of teleported positions (floor heights, walls), `scripts/psxexplore.py`
   walks a room by breadth-first search over real moves (save states as nodes) to reach a
   target or find the exits. `--memcard` loads a memory card saved by hand in any emulator.
   The running game is the oracle.
4. **Behaviour, not code.** Every finding goes into `RE_NOTES.md` under one of three labels:
   **OBSERVED IN <GAME>** (measured or read), **LIKELY INTERPRETATION** (what it means),
   **TARGET IMPLEMENTATION** (what we build, and why it may differ). Original numbers are
   recorded, then adapted for the TI (frame rate, tile size, readability) with the reason.
5. **Simplest model that plays the same.** Discrete heights instead of continuous, a jump
   lookup table instead of a parabola, rectangles instead of collision volumes, Y + Z sort
   instead of the PS1 ordering tables, our own coordinates. Whenever reproducing the original
   and a simple robust TI design conflict, the TI design wins.

Constraints (`CLAUDE.md`): **1. performance**, **2. visibility** (white outline on the main
sprites), Titanium only, **ASM only after asking**. Answer the user in French, write files in
English.

References (read when the step says so):
- `reference/ps1-facts.md`: the machine as a study sees it (memory map, frame and logic rates,
  pad, disc layout, overlays, MIPS decompilation pitfalls), the tools and their traps.
- `reference/behaviour-study.md`: the method: `RE_NOTES.md` layout, the three labels, how to
  find and measure a mechanic with the tools, stop rules, what not to study.
- `reference/grilling.md`: the questions to put to the user before any study (defaults).

## 0. Tools (once per machine)

- Ghidra in `tools/ghidra` (`ti-port-gb` §0; MIPS is built in, JDK ≥ 21; post-scripts are
  Jython because Ghidra 11.4 refuses Java scripts under JDK 25).
- The emulator core (local, not in git), built in about a minute:
  ```sh
  git clone --depth 1 https://github.com/libretro/pcsx_rearmed.git tools/pcsx_rearmed
  make -C tools/pcsx_rearmed -f Makefile.libretro -j20     # -> pcsx_rearmed_libretro.so
  ```
  Then expose the VRAM (libretro's `RETRO_MEMORY_VIDEO_RAM`, absent upstream) with our patch,
  needed by `psxrun.py --vram` for the art:
  ```sh
  git -C tools/pcsx_rearmed apply ../../.claude/skills/ti-port-ps1/scripts/pcsx_vram.patch
  make -C tools/pcsx_rearmed -f Makefile.libretro -j20
  ```
  `psxrun.py` loads it with ctypes from `tools/pyenv` (Pillow for the pictures). No BIOS file:
  the core's HLE BIOS boots the disc (Alundra: verified to the first playable scene).
- Discs live in `roms/ps1/<game>/` (not in git: commercial). Everything generated from a disc
  (extracted files, decompiled code, dumps, states, screenshots) goes in `sources/<name>_ps1/`
  (not in git).

## 1. The disc (facts only, no study yet)

```sh
P=tools/pyenv/bin/python; S=.claude/skills/ti-port-ps1/scripts
$P $S/psxiso.py roms/ps1/<game>/<game>.cue sources/<name>_ps1/disc   # info.json, files.txt, boot exe
$S/psxdecomp.sh sources/<name>_ps1/disc/info.json sources/<name>_ps1/ghidra_out   # ~1 min
$P $S/psxrun.py roms/ps1/<game>/<game>.cue --frames 2700 --keys boot.txt \
    --shot 590:a.png,1190:b.png,2690:c.png --save play.state      # boot to the first control
```
- Check: the executable decompiles (function count ≈ the number of `jr ra`, 0 failed); a key
  script reaches a scene where the player moves; `play.state` reloads into it (`--load`).
- Write the boot key script and the frames it needs in the game's `RE_NOTES.md` § Tools: the
  injection door of the reference is that script plus saved states made from it (regenerate
  them with the script; never keep a hand-made state whose origin is unknown).
- Take a few screenshots of candidate places for the mechanic (rooms with heights, ledges):
  they are data for the grilling.

## 2. Grill (before any study)

`/grilling` with `reference/grilling.md`: what the experiment is for, which mechanic, how
faithful, the target controls, heights, collision resolution, sprite size, scrolling, the test
room, and **where the art comes from** (the disc's own graphics converted, or placeholders /
our own drawing: ask it explicitly, Alundra's user wanted the disc's art although the engine is
not a port). Questions are technical and specific to the mechanic, each with a default; challenge any
request that grows the scope and propose the smallest useful alternative. Record the agreed
constraints in `games/<name>/RE_NOTES.md` § Decisions (and a short summary to the user), then
go on. The grilling must not grow the project.

## 3. Study the original (static + dynamic, only what the mechanic needs)

Per `reference/behaviour-study.md`:
- **Find the player's variables by experiment**: dumps standing vs moving (`--dump`,
  `ramdiff.py`), then a trace (`--trace ADDR:s2,...`) while a key script moves, jumps, climbs,
  falls. Objects are structs: once x is found, y, z, velocity and state sit next to it.
  **The first struct found is often a copy** (display side, rewritten every frame): search the
  RAM for the same value, poke each copy, keep the one that makes the player move. Teleport
  with x, y **and z** (z set high: the player falls onto the floor there).
- **Maps of a rule**: `psxgrid.py` reloads the state per point, pokes the position, optionally
  holds a key, and prints ASCII maps (floor height, where a move ends).
- **Read the code that writes them**: find the stores to those addresses in `disasm.s`
  (`sh`/`sw` with the struct base in a register: search the offset), or decompile a RAM dump
  taken in play when the code is an overlay. Read to answer one question at a time (when does
  a landing happen? what blocks a ledge?), then write the answer, not the code.
- **Measure, do not derive**: jump duration and apex, horizontal reach, speeds, the height a
  jump clears, what happens against a wall, on landing, when falling, all from traces.
- **Stop** when the next milestone's questions are answered. Any subsystem that grows (combat,
  AI, scripts, menus, sound, cut-scenes): ask "do I need this for the mechanic?" and drop it.

Deliverable: `RE_NOTES.md` § Behaviour, a concise description of the mechanic (OBSERVED,
INTERPRETATION, TARGET per point), plus the original numbers in a table with how each was
measured (the psxrun command).

## 4. The model, then milestones on the runtime

- Write the model in `games/<name>/README.md` (structs, units, per-frame order, the jump
  table, the height and collision rules) and a `ROADMAP.md` of small milestones, each one
  playable and tested on the PC before the next (Alundra's are in `reference/grilling.md`
  § Milestones). Build with `include ../../runtime/rt.mk` like the other games.
- Engine first, presentation last: debug rendering (rectangles, outlines, height numbers, a
  text overlay of x, y, z, ground, vz, state, tile) until the mechanic is right, then the
  16×16 presentation.
- **Art from the disc** (when decided): dump the VRAM in play (`psxrun.py --vram F:vram.bin`),
  find each sprite's texture page, CLUT and UV in the GPU packets the game builds in RAM (or
  by eye on the VRAM PNG), decode 4/8-bit textures with their CLUT, convert to 4 greys, fit to
  the target size, with a white outline; a build tool regenerates it from the local disc
  (`games/<name>/tools/extract.py`), output gitignored, as `ti-port-gb`'s big-game track.
  Recipe that worked (Alundra, `games/alundra/tools/extract.py`): play key sequences from a
  state (walk each direction, stop, jump) and, **every frame, read the character's packets
  from RAM** (textured quads GP0 0x2C-0x2F filtered by its CLUT; the tag word before gives
  the length): each new set of parts is a pose, with its layout (characters are often
  several overlapping quads) and its timing (frames per image). Compose from the VRAM dump,
  anchor at the feet (bottom centre of the lowest part). Scaling ~0.5: map the 16 palette
  entries to 4 greys **by hand** (by material: hair, skin, cloth, outline) then take a vote
  per target pixel over its source area; averaging luminance gives mush. Scenery: 16 × 16
  cells of a clean rendered frame (`psx.image()`), two greys by a luminance percentile,
  grey pairs chosen per surface in the game.
- **A purpose-built test room**, designed to exercise every rule (open ground, each height,
  a reachable ledge, an unreachable one, a drop, corners, narrow passages), not an imported
  original room. An approximation of an original room is an optional last milestone:
  read the game's height map from RAM (its cell size, height unit and wall flags found in
  the floor code), resample a screen-sized window to the target tiles in the build tool
  (the most common cell per tile; Alundra: `games/alundra/tools/extract.py` `room_levels`),
  and expect to compress: real rooms are rarely built like a test room.
- **Render budget from the first milestone**: a room of raised blocks drawn as rectangles
  every frame cost 381k cycles on the TI (Alundra milestone 3, over the ~360k budget); the room
  is static, so compose it once into a background (`rt_light`/`rt_dark` pointed at two
  `RT_PSIZE` buffers, as `games/desolate`), copy it each frame (u32 unrolled), draw the
  player, then redraw only the tiles in front of the player's row that overlap it: 68k, same
  pixels (xcheck checksums unchanged).
- Tests (`make test`): unit tests on each rule (`sw_step` + asserts: a jump's z per frame, a
  landing on each height, blocked by a ledge one level too high, a drop, walls on both sides of
  every edge, corners, collision during ascent and descent), key scripts that walk the test
  room (one winning route over every ledge, one that fails a too-high ledge), the injection
  door (`game_scenario(n)`: the player placed at each test spot).

## 5. PC, then TI without UI, then the emulator once

As `ti-port-gb` §5-8 and the big-game track: `make pc` (SDL, feel check side by side with the
original running in a windowed emulator if needed), `make cycles` under `ti-cycles`,
`make xcheck` (TI = PC per scenario), budget ~360k cycles per 30 fps frame with grayscale; one
review PNG per milestone; the emulator once at the end (`ti-run`, one screenshot or printed
numbers, a demo GIF with `ti-play` + `ti-gif`; the route made of wall stops, not timed to the
frame). No comparison against the PS1's screens: the presentation differs by design.

## 6. Conclude, knowledge, commit

Report (French): what was learnt from the original (the behaviour summary), what the target
does differently and why, tests, cycles per frame, `.89z` size, limits. Update
`games/<name>/README.md`, `RE_NOTES.md`, the root `README.md` and `CLAUDE.md` layout, verified
platform facts in the knowledge base, a lesson below, then `/ti-commit` (our code and notes
only: nothing extracted from the disc).

## Lessons

- **Alundra** (`roms/ps1/Alundra (USA) (Rev 1)/`, SLUS_005.53, Matrix Software / Working
  Designs 1997; first test of this skill, tools verified 2026-10-04): one MODE2/2352 track,
  19 files, the boot executable 1.2 MB loaded at 0x80020000 (entry 0x80036044) of which only
  ~130 KB is code (Ghidra: 547 functions, 0 failed, 43 s); `DATA/DATAS.BIN` (104 MB) holds the
  rest. The HLE BIOS boots it; headless at ~2,000 frames/s: START at 600, 900, 1200 (logos,
  title, START), CROSS at 1500-2400 (intro text) reaches the ship prologue at frame ~2700;
  a state saved there reloads.
  First study (same day, ~1 h): the player's physics object at `801ac7e0` (x, y, z 16.16, vz)
  found after a display copy at `80133990` that ignored pokes; the jump read off one trace
  (vz 5.0, gravity 0.5, apex 27.5 px, 60 Hz logic, full air control); the floor map on an 8-px
  grid by teleporting (heights in multiples of 16, ramps, blocks not aligned to the 16-px
  graphic tiles). The ship's deck was a poor place for ledge tests: railings and cabins are
  walls separate from the height map and hid the ledge rule. Pick a place with plain steps.
  Advancing the story headless is the slow part: SQUARE talks, any other button closes a
  text box, but the prologue's next door was out of reach of every walk the search tried
  (5,000 positions). Teleporting by pokes froze the player near walls (stale collision
  cells). A save made by hand in a windowed emulator (`--memcard`) is the cheap way past
  story gates; automated play only for short, known sequences.
  **Reading the rule in the code won** (~30 min): the executable is only a loader, the field
  engine is an overlay; a RAM dump decompiled with every prologue as a seed
  (`psxdecomp.sh ram.bin OUT LO-HI`, 1,428 functions) showed the player as object 0 of a
  global array, so grepping the globals' names and then the struct offsets (`sw ...,0x138(`)
  led to the floor sampler, the vertical step and the wall test in four functions. Then
  **synthetic test geometry**: poking map cells (height, flags) next to the player verified
  the rule in five runs (walk/jump into +16 and +32), no need to find such a place in the game.
  Corners (milestone 5): the resolver's switch on the facing direction gave the corner
  rounding (one front corner blocked → a sideways nudge of ~1/3 the walk speed, both → stop),
  verified with one synthetic cell straddled by the box; target: a 4-bit corner mask and a
  1 px nudge, ~2k cycles per frame more on the TI.
  Art (milestone 6, ~1 h): Alundra's whole sheet sits in VRAM at page (320, 256), CLUT
  (192, 497); the scenery is 24 × 16 4-bit sprites in layers. 36 poses read from the packets in
  4 sequences, 0.48 scale to 20 px tall at the game's proportions (~11 wide), readable with
  the outline; extraction 24 s. Render 55k cycles per frame with sprites and textures.
  A memory card save got past the story gate: the user's `.gme` (DexDrive; `psxrun.py
  --memcard` keeps the last 128 KB of any header format) loaded through CONTINUE and the
  save-book room in ~5 runs; the house it starts in took `psxexplore.py --all` (every exit
  saved with a picture: the stairs came first, the door was on the floor below). Outdoor
  scenery (grass, retaining walls) reads far better in 4 greys than the ship's planks: take
  the scenery from the place that has the terrain the mechanic is about. The shadow matters
  to the user as much as the sprite: measure it (size, offset to the feet, in the air) and
  draw it as a one-grey darkening, the PS1's semi-transparency.
  **One scale for everything**: the user judged the speed against Alundra's body, not the
  tiles. Pick the sprite's scale (here 22 / 42 = 0.524) and scale every measured distance
  and speed by it (walk per axis, keeping the game's slower depth axis; jump apex; level
  height; corner nudge); "same tiles per second" on tiles drawn larger than the sprite felt
  1.4-2× too fast. A shadow must also stay readable on every floor: measure the greys under
  it and pick the drawing (two greys darker on light floors, black with a light ring on dark).
  Scale sprites into a wide canvas anchored at the feet, then cut each image to its own
  16-column window and keep the offset: the game's images sway around the feet, a fixed
  16-wide box clips them. Look for idle animations (Alundra breathes: 40/10/4 frames) by
  standing still a few hundred frames and logging the poses. Milestone 7: restore only the
  player's last rectangle per hidden plane (two alternate) instead of copying the room: render
  84k → 36k cycles; prove it with a frame-by-frame checksum test against the full copy.
  TiEmu needs an X display: a session without one (no `/tmp/.X11-unix/X0`) cannot do the
  emulator step; leave it for the user's desktop.
- **Original rooms are not test rooms** (Alundra milestone 8, 2026-10-04): the village and the
  ship have no +1 steps, only terraces 3-4 units apart joined by stairs (ramps); the +1 jump
  serves objects and puzzles. A 3-level engine without ramps can only compress them (terrace
  2, stairs 1, ground 0: the retaining wall stays unclimbable, the stairs become jumps): find
  that out from the map before promising a faithful room, and ask which compromise. Keep the
  highest level out of the screen's first rows (a level-2 tile in row 0-1 puts a 25-px player
  above the top of a 100-row screen).
- **A whole original area** (Alundra milestone 9, 2026-10-04): resample the game's height map
  in the build tool, then sort its values by how many tiles hold them (terraces) before
  mapping them to levels: the rare heights are objects or roofs. Height maps mix the solid
  and the walkable (Alundra: roofs and stairs are both slope cells; a stair joins two
  terraces, a roof does not). For the look, paste the game's screens into one image of the
  area (teleport over a grid, each screen placed by its camera = the player's position minus
  his feet on screen, read from the GPU packets) and classify or cut textures from it.
  **To look like the game, show the game's own image** (Alundra's village, 2026-10-05):
  textured blocks on a simplified map never did. Paste the screens (keep only those where the
  player's pose is the same in both GPU packet buffers and the camera has not moved for a few
  frames; move the other objects out of the map each frame: the NPCs vanish), take the
  median of several screens per pixel (the player vanishes), scale by the sprite's scale
  (area average), 4 greys by luminance percentiles, smooth only the noisy material (grass).
  Then the collision must use the game's true heights (a level = the game's unit × scale,
  kept exact in 1/16 px), or the image and the floor disagree on the high terraces. The
  image is big (656 × 494 × 2 planes: two 40 KB data files, read in place); the view is copied
  from it each frame shifted to any pixel (~150k cycles in C), and the image again over the
  player where tiles in front cover him (one span per tile column).
  A first version (milestone 9 before) drew the world once with the game's own tile drawing
  (`games/alundra/tools/bake.c`, `-DBAKE`), cut into 16 × 16 tiles for `draw_tilemap`: the
  same pixels, ~80k cycles for the view, but only for a world drawn from tiles.
- **Scale as a parameter**: keep every size and speed in one macro of the reference scale
  (`SC(v)`), computed in `long` (`int` is 16 bits on the TI: `128 * 22 * 42` overflowed and
  only the TI binary differed, caught by `make xcheck`).
