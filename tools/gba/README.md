# GBA reference tooling

`gbarun.py` runs a local pinned [mGBA](https://github.com/mgba-emu/mgba) libretro
core without a UI. This is a PC reference instrument for native TI ports.
No GBA emulator runs on the calculator. The method is
[ti-port-gba](../../.claude/skills/ti-port-gba/SKILL.md).

```sh
make -C tools/gba core
make -C tools/gba check
# Or one chosen local ROM (path relative to tools/gba):
make -C tools/gba check ROM="../../roms/gba/Legend of Zelda, The - The Minish Cap (USA).gba"
```

The core is mGBA 0.10.5, revision `26b7884bc25a5933960f3cdcd98bac1ae14d42e2`,
built under ignored `sources/gba_core/`. It already exports memory maps;
no emulation patch is required. CMake, a C/C++ compiler and make build the
core. Python3 runs the frontend; Pillow in `tools/pyenv` is needed for images
and the RGB integration check. Only one active GBA instance is supported per
process because libretro stores global core state.

## Runner

From the repository root:

```sh
tools/pyenv/bin/python tools/gba/gbarun.py \
  "roms/gba/Legend of Zelda, The - The Minish Cap (USA).gba" \
  --frames 300 --shot 299:sources/gba_checks/boot.png \
  --dump 299:sources/gba_checks/boot \
  --save sources/gba_checks/boot.state \
  --metadata sources/gba_checks/boot.json

tools/pyenv/bin/python tools/gba/gbarun.py \
  "roms/gba/Legend of Zelda, The - The Minish Cap (USA).gba" \
  --load sources/gba_checks/boot.state --frames 120 \
  --trace 04000130:2 --dump 119:sources/gba_checks/replay
```

This example captures boot, not a gameplay door. Build a game-specific boot
script before studying mechanics. Key scripts contain `<frame> <buttons...>`
held until the next line; a line without buttons releases all. Buttons are
UP, DOWN, LEFT, RIGHT, A, B, L, R, SELECT, START. Source buttons are distinct
from the target's runtime/TI button mapping. Frames are zero-based, sampled
after `retro_run()`. Load clears held input and stale video; the next frame
gets the first post-load sample. There is no hidden frame advance on load.

`--trace` accepts hexadecimal addresses and sizes 1/2/4 or s1/s2/s4 for signed
values; output values are decimal. `--poke 02000000:2=123` edits canonical
EWRAM/IWRAM before frame 0. It is a raw memory edit, not a CPU-bus transaction.
`--dump F:directory` writes ewram, iwram, io, palette, vram, oam and savedata
as separate `.bin` files. `--shot F:path.png`, `--gif path.gif --every N`,
`--save` and `--metadata` support inspection and provenance. Output parents
are created automatically. Metadata records ROM/core hashes, options, BIOS,
input/state paths and hashes, pokes and the sampling convention.

## Memory, states and reproducibility

Canonical memory ranges and interpretation cautions are in
[the GBA reference](../../.claude/skills/ti-port-gba/reference/gba.md).
All regions are little-endian byte arrays, without MD-style byte swapping.
Mirrors, ROM bus reads and raw I/O/VRAM pokes are rejected. I/O is a register
mirror snapshot, not a complete account of raster DMA or latched video state.
Save size is queried dynamically; zero bytes is valid for a save-less game.

States use our `GBARUN1` container (20-byte little-endian header: magic,
metadata length, core-state length, save length; then JSON, core and save
bytes). They are not bare libretro or native mGBA save states. Loading checks
the ROM hash, core binary hash, options and BIOS choice. The pinned libretro
core serializes saves but intentionally does not reload them by default;
the frontend restores the companion cartridge bytes explicitly.

Cold boot imports no persistent cartridge save. External BIOS is disabled
explicitly; mGBA HLE BIOS is recorded. Frameskip is 0 and idle-loop removal is
disabled. Reported GBA video rate is approximately 59.73 Hz, including European
ROMs; country labels alone do not establish a PAL 50 Hz schedule. RTC is not
frozen; link, sensors and external BIOS selection are not exposed by this
frontend. Add validated support when needed by the chosen game.

`check` discovers local `.gba` ROMs or checks `ROM`. For each it repeats a
360-frame cold boot, checks 240×160 nonblank video and native keypad bits,
tests RAM endian/bounds and cartridge save restoration, and compares every
frame of three restored 120-frame input sequences, one in a fresh core.
Hashes cover EWRAM/IWRAM, raw I/O, VRAM, palette, OAM, cartridge saves and RGB.
The default boot's START pulses at 120/240 are instrumentation probes; the
check does not claim entry into gameplay or a game-specific movement rule.

ROMs, third-party core sources, states, captures and extracted assets remain
local under ignored `roms/` and `sources/`. Commit our tools and measured
notes only.
