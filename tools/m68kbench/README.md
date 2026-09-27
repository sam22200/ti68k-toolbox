# ti-cycles: a 68000 cycle counter for TI-89 programs

`tools/bin/ti-cycles prog.89z` runs a NOSTUB program on the PC under [Musashi](https://github.com/kstenerud/Musashi) (MIT, cloned in `tools/musashi/`, not in git). It needs no window or emulator, and its cycle counts follow the MC68000 datasheet.

Build it with `git clone https://github.com/kstenerud/Musashi tools/musashi && make -C tools/m68kbench`.

- **Program under test**: the program marks its zones with `bench.h` (`BENCH_BEGIN(id)`, `BENCH_END(id)`, `BENCH_NAME`, `BENCH_VALUE`, `BENCH_ARG`). `BENCH_SHOT(ptr)` saves a 240×128 two-plane screen from memory as a PNG (`--png F`).
- **AMS environment**: a ROM call table, `ScrRect`, a VAT and a host heap. The ROM calls a benchmark needs are emulated with an estimated cost, which is reported apart: `HeapAlloc`, `HLock`, `malloc`, `memcpy`, `memset`, `memcmp`, `strlen`, the 32-bit divisions, `SymFindPtr`, `SymAdd`, `DerefSym`, `EM_moveSym*`, `OO_CondGetAttr` (the AMS fonts, from `runtime/platform-sw/amsfont.h` when it exists at build time), `FontGetSys`. Drawing and text calls do nothing.
- **Files**: `--file F.89y` (repeatable) puts that variable in the VAT, archived, where `SymFindPtr` finds it. `--save-dir D` writes the variables the program created with `SymAdd` as `D/NAME.89y`, byte-identical to `ttbin2oth`, so a save can be fed back with `--file`.
- **Input and frames**: `--keys F` (the PC script format, `<frame> <keys...>`) answers `BENCH_KEYS(frame)` with the rt.h key mask; `--frames N` answers `BENCH_FRAMES`.
- **Screen checksum**: each `BENCH_SHOT` prints `shot N checksum XXXX`, the Fletcher-16 of the 160×100 view as the PC `--headless` run prints it.
- **Portable Game Runtime games**: `make cycles` builds `NAMEc.89z` (`-DRT_CYCLES`: no grayscale, interrupts or keyboard; the PC headless frame loop, zones 1 update and 2 render). `make xcheck` runs it against `NAME_pc --headless` for the scenarios of `XCHECK` (`FRAMES`, `KEYS`, `TI_FILES`) and fails on a different checksum. Verified on FFA (18 scenarios, a key script, save then Continue), life, flappy and the runtime demo: every checksum identical.
- **Hardware**: not emulated. Writes to the I/O ports are ignored with a warning, and CPU exceptions stop the run.
- **Self-test**: `test/cyctest.c` builds with `TI_CC_PLAIN=1 ti-cc -DUSE_TI89 -o cyctest cyctest.c`. `ti-cycles cyctest.89z` must print the datasheet counts plus 10 for the `dbra`: `nop` 4, `lsl.l #8` 24, `movem.l` of 10 registers 92, `mulu.w #$FFFF` 70.
- **Examples**: `games/mode7/` (`make bench`, `tools/bench.py`); `games/ffa/` (`make xcheck`).
