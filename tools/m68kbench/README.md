# ti-cycles: a 68000 cycle counter for TI-89 programs

`tools/bin/ti-cycles prog.89z` runs a NOSTUB program on the PC under [Musashi](https://github.com/kstenerud/Musashi) (MIT, cloned in `tools/musashi/`, not in git). It needs no window or emulator, and its cycle counts follow the MC68000 datasheet.

Build it with `git clone https://github.com/kstenerud/Musashi tools/musashi && make -C tools/m68kbench`.

- **Program under test**: the program marks its zones with `bench.h` (`BENCH_BEGIN(id)`, `BENCH_END(id)`, `BENCH_NAME`, `BENCH_VALUE`, `BENCH_ARG`). `BENCH_SHOT(ptr)` saves a 240×128 two-plane screen from memory as a PNG (`--png F`).
- **AMS environment**: a ROM call table, `ScrRect`, an empty VAT and a host heap. The ROM calls a benchmark needs are emulated with an estimated cost, which is reported apart: `HeapAlloc`, `HLock`, `malloc`, `memcpy`, `memset`, `memcmp` and the 32-bit divisions. Drawing and text calls do nothing.
- **Hardware**: not emulated. Writes to the I/O ports are ignored with a warning, and CPU exceptions stop the run.
- **Self-test**: `test/cyctest.c` builds with `TI_CC_PLAIN=1 ti-cc -DUSE_TI89 -o cyctest cyctest.c`. `ti-cycles cyctest.89z` must print the datasheet counts plus 10 for the `dbra`: `nop` 4, `lsl.l #8` 24, `movem.l` of 10 registers 92, `mulu.w #$FFFF` 70.
- **Example**: `games/mode7/` (`make bench`, `tools/bench.py`).
