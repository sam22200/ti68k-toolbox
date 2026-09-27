# 68000 assembly for the TI-89: when, how, and what the old sources teach

**Project rule: ASM only for a measured hot spot, and only after asking the user.** C first, then
tables, pointers, `muls16`, ExtGraph (`ti68k-performance.md`); ASM is the last step.
**Exception (user rule, 2026-09): common, reusable operations that have a verified asm version are
always used in that version, even in a C program**: link the `.s` and call it with explicit register
bindings (§2), or use an `asm()` block for a few instructions; no need to ask again, and never write
a C version instead. Verified asm routines to reuse:

| Routine | File | Gain over C | C prototype |
|---|---|---|---|
| LZ4 block decoder | `lib/unpack68k.s` (prototypes `lib/unpack68k.h`) | 1.5–1.6× | `u8 *lz4_asm(const u8 *s asm("%a0"), u16 slen asm("%d0"), u8 *d asm("%a1")) __attribute__((__regparm__(3)));` |
| ZX0 v2 decoder (used by `games/campfire`; packer `lib/zx0pack.py`, recipe in `ti68k-c-patterns.md` §10) | `lib/unpack68k.s` | 2.5× | `u8 *zx0_asm(const u8 *s asm("%a0"), u8 *d asm("%a1")) __attribute__((__regparm__(2)));` |
| `muls16` (16×16 → 32 multiply) | inline `asm()`, `ti68k-performance.md` §4 | avoids `__mulsi3` | |

Add each new verified asm routine to this table.

Old ASM sources studied: `sources/asm/` (SMA 0.38, Chrono Fantasy, BomberBoy, MegaCar, small demos; 1998–2001,
Fargo/DoorsOS kernel era, A68k syntax). Motorola manual: https://www.nxp.com/docs/en/reference-manual/MC68000UM.pdf

## 1. What it buys (verified)

`experiments/bench/bench3.c` + `m7row.s`, HW3: the Mode 7 row sampler (40 pixels, one table read,
add, shift, mask, texture read per pixel) costs **4,315 cycles in C (-Os) and 2,674 in hand-written
asm: 1.6× faster**, identical output (checked by the program). The gain comes from a tight `dbra`
loop, `(An)+` for both streams and indexed addressing `0(a2,d1.w)` for the texture read. Expect
1.3–2× on such inner loops, nothing on code dominated by ROM calls or ExtGraph routines (they are
already asm). Both versions contain the same `lsr.w #8`, which TiEmu undercounts (it ignores the 2 cycles
per shifted bit, performance §1): the real times are ~14 cycles per pixel higher for both, the gain
smaller in ratio (~1.4×).

Decompressors (`lib/unpack68k.s`, verified on the camp-fire data, game-techniques
§13): LZ4 1.5–1.6× and ZX0 2.5× faster than the same decoders in C (-Os). Bit-stream decoders gain
the most: GCC cannot keep the bit buffer in a register with the carry as the bit.

## 2. Recipes (GCC4TI)

**Separate `.s` file, GNU as syntax** (**verified**: builds, runs, correct result):

```asm
| m7row.s  —  comments start with '|'; registers are written %d0, %a0
	.text
	.even
	.globl m7row_asm
m7row_asm:
	move.l	%a2,-(%sp)          | a2 is callee-saved: save it
	lea	m7map,%a2           | a global C array, referenced by name
	...
0:	move.w	(%a1)+,%d1
	...
	dbra	%d2,0b              | local numeric labels: 0: / 0b (backward) / 0f (forward)
	move.l	(%sp)+,%a2
	rts
```
```c
// C side: bind every parameter to an explicit register, as ExtGraph's _R functions do,
// so the ABI does not depend on -mregparm or any other flag.
void m7row_asm(unsigned char *out asm("%a0"), const short *horz asm("%a1"),
               short cx asm("%d0"), short vrow asm("%d1")) __attribute__((__regparm__(4)));
```
Build: `ti-cc -o name main.c m7row.s` (tigcc assembles `.s` itself).

- **A68k syntax** (`.asm`, the syntax of all the old sources: `xdef`, `dc.b`, `d0` without `%`) is
  also accepted by tigcc (it calls its bundled `a68k`): compiled by the analysis, not run here.
