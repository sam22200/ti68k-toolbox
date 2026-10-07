# SNES reference tooling

`snesrun.py` runs a pinned local Snes9x libretro core without a UI. It is a
reference instrument on the PC, not an emulator intended for the calculator.
Its first integration target is the local European Yoshi's Island ROM.
The second is Mega Man X USA: `make -C games/megamanx reference` verifies a
cold Highway boot and100 restored WRAM/PPU/video frames. Its SRAM export is
empty, so the check deliberately uses the regions actually present.

`ppu.py` is shared offline art decoding: planar2/4/8-bpp tiles, OAM size/flip/
palette and Mode1 background maps. It does not infer latched scroll/HDMA.
Mega Man X's drawn OAM uses pre-update object anchors; compare memory/video
timing before interpreting a post-frame snapshot as a canonical sprite.
See [the measured MMX notes](../../games/megamanx/RE_NOTES.md).

```sh
make -C tools/snes core
make -C tools/snes check
make -C games/yoshi measure
```

The core is [Snes9x](https://github.com/libretro/snes9x) revision
`fae2fea08f74180759ef540ee94259213f503480`, built under ignored
`sources/snes_core/`. Our small idempotent `expose_ppu.py` patch adds snapshot
exports, without changing emulation. Reference runs retain the core's recorded
defaults: automatic region, Super FX at 100%, compatibility timing, CPU
overclock disabled. No persistent SRAM file is imported on cold boot.

`check` verifies the exact local ROM hash, the documented boot/level door,
PAL frame rate, real input and cartridge-RAM pokes, snapshot sizes, and
identical WRAM, SRAM, PPU and RGB video on every frame of two state replays.
These boot/address checks are Yoshi-specific, not generic SNES assertions.

## Runner

From the repository root, after `make -C games/yoshi reference`:

```sh
tools/pyenv/bin/python tools/snes/snesrun.py \
  "roms/snes/Super Mario World 2 - Yoshi's Island (Europe) (En,Fr,De).sfc" \
  --load sources/yoshi_snes/start.state --frames 40 \
  --keys games/yoshi/keys/ref_jump.txt \
  --trace 70008c:2,700090:2,7000b4:s2,7000aa:s2 \
  --shot 39:sources/yoshi_snes/jump.png \
  --ppu 39:sources/yoshi_snes/jump_ppu \
  --metadata sources/yoshi_snes/jump.json
```

Key scripts contain `<frame> <buttons...>`, held until the next line. A line
without buttons releases them. Buttons: arrows, A, B, X, Y, L, R, SELECT,
START. They are original SNES buttons; B jumps in Yoshi, while runtime/TI
`K_A`/[2nd] is the proposed target jump button. Frames are zero-based, sampled
after `retro_run`; state load restores neither frontend held keys nor a fresh
video callback until the next run.

`--trace` accepts hexadecimal addresses and sizes 1/2/4, or s1/s2/s4 for signed
values. `--poke address:size=value` writes before the first sampled frame;
`--dump frame:path` exports 128 KiB WRAM. `--save`, `--gif`, `--every` and
comma-separated `--shot`/`--ppu` events match the MD runner's flow. Parent
directories for individual output files must exist. Metadata records core/ROM
hashes, options, input/state paths, pokes and sampling convention.

## Memory conventions

- WRAM `7E0000..7FFFFF`: contiguous bytes, little-endian words; no MD-style
  swapped-byte normalization. Low-bank WRAM mirrors are intentionally rejected.
- The runner's `700000` cartridge-RAM window is a linear view of the exported
  SRAM workspace, bounded by the core's reported size (Yoshi: 32 KiB). It is
  **not** a general SNES CPU-bus mapper. Establish the actual SRAM mapping for
  other cartridges before using these aliases.
- `SNES.ppu()` returns VRAM (65536 bytes), CGRAM (512 bytes of native uint16
  colors, little-endian on this Linux host), OAM (544 bytes), and raw register
  mirrors `2100..213F` (64 bytes). `--ppu` also writes the cartridge RAM.
  Core export IDs are 3, 0x10000, 0x10001, 0x10002; SRAM is ID0, WRAM ID2.
- Register mirrors are not complete latched PPU state: do not infer all
  scroll/window/HDMA state from them. The game's camera variables provide
  useful confirmed scroll values. Video/PPU snapshots alone do not prove
  hidden collision geometry.

All ROMs, third-party sources, states, extracted graphics, traces and images
remain local under ignored `roms/` and `sources/`. See
[the Yoshi notes](../../games/yoshi/RE_NOTES.md) for the tutorial injection,
measured units, tests and limits.
