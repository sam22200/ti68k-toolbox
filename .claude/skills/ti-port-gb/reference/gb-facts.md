# The Game Boy as a port sees it

What a TI-89 port of a DMG game depends on. Hardware numbers are the documented DMG ones
(Pan Docs, gbdev.io/pandocs); the tool facts were run here (2026-10-03).

## 1. Machine and timing

| | Game Boy (DMG) | TI-89 Titanium |
|---|---|---|
| CPU | SM83 (LR35902, Z80-like, 8-bit), 4.194 MHz | 68000, ~12 MHz, 16-bit bus |
| Frame | 70,224 cycles, 154 lines × 456, **59.73 Hz** | runtime 256 / `RT_FRAME_TICKS` Hz (~30) |
| Screen | 160 × 144, 4 shades | 160 × 100, 4 greys (GrayDBuf) |
| Sound | 4 channels | none |

- One SM83 "cycle" here is a T-state; instructions take 4 to 24. A game's whole frame of
  logic rarely uses more than a fraction of the 70k: the logic in C on the 68000 is usually
  cheap, the **rendering** is what costs on the TI.
- Logic rate: most games run their update once per VBlank (`halt`, then wait for a flag the
  VBlank handler sets). Some update every 2nd frame or spread work over frames: measure it
  in the trace (a position that moves every other frame).

## 2. Memory map

| Range | What | Port note |
|---|---|---|
| 0000-3FFF | ROM bank 0 (header at 0100-014F, `entry` at 0100, RST and IRQ vectors below) | code + data |
| 4000-7FFF | ROM bank n (MBC1/3/5 switch it by writing 2000-3FFF) | GhidraBoy: overlay blocks per bank |
| 8000-97FF | VRAM tile data: 384 tiles × 16 bytes | dumped decoded by `gbextract.py` |
| 9800-9BFF, 9C00-9FFF | two 32 × 32 tile maps (BG, window) | the level screen |
| A000-BFFF | cartridge RAM (battery = save) | `rt_save` |
| C000-DFFF | WRAM: **the game state** | traced, named in `<name>.sym` |
| FE00-FE9F | OAM: 40 sprites × (y, x, tile, flags) | built from a shadow copy |
| FF00-FF7F | I/O registers | see §3, §5 |
| FF80-FFFE | HRAM: fast variables, the OAM DMA routine, often the joypad state | traced too |
| FFFF | IE | |

- **OAM shadow**: games build the sprite table in WRAM (often C000-C09F) and copy it during
  VBlank with a DMA routine run from HRAM (write the page to FF46: Ghidra shows it as a
  function at `ff80`). The shadow holds *screen* positions (y + 16, x + 8): a trace of it is a
  check of the drawing, not of the game state.
- Joypad: FF00 is read twice (directions, buttons) and stored in HRAM: a "held" byte and a
  "newly pressed" byte (Bubble Ghost: ff8b, ff8c). A+B+Select+Start = soft reset in many games.

## 3. PPU (what the screen is made of)

- **LCDC (FF40)**: bit 7 LCD on, 6 window map (9C00 if 1), 5 window on, 4 tile data
  (1 = 8000 unsigned indices, 0 = 9000 signed: indices 0-127 at 9000, 128-255 at 8800),
  3 BG map (9C00 if 1), 2 sprite size (8 × 16 if 1), 1 sprites on, 0 BG on.
- BG: 256 × 256 wrapping, viewport at SCX/SCY. Window: an opaque layer at (WX - 7, WY) drawn
  over the BG, never scrolled (HUDs, dialogue boxes). Sprites: 40, 8 × 8 or 8 × 16, at most
  **10 per line**, flags bit 7 behind BG colours 1-3, 6 y-flip, 5 x-flip, 4 palette OBP1.
- **Tiles**: 2 bits per pixel, 16 bytes, each row = a low byte then a high byte (bit 7 = left
  pixel); colour index c = lo | hi << 1, shade = (palette >> 2c) & 3 (BGP for BG/window,
  OBP0/OBP1 for sprites; sprite colour 0 = transparent).
- **Shades map 1:1 onto the runtime's greys**: shade s (0 white … 3 black) = `C_WHITE`,
  `C_LGRAY`, `C_DGRAY`, `C_BLACK` (`rt.h`), i.e. light plane bit = s & 1, dark plane bit =
  s >> 1. After the palette lookup a GB tile row becomes the two ExtGraph planes with bit
  operations; a sprite's mask = its colour-0 pixels.
- **Raster effects**: STAT/LYC interrupts (writes to FF41/FF45) or HBlank code changing
  SCX/SCY/LCDC/BGP mid-frame: split status bars, parallax, wobble. The census in
  `info.json` counts those writes; each one is a port decision (redraw the effect, drop it).
- Sizes: the GB's 384 tiles = 6 KB of 2bpp, the same once split in two planes; a TI plane
  is `LCD_SIZE` = 3,840 bytes (240 × 128 bits), two per grey screen.

## 4. Cartridges