- **Inline asm** for one instruction (**verified**, `muls16` in `ti68k-performance.md` §4):
  `asm("muls.w %2,%0" : "=d"(r) : "0"(a), "dmi"(b));` Constraints: `d` data register, `a` address
  register, `r` either, `m` memory, `i` immediate, `"0"` = same register as operand 0. Add
  `"memory"` to the clobbers if the asm writes memory the compiler can see.

**Register convention** (GCC m68k): **d0–d2 and a0–a1 are scratch** (the callee may destroy them;
d0 holds the return value); **d3–d7 and a2–a6 must be preserved** (save them with `movem.l` if you
use them). a7 is the stack pointer; a6 may be the frame pointer; with `OPTIMIZE_ROM_CALLS` a5 holds
the ROM-call table, never touch it. With `-mregparm=5` (our default) and no explicit bindings,
scalar arguments arrive in d0, d1, d2 in order and pointers in a0, a1 (checked in the caller's asm),
the rest on the stack. Use explicit `asm("%reg")` bindings anyway.

## 3. 68000 costs for hand optimisation (cycles, no wait states)

| Instruction | Cycles | Note |
|---|---|---|
| `moveq #n,Dn` (−128..127) | 4 | vs 12 for `move.l #n,Dn`: always moveq for small constants |
| `addq`/`subq #1..8` | 4 (reg, .w), 8 (.l) | cheaper than `add #imm` |
| `lea d16(An),Am` | 8 | address arithmetic without touching flags or data registers |
| `move.w (An)+,Dn` | 8 | pointer walk; `d16(An)` costs 12, `d8(An,Xn)` 14 |
| shift/rotate by n | 6 + 2n (.w), 8 + 2n (.l) | `lsr.w #8` = 22: a byte move is often cheaper |
| `mulu.w`/`muls.w` | 38–70 (TiEmu: 66) | the only multiply, 16×16→32 |
| `divu.w` | ~140 | quotient in the low word, remainder in the high word (`swap` to reach it) |
| `dbra Dn,label` | 10 taken, 14 at exit | the cheapest counted loop (16-bit counter) |
| `movem.l regs,(An)` | 8 + 8/register | moves 4 bytes per register: bulk copies and saves |
| `tst.w Dn` | 4 | same speed as `cmp.w #0`, one word shorter |
| `swap Dn` | 4 | exchange the 16-bit halves |
| `bra`/`bcc` taken | 10 | not taken 8 (`.s`) / 12 (`.w`) |
| `jsr`/`rts` | 16–20 / 16 | a call costs ~40 cycles with the stack traffic |

What GCC -Os already does from good C (no ASM needed): `dbra` for `short` count-down loops,
`moveq`, `addq`, pointer post-increment, `muls.w` for 16-bit products used once, reciprocal
multiplies for **unsigned** constant division (`n / 10` on an `unsigned short` compiles to
`mulu.w #52429` + `swap` + shift, and `n % 10` reuses it: no `divu` at all; verified). What only
hand ASM guarantees: keeping loop invariants in registers across a whole loop, `movem.l` bulk
moves, `0(An,Dn.w)` indexed reads in the right place, variable rotates (`rol.w Dn,Dm`) to merge
words, jump chains.

## 4. Idioms from the old sources

- **`movem.l` block moves** (SMA `tiles.asm`): `movem.l (a0)+,d0-d7` / `movem.l d0-d7,(a1)` moves
  32 bytes in two instructions; the basis of every fast screen copy/clear (ExtGraph uses it: use
  `FastCopyScreen_R` rather than writing your own).
- **Threaded dispatch** (SMA/Chrono sprite managers): each object stores the address of its current
  state routine; the scheduler `jsr (a0)`, and a routine can chain to the next with `jmp` instead of
  returning. In C the closest equivalent is an array of state function pointers (one indirect call
  per object), which is usually enough.
- **Many animation rates from one counter without division** (SMA): a rotating one-hot mask
  (`rol.l #1,d3`) or, in C, `if (!(frame & 1))`, `& 3`, `& 7`… per rate.
- **Horizontal flip by table**: `flip_tab[256]` reverses the bits of a byte (SMA `TFlipH`); same in C.
- **Two-plane OR overlay** for "transparent" tiles (SMA `Multi_texturing`): OR the tile planes
  instead of a masked blit when true transparency is not needed.
- **Variable-offset merge with `rol.w Dn,Dm`** (MegaCar `Handle2Screen`): read two words of a big
  bitmap, mask, add, rotate by the runtime bit offset: one aligned output word. In C use the
  aligned-`long` rule (`ti68k-performance.md` §7).
- **Digits with `divu.w #10`** (all the small games): one instruction gives quotient and remainder;
  in C with `unsigned` values GCC does even better (reciprocal multiply, §3).
- **Priority draw list without sorting** (Chrono `sprite::generate`): repeated min-scans over a
  small table build the draw order; a magic end marker (`$01020304`) detects corruption.
- **Threaded dispatch, 2 instructions per emulated opcode** (gb68k `gameboy_cpu.S`, `gbasm.h`): a
  64 KB block of 256 pages × 256 bytes, a6 at its middle; each handler ends with
  `move.b (a4)+,(d16,a6)` which writes the next opcode byte into the high byte of the displacement
  of the following `jmp (0x7fBA,a6)`, landing on that opcode's page (~26 cycles vs ~130 for a C
  `switch`, measured, `ti68k-performance.md` §6). The pages are built at run time from templates
  (with the cycle count patched into a `subq`). Self-modifying: it only patches the jump's
  *extension word*, beyond the 68000's 2-word prefetch; and code in the heap needs the +0x40000
  mirror on HW2 and HW3Patch on the Titanium (verified, patterns §4).
