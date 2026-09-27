# TI-68k C patterns (GCC4TI / TIGCC, NOSTUB, TI-89 / TI-89 Titanium)

Practical knowledge for writing games and fast programs. Performance rules and measured costs are in
`ti68k-performance.md`. Sources: the GCC4TI docs
(`tools/gcc4ti-bin/doc/html/`, e.g. `kbd__rowread.html`, `intr_SetIntVec.html`), the ExtGraph docs
(`tools/extgraph/DOCS/extgraph.html`), the Puzzle Bobble analysis (`puzzle-bobble-analysis.md`), the
old sources in `sources/` (TICT tutorials, TI-Chess, TICT-Explorer, small games; index in
`sources/README.md`) and tests run in TiEmu (marked **verified**). Game algorithms (AI, 3D,
pathfinding, networking, data encoding) are in `ti68k-game-techniques.md`.

## 1. Program skeleton and global directives

```c
#define USE_TI89              // target TI-89 + Titanium → .89z (add USE_TI92PLUS / USE_V200 for others)
#define OPTIMIZE_ROM_CALLS    // keeps the ROM-call table in a register: smaller/faster calls
#define SAVE_SCREEN           // startup code saves and restores the LCD for you
// #define NO_EXIT_SUPPORT    // drop exit()/atexit support (smaller); then never call exit()
// #define MIN_AMS 200        // refuse to run on older AMS if you use newer ROM calls
#include <tigcclib.h>

void _main(void) { ... }      // entry point, NOT main()
```

- `OPTIMIZE_ROM_CALLS` is safe in `DEFINE_INT_HANDLER` handlers of a running program, but **not**
  in callbacks (event handlers, `OSVRegisterTimer`, `vcbprintf`…) nor in handlers that outlive
  `_main` (TSRs: a5 no longer holds the ROM table, TICT S1P3); the compiler warns about some of them.
- **Global and static variables keep their values between runs** when the program runs in place
  (unarchived, not packed): NOSTUB programs execute from their own file (GCC4TI `faq_29.html`,
  TI-Chess re-initialises everything in `_main`). Initialise state in code, not only in the
  declaration. Old games use it on purpose to store high scores in the program itself; it breaks as
  soon as the program is archived or compressed, prefer a data file (§10).