- ROM only (32 KB, no banking): the simplest case (Bubble Ghost, Tetris). MBC1/3/5: banked
  code and data; Ghidra needs the bank of each far call (the code writes the bank number to
  2000-3FFF, then calls into 4000-7FFF). Battery RAM = high scores or saves.
- Header: title 0134-0143, CGB flag 0143 (80 compatible, C0 CGB only), cartridge type 0147,
  ROM size 0148 (32 KB << n), RAM size 0149, header checksum 014D (verified by
  `gbextract.py`). CGB-only games use colour palettes, VRAM bank 1 and double speed: out of
  this skill's scope.

## 5. Interrupts, timer, sound

- Vectors 0040 VBlank, 0048 STAT, 0050 timer, 0058 serial, 0060 joypad. The timer (TMA, TAC)
  usually drives the music player at a fixed rate (Bubble Ghost: TMA = 0xBC, TAC = 4 → 4096 /
  (256 - 188) ≈ 60.2 Hz). Music and sound effects: dropped on the TI (no sound hardware); the
  writes to FF10-FF26 (or the game's sound request variable) mark events in the traces.
- Serial (link cable) multiplayer: the TI link exists (`ti68k-game-techniques.md`), a
  separate decision.

## 6. SM83 semantics the C must keep

- Registers are 8-bit (A, B, C, D, E, H, L) and pairs (BC, DE, HL, SP); all arithmetic
  wraps at 8 bits (16 for `add hl` / `inc rr`): use `u8` / `u16` variables, never `int`
  (GCC4TI's `int` is 16-bit, the PC's 32: same result only with explicit widths).
- Flags: Z, N, H (half carry, for DAA), C. Multi-byte arithmetic chains `add`/`adc` and
  `sub`/`sbc` through C: in C, a `u16` or `u32` sum with the carry taken from the bit above.
- `daa` after `add`/`sub` = **BCD** (scores, timers, lives displayed as digits): keep BCD in
  C (two digits per byte, a small `bcd_add`), the HUD reads it directly.
- Signed values exist only by convention (`jr` offsets, `bit 7` tests, `cpl; inc a`):
  `s8` in C where the game treats a byte as signed (velocities, offsets).
- Rotates (`rlca`, `rra`, `swap`) for pseudo-random generators and bit packing: keep them
  bit-exact, the trace checks them.
- Jump tables (`rst 00`/`jp hl` on a table of addresses indexed by a state byte): a `switch`
  in C. Ghidra misses the targets (they stay data): pass the tables to `gbdecomp.sh`
  (`TABLE:N`), Bubble Ghost's ghost, bubble and hazard handlers were all behind them.
- Code that relies on timing (busy loops waiting for LY, `halt`) becomes nothing: the frame
  loop of the runtime replaces it.

## 7. Tools (verified here)

- **Ghidra 11.4.2 + GhidraBoy 20250830** (the last GhidraBoy release targets 11.4.2; Ghidra
  12 has no matching build). Loader `GameBoyLoader`, language SM83. Headless import, analysis
  and export of 32 KB in ~13 s (`scripts/gbdecomp.sh`). JDK 25: Java post-scripts fail in the
  OSGi loader (`osgi.ee=UNKNOWN`); Jython post-scripts work.
- Decompiler quirks: every `call` shows the return address stored to RAM
  (`uRamfffc = 0x162;` = the push of the return address when the stack is at FFFE): ignore
  those lines; `CONCAT11(hi, lo)` = a register pair; `(&DAT_c0b4)[i]` = a table indexed by a
  byte; functions that the main loop re-enters (soft reset) appear inlined twice.
- **PyBoy 2.7** (`pip install pyboy`, SDL2 bundled): `PyBoy(rom, window='null',
  sound_emulated=False)`, `tick()` = one frame, `memory[addr]` read/write, `button_press` /
  `button_release` (held until released), `save_state` / `load_state` (file objects),
  `screen.image` (PIL). ~3,000 frames per second headless here (6,000 frames in 1.9 s with
  start-up). Trust it for game state; traces of timing-sensitive raster code
  are less trustworthy (check against a second emulator, e.g. SameBoy, when it matters).
  Verified 2026-10-03: inputs sent from inside a hook (`hook_register`) are dropped (queue
  them for the next `tick()`); a hook runs mid-frame while the PPU draws, so
  `screen.image` read there mixes new and old lines (read it after the tick); hooks on
  bank 0 addresses work as breakpoints for `--logic` and `--poke-at`.

## 8. Pitfalls

- A key pressed for exactly the frames the game ignores (title fades, transitions) does
  nothing: the boot script needs waits (Bubble Ghost: Start at 400, 600, 800, A at 1,000).
- Input is read once per frame in the VBlank or the loop: a 1-frame press can be missed if
  the game reads "newly pressed" on its own clock; key scripts hold keys ≥ 2 frames.
- States saved by one PyBoy version may not load in another: regenerate them from the boot
  script (keep the script, not only the state).
- Copyright: commercial ROMs, their tiles, maps and any trace containing their data stay out
  of git (`roms/`, `sources/`, generated `gfx.h`): the repository holds our code, the spec in
  our words and the scripts that regenerate the rest from a local ROM.
