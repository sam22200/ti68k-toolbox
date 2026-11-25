// What one module load costs: mod_install (games/fft/fft.c) copies the image out
// of the archive into the execution zone with memcpy, so the price of adding a
// module is one copy of its bytes. This times that copy for the sizes FFT's
// twenty-eight images actually have.
//
// Measured under ti-cycles (datasheet 68000 timings), which sees a flat memory:
// the figure is the RAM-to-RAM floor, Flash reads can only be slower.
#define USE_TI89
#include <tigcclib.h>
#include "../../tools/m68kbench/bench.h"

#define BIG 8932                /* MOD_ZONE: the largest image */
#define SMALL 4466              /* what halving it would give */

void _main(void)
{
    unsigned char *src = malloc(BIG), *dst = malloc(BIG);
    if (!src || !dst) { BENCH_VALUE(1); return; }
    memset(src, 0x5A, BIG);

    BENCH_BEGIN(0);
    memcpy(dst, src, BIG);
    BENCH_END(0);

    BENCH_BEGIN(1);
    memcpy(dst, src, SMALL);
    BENCH_END(1);

    /* A whole battle's worth if every one of the twenty-eight were loaded once. */
    BENCH_BEGIN(2);
    {
        short k;
        for (k = 0; k < 28; k++) memcpy(dst, src, BIG);
    }
    BENCH_END(2);

    BENCH_VALUE(dst[BIG - 1]);
    free(src); free(dst);
}
