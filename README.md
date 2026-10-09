# TI-68K Game Development Toolbox

Native games for the TI-89 / TI-89 Titanium (Motorola 68000), written in C with GCC4TI and
ExtGraph 2, NOSTUB. Roadmap: [`TI68K_Game_Development_Toolbox.pdf`](TI68K_Game_Development_Toolbox.pdf)
(1 portable runtime, 2 Open Flappy Bird port, 3 TI-BASIC → C, 4 keypad "touch" gestures,
5 Game Boy ROM → C, 6 Prince of Persia).

| Path | What |
|---|---|
| `runtime/` | **Portable Game Runtime**: one C engine, PC (SDL2) and TI backends, unit tests, PC/TI cross-check ([README](runtime/README.md)) |
| `games/` | `campfire/` (Chrono Trigger camp-fire scene, TileMap + ZX0), `puzzle_bobble/` (ported), `life/` (Game of Life on the runtime, glider), `flappy/` (Flappy Bird ported from sdlbird), `ffa/` (Final Fantasy Alternative remade from TI-Basic, in progress), `mode7/` (Mode 7 demo decompiled and optimised, [OPTIMISATIONS](games/mode7/OPTIMISATIONS.md)), `celeste/` (Celeste Classic ported from the PICO-8 cart, room 0, bit-exact with the cart), `mgs/` (Metal Gear Solid GBC, VR Sneaking Lv.01 on our own engine, [ROADMAP](games/mgs/ROADMAP.md)), `alundra/` (Alundra PS1's jumps, terrain heights and stairs in a test room and the whole village of Inoa drawn with the game's own image and depth, roofs walked on, scrolling, our own engine, art, image and map generated from the local disc) |
| `lib/` | shared code: ZX0/LZ4 decoders in 68000 asm, ZX0 packer |
| `experiments/` | small measured tests (hardware, timers, keyboard, graphics, benchmarks) |
| `hello/` | reference Hello World |
| `tools/bin/` | `ti-cc` (build), `ti-emu`/`ti-run`/`ti-send`/`ti-group`/`ti-key`/`ti-shot` (TiEmu in Docker, `Dockerfile.tiemu` + `tiemu-keyfix.c`), `ti-table`, `ti-view` (PNG/GIF gallery on the LAN, e.g. for an iPad), `zx0`, `ti-cycles` (cycle counter, `tools/m68kbench/`) |
| `.claude/skills/` | Claude Code skills and the knowledge base (`ti89-c-dev/reference/*.md`) |
| `CLAUDE.md` (`AGENTS.md` links to it) | project rules and workflow, for any coding agent; collaboration rules in `docs/working-rules.md` |

**Install**: [docs/INSTALL.md](docs/INSTALL.md) (Ubuntu / Linux, macOS).

[Sonic 1 traversal prototype](games/sonic/README.md): the first 4.8 Mega Drive
view widths of Green Hill, half scale, C physics and real collision terrain,
rigid bridge, rings, three enemy types and damage, with PC/TI checks.
ROM scenery and animated actors in four greys, with white outlines. The original
ROM runs headless through [the MD tooling](tools/md/README.md).
The reusable method is [ti-port-md](.claude/skills/ti-port-md/SKILL.md).

