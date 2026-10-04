# The TI-83 / TI-83+ / TI-84+ as a port sees it

Facts a port needs, and what each maps to on the TI-89 Titanium. Checked on Desolate (TI-83,
Venus) with `scripts/ti83run.py` unless marked otherwise.

## 1. Machine

| | TI-83 | TI-83+ | TI-83+ SE / TI-84+ (SE) | TI-89 Titanium |
|---|---|---|---|---|
| CPU | Z80 6 MHz | Z80 6 MHz | Z80 15 MHz (6 MHz by default) | 68000 12 MHz |
| Screen | 96×64 mono | same | same | 160×100, 4 greys (GrayDBuf) |
| RAM for a program | ~24 KB | ~24 KB, 8.8 KB exec limit (C000) | same | program 64 KB, heap |
| Keys | 50 | 50 | 50 | 50 + |

A Z80 instruction takes 4-23 T-states; one 68000 cycle does about what one Z80 T-state does,
so a TI-83 game's logic ported to C fits easily. What costs is the screen: 160×100 is 2.6
times the pixels.

## 2. Program files and load addresses

- `.83p` (TI-83) and `.8xp` (TI-83+/84+): header `**TI83**` / `**TI83F*`, a variable entry
  (header length, data length, type 06 = protected program, name), then the data: a size
  word and the program bytes. `scripts/ti83var.py` reads both and finds the load address
  by scoring (absolute CALL/JP targets landing on plausible code).
- Load addresses: TI-83 `Send(9prgm` runs at **9327**; Venus programs (`E7 "9_[V?" 00`,
  a BASIC stub calling `prgmθV`, then a description string) run at **9329**; TI-83+ asm
  programs start with the token pair `BB 6D` (AsmPrgm) and are assembled at **9D93** (code at
  9D95, `userMem`); MirageOS and Ion add a header after that (a `ret`/`jr`, then a
  description and an icon).
- A program larger than the free RAM is split: a data variable in the archive (Desolate 83+:
  `DesData`), a shell-specific way to find it. The TI-83 version of a game is often one file
  (no archive on the TI-83): prefer it.
- The game's variables usually live **inside the program image** (writeback by the shell
  keeps them: that is how saves work in many games) or in the safe RAM areas below.

## 3. Memory map (TI-83 / TI-83+)

| | TI-83 | TI-83+ |
|---|---|---|
| ROM calls | `call addr` (0000-7FFF) | `rst 28h` + word (`bcall`) |
| plotSScreen (graph buffer, 768 B) | 8E29 | 9340 |
| saveSScreen (768 B) | 8265 | 86EC |
| appBackUpScreen | n/a | 9872 |
| textShadow (128 B) / cmdShadow | 80C9 / 9157 | 8508 / 966E |
| OP1..OP6 (11 B each) | 8039.. | 8478.. |
| penCol / penRow (small font) | 8252 / 8253 | 86D7 / 86D8 |
| curRow / curCol (large font) | 800C / 800D | 844B / 844C |
| flags (IY) | 89F0 | 89F0 |

Screen buffers are 12 bytes per row, 64 rows, MSB = leftmost pixel: the same bit order as
the TI-89 planes (30 bytes per row there): a row copies with an offset, no bit reversal.

## 4. Hardware a game touches

- **LCD (T6A04)**: port 10h command, 11h data. 01 = 8-bit mode, 05 = auto-increment
  row (X), 07 = auto-increment column (Y), 20h+c = column (8-pixel group), 80h+r = row,
  40h+z = scroll, C0h+ = contrast. Real hardware needs a delay between writes (games use
  `ionFastCopy`-style loops tuned for it). The runner never reports busy.