- **Build flags** (now the `ti-cc` default, from L. Debroux's `ti89_authenticator` Makefile):
  `-ffunction-sections -fdata-sections -fomit-frame-pointer -mregparm=5 -mno-bss` plus the linker
  options `--optimize-code --cut-ranges --reorder-sections --remove-unused --merge-constants`.
  **Verified**: Puzzle Bobble 12,849 → 12,067 bytes and still runs on HW2 and HW3; the GrayDBuf,
  rotation and keyboard demos run unchanged. `-mregparm=5` passes parameters in registers for every
  function: hand-written asm routines that read their arguments from the stack must be declared
  `__attribute__((__stkparm__))` (or build with `TI_CC_PLAIN=1`).
- **`-DCOMPRESSED_FORMAT_RELOCS`** (GCC4TI, compressed relocation table): Puzzle Bobble 12,067 →
  **11,773 bytes** (−2.4 %), runs on the Titanium and the unpatched TI-89 (**verified**). Now in
  `ti-cc`. `-fmerge-all-constants` and `-DMERGE_BSS` changed nothing here. Fischer's games also
  use F-Line ROM calls with `USE_INTERNAL_FLINE_EMULATOR` (−780 B on Sumo, which has many ROM
  calls; +58 B on Bobble) and drop `OPTIMIZE_ROM_CALLS` when a4/a5 hold globals.
- **`register T *g asm("a4")` must be declared before `#include <tigcclib.h>`** (Sumo: a month-long
  "insidious bug"), and set before any `TRY`.
- **Emergency exit from anywhere**: a `noreturn` `LeadOut()` that calls `ER_success()` (pops the
  `TRY` frame), restores vectors/grayscale, frees the bulk block and `exit(0)`s.
- **One bulk allocation** for all planes, buffers, generated sprites and globals, carved with
  alignment and freed once (Hockey/Sumo credit ~700–1,200 bytes to it; keep a guard row after each
  plane: Hockey's 'g' descender overwrote the struct placed after a plane).
- **Program size limits** (**verified**, `experiments/bigprog/`, profiles `89t` and `89u` = TI-89
  HW2 with the official, unpatched AMS 2.09):

  | Program | Titanium, AMS 3.10 | TI-89, AMS 2.09 unpatched |
  |---|---|---|
  | plain, 20 KB / 24.5 KB | runs | runs |
  | plain, ≥ 24.6 KB (30, 60 KB) | **runs** (also archived) | error "ASAP or Exec string too long" |
  | plain + `EXECUTE_IN_GHOST_SPACE`, 30 KB | not needed | same error (the check comes first) |
  | `-pack NAME` launcher, 30 and 60 KB | runs | **runs** |
  | small program + 60 KB data file, archived | runs | runs |

  The AMS 2.xx limit is **24,576 bytes (0x6000) of on-calc size** (the .89z file is ~90 bytes
  bigger); AMS 3.10 has none, only the **64 KB per variable** limit (65,518 bytes) remains. AMSpatch
  (profile `89`) and tiosmod remove the 2.xx limit too. So, from cheapest to heaviest:
  1. **Put the data outside the program** (sprites, maps, levels, texts, tables): one or more data
     variables of up to 64 KB each (`ttbin2oth -89 dat file.bin name`, or written by the program
     itself, §10). Read them **in place** with `SymFindPtr(SYMSTR("name"),0)->handle` +
     `HeapDeref` (+2 to skip the size word): **archived variables are read directly from Flash**
     (pointer 0x37xxxx on the TI-89, 0x9Axxxx on the Titanium), no RAM used; verified on both. Do
     not keep the pointer across heap operations if the file is in RAM (lock it: `HLock`).
  2. **Compress with `-pack NAME`**: `ti-cc -pack gamed -o game game.c` gives a ~1 KB launcher
     `game.89z` plus the compressed program `gamed.89y` (to archive); the launcher decompresses
     into RAM and runs it, bypassing the 24 KB check. Puzzle Bobble: 12,067 → 7,215 (data, .89y file) + 1,095 bytes (launcher)
     (−31 %). Costs a decompression at start-up and RAM for the full program; globals no longer
     persist between runs. Do not use it for DLL-based programs (froze on HW3, §4).
  3. Generate at start-up what can be computed (mirrored sprites §3, tables) into `malloc`
     buffers.
  4. More than 64 KB of **code**: DLLs (§4, model-dependent) or split into several programs
     chained through a data file. Last resort; flash apps (.89k) need a signature.
- **`-mno-bss` stores uninitialised globals in the file** (**verified**): `unsigned short
  buf[5000];` → 10,399-byte program with the `ti-cc` flags, 785 bytes without `-mno-bss` (the BSS
  block is then allocated and zeroed at each start). `-mno-bss` stays the default (Bobble is 160
  bytes smaller with it), but **big buffers (screen buffers, generated sprites, maps) must be
  `malloc`ed**, never declared as globals.
- `-DUSE_FLINE_ROM_CALLS` (needs `-DMIN_AMS=207`) makes each ROM call a line-1111 trap: slightly
  smaller in some programs, but here **bigger** (12,125 bytes vs 12,067) and every call pays an
  exception: not for games.
- Without `SAVE_SCREEN`, save and restore the screen yourself: `LCD_BUFFER s; LCD_save(s); … LCD_restore(s);`.
- `int` is **16-bit** (TIGCC default). Use `long` for anything above 32767 (scores, timers,
  products like `norm * cos`). `sizeof(int) == 2`.
- No FPU: avoid `float` in the game loop. Use fixed point (§6) and precomputed tables.
  `sqrt`/`atan2` are fine once at init (Puzzle Bobble's `calc()`).

## 2. The screen

- LCD memory: `LCD_MEM`, **240×128 bits = 3840 bytes (`LCD_SIZE`), 30 bytes per row**, on the
  TI-89 too. Only the top-left **160×100** is visible on a TI-89.
- Pixel address: `base + y*30 + (x>>3)`, mask `0x80 >> (x&7)`. A fast form is `(y<<5)-(y<<1)`.
- Double buffering: draw into a `malloc(LCD_SIZE)` buffer, then `FastCopyScreen(buf, LCD_MEM)`
  (ExtGraph) once per frame. Never draw straight to the visible screen in a game (flicker).
- `PortSet(buf, 239, 127)` redirects every AMS drawing routine (`DrawStr`, `DrawLine`, `printf`…)
  into a buffer (any width/height), `PortRestore()` switches back: build menus, HUD frames and title
  screens off-screen with AMS, then blit. Also the way to pre-render an AMS font into your own glyph
  table once at start-up (`ti68k-performance.md` §7, **verified**).
- **Guard band**: allocate buffers a sprite-height larger than what is shown (e.g. 8 hidden rows) and
  draw new content there; then the unclipped `Fast…` routines are safe everywhere (TICT S1P6).
- Keep your own bitmap strides even (better: a multiple of 4) so rows can be moved with word/long
  copies; an odd stride (World Map: 69 bytes) forbids them.
- Status line: AMS redraws indicators (2ND, BUSY…) from auto-int 1. Redirect it (§4) or it trashes
  your screen. `ST_showHelp("text")` writes a message in the status line after you exit.

## 3. Grayscale (4 levels) with ExtGraph 2

```c
void *light = malloc(LCD_SIZE), *dark = malloc(LCD_SIZE);   // virtual planes; check for NULL!
ClearGrayScreen2B(light, dark);
if (!GrayOn()) { /* free and quit */ }
... per frame:
FastCopyScreen(light, GetPlane(LIGHT_PLANE));   // GetPlane(0)
FastCopyScreen(dark,  GetPlane(DARK_PLANE));    // GetPlane(1)
... on exit:
GrayOff();                                       // BEFORE freeing anything it uses / restoring int 1
```

- Colour = combination of the two planes: `COLOR_WHITE` (neither), `COLOR_LIGHTGRAY` (light
  only), `COLOR_DARKGRAY` (dark only), `COLOR_BLACK` (both). **Always use the enum names**: the
  numeric values changed between ExtGraph 1.x and 2.x (real bug fixed in Puzzle Bobble).
- Plane choice has **logic consequences**: pixel collision tests read one plane. Put decorative
  lines in the plane you do not test.
- Gray sprites take one bitmap per plane: `GraySprite32_XOR(x, y, h, spr_light, spr_dark, light, dark)`.
- **ExtGraph 2 bug: `ClipSprite8_MASK_R` (mono) draws garbage when `x & 15` is 9..15** (the sprite
  straddles a word; its long path does `swap %d1` where `swap %d0` is meant, so the mask is not
  rotated and the data is rotated wrongly). **Verified** HW3: `runtime/tests/xcheck.c` scene 1
  differed from the PC renderer, and emulating the typo on the PC reproduced the calculator's
  checksum exactly. `GrayClipSprite8_MASK_R` is correct: in mono, call it with the same plane
  twice (idempotent) for those x (`rt_ti.c`). The other clipped MASK/RPLC routines (8/16/32, mono
  and grey) matched the PC renderer bit for bit.
  Puzzle Bobble packs both in one array: `long sprites[n][2*h]` → `sprites[i]` (light) and
  `sprites[i]+h` (dark).
- ExtGraph function naming: `GraySprite{8,16,32}_{OR,AND,XOR,MASK,…}`, `…2B` = works on 2 planes,
  `Fast…` = no clipping (**you** must keep coordinates on-screen). Old names such as
  `FastDrawGrayHLine2B` still exist as deprecated aliases of `GrayFastDrawHLine2B`.
- Masked sprite on a background: `GraySprite16_AND(x, y, h, mask, mask, light, dark)` with an
  **inverted** mask (`~shape`) to punch a hole, then `GraySprite32_OR(...)` the sprite into it.
- **White outline for the important sprites** (hero, NPCs, enemies): with 4 greys on a busy
  background a sprite easily melts into the scenery. Make the mask **1 pixel larger than the
  shape** (the shape dilated by one pixel in the 4 or 8 directions): the AND step then clears a
  white ring around the sprite and the OR step draws it inside. Free at run time (a masked sprite
  already costs AND + OR; only the mask data changes), and it can be computed once at start-up
  (`m = s | s<<1 | s>>1 | row above | row below`, on the raw shape rows). Not always needed
  (small items, sprites on a plain background), but always ask the question for the main
  character. Leave room: a 16-pixel-wide sprite with an outline needs a 14-pixel shape, or a
  32-bit-wide routine.
- **Second colour set for free: swap the plane pointers** (Hockey team 2, Sumo's second wrestler):
  draw the same sprite with `(dark, light)` instead of `(light, dark)`: light grey ↔ dark grey,
  black stays black. Works with a mask shared by both planes.
- **Masks generated at start-up** from the planes (`mask = ~(light | dark)`: white = transparent,
  3 colours + transparency): no mask data stored, a third less sprite data (Sumo). Or skip the mask
  entirely: `*p &= ~((l | d) << s); *p |= l << s` per plane (Hockey).
- **Big composite frames** (Sumo grappling: both wrestlers in one sprite) halve the draw and
  save/restore work when two sprites always overlap.
- **Sprite table that encodes mirrors** (Sumo): an entry with `light == NULL` is a mirror; its height
  field is the source index (0 = previous entry), a magic mask value means vertical. 12 tumbling
  frames from 4 drawn; a 160×100 frame from one mirrored 80×50 quarter. Keep light and dark planes
  contiguous (`[2][h * bw]`) and one `SpriteX8_MIRROR_H_R(2 * h, …)` mirrors both at once (a
  vertical mirror needs one call per plane). Round every generated block to an **even size**
  (odd-sized packing gave Sumo an Address Error with 16/32-bit routines), and do not patch pointers
  into the program's own globals (only works because Sumo is packed: patterns §1).
- **Generate mirrored sprites at start-up** (advanced, saves program size): store each sprite
  facing one way only and build the other orientations into a `malloc` buffer before the game
  starts (not a global array: `-mno-bss`, §1). A few ms of start-up for up to 2–4× less sprite data;
  worth it for animated characters (walk cycles left/right, top-down games up/down). Tested on PC:
  ```c
  static unsigned char rev8[256];                     // bit-reversed bytes, built once
  void make_rev8(void) { unsigned short i; for (i = 0; i < 256; i++) { unsigned char b = i, r = 0, k;
      for (k = 0; k < 8; k++) { r = (r << 1) | (b & 1); b >>= 1; } rev8[i] = r; } }
  // horizontal mirror, 16-pixel-wide rows; the shape uses the w leftmost pixels
  void hflip16(const unsigned short *s, unsigned short *d, unsigned short h, unsigned short w)
  { while (h--) { unsigned short r = *s++; *d++ = (unsigned short)((rev8[r & 0xFF] << 8) | rev8[r >> 8]) << (16 - w); } }
  // vertical mirror: rows in reverse order
  void vflip(const unsigned short *s, unsigned short *d, unsigned short h)
  { const unsigned short *e = s + h; while (h--) *d++ = *--e; }
  ```
  **Prefer ExtGraph's own routines** (**verified** identical to the code above, HW3):
  `SpriteX8_MIRROR_H_R(h, src, bytewidth, dst)`, `SpriteX8_MIRROR_V_R`, `SpriteX8_MIRROR_HV_R`,
  `FastSprite16_MIRROR_H_R(h, src, dst)` (8/32 variants, 256-byte table, faster), and 90° rotations
  `SpriteX8X8_ROTATE_RIGHT_R`/`_LEFT_R` (both dimensions multiples of 8). Widths must be multiples of
  8: for a narrower shape shift the mirrored rows left by `16 - w` afterwards (Sumo does). Sumo
  builds all its mirrored tiles at start-up and marks "mirror of the previous entry" with a NULL
  pointer in the sprite table. Flip **both planes and the mask**. Not worth it for asymmetric sprites (a sword in the right
  hand, lit from one side) or when the flipped copies do not fit in RAM.
- XOR drawing = cheap erase: drawing the same sprite twice with XOR restores the background. That
  is how Puzzle Bobble moves the ball (erase, move, redraw), but XOR only works if nothing else was
  drawn over it in between.
- Grayscale uses auto-int 1 internally: `GrayOn()` installs its own int-1 handler, which **chains to
  whatever int-1 vector was installed before it**. That is why you redirect int 1 *before*
  `GrayOn()` (§4). `GraySetInt1Handler(h)` changes the chained handler after `GrayOn()` (restore
  `DUMMY_HANDLER`/the old one before `GrayOff()`): a counting handler there gives a **free 256 Hz
  clock** that leaves int 5 and the AMS timers alone (**verified**: still 256 Hz with grayscale on,
  HW2 and HW3).

### Double buffering with GrayDBuf (preferred for grayscale games)

GCC4TI's grayscale has built-in double buffering: two pairs of planes, the visible pair is switched
by changing a pointer, so there is **no copy at all**, and the switch is synchronised with the plane
switch, so there is no tearing and no "phasing" (one plane showing the new frame, the other the old).

```c
void *dbuf = malloc(GRAYDBUFFER_SIZE);          // check NULL
SetIntVec(AUTO_INT_1, DUMMY_HANDLER);
if (!GrayOn()) ...;
GrayDBufInit(dbuf);                             // current planes become buffer 0
while (running) {
    void *l = GrayDBufGetHiddenPlane(LIGHT_PLANE), *d = GrayDBufGetHiddenPlane(DARK_PLANE);
    FastClearScreen_R(l); FastClearScreen_R(d); ...draw...;
    GrayDBufToggleSync();                       // waits for the next plane switch, then swaps
}
GrayOff();                                      // also ends double buffering; then free(dbuf)
```

- **Verified** (`experiments/gray/dbuf.c`, same numbers on HW2 and HW3): the plane switch rate is
  **~85/s**; with `GrayDBufToggleSync()` the loop runs at most one frame per switch (85 fps), and
  each frame saves the two `FastCopyScreen_R` (~35,000 real cycles; TiEmu shows ~4,800 because it
  undercounts `movem`, performance §1). But `GrayDBufToggleSync()` **waits** for the next switch: in a
  loop that is not limited anyway, a partial 160×100 copy (ExciteBike, Sumo) can beat it. Drawing into RAM planes and copying
  without sync ran at 570 fps in the same test but tears. For a 30 fps game: frame limiter on the
  timer (§6), then `GrayDBufToggleSync()`.
- **What grayscale really costs on HW2 and the Titanium** (read in GCC4TI's
  `tools/gcc4ti/trunk/tigcc/archive/gray.s`, `__gray_int1_handler_hw2`). The HW2+ LCD does not read RAM:
  its controller keeps its own copy and watches CPU writes to 0x4C00. So the driver *copies* the
  plane to show into 0x4C00. At each flip of the LCD sync bit (below, ~85/s), it copies one third of a
  plane (1,280 bytes) per auto-int 1, using 14-register `movem.l`. The plane sequence is light, dark,
  dark, and the second dark frame is not copied. That makes ~57 plane copies/s at ~17k real cycles
  each, plus a 15-register save per int 1 (256/s): **~1M cycles/s, ~9 % of the CPU**, on hardware. TiEmu
  undercounts `movem`, so this is invisible in the emulator. Budget ~360k cycles per frame at 30 fps,
  not 400k. GrayDBuf only avoids *our* copies. HW1 flips the LCD base address (port 0x600010)
  instead and pays nothing.
- **LCD frame sync bit** (**verified**, `experiments/hwsync/hwsync.c`, HW2 and HW3): bit 7 of the byte
  port `0x70001D` toggles each time the LCD restarts at line 0. It toggles **85 times per second** on
  both models. This is the signal the grayscale driver waits for. In a black-and-white game, poll it
  (`while (!((*(volatile unsigned char *)0x70001D ^ prev) & 0x80));`) before drawing, to avoid tearing.
  HW1 has no such bit.
- `GrayAdjust(n)` changes the logical LCD height (port 0x600012) to move the refresh rate away from a
  beat with the plane switches (±10 useful). It persists after exit and darkens the screen. The docs
  call it mostly obsolete on HW2+. Offer it as a hidden key at most.
- After `GrayDBufInit`, do not use `GrayGetPlane`/`GetPlane` for drawing; use
  `GrayDBufGetHiddenPlane`. `GrayGetSwitchCount()` / `GrayWaitNSwitches(n)` sync other loops to the
  plane switches (2 switches = one full gray frame).
- Do not call `idle()` while grayscale is on (GCC4TI docs: it disturbs the grayscale timing).
- **Sleep in the frame limiter instead** (**verified**, HW2 and HW3, `experiments/gray/sleep.c`):
  `while ((unsigned short)(ticks - last) < 8) pokeIO(0x600005, 0x1D);` stops the CPU until the next
  interrupt (bits 0–4 of 0x600005 = auto-ints 1–5 that wake it; 0x1D leaves out int 2, so a held
  key does not wake it; ON always does). Same 32 fps and 85 plane switches/s as a busy wait, but
  the loop runs 269 times per second instead of 183,000: battery saved, and unlike an empty loop
  the compiler cannot delete it. Use it inside a tick-based limiter, never as a fixed number of
  sleeps per frame (Fischer's games: speed then depends on the model).

## 4. Interrupts (auto-ints)

| Vector | Rate | Used for |
|---|---|---|
| `AUTO_INT_1` (0x64) | 256 Hz on HW2/HW3/HW4 (~350 Hz HW1) | AMS keyboard scan, status line, grayscale |
| `AUTO_INT_2` | ~600 Hz while a key is held | key press |
| `AUTO_INT_3` | 1 Hz HW2 clock; USB on HW3/HW4 | |
| `AUTO_INT_5` (0x74) | ~19 Hz, programmable | AMS timers (`OSRegisterTimer`, APD) → the usual **game clock** |
| `AUTO_INT_6` | ON key | |

Standard game setup (as in Puzzle Bobble, and the pattern from `intr_DUMMY_HANDLER.html`):

```c
INT_HANDLER old_int1, old_int5;
volatile long ticks = 0;                 // every variable shared with a handler is volatile

DEFINE_INT_HANDLER(my_int5) {            // not a normal function: only pass it to SetIntVec
    ticks++;
    ExecuteHandler(old_int5);            // chain to AMS so its timers keep running
}

old_int1 = GetIntVec(AUTO_INT_1);
SetIntVec(AUTO_INT_1, DUMMY_HANDLER);    // AMS keyboard/status line off; grayscale chains to "nothing"
old_int5 = GetIntVec(AUTO_INT_5);
SetIntVec(AUTO_INT_5, my_int5);
unsigned char old_start = PRG_getStart();
PRG_setStart(0xFC);                      // optional: faster int 5 (see below)
GrayOn();
... game ...
GrayOff();                               // first: grayscale's int-1 handler goes away
PRG_setStart(old_start);                 // restore exactly what AMS had
SetIntVec(AUTO_INT_5, old_int5);
SetIntVec(AUTO_INT_1, old_int1);
GKeyFlush();                             // don't leave the game's keypresses in the AMS buffer
```

- **Restore every vector and port you touched, in reverse order**, on every exit path. A missing
  restore crashes the calculator later, not right away.
- Auto-int 5 rate: a counter counts up from the "start" value (`PRG_setStart`, I/O port
  `0x600017`) through 0xFF to 0x00, fires, and reloads: **period = 257 − start** counts of a
  **~1024 Hz** base (`PRG_setRate(1)`, OSC2/2⁹). **Verified** (`experiments/timers/irate.c`,
  against the 256 Hz int 1, itself checked against the wall clock), **identical on HW2 and HW3**:

  | start | 0xB2 | 0xCC (AMS default) | 0xF2 | 0xF7 | 0xFC |
  |---|---|---|---|---|---|
  | int-5 rate | 13 Hz | 19.3 Hz | 68 Hz | 102 Hz | 205 Hz |

  The **AMS default is 0xCC on both the Titanium (AMS 3.10) and the TI-89 HW2 (AMS 2.09)**
  (**verified** from a fresh state). 0xB2 is the HW1 value (slower base clock, ~13 Hz × 1.5 ≈ 20 Hz);
  old code that writes 0xB2 "if not HW2" treats the Titanium as HW1 and leaves AMS running at 13 Hz
  after exit (Puzzle Bobble did: **verified**, fixed in our port). An earlier note here claimed 0xB2
  was the Titanium default: that reading was taken after Puzzle Bobble had run. Always
  `old = PRG_getStart()` … `PRG_setStart(old)`.
- To get the same tick rate on every model, pick the start value from the table (e.g. 0xF7 ≈ 100 Hz
  for a stopwatch, Stopwatch89 converts with `ticks * 100 / 102`); on HW1 the base is slower.
- Speeding up int 5 also speeds up **AMS timers** (`OSRegisterTimer` waits, auto power-down):
  anything timed with `USER_TIMER` runs faster while the game runs.
- `ExecuteHandler` may only be called from inside a `DEFINE_INT_HANDLER` (else Privilege Violation).
- Keep handlers tiny: increment counters, set flags. No drawing, no `malloc`, no ROM calls that are
  not interrupt-safe.
- Reading a `long` written by an interrupt is safe on the 68000 (`move.l` is not interruptible).
  A read-modify-write on shared state (`x = x + 1` from both sides) is not.
- Need to detect the hardware? Use `HW_VERSION` (1, 2, 3 = Titanium; verified), not the old
  hand-written `GetHardwareVersion()` that reads the ROM base with `*(long*)0xC8 & 0x600000`: the
  Titanium's ROM is at 0x800000, the mask gives 0 and the function reads garbage and answers "HW1"
  (Puzzle Bobble, TICT-Explorer's battery code). Old `hw == 2 ? a : b` tests therefore give the
  Titanium the HW1 value.
- **Cheap black-and-white setup**: `short old_sr = OSSetSR(0x0400); … OSSetSR(old_sr);` masks
  interrupt levels 1–4 (AMS keyboard/status line, key interrupt, int 3, **link**) while int 5 and
  the AMS timers keep running (Nibble, Tunnel, Falldown, Jezzball). Useless with grayscale (needs
  int 1) and kills link-cable play (int 4). Old games never restore SR: always do. `ngetchx()`
  still works under this mask (**verified** HW2 and HW3: AMS lowers the mask while it waits).
- **Game logic inside the interrupt** (Mode7 Engine, F-Zero): the main loop only draws, as fast
  as it can, and the int-1 handler runs the game step on a countdown (`if (!--next) { next =
  SPEED; step(); }`). It decouples logic rate from frame rate, but the handler then runs arbitrary
  code (physics, link I/O): only safe if that code never calls non-reentrant ROM routines and shares
  no half-updated state with the renderer. Prefer the main-loop fixed timestep (§6) unless you
  need it.
- HW1 correction without touching the timer (Mode7 Engine): a fractional accumulator in the
  handler (`acc += FREQ; if (acc <= 32768) return; acc -= 32768;`) skips the right share of ticks.
- TSRs and "ghost space" tricks (`enter_ghost_space()`, `EX_patch(p + 0x40002)`, running code
  through the 0x40000 RAM mirror: TICT S1P3, TICT-Explorer, liss89 launchers) are HW1/HW2-only
  ways around the execution protection; they fail on the Titanium without HW3Patch. Never call
  non-reentrant ROM routines (`sprintf`, `ST_showHelp`) from an interrupt, and never restore a
  vector blindly in a TSR (another program may have hooked it since).
- **DLLs** (TIGCC `LoadDLL`/`_DLL_call`, used by Crystal Engine to go past the program size limit;
  the FAT engine's own loader works the same way). **Verified** with the GCC4TI example DLL
  (`experiments/dll/`, TiEmu, no HW3Patch):

  | Loader built… | TI-89 HW2 (AMS 2.09) | Titanium (AMS 3.10) |
  |---|---|---|
  | plain | `LoadDLL` = 1 (`DLL_NOTINGHOSTSPACE`) | **works** |
  | `#define EXECUTE_IN_GHOST_SPACE` | **works** | refused at start ("HW3Patch required") |
  | exe-packed (`-pack name`) | **works** | `LoadDLL` = 0, then the first DLL call **freezes** the calculator |

  No single unpatched binary works on both: a DLL game needs HW3Patch on the Titanium, or two
  builds. Exports are called by **ordinal**: never reorder the export list. For big games, split the
  **data** into separate files (§10) instead: HW3-safe and simpler.
  Useful idiom from FAT: fill a function-pointer table with a "not linked" stub that flashes the
  screen, never NULL, so a call before loading fails visibly.
- A handler that owns its state avoids races: the main loop only sets a request flag
  (`reset = 1`), the handler does the reset (TICT S1P2, TI-Chess keyboard latch). For clocks keep
  **one** tick counter and derive h:m:s in the main loop; reading three variables separately can
  show 00:59:59 → 01:59:00.
- **Changing the game speed via the int-5 rate** (ExciteBike cheat): period = 257 − start, so
  "half speed" is `start' = 257 − 2 × (257 − start)`, not `start / 2` (0xCC/2 made it 2.9× slower).
- **Replacing int 5 breaks AMS timers** (**verified**): `LIO_RecvData`'s timeout never expires
  (blocks until ON; `ti68k-game-techniques.md` §7), nor does `OSTimerExpired(APD_TIMER)`.
- **Running code you generated or copied into RAM** (**verified**, `experiments/heapcode/`): a heap
  block containing `moveq #42,d0; rts`:

  | Model (unpatched) | call the block's address | call address + 0x40000 |
  |---|---|---|
  | TI-89 HW2, AMS 2.09 | works near the top of RAM (0x3Fxxx), **freezes** at 0x311B4 | **works** (ghost mirror) |
  | Titanium, AMS 3.10 | works at 0x3Fxxx, **freezes** at 0x311B4 | Address Error |

  The execution protection is per RAM region on both models; HW2 is bypassed through the
  0x40000 mirror (what gb68k does: `if (HW_VERSION == 2) addr += 0x40000`), the Titanium has no
  mirror and needs HW3Patch. So self-generated code (dynamic recompilers, code in data files) is not
  portable unpatched; keep code in the program itself.
- ON key as a "boss key" (gb68k): an auto-int 6 handler that only sets a flag, polled by the game to
  quick-save and quit (not tested here).

## 5. Keyboard

Two worlds; do not mix them in the same phase:

- **"Just pressed" in the same word** (Sumo): keep last frame's action keys in spare high bits
  (`keys = (keys & 0x30) << 2` before reading), then
  `#define TAP(k) ((HELD(k) | (keys & ((k) << 2))) == (k))`; choose the logical bit layout equal to
  the TI-89's row 0 (up, left, down, right, 2nd, shift) so the TI-89 read is one `_rowread(~1) & 0x3F`.
  Remappable keys: store `{row, col}` pairs, `(RowKey){RR_ESC}` compiles since `RR_*` expands to
  `row, col`; but read each needed row once, not one `_keytest` per key (Hockey does 7+ reads).
- A **"teacher key"** (instant quit from every screen) is standard in these games.
- **Read the keyboard once per frame into a bitmask** (Hockey `KeyScan`): read the needed
  `_rowread` rows once, pack the game keys into one `unsigned short` (per-model row/bit tables),
  then `keys & K_UP` everywhere; press edges = `keys & ~old_keys`, combinations are one test, and
  the whole frame sees one snapshot. GCC4TI's `BEGIN_KEYTEST … _keytest_optimized(RR_…) …
  END_KEYTEST` groups keys of the same row into one read and stays portable across models.

| AMS keyboard (`ngetchx`, `kbhit`, `GKeyIn`) | Direct matrix (`_rowread`, `_keytest`) |
|---|---|
| needs auto-int 1 running | works with int 1 redirected (recommended) |
| one key at a time, buffered, with repeat | any number of keys held at once, no buffer |
| menus, text input, "press a key" screens | game loops |

- Use `ngetchx()` **before** redirecting int 1 (title screen), `_rowread` after. With int 1 on
  `DUMMY_HANDLER`, `ngetchx()` would wait forever.
- The docs recommend redirecting **both auto-int 1 and 5** while using `_rowread`, because the AMS
  keyboard code in those interrupts can interfere. Puzzle Bobble redirects 1 and chains 5 to AMS;
  it works, but a key read can occasionally glitch.
- `_rowread(mask)`: bits **set** in `mask` mask rows out; the result has a bit set for each
  pressed key of the selected rows. Read row *r* with `_rowread(~((short)(1<<r)))`. Prefer
  `_keytest(RR_ESC)` style constants from `compat.h` (portable to the TI-92+/V200).

TI-89 matrix (row = bit of the mask, column = bit of the result):

| Row | b7 | b6 | b5 | b4 | b3 | b2 | b1 | b0 |
|---|---|---|---|---|---|---|---|---|
| 0 | alpha | ◆ | shift | 2nd | right | down | left | up |
| 1 | F5 | CLEAR | ^ | / | * | − | + | ENTER |
| 2 | F4 | ← | T | , | 9 | 6 | 3 | (−) |
| 3 | F3 | CATALOG | Z | ) | 8 | 5 | 2 | . |
| 4 | F2 | MODE | Y | ( | 7 | 4 | 1 | 0 |
| 5 | F1 | HOME | X | = | \| | EE | STO | APPS |
| 6 | | | | | | | | ESC |

So `_rowread(0x7E) & 0x02` is **LEFT** and `& 0x08` is **RIGHT** (`0x7E` = `~(1<<0)` in the low
byte). Puzzle Bobble's comments call 0x08 "Left", which is wrong; the angle maths makes the
behaviour right anyway. ON is not in the matrix (auto-int 6).

- Pressing three keys at the corners of a rectangle makes the fourth read as pressed (ghosting).
- No edge detection is built in: `_rowread` reports "held". For "fire once", either compare with
  the previous frame's state or add a cooldown (Puzzle Bobble: `ticks - last_shot > 50`).
- **Keyboard latch in auto-int 1** (TI-Chess `interrupt.c`): scan one row per interrupt, latch
  press edges, let the handler clear the latches on request. **Verified** on HW2 and HW3
  (`experiments/keys/latch.c`): with a main loop polling once per second, the latch caught 5 of 5
  ENTER taps, plain `_rowread` polling 1 of 5. Initialise `orow[]` with the current rows before
  installing it, or the key that launched the program counts as a press.
  ```c
  static volatile unsigned char trow[7], clear_req; static unsigned char orow[7], r;
  DEFINE_INT_HANDLER(kbd) {                    // int 1; under grayscale: GraySetInt1Handler(kbd)
      unsigned char n;
      if (clear_req) { memset((void *)trow, 0, 7); clear_req = 0; }
      n = _rowread(~(1 << r));
      trow[r] |= ~orow[r] & n;                 // new presses since the last clear
      orow[r] = n;
      if (++r == 7) r = 0;                     // each row sampled ~36 times/s
  }
  ```
  Main loop: `if (trow[1] & 1) …ENTER pressed…; clear_req = 1;`. Polling is a bit test, short
  taps are never lost during a slow frame, and edges come free. Held state: keep `orow` too.
  Debounce by requiring two equal samples rather than busy-waiting after each key (TI-Chess waits
  150 ms).
- Without a latch, **poll the keys inside the frame wait** and keep the last direction pressed
  (Nibble), so short taps between frames are not lost. Snake-like games: ignore the 180° reverse.
- Auto power-down in a custom input loop: `OSTimerRestart(APD_TIMER)` on entry, then
  `if (OSTimerExpired(APD_TIMER)) { off(); OSTimerRestart(APD_TIMER); }` (TI-Chess, TICT-Explorer,
  TICT S1P6); needs int 5 still chained to AMS. After `off()`, grayscale may need resyncing.
- `_rowread(0)` (no row masked) returns the OR of all rows: `while (_rowread(0));` waits until
  every key is released (keyreleased demo; used in our benchmarks).
- AMS key codes: the auto-repeat flag is 0x800 (`key & ~0x800`); `OSdequeue(&key, kbd_queue())`
  is a non-blocking read for menus. Use the `KEY_*` constants, not per-model numbers.
- Before returning to AMS: `GKeyFlush()` and wait until all keys are released, or the keys still
  held are interpreted by the home screen.
- `off()` (Puzzle Bobble on CLEAR) powers the calculator down and resumes after ON. Test it: it
  runs with your interrupt vectors installed.

## 6. Timing and game loop

```c
long frame = ticks;
while (running) {
    if (ticks - frame < FRAME_TICKS) continue;   // frame limiter driven by int 5
    frame = ticks;
    update(); draw_to_buffers(); copy_buffers_to_screen();
}
```

- The frame rate then follows the int-5 speed (start value) and not the CPU speed, which differs
  between hardware versions: good. If you busy-wait on `ticks`, the variable must
  be `volatile`.
- Animations can be driven by "time since event" (Puzzle Bobble: erase fade = `(ticks - t0) >> 4`,
  falling = quadratic in elapsed ticks) instead of per-frame counters: they keep their speed when a
  frame is slow.
- `OSRegisterTimer(USER_TIMER, n)` + `OSTimerExpired` gives a delay in int-5 ticks (~20/s at the
  default rate). Call `OSFreeTimer` first; it runs faster if you sped int 5 up.
- **Never time anything with an empty loop.** GCC4TI at -Os deletes `for(i=0;i<n;i++){}` entirely
  (**verified**: `experiments/codegen/delay.c` compiles to a bare `rts`): every old game timed this way (Falldown, Tunnel, Jezzball, both
  Tetris, TI-Chess `WaitForMillis`) runs with no delay at all once recompiled. The loops were also
  calibrated per CPU. Replace them with the timer-driven frame limiter above. A loop that calls
  `_rowread` survives only by luck.

## 7. Maths without floats

- Fixed point: store positions as `x << 4` (1/16 pixel), draw at `x >> 4`. Speeds are then fractional.
- Angles as `unsigned char` 0–255 (wrap for free), trig from a `signed char sin_tab[256]` scaled
  by 127 (`tools/bin/ti-table sin`), cos = `sin_tab[(unsigned char)(a + 64)]`, `(r * SIN(a)) >> 7`.
  Beware 16-bit overflow on the products (r × 127 must stay below 32768). Puzzle Bobble uses two
  `int` tables (1 KB); one `signed char` table (256 bytes) is enough. Rotation, atan2, reciprocal
  tables: `ti68k-performance.md` §3.
- `random(n)` / `randomize()` from TIGCC's stdlib (seeded from the clock). Seed **once** at start:
  calling `randomize()` before every piece (both Tetris) correlates the pieces with time.
  `random(1)` is always 0.
- Rounding: `>> n` on a negative value rounds towards −∞. Chaining several rotations, each with its
  own `>> 7`, accumulates a bias (3D tutorial ex5: ~1.5 units on a 20-unit cube). Add half before
  shifting (`(v + 64) >> 7`) or keep more fraction bits and shift once at the end.

## 8. Collisions

- Pixel-perfect, by reading a plane: AND the sprite with the screen bytes under it (Puzzle
  Bobble's `test_sprite`, reading 4 bytes per row with a 30-byte stride). It is exact, but it tests
  **everything** drawn in that plane (walls, lines, text). Keep a dedicated plane or buffer, or
  test a logical grid instead.
- Grid games: keep a logical array (`storage[row][col]`) as the source of truth, redraw from it,
  and use flood fill (BFS with an explicit queue; avoid deep recursion on a small stack) for
  "matching groups" and "floating pieces" (Puzzle Bobble's `three()` and `fall()`). Reading the
  screen with `GetPix` instead (tetris_src, Nibble, Tunnel) is ~600 cycles per test and ties the game
  logic to what is drawn.
- **Sentinel borders**: surround the logical grid with "wall" cells (tetrisc: column 0 and 12, rows
  19–21 set to 1; TI-Chess 10×12 board with `OUTSIDE` squares) so movement, collision and scan
  loops need no bounds checks and stop by themselves.
- Ball against axis-aligned walls: test and bounce **one axis at a time** (`if hit(x+vx, y) vx=-vx;
  if hit(x, y+vy) vy=-vy;` then move): robust in corners (Jezzball). With 8-bit angles the
  reflections are free: `a = -a` (horizontal wall), `a = 128 - a` (vertical wall).
- **Tile maps** (Zelda): a per-tile-type property table (solid, water, animated…) instead of
  per-cell flags; a short list of the animated cells of the current screen (no map scan per frame);
  collide a 16×16 sprite with up to 4 tiles, each blocking only a smaller "core", axes resolved one
  at a time and the position snapped to the tile edge: corner sliding for free.
- Exception to "never test the screen": an effect that must interact with whatever is already
  displayed (snow settling on the current screen, TICT snow demo) is right to use the framebuffer
  as its state.
- Draw and test in the same pass when a line or trail grows: `hit |= GETPIX(obstacles, x, y);
  SETPIX(plane, x, y);` (Jezzball).

## 9. Memory and stack

- Check every `malloc`/`calloc` result, and free everything on exit (Puzzle Bobble checks none).
- **Never `free(NULL)`**: AMS `HeapFreePtr` crashes with an Address Error (**verified** HW3,
  `runtime/platform-ti68k/rt_ti.c`: a lazily allocated buffer freed unconditionally). Guard with
  `if (p) free(p);`. Combined with statics surviving between runs (§1), a pointer freed in one run
  and freed again in the next crashes too: reset such statics at the top of `_main`.
- The stack is limited: big local arrays are risky. Puzzle Bobble puts `int check[12][15]` and
  two `int store[200]` (~1.2 KB) on the stack in both `three()` and `fall()`, and `three()` calls
  `fall()`, so ~2.3 KB are live at once. Make such work arrays `static` or global.
- `NO_EXIT_SUPPORT` makes the binary smaller but forbids `exit()`; cleanup must go through the end
  of `_main`.
- **Reserve block** (TI-Chess): `malloc` a spare ~8 KB block first, allocate everything else,
  then free the reserve just before `GrayOn()` (which allocates) and before writing save files: the
  last critical allocation cannot fail even when RAM is nearly full.
- Free-memory check before big allocations: `HeapCompress(); HeapAvail()`, `FreeHandles()`,
  `EM_survey()` for the archive (TICT-Explorer info screen).
- **Cleanup that always runs**: wrap the game in `TRY … FINALLY … ENDFINAL` (`error.h`, with `#define ENABLE_ERROR_RETURN`) so
  `GrayOff()`, vector and timer restores run even when a ROM call throws (memory error…).
  **Verified** (`experiments/errors/tryfin.c`, HW3): an AMS error thrown while grayscale is on runs
  the `FINALLY` block (grayscale off, int 1 restored), AMS then shows the "Memory" error dialog and
  the calculator stays usable. A **CPU exception is not caught**: a word write at an odd address
  gives AMS's fatal "Address Error" screen (calculator hung, reset needed) and `FINALLY` never
  runs. Keep `short`/`long` accesses even.
  TICT-Explorer goes further and redirects CPU exception vectors (address error, illegal
  instruction, divide by zero) to a stub that raises an AMS error (`ER_throw`, line-1010 opcode
  `0xA006`) caught by that frame; its table has an offset bug (only the first three vectors are
  right) and it leaks the exception frame: do not copy it as is.
- Stack size: measure instead of guessing: fill the free stack with a pattern at `_main`, run the
  game, scan for the high-water mark. Never put screen-sized buffers on the stack (TI-Chess dialogs
  put 7.5 KB there, even from inside its recursive search; path.c puts 5.8 KB).
- Never `HeapUnlock` a handle you did not lock (`if (!HeapGetLock(h)) { HLock(h); mine = 1; }`):
  unlocking the running program lets the heap compressor move code that is executing
  (TICT-Explorer crash, history v0.26).

## 10. Files: save games, high scores, level packs

A variable's type is its **last byte** (`STR_TAG`, `PIC_TAG`, `OTH_TAG`…); an OTH ("other") file
carries a custom extension of up to 4 characters just before it. Data files are the clean way to
save; they survive archiving, unlike data stored in the program (§1).

Layout: `[size.w][data][0]["ext"][0][OTH_TAG]`, where the size word counts the bytes after it.
**Verified** round trip on HW2 and HW3 (`experiments/files/savetest.c`), including the archived case:

```c
#define EXT "sav"
#define DATA_LEN (sizeof(SAVE) + 1 + sizeof(EXT) + 1)          // data, 0, "sav\0", OTH_TAG

SYM_ENTRY *se = SymFindPtr(SYMSTR("tsave"), 0);              // load (works if archived too)
if (se) { const unsigned char *p = HeapDeref(se->handle);
          if (*(const unsigned short *)p == DATA_LEN && p[2 + DATA_LEN - 1] == OTH_TAG)
              memcpy(&save, p + 2, sizeof(SAVE)); }              // then check magic + version

if (se && se->flags.bits.archived)                            // save: unarchive FIRST
    EM_moveSymFromExtMem(SYMSTR("tsave"), HS_NULL);
HANDLE h = HeapAlloc(2 + DATA_LEN);  if (h == H_NULL) fail;
HSym hs = SymAdd(SYMSTR("tsave"));   if (hs.folder == 0) { HeapFree(h); fail; }  // replaces old var
DerefSym(hs)->handle = h;
unsigned char *p = HeapDeref(h);
*(unsigned short *)p = DATA_LEN;  memcpy(p + 2, &save, sizeof(SAVE));
p += 2 + sizeof(SAVE);  *p++ = 0;  memcpy(p, EXT, sizeof(EXT));  p[sizeof(EXT)] = OTH_TAG;
```

- **`SymAdd` over an archived variable keeps its archived flag** while you attach a RAM handle: the
  variable is then corrupt (VAR-LINK still shows it archived; the next unarchive fails with
  "Variable is locked, protected or archived"). **Verified**: always unarchive before rewriting.
- VAR-LINK shows the file with type `sav` (its extension) and its size.

**Simpler, also verified** (HW3, `experiments/files/fsave.c`; Xchange does this): stdio in binary
mode writes the size word itself, and the result is byte-for-byte the layout above:
```c
FILE *f = fopen("mydata", "wb");               // NOT the running program's own name (NULL)
fwrite(&save, sizeof save, 1, f);
fputc(0, f); fputs("sav", f); fputc(0, f); fputc(OTH_TAG, f);
fclose(f);                                      // → size word = sizeof save + 6
```
Unarchive first and archive after if needed, as above.
- Put a **magic number and a version** at the start of the data (`C4UL('B','K','W','0')`) and fall
  back to defaults when they do not match; check the length before reading.
- Keep every `short`/`long` field at an **even offset** (68000 address error otherwise).
- `symptr->flags.bits.hidden = 1` hides the file from VAR-LINK.
- Read in place with `HeapDeref` (works for archived files too); lock only if not already locked.
- Write saves **after the teardown** (grayscale off, vectors restored): `SymAdd` can open a
  "create folder?" dialog and archive operations can start a garbage-collect dialog, both need AMS.
  Archive afterwards (`EM_moveSymToExtMem`) if the save must survive a RAM reset.
- Finding your level packs anywhere: `SymFindFirst(NULL, 2)` / `SymFindNext()` walks every folder;
  recognise your files by extension and magic. Hold `FolderOp(NULL, FOP_ALL_FOLDERS | FOP_LOCK)`
  during the walk and restart it after any allocation: `SYM_ENTRY*` pointers die when the heap is
  compacted.
- Compact encodings (TI-Chess): 4-bit cells, 16-bit moves (from 6 | to 6 | extra), flags in spare
  top bits; store the start state plus the move list and replay it on load (it rebuilds undo
  history for free). More in `ti68k-game-techniques.md`.
- **Big content in a separate data file** (CalcRogue): all monster/item/map data and strings live
  in an OTH variable found and locked once at start-up; every cross-reference is an **offset** from
  the file start, never a pointer, so the file can be archived, read in place and moved. It keeps
  the program small, needs no DLL and is HW3-safe: the preferred way to grow a big game.
- General RLE (CalcRogue): escape byte + (value, count) for runs of any value, short runs written
  literally. Write struct arrays **field by field** (byte 0 of every record, then byte 1…) before
  RLE, so mostly-constant fields become long runs.
- **Large level/graphics data: ZX0 + `zx0_asm`** (**verified**, `games/campfire`: 20,776 bytes of
  tiles, sprites and fire → an 11,138-byte stream, program 24,365 → 15,457 bytes, unpacked in
  ~0.1 s at start-up; runs on the Titanium and on an unpatched TI-89). Shared files in `lib/`:
  `unpack68k.s` (asm decoders), `unpack68k.h` (prototypes), `zx0pack.py` (packer on the PC, checks
  the round trip with `tools/bin/dzx0`). Recipe:
  1. On the PC, pack the raw bytes exactly as the 68000 reads them (big-endian words/longs):
     `python3 lib/zx0pack.py level.bin zlevel.h zlevel` writes `zlevel[]` and `ZLEVEL_RAW_LEN`. For
     several arrays, write a small script that concatenates them, records each offset and calls
     `zx0pack.pack(raw)` / `zx0pack.c_array(name, packed)` (example: `games/campfire/tools/pack.py`,
     run after `extract.py`, turns `data.h` into `zdata.h`).
  2. In C: `#include "../../lib/unpack68k.h"`, `buf = malloc(ZLEVEL_RAW_LEN)`,
     `zx0_asm(zlevel, buf)`, then point the tile map, sprites… at `buf + offset` (store **offsets**
     in the header, not pointers; fill the pointer tables at start-up every run). Keep offsets even
     for word/long data.
  3. Build: `ti-cc -o name main.c ../../lib/unpack68k.s`.
  Pitfall: never name a struct field `off` (or another ROM-call name): tigcclib defines it as a
  macro. For data unpacked during play, LZ4 (`lz4_asm`) is ~3× faster to unpack but packs worse
  (game-techniques §13). Older route: `ttpack` (`tools/gcc4ti-bin/bin`) + ExtGraph's `UnpackBuffer`
  (`extgraph.h`, ttunpack), 2× slower than `zx0_asm` and 20 % bigger. For data dominated by one
  byte value, a 15-instruction RLE (literal bytes + `count, ESC` for runs of that value, ESC chosen
  to never appear) saved >10 % in TICT S1P6.

- **Archive layout** (gb68k readme, not tested): the archive is made of 64 KB Flash blocks and a
  variable cannot span two; 48 KB files leave 16 KB holes. Size big data files to pair up (gb68k
  uses 56 KB) or keep them small.
- **Auto-archive after saving** (ExciteBike, Sumo) so the save survives a crash; call
  `EM_findEmptySlot(size)` first to avoid the garbage-collection prompt. Their size argument is
  wrong (they pass one byte of the size word): pass the full `unsigned short` size + 2.
- **Standard AMS open / save-as dialogs**: `VarOpen`/`VarNew` (chip8-ti68k `startup.c`) give a file
  picker for OTH files for free (check the minimum AMS version before relying on them).
- **Text input under grayscale** (ExciteBike): `GraySetInt1Handler(ams_int1)` temporarily restores
  AMS's int 1 so `ngetchx`/`OSdequeue` work, then back to `DUMMY_HANDLER`.
- Save the RNG seed (`__randseed`) in save states: a save/load round trip stays deterministic
  (chip8-ti68k).
- **Never `memcpy` a range of struct fields to save settings** (Sumo, Hockey): padding between
  fields shifts everything (Sumo loses the low byte of its best score: 95 bytes copied, span 96,
  checked with `offsetof`). Write an explicit record with a magic number and a version.
- **Files found by scanning every variable for a magic word** at offset 2 (ExciteBike tracks, Sumo
  wrestlers): also check the OTH tag and the extension, lock the folder table (`FolderOp`) and bound
  the result array.
- **Text input inside a grayscale game**: `GrayOff`, restore AMS int 1/5, `DialogNewSimple` +
  `DialogAddRequest` + `DialogDo`, then reinstall everything (Sumo editor). Simpler than a home-made
  text field; `ngetchx`-based input with the 0x800 repeat flag masked is the alternative.
- Save and archive on exit only if something changed (Hockey rewrites and re-archives its config
  on every exit: Flash wear), and after restoring the interrupt vectors, not before (Sumo).
- **C → TI-Basic channel and program chaining** (Worms68k): a Basic launcher
  `Loop: wmenu(): If wormrun="1": Exit: EndIf: wgame(): EndLoop` runs a menu program and the game
  alternately; the C side writes the string variable `wormrun` to say "quit"
  (`fputc(0); fputc('1'); fputc(0); fputc(STR_TAG)` via `fopen("wormrun","wb")`), and settings pass
  through a data file. Splits a big game into several programs (each under 24 KB for AMS 2.xx) and
  lets Basic glue them. Not tested here.

## 11. Portability checklist (TI-89 HW2 ↔ Titanium)

- Recompile old sources; do not trust old `.89z` binaries (HW3 crashes).
- Compile once, test on both emulator profiles (`TI_CALC=89t`, `TI_CALC=89`).
- Use `HW_VERSION`, `PRG_getStart()`, `GetPlane()`, and enum colours: nothing hard-coded.
- Old code traps when porting: empty delay loops (deleted by the compiler, §6), ROM-base hardware
  detection (§4), ghost-space launchers and TSRs (§4), multi-line inline-asm string literals
  (GCC 4.x rejects some: use `\n` escapes or the ExtGraph routine), `return` inside a `TRY` block
  (leaks the error frame: leave through `ENDTRY`/`ENDFINAL`), fonts pulled out of the ROM at
  hard-coded AMS offsets (embed or pre-render your own, §2), ExtGraph 1.x colour values (§3).
- Heavy maths loops: C first. Only consider 68000 ASM after profiling, and **ask first** (project rule).
