# The PlayStation as a study sees it

What matters to measure a mechanic on a running PS1 game and read its code; not a hardware
manual. Verified facts carry a date; the rest is the standard PS1 documentation
(psx-spx, `https://psx-spx.consoledev.net/`).

## Machine

| | PS1 | TI-89 Titanium |
|---|---|---|
| CPU | MIPS R3000A, 33.87 MHz, 32-bit, + GTE (fixed-point 3D coprocessor, cop2) | 68000, ~12 MHz, 16-bit bus |
| RAM | 2 MB main at 0x80000000 (KSEG0; mirrors at 0x00000000 and 0xA0000000), 1 KB scratchpad at 0x1F800000 | ~180 KB free |
| Video | 1 MB VRAM (1024×512 × 16 bit), typical 320×240, 15-bit colour, textured polygons and sprites | 160×100, 4 greys |
| Frame | 59.94 Hz NTSC (50 PAL); **game logic often at 30 Hz or 20 Hz** | ~30 fps budget |
| Disc | 2× CD, ~650 MB, ISO 9660; XA audio, STR movies | — |

- **Game rate**: one `psxrun.py` frame = one displayed field (60 Hz). Find the logic rate
  first: a counter or a position that changes every frame, every 2nd or every 3rd. All
  measured timings are then given in logic frames *and* seconds, so they convert to the TI's
  rate.
- **Fixed point everywhere**: no FPU. Positions are often 16.16 or x.4/x.8 in 32-bit words, or
  16-bit world coordinates plus a sub-pixel byte; 2D games on the PS1 frequently keep 3D-style
  coordinates (x, z on the ground, y up, or x, y plus a separate height). Expect signed words.
- **Pad**: 16 buttons, active low on the hardware; games keep a decoded copy in RAM
  (pressed/held/edge words): a fine first trace target. `psxrun.py` names: UP DOWN LEFT RIGHT
  CROSS CIRCLE SQUARE TRIANGLE START SELECT L1 R1 L2 R2.

## VRAM (where the art is)

- 1024 × 512 pixels of 16 bit (1 MB), 15-bit colour with **red in the low bits**
  (r = v & 31, g = v >> 5 & 31, b = v >> 10 & 31; bit 15 = semi-transparency / mask).
- Framebuffers (display and draw) at the left (Alundra: two 320 × 240 at x 0, y 0 and y 256).
- Texture pages: 64 × 256 VRAM pixels each (x multiple of 64, y 0 or 256); textures are 4-bit
  (one VRAM pixel holds 4 texels: 256 texels wide per page), 8-bit (2 per pixel) or 15-bit.
  Seen as colour they look striped; decode with their palette.
- CLUTs (palettes): 16 or 256 entries of 15-bit colour, one row of VRAM each, usually packed
  in the bottom rows. A sprite's packet (GP0 0x64-0x7F, or polygons 0x24-0x3F) carries its
  CLUT position (x/16, y) and texture page; the game's ordering table in RAM holds them.
- The ordering-table packets stay in RAM after a frame (usually two buffers, double-buffered):
  sprites GP0 0x64-0x67 = tag, colour+cmd, xy, uv+CLUT, wh; textured quads 0x2C-0x2F = tag,
  colour+cmd, then 4 × (xy, uv) with the CLUT in the first uv word's high half and the
  texture page in the second's. Scan a RAM dump for those command bytes with a plausible tag
  length before them (sprites 3-4 words, quads 8-9): a character's packets share one CLUT.
- Read it with `psxrun.py --vram F:FILE.bin` (patched core, `scripts/pcsx_vram.patch`):
  Alundra on the ship: character sprites 4-bit from x 320, scenery tiles further right.

## Executables and overlays

- `SYSTEM.CNF`: `BOOT = cdrom:\SLUS_005.53;1` (the main executable), `TCB`, `EVENT`, `STACK`.
- PS-X EXE header (0x800 bytes): `pc0` 0x10, `gp0` 0x14, `t_addr` 0x18 (load address of the
  text, usually 0x80010000 or above), `t_size` 0x1C, stack 0x30/0x34, region string 0x4C. The
  text follows the header; `psxiso.py` writes it alone as `<EXE>.bin` for Ghidra.
- **The executable is often mostly data** (Alundra: 1.2 MB, ~130 KB of code), and part of the
  code can be **overlays** loaded from the disc into a fixed RAM area at run time (per mode:
  field, battle, menu). Code missing from `decomp.c` while the game runs it: dump RAM in that
  mode (`--dump F:ram.bin`) and decompile the dump (`psxdecomp.sh ram.bin OUT <addr>...`).
  Addresses of functions to start from: the return addresses (`jal` targets) seen in the
  executable's calls into that area, or a `jr` target found by hand.
- Other `.EXE` files on the disc (Alundra: `ALUN_CD.EXE`, `END.EXE`, `CLOSING.EXE`) are
  separate programs chained with `LoadExec` (often the ending, a demo): ignore them unless the
  mechanic lives there.

## Reading MIPS (Ghidra's output)

- **Branch delay slots**: the instruction after a branch or jump runs before the target. The
  pseudo-C handles it; the disassembly lists it after the branch: read both.
- **Load delay**: on the R3000 a loaded register is not ready for the next instruction;
  compilers put a `nop` or an unrelated instruction there. Harmless when reading.