- **Memory map as executable pages** (gb68k): each 256-byte page starts with `movea.l #hostptr,a0`,
  so the pointer is both data (reads) and code (writes jump into the page's own handler: RAM store,
  bank switch, I/O). Emulated PC and SP are host pointers (a4, a2), converted back only on jumps.
- **`addx`/`subx` only clear Z, never set it**: set Z first (`move #4,%ccr`) before a multi-precision
  chain, or the final Z is wrong (gb68k v0.5.0 bug).
- `move.w %sr,Dn` after an add stores Z and C at bits 2 and 0 (the Game Boy flag layout); allowed in
  user mode on the 68000 (not on the 68010+).
- **Global register variables** (`register GB_DATA *g asm("%a5");` in gb68k, `asm("a4")` in
  Fischer's games, who credit it with ~3 KB saved): every access becomes `d16(An)`. Traps: a5
  conflicts with `OPTIMIZE_ROM_CALLS` (use a4); GCC does not save the register for AMS, so wrap
  `_main` with a `movem.l` save/restore (gb68k's asm `__main`); interrupt handlers must not rely on
  it (reload from memory); set it before `TRY` (`ONERR` restores the registers saved at `TRY`).
  Not measured here.

## 5. Porting kernel-era ASM (Fargo / DoorsOS / PreOS) to NOSTUB

- `tios::Name` = an AMS ROM call (`jsr tios::DrawStrXY`): in GCC4TI, the C function of the same
  name. `util::`, `graphlib::`, `genlib::`, `userlib::`, `shrnklib::`, `pk92lib::` are kernel
  libraries that do not exist in NOSTUB: replace them with ExtGraph (sprites, planes, scrolling,
  tilemap), GCC4TI's `gray.h` and our file/RLE code. SMA's `AntiCras.asm` scanning for unresolved
  `jsr $00000000` shows how much these programs depend on the kernel's relocator.
- Arguments of ROM calls are pushed on the stack right to left and popped by the caller
  (`move.w #2,-(a7)` … `jsr` … `addq.l #2,a7`).
- **Hardware hazards to remove**:
  - direct keyboard reads (`move.w mask,$600018` + delay loop + `move.b $60001B,d0`): the settle
    delay is tuned for HW2 timing; use `_rowread`;
  - vector writes through `bclr #2,$600001` (vector-table write protection) and `trap #1` to change
    SR: use `SetIntVec`/`GetIntVec` and `OSSetSR`;
  - interrupt handlers that **replace** int 1 without chaining (Fargo `time.asm`) stop the AMS
    bookkeeping (APD) while running; chain or restore (patterns §4);
  - hard-coded LCD address / writes to the LCD base port `$600010` (Chrono `crash.asm`), raw
    exception-vector patches, reads of private AMS structures (`tios::kb_vars+$1c`): all
    version- and hardware-specific;
  - self-modifying code (SMA's commented-out `Deca_Code`): breaks when the program runs from
    archive or a packed launcher's copy; not worth it (the author replaced it with variable shifts).
- Grayscale: old programs use kernel grayscale; use GCC4TI `GrayOn`/GrayDBuf.
