/* ti-cycles self-test: loops of one instruction, compared with the MC68000 datasheet. */
#include <tigcclib.h>
#include "../bench.h"
static char buf[64];
#define LOOP(id, name, body) do { BENCH_NAME(id, name); BENCH_BEGIN(id); \
    asm volatile("move.w #99,%%d2\n0:\n" body "\n\tdbra %%d2,0b" : : "a"(buf) : "d0","d1","d2","d3","d4","d5","d6","d7","a2","a3","a4","a5","memory"); BENCH_END(id); } while (0)
void _main(void)
{
    LOOP(1, "dbra only (10)", "");
    LOOP(2, "nop (4)", "nop");
    LOOP(3, "lsl.l #8 (24)", "lsl.l #8,%%d0");
    LOOP(4, "lsl.w #7 (20)", "lsl.w #7,%%d0");
    LOOP(5, "movem.l 10 regs (92)", "movem.l (%0),%%d1/%%d3-%%d7/%%a2-%%a5");
    LOOP(6, "mulu.w #0xFFFF (70)", "mulu.w #0xFFFF,%%d0");
    LOOP(7, "mulu.w #0 (38+4)", "mulu.w #0,%%d0");
    LOOP(8, "move.b (a0,d1.w),d0 (14)", "move.b (%0,%%d1.w),%%d0");
    LOOP(9, "divu.w #7 (140 max)", "move.l #1000,%%d0\n\tdivu.w #7,%%d0");
}
