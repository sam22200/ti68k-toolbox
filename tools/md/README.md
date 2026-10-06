# Mega Drive reference tooling

`mdrun.py` drives a local Genesis Plus GX libretro core without a UI. It is a
reference instrument for TI ports, not an emulator to put on the calculator.
It follows the PS1 runner's approach: scripted input, saved states, RAM traces,
CPU-order RAM dumps, PNGs and GIFs. Pillow is needed only for images.

## Setup and integration check

From the repository root:

```sh
make -C tools/md core
make -C tools/md check
```

The core is pinned to `58c341487e5bfcf979ea68413c7987633adb0c56` and built in
`sources/md_core/` (ignored). The check requires the local Sonic REV00 ROM
at `roms/md/Sonic_1.md`. It boots to GHZ1, checks real movement and jumping,
replays a saved state twice with identical RAM and video, and checks that a
RAM poke affects the player. It also checks VDP snapshot sizes and the active
sprite table register. No calculator emulator is involved.

## Sonic's reproducible reference door

```sh
mkdir -p sources/sonic1_md/x
tools/pyenv/bin/python tools/md/mdrun.py roms/md/Sonic_1.md \
  --frames 1000 --keys games/sonic/keys/ref_boot.txt \
  --save sources/sonic1_md/start.state \
  --shot 999:sources/sonic1_md/x/start.png \
  --metadata sources/sonic1_md/boot.json

tools/pyenv/bin/python tools/md/mdrun.py roms/md/Sonic_1.md \
  --load sources/sonic1_md/start.state --frames 70 \
  --keys games/sonic/keys/ref_jump.txt \
  --trace ffd008:2,ffd00c:2,ffd010:s2,ffd012:s2,ffd014:s2,ffd022:1 \
  > sources/sonic1_md/x/jump.trace
```

Frame indices are zero-based: frame 0 is the first `retro_run()` after boot or
load, and samples are taken after that frame. A key script uses `<frame> <MD
buttons>`; a line without buttons releases them. Buttons: UP, DOWN, LEFT,
RIGHT, A, B, C, START. State load does not restore the frontend's held keys.

Trace addresses are hexadecimal CPU RAM addresses (`FF0000..FFFFFF`); sizes
are 1, 2, 4, s1, s2 or s4, and printed values are decimal. `--poke
ffd008:2=120` writes a big-endian word before the first frame. `--dump
19:path.bin` exports all 64 KiB in CPU byte order. `--shot F:path.png`,
`--gif path.gif --every N`, and `--metadata path.json` support inspection and
provenance. Output directories must exist. Metadata includes core/ROM hashes,
the reported frame rate and input/state paths; record pokes alongside the
experiment command.

The normal little-endian core build stores CPU byte `a` at `work_ram[a ^ 1]`.
The runner normalizes reads, writes and dumps. This is a Genesis Plus GX
implementation detail, not a generic libretro assumption. `make core` applies
the small idempotent `expose_vdp.py` patch to the pinned local source: the
upstream API does not expose these VDP arrays. `MD.vdp()` returns immutable
snapshots of CPU-order VRAM (64 KiB), core CRAM (128 bytes) and VDP registers
(32 bytes), via IDs 3, 0x10000 and 0x10001. CRAM is the core's packed internal
format; Sonic's CPU palette at FFFB00 is easier to interpret for extraction.
These read-only exports do not change emulation. Rebuild the core if the
runner reports unavailable VDP memory.

Upstream: [Genesis Plus GX](https://github.com/libretro/Genesis-Plus-GX),
[RAM macros](https://github.com/libretro/Genesis-Plus-GX/blob/58c341487e5bfcf979ea68413c7987633adb0c56/core/macros.h),
[libretro API](https://github.com/libretro/Genesis-Plus-GX/blob/58c341487e5bfcf979ea68413c7987633adb0c56/libretro/libretro.c).
All ROMs, reference states, images and extracted data stay local under ignored
`roms/` and `sources/`; only our tooling and notes belong in the repository.