[Yoshi's Island interaction prototype](games/yoshi/README.md): a playable traversal
of five original level 1-1 view widths at half scale, using actual collision
terrain. Movement and collision probes match the PAL original on PC/TI;
the full native replay is cross-checked and profiled per frame. Tongue, Shy Guy
capture/swallow/spit and an egg reserve work. ROM scenery and animated
Yoshi/Baby Mario, Shy Guys and eggs use four greys and white outlines;
contact damage, the crying Baby Mario bubble, rescue and a ten-second
countdown work. Coins rotate and collect, eggs follow the player and can be
aimed/thrown; resting and tongue-capture animations are present. The reusable method is
[ti-port-snes](.claude/skills/ti-port-snes/SKILL.md).

[Mega Man X opening section](games/megamanx/README.md): a second SNES
experiment, with running, short/held jumps, wall slide/kick, three buster
strengths, roller combat and contact damage. Original scenery and animated
actors are reduced to four greys with white outlines. Source mechanics,
complete PC/TI states/screens and individual frame costs are checked;
reusable ROM layout and animation-anchor findings extend `ti-port-snes`.

[GBA porting skill](.claude/skills/ti-port-gba/SKILL.md): the same measured
approach for Game Boy Advance ROMs, using a pinned headless mGBA core.
[Reference tooling](tools/gba/README.md) checks cold boot, native inputs,
RAM/video exports, cartridge save restoration and per-frame replay on the
three local Advance Wars, Final Fantasy Tactics Advance and The Minish Cap
ROMs. These validate instrumentation; game-specific mechanics and native
playable milestones follow the user's chosen scope.
[Minish Woods](games/minish/README.md) extracts nine ROM data
streams without gameplay and directly loads the original forest from cold
boot with an explicit study save. Walking, 56,816 decoded bytes against loaded
RAM and three 120-frame forest replays pass. Its first native traversal walks
the opening at provisional 1:1 scale, with real partial terrain collisions,
corner slides, slopes and a tighter camera. M2 adds original scenery, 44
outlined Link poses and canopy occlusion: 6310 original walking steps and
400 displayed animation poses match; source art matches 16.8M RGB pixels.
3606 PC/TI state hashes and 56 screens pass; peak frame cost is under 297k
cycles. M3 adds ordinary sword swings, 40 attack poses and cutting all 53 bush
cells with changed scenery/collision: 1947 source steps and 672k additional
oracle pixels per build pass. M3 PC/TI checks cover 4639/4675 hashes and
216/252 screens at 100%/70%, with peaks under 327k/332k cycles.
An additional `minishz` build shows scenery and Link at 70% scale, preserving
source mechanics: 1.312M oracle pixels, 3642 PC/TI state hashes and 92 screens
pass; peak frame cost is under 307k cycles. The original-scale build remains.
M4 adds the two opening Octoroks, ordinary shots, sword kills, contact/shot
damage, recoil, hearts and retry in both views. Twenty original trials check
608 targeted updates; twenty enemy poses add 960k oracle pixels per scale.
Current PC/TI checks cover 7521/7557 hashes and 346/382 screens, with peaks
under 360k/349k cycles. Hearts keep a one-pixel white outline, and nearby
targeting with larger round balls makes ordinary shots visible.
AI choices and projectile impact/expiry are native
adaptations; original bouncing deflections, drops and later actions remain.
M5 adds Link's original roll (330 source updates), leaf bursts, readable cut
earth, fading Octorok deaths, the original knockback poses and a four-phase
damage flash: 2.35M oracle pixels per scale; 10002/10038 PC/TI hashes and
715/751 screens, peaks under 359k/345k cycles; ran in TiEmu.

[Neo Geo porting skill](.claude/skills/ti-port-neogeo/SKILL.md): the same
measured approach for MVS/AES cartridges, using local Windjammers files as
the first preparation case. [Reference preparation](tools/neogeo/README.md)
verifies the eleven chips and exports canonical archive/program inputs.
The pinned FBNeo headless runner passes original boot, native input,
CPU-bus and 120-frame deterministic replay checks with the local BIOS.
The first-service door, both characters' walking and ordinary disc trajectories
pass 1882 reference replay frames. A separate 1390-frame rules check validates
exact wall contact positions, neutral contact boxes/catch classification,
Beach points and both losing players' next service.
[Windjammers native prototype](games/windjammers/README.md) now provides a
playable Beach training court with outlined animated ROM actors and measured
ordinary captures/recoil (660 complete action steps), automatic possession
releases and delayed return strength (4058 additional original fixture steps).
Timed stationary lifts, complete charging/recapture and powerful immediate
returns add 6704 equality steps across 78 original input trials, including
high-speed captures. Lobs, charged character specials, airborne rebounds and
counter-return windows add 8573 bounded fixture steps across 62 twice-replayed
trials. Normal lob targets condition the native jitter; full RNG and goal
celebrations remain adapted. Directional arc throws, curved-wall transitions
and immediate/settled returns add 17764 equality steps across 180 twice-replayed
trials. Dash movement adds 816 motion steps from 48 twice-replayed trials.
Larger outlined discs, fixed lob targets and explicit charge cues improve
action readability. Powerful throws and charged specials have distinct fading
flight trails, with sparks for specials. Twenty-nine scenarios pass 7380
PC/TI state-hash samples, 30 final screens and six active-effect screen checks,
including the central
30-second countdown reaching zero. Compact two-digit scores, centered numbered
3/5/3 bands and an outlined net without shadow use the full LCD, on plain white
sand without decorative marks.
Scenery decoding matches 41,344 original RGB scene pixels. Original actor decoding
matches 1,329,378 RGB pixels across 1256 scenes, with TI frame costs below
360k cycles. Moving/action-pose contacts, rear flight and selected remaining
scenery follow.

## Quick start (runtime game)

```sh
cd runtime/demo
make test            # unit tests on the PC, no window
make pc && ./demo_pc # play on the PC (arrows, 2nd = Ctrl/Space, ESC)
make ti              # demo.89z for the calculator
../../tools/bin/ti-run demo.89z   # clean TiEmu restart, file sent at boot, runs demo()
```

In TiEmu, play with the PC keyboard (TiEmu window focused): arrows, **AltGr = 2nd**, Shift =
shift, **Right Ctrl = alpha**, Left Ctrl = ◆, Enter, Esc. The Docker image preloads
`tools/tiemu-keyfix.c`, which translates the host's keycodes for TiEmu 3.04 and releases keys
TiEmu missed (no more stuck keys); `TI_KEYFIX=0 ti-emu start` runs TiEmu without it.

## Not in the repository (local setup)

Third-party, copyrighted or generated files are kept out of git (`.gitignore`):

- `tools/gcc4ti-bin/`: GCC4TI, built in Docker by `tools/build-gcc4ti.sh` (sources in `tools/gcc4ti/`,
  `tools/tarballs/`).
- `tools/extgraph/`: ExtGraph 2 (LGPL), with `lib/extgraph.a`, `tilemap.a` and headers.
- `tools/rom/`: TI OS images (TI-89 AMS 2.09, Titanium AMS 3.10, AMSpatch); `tools/tiemu/`:
  TiEmu profiles and saved states built from them; `tools/patches/`: HW3Patch.
- `tools/sdl2/`: SDL2 2.30 headers extracted from `libsdl2-dev` (`apt-get download` + `dpkg -x`;
  the library is the system's `libSDL2-2.0.so.0`), plus `lib/libSDL2.so` symlink.
- `tools/pyenv/`: Python venv with numpy, scipy, pillow.
- `tools/bin/zx0`, `tools/bin/dzx0`: ZX0 v2 host packer/unpacker, built from
  [einar-saukas/ZX0](https://github.com/einar-saukas/ZX0) (`gcc -O2 -o zx0 src/zx0.c src/optimize.c src/compress.c src/memory.c`,
  `gcc -O2 -o dzx0 src/dzx0.c`).
- `tools/musashi/`: the Musashi 68000 core (MIT), `git clone https://github.com/kstenerud/Musashi tools/musashi`;
  `make -C tools/m68kbench` then builds `tools/bin/ti-cycles`.
- `runtime/platform-sw/amsfont.h`: AMS fonts, extracted from `tools/rom/` by `make`.
- `sources/`, `ffa_en/`: third-party reference sources and TI-Basic programs.
- `sources/md_core/`, `sources/sonic1_md/`, `roms/md/`: pinned Genesis Plus GX
  reference core, original Sonic ROM, states and measurements. Sonic terrain,
  collision profiles and graphics generated into `games/sonic/sonterr*` / `sonart*`
  remain ignored, as do reference headers and captures.
- `sources/snes_core/`, `sources/yoshi_snes/`, `sources/yoshi_disasm/`,
  `roms/snes/`: pinned Snes9x, local original traces/states/PPU captures,
  reading aids and SNES ROMs. `sources/megamanx_snes/` holds the second
  experiment; both games' generated trace fixtures and art remain ignored.
- `sources/gba_core/`, `sources/gba_checks/`, `roms/gba/`: pinned mGBA,
  local GBA ROMs and headless reference states, memory/video captures and
  measurements. Extracted assets and source-trace fixtures remain ignored.
- `roms/neogeo/`, `sources/neogeo_core/`, `sources/windjammers_neogeo/`,
  `sources/neogeo_reference/`:
  local cartridge/BIOS files, prepared Windjammers inputs and third-party
  driver sources used as reading aids. No original ROM data is distributed.
- `tools/ghidra/`, `roms/`: Ghidra 11.4.2 with the GhidraBoy extension and the Game Boy ROMs
  for `ti-port-gb` (install steps in its `SKILL.md`; PyBoy goes into `tools/pyenv`), the TI-83
  programs for `ti-port-ti83` (`roms/ti83/`; the `z80` and `z80dis` packages go into `tools/pyenv`).
- Build outputs: `*.89z`, `*_pc`, `*_test`, …
