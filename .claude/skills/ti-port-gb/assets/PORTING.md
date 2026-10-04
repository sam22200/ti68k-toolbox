# Translating Bubble Ghost's routines (conventions)

The whole game is translated by hand, routine by routine, from `sources/bubble_ghost_gb/
ghidra_out/disasm.s` (the reference: exact instruction order, flags, registers) with
`decomp.c` as a reading aid. Names: `bubble_ghost.sym`. The C runs on the GB's own memory
(`gb.h`): every RAM variable stays at its GB address, so the traces compare memory.

## Memory

- `W8(a)` / `W16(a)` / `w16(a, v)`: WRAM C000-DFFF. `H8(a)`: HRAM FF80-FFFF. `R8(a)` /
  `R16(a)`: ROM (bank 0 and bank 1: 0000-7FFF, no MBC). `IO(a)`: the I/O registers.
- `rd(a)` / `wr(a, v)`: any address, for pointers held in RAM or registers (a script pointer
  into ROM, a VRAM destination). **Every VRAM write goes through `wr`** (the renderer tracks
  dirty BG cells and tiles); `copy(dst, src, n)` (0753/074C/243C) and `fill(dst, v, n)`
  (0747/073F) use `rd`/`wr`.
- Use `u8`/`u16` and explicit casts for every wrap (`(u8)(a + b)`); never `int` (16-bit on
  the TI). 16-bit register pairs = `u16`. `mul16` = 042B.

## Routines

- One C function per GB routine, named `r_XXXX` (its address), with the `.sym` name in a
  comment: `// 05AD load_hall`. Registers in = parameters (`u8 a`, `u16 hl`...), registers out
  = return value or `u16 *` out parameters **when a caller reads them after the call**
  (read the caller: Ghidra's C hides that; `blow_bubble` leaving E = 2 or 3 was a real bug).
- Flags as results: a routine ending `RET Z/NZ` on a test returns the value the caller tests.
- **Blocking code** (anything that waits for a VBlank: `CALL 0390`, `CALL 0387` n times,
  `CALL 04F6`, `HALT` loops): a protothread, `u8 r_XXXX(void)` returning 1 when finished, 0
  when it yields:
  ```c
  static Pt pt_1b9b;
  u8 r_1b9b(void)                  // 1B9B title_sequence
  {
      PT_BEGIN(pt_1b9b);
      r_0581();
      PT_CALL(pt_1b9b, wait_frames(10));     // 0387 with A = 10
      ...
      PT_END(pt_1b9b);
  }
  ```
  Locals that live across a yield (a loop counter, a pushed register) go in a `static` and
  are listed in `pt_reset_*()` (statics survive between runs on the TI). Arguments to a
  blocking routine: set statics before `PT_CALL`. Results: `gb_ret`.
- Helpers provided by `flow.c`: `u8 wait_frames(u8 n)` (0387: n VBlanks), `u8 wait_vbl(void)`
  (0390: one), `u8 wait_start(u8 n)` (04F6: up to n VBlanks, `gb_ret` = 1 if Start was pressed),
  `void read_keys(void)` (04A4, from the runtime's keys).
- **Not translated**: the sound driver (0850-12FF and anything writing FF10-FF26); keep the
  writes to the request bytes DFD8 (music) and DFDB (effects). The OAM DMA (FF80) is a no-op
  (the renderer reads the OAM shadow C000-C09F). LCD on/off (LCDC bit 7) and the interrupt
  enables: just store them.
- Comments: the GB address of each block when it is not obvious, nothing more.

## Checks

The traces (`make test`) compare WRAM and HRAM with the ROM under PyBoy at every logic frame;
the screens (`make screens`) compare pixels. A translation is done when both pass.