- **Keypad**: port 1: write a group mask (active low), read the 8 keys of the selected groups
  (active low). Groups: 0 arrows (down left right up), 1 enter + - × ÷ ^ clear, 2 (-) 3 6 9 )
  tan vars, 3 . 2 5 8 ( cos prgm stat, 4 0 1 4 7 , sin matrx X, 5 sto ln log x² x⁻¹ math
  alpha, 6 graph trace zoom window y= 2nd mode del. `_GetCSC` codes: down 1, left 2,
  right 3, up 4, enter 9, clear 0F, 2nd 36, mode 37, del 38, alpha 30, X,T,θ,n 28.
  Many games have their own GetCSC (Desolate 964C: edge-triggered, returns the code once).
- **Interrupts**: IM 1 (the OS handler at 0038: keys, APD, cursor) or IM 2 with a 257-byte
  vector table (Desolate: I = 86, 8600-8700 filled with 87, `jp handler` at 8787). Port 3 =
  interrupt mask, port 4 = timer speed (TI-83: the game asks the user; ~100-200 Hz).
- **Grayscale**: no grey hardware: the interrupt alternates or dithers two (or three) buffers
  on the LCD. Durk Kingma's package (Desolate): two buffers mixed with rotating masks (92/6D,
  49/B6, 24/DB) so a pixel set in the "dark" buffer shows 2/3 of the time and in the "light"
  one 1/3: four levels. The interrupt copies 768 bytes per tick (~59k T-states): **a grey
  TI-83 game spends most of its CPU in the interrupt**, the logic loop is slow (Desolate's
  menu loop: ~40 ticks per iteration at 140 Hz in the runner). The runner averages the LCD
  over the last 6 ticks: grey planes come out as levels 0-3. On the TI-89: GrayDBuf, the
  dark buffer → dark plane, the light buffer → light plane (check the weights per game).
- **Link port** (port 0): rarely used by games (two-player); not ported.

## 5. OS and shell routines

- TI-83 ROM calls are plain `call`s into 0000-7FFF; TI-83+ ones are `rst 28h` followed by the
  routine number (`bcall`). The names come from `ti83asm.inc` / `ti83plus.inc` (spasm-ng
  `inc/`: fetched next to `scripts/ti83var.py`, not committed).
- Usual ones: `_vputs`/`_vputmap` (small variable-width font at penCol/penRow, into
  plotSScreen when `textWrite` (IY+14h bit 7 on the 83) is set, else the LCD), `_puts`/`_putc`
  (homescreen 6×8 font), `_clrLCDFull`, `_homeUp`, `_GetCSC`, `_GetKey` (blocking, needs the
  OS interrupt), `_DispOP1A`/`_SetXXOP1` (numbers through the floating-point OPs),
  `_cphlde`, `_grbufcpy` (copy plotSScreen to the LCD), `_ChkFindSym` (find a variable).
- Shell libraries: Ion (`ionFastCopy`, `ionRandom`, `ionPutSprite`, `ionLargeSprite`,
  `ionDecompress`, `ionDetect`, `ionVersion`), MirageOS (Ion's plus its own), Venus (Ion-like,
  resident in high RAM: `vRandom` at FE72 returns HL random). The runner breaks on every
  address above the program: add the ones a game calls to `SHELL` in `ti83run.py`.
- Nothing of the OS is ported: text becomes the runtime's fonts (`F_SMALL` is the AMS 4×6,
  close to the TI-83 small font), numbers `sprintf`-free formatting, the key routines
  `input_*`.

## 6. Data formats met in TI-83 games

- **Sprites**: 8×8 1bpp (`ionPutSprite`, XOR or OR), masked as two 8-byte arrays, "large"
  sprites as rows × byte-width; grey sprites as two layers (dark, light). Convert to ExtGraph
  planes at build time.
- **Maps**: tile index arrays (12×8 tiles of 8×8 fill the screen), often RLE (Joe Pemberton's
  RLE: a marker byte, then value and count), sometimes nibble-packed.
- **Text**: plain zero-terminated strings, or compressed (Huffman, Kerey Roper's routine:
  Desolate) or a token dictionary. Decode once with a Python copy of the routine (or run the
  routine itself in `ti83run.py` and read the buffer) and keep the text as C strings or in
  the data file.
- **Saves**: the variables written into the program image (writeback) or a separate
  AppVar/program. Port as `rt_save`.

## 7. The runner (`scripts/ti83run.py`)

Z80 core: the `z80` package (C, `pip install z80`); ~15,000 ticks of a grey game per 10 s.
No ROM: every address below 8000 and above the program end is a breakpoint; known routines
are Python (`ROM`, `BCALL`, `SHELL` tables), unknown ones return at once and are logged once
(`unknown ROM call 4xxx`): a game that misbehaves usually needs one of them written. The
fonts are the TI-89's (AMS), so text widths differ slightly from a real TI-83. The timer rate
(`--rate`, default 140 Hz) sets how much CPU each tick gives: grey games spend most of it in
the interrupt, so menus advance slowly and keys must be held 30+ ticks. Known gaps:
`_DispOP1A` draws in the large font on the LCD (a game drawing it in the small font into a
buffer shows nothing there: Desolate's HP); after a `--poke` that changes the screen, wait
~150 ticks before a shot; compare the game's own buffers in a `--dump` rather than the
averaged LCD when a pixel-exact check matters.
