# TI-68K Game Development Toolbox

Native games for the TI-89 / TI-89 Titanium (Motorola 68000), written in C with GCC4TI and
ExtGraph 2, NOSTUB. Roadmap: [`TI68K_Game_Development_Toolbox.pdf`](TI68K_Game_Development_Toolbox.pdf)
(1 portable runtime, 2 Open Flappy Bird port, 3 TI-BASIC → C, 4 keypad "touch" gestures,
5 Game Boy ROM → C, 6 Prince of Persia).

| Path | What |
|---|---|
| `runtime/` | **Portable Game Runtime**: one C engine, PC (SDL2) and TI backends, unit tests, PC/TI cross-check ([README](runtime/README.md)) |
| `games/` | `campfire/` (Chrono Trigger camp-fire scene, TileMap + ZX0), `puzzle_bobble/` (ported), `life/` (Game of Life on the runtime, glider), `flappy/` (Flappy Bird ported from sdlbird), `ffa/` (Final Fantasy Alternative remade from TI-Basic, in progress), `mode7/` (Mode 7 demo decompiled and optimised, [OPTIMISATIONS](games/mode7/OPTIMISATIONS.md)), `celeste/` (Celeste Classic ported from the PICO-8 cart, room 0, bit-exact with the cart), `mgs/` (Metal Gear Solid GBC, VR Sneaking Lv.01 on our own engine, [ROADMAP](games/mgs/ROADMAP.md)), `alundra/` (Alundra PS1's jumps, terrain heights and stairs in a test room and the whole village of Inoa drawn with the game's own image, scrolling, our own engine, art, image and map generated from the local disc) |
| `lib/` | shared code: ZX0/LZ4 decoders in 68000 asm, ZX0 packer |
| `experiments/` | small measured tests (hardware, timers, keyboard, graphics, benchmarks) |
| `hello/` | reference Hello World |
| `tools/bin/` | `ti-cc` (build), `ti-emu`/`ti-run`/`ti-send`/`ti-group`/`ti-key`/`ti-shot` (TiEmu in Docker, `Dockerfile.tiemu` + `tiemu-keyfix.c`), `ti-table`, `ti-view` (PNG/GIF gallery on the LAN, e.g. for an iPad), `zx0`, `ti-cycles` (cycle counter, `tools/m68kbench/`) |
| `.claude/skills/` | Claude Code skills and the knowledge base (`ti89-c-dev/reference/*.md`) |
| `CLAUDE.md` (`AGENTS.md` links to it) | project rules and workflow, for any coding agent; collaboration rules in `docs/working-rules.md` |

**Install**: [docs/INSTALL.md](docs/INSTALL.md) (Ubuntu / Linux, macOS).

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
- `tools/ghidra/`, `roms/`: Ghidra 11.4.2 with the GhidraBoy extension and the Game Boy ROMs
  for `ti-port-gb` (install steps in its `SKILL.md`; PyBoy goes into `tools/pyenv`), the TI-83
  programs for `ti-port-ti83` (`roms/ti83/`; the `z80` and `z80dis` packages go into `tools/pyenv`).
- Build outputs: `*.89z`, `*_pc`, `*_test`, …