- **Addresses are built in two halves**: `lui v0,0x800f` then `lw v1,0x1234(v0)` (or
  `addiu`, with sign extension: `lui 0x8010` + `-0x7ff0` = 0x800F8010). To find who uses
  RAM address A, search the low half (signed) in `disasm.s` near a `lui` of the high half;
  Ghidra's pseudo-C usually shows the full constant (`DAT_800f1234`).
- **gp-relative globals**: small globals are addressed from `gp` (`lw v0,-0x7e10(gp)`). The
  header's `gp0` may be 0; the startup code loads `gp` itself: find its value in the entry
  function and set it in Ghidra (register value on the whole range) for those to resolve.
- **Structs through a register**: object handlers take the object pointer in `a0`:
  `lh v0,0x14(a0)` = field at offset 0x14. Once a field's offset is known (from a RAM diff),
  grep `0x14(` with the struct register to find every reader and writer.
- **Jump tables**: `sll v0,v0,2` + `lw v0,TABLE(v0)` + `jr v0`: a state machine. The targets
  are data for Ghidra: pass them to `psxdecomp.sh` as extra addresses.
- **Function pointer tables** (object type → update handler) are common in action games: the
  player's update routine is usually found faster from its struct (who writes x?) than from
  the main loop.
- Libraries: the Psy-Q SDK (`libgpu`, `libgte`, `libcd`, `libetc`, `libapi`) is linked in;
  ignore its functions (GPU packet building, CD reads, pad reading); their recognisable
  shapes (`syscall`, writes to 0x1F801xxx) mark the boundary of the game code.

## Level data read straight from the disc (verified 2026-10-09 on Final Fantasy Tactics)

Some games keep their levels as plain data files in a format the modding community has
documented: read them with `psxiso.py --file` and a small parser, without running the game.
Check every field on the map at hand (a neighbour's height, a flag on a known wall) before
trusting the documentation. FFT (SCUS-94221): `MAP/MAPnnn.GNS` lists 20-byte resource
records (type at +4: 0x1701 texture, 0x2E01 primary mesh, 0x3001 alternative mesh, 0x3101
end; sector at +8, length at +12); the primary mesh's u32 at 0x68 points to the terrain:
x and z counts, then 2 levels of z x x tiles of 8 bytes (surface & 0x3F, -, height,
slope height & 0x1F | depth << 5, slope type = four 2-bit edges N S W E with N = +z and
E = +x, -, flags: bit 6 can't walk, auto camera). `MAP022` is Magic City Gariland (10 x 15).
The primary mesh itself (u32 at 0x40): four u16 counts (textured triangles, textured quads,
untextured triangles, quads), the vertices (s16 x, y, z: a tile is 28, a height unit 12, up
is -y), the textured polygons' normals, then 10 bytes per textured triangle (u, v, palette,
-, u, v, page, -, u, v) and 12 per quad; quads split ABC + BDC. Texture record 0x1701: 256 x
1024 at 4 bits (the page adds 256 to v); 16 palettes of 16 BGR555 colours at the u32 0x44,
colour 0 transparent. Verified by drawing Gariland in four views (`games/fft/tools/extract.py`).
Sources: FFHacktics, `github.com/adamrt/fft_toolkit` (`src/terrain.c`, `src/map.c`).

## The tools (verified 2026-10-04 on Alundra)

- `psxiso.py`: ISO 9660 from a raw MODE2/2352 track (user data at offset 24 of each sector),
  MODE1, or a 2048 `.iso`; first data track only. Form-2 (XA, STR) sectors are copied as 2048
  bytes: their content is truncated, fine for listing.
- `psxdecomp.sh`: Ghidra `BinaryLoader` at `t_addr`, `MIPS:LE:32:default`, the entry made a
  function by `psx_funcs.py` before analysis (a raw binary has no symbols). Alundra: 43 s,
  547 functions. A second run on the same `OUT` re-exports without reanalysis (rename in the
  GUI between runs).
- `psxrun.py` (pcsx_rearmed libretro, Lightrec dynarec, HLE BIOS):
  - ~2,000 frames per second headless (Alundra boot to the ship prologue, 2,700 frames:
    ~1.5 s).
  - The video callback passes NULL for a repeated frame: the script keeps the last real
    frame, so a `--shot` on such a frame still writes a picture.
  - The core prints to stdout; the script sends the core's fd 1 to `/dev/null` and writes the
    trace on a duplicate of the original stdout. Its stderr messages (mmap, Lightrec stats)
    are harmless: `2>/dev/null`.
  - `--load` runs one frame before unserializing (the core must be started); frame 0 of the
    key script is the first frame after the load.
  - RAM is `retro_get_memory_data(SYSTEM_RAM)`: the 2 MB main RAM, written live by `--poke`.
    No VRAM, no scratchpad, no breakpoints through libretro.
  - Determinism: the same disc, state and key script give the same trace (no wall clock in the
    core when frameskip and threads are off: the script forces both).
- `PSX_OPTS="pcsx_rearmed_drc=disabled"` (any core option) for a game that misbehaves under
  the dynarec; `--memcard` takes a raw card or a headered one (.gme, .vgs: the last 128 KB).
- `psxexplore.py --all`: every exit found is saved (`<out>_N.state/.png/.txt`) and the search
  goes on, for places with several exits (stairs inside a house before its door).
- For breakpoints and watchpoints (who writes this address?), when reading `disasm.s` is not
  enough: PCSX-Redux (GUI debugger, memory watch, Lua; needs a display) on the same disc and
  states recreated by the same key script. Not installed yet: install when first needed and
  note it here.
