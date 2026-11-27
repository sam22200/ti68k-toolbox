// Darkening a four-grey scene by one or two levels, for a "focus" effect: everything
// dimmed except what the game wants looked at. runtime/core/rt.h: grey = light + 2*dark,
// RT_PBYTES 30, and a 160-pixel line is the first 20 bytes of each 30-byte row.
//
// Both maps are pure bitwise, no LUT and no per-pixel branch (checked exhaustively over
// the four levels, see the table below):
//     Dark 1   v -> min(v+1, 3)          d' = d | l      l' = d | ~l
//     Dark 2   0,1 -> 2 ; 2,3 -> 3       d' = ~0         l' = d
//
//     level  light dark   Dark 1   Dark 2
//       0      0    0       1        2
//       1      1    0       2        2
//       2      0    1       3        3
//       3      1    1       3        3
//
// Measured 2026-11-27, ti-cycles, MC68000 datasheet cycles, whole 160x100 both planes:
//
//     Dark 1, indexed inner loop          49212
//     Dark 1, post-increment pointers     51812   <- SLOWER than indexing
//     Dark 1, row unrolled                46638
//     Dark 2, indexed inner loop          40412
//     Dark 2, row unrolled                25234
//     Dark 1, flat over the 3000-byte plane   70534
//     Dark 2, flat over the 3000-byte plane   45034
//
// Three things worth keeping:
//  - pointers lose to indexing on a five-iteration inner loop. The usual rule
//    ("pointers instead of indexing") does not amortise that short.
//  - unrolling the row is worth 5% on Dark 1 and 38% on Dark 2.
//  - darkening the full 30-byte stride to get a flat loop costs more than the row
//    arithmetic it saves: 50% more data beats the simpler loop. Clip to the visible 20.
//
// A quarter of the rows is 11660 cycles for Dark 1, which is about what FFT already
// pays to keep a quarter of its screen copy (SAVE_K, 13 kilocycles), so the dimming
// can ride along with that existing pass and spread over four frames.
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#include "../../tools/m68kbench/bench.h"

#define ROWS   100
#define STRIDE 30               /* physical plane row */
#define VIS    20               /* the 160 visible pixels of it */
#define PBYTES (STRIDE * ROWS)  /* 3000 */

static unsigned char pl[PBYTES], pd[PBYTES];

static void fill(void)
{
    unsigned short i;
    for (i = 0; i < PBYTES; i++) { pl[i] = (unsigned char)(i * 7); pd[i] = (unsigned char)(i * 13); }
}
static unsigned long sum(void)
{
    unsigned short i; unsigned long s = 17;
    for (i = 0; i < PBYTES; i++) s = (s << 1 | s >> 31) ^ pl[i] ^ pd[i];
    return s;
}

/* visible window only: 5 longs a row, then skip the 10 off-screen bytes */
static void dark1_rows(void)
{
    unsigned short y, i;
    unsigned char *lp = pl, *dp = pd;
    for (y = 0; y < ROWS; y++) {
        unsigned long *l = (unsigned long *)lp, *d = (unsigned long *)dp;
        for (i = 0; i < VIS / 4; i++) { unsigned long a = l[i], b = d[i]; d[i] = b | a; l[i] = b | ~a; }
        lp += STRIDE; dp += STRIDE;
    }
}
static void dark2_rows(void)
{
    unsigned short y, i;
    unsigned char *lp = pl, *dp = pd;
    for (y = 0; y < ROWS; y++) {
        unsigned long *l = (unsigned long *)lp, *d = (unsigned long *)dp;
        for (i = 0; i < VIS / 4; i++) { l[i] = d[i]; d[i] = 0xFFFFFFFFUL; }
        lp += STRIDE; dp += STRIDE;
    }
}
/* whole plane, no row arithmetic: 1.5x the data, a flat loop */
static void dark1_flat(void)
{
    unsigned short i;
    unsigned long *l = (unsigned long *)pl, *d = (unsigned long *)pd;
    for (i = 0; i < PBYTES / 4; i++) { unsigned long a = l[i], b = d[i]; d[i] = b | a; l[i] = b | ~a; }
}
static void dark2_flat(void)
{
    unsigned short i;
    unsigned long *l = (unsigned long *)pl, *d = (unsigned long *)pd;
    for (i = 0; i < PBYTES / 4; i++) { l[i] = d[i]; d[i] = 0xFFFFFFFFUL; }
}

/* CLAUDE.md: pointers instead of indexing. Same maps, post-increment. */
static void dark1_ptr(void)
{
    unsigned short y;
    unsigned char *lp = pl, *dp = pd;
    for (y = 0; y < ROWS; y++) {
        unsigned long *l = (unsigned long *)lp, *d = (unsigned long *)dp;
        unsigned long *end = l + VIS / 4;
        while (l < end) { unsigned long a = *l, b = *d; *d++ = b | a; *l++ = b | ~a; }
        lp += STRIDE; dp += STRIDE;
    }
}
/* the five longs of a row written out: no inner loop at all */
static void dark1_unroll(void)
{
    unsigned short y;
    unsigned char *lp = pl, *dp = pd;
    for (y = 0; y < ROWS; y++) {
        unsigned long *l = (unsigned long *)lp, *d = (unsigned long *)dp;
        unsigned long a0 = l[0], a1 = l[1], a2 = l[2], a3 = l[3], a4 = l[4];
        unsigned long b0 = d[0], b1 = d[1], b2 = d[2], b3 = d[3], b4 = d[4];
        d[0] = b0 | a0; d[1] = b1 | a1; d[2] = b2 | a2; d[3] = b3 | a3; d[4] = b4 | a4;
        l[0] = b0 | ~a0; l[1] = b1 | ~a1; l[2] = b2 | ~a2; l[3] = b3 | ~a3; l[4] = b4 | ~a4;
        lp += STRIDE; dp += STRIDE;
    }
}
static void dark2_unroll(void)
{
    unsigned short y;
    unsigned char *lp = pl, *dp = pd;
    for (y = 0; y < ROWS; y++) {
        unsigned long *l = (unsigned long *)lp, *d = (unsigned long *)dp;
        l[0] = d[0]; l[1] = d[1]; l[2] = d[2]; l[3] = d[3]; l[4] = d[4];
        d[0] = d[1] = d[2] = d[3] = d[4] = 0xFFFFFFFFUL;
        lp += STRIDE; dp += STRIDE;
    }
}

void _main(void)
{
    BENCH_NAME(1, "dark1 visible 160x100");
    BENCH_NAME(2, "dark2 visible 160x100");
    BENCH_NAME(3, "dark1 flat 3000B plane");
    BENCH_NAME(4, "dark2 flat 3000B plane");
    fill(); BENCH_BEGIN(1); dark1_rows(); BENCH_END(1); BENCH_VALUE(sum());
    fill(); BENCH_BEGIN(2); dark2_rows(); BENCH_END(2); BENCH_VALUE(sum());
    fill(); BENCH_BEGIN(3); dark1_flat(); BENCH_END(3); BENCH_VALUE(sum());
    fill(); BENCH_BEGIN(4); dark2_flat(); BENCH_END(4); BENCH_VALUE(sum());
    BENCH_NAME(5, "dark1 visible, pointers");
    BENCH_NAME(6, "dark1 visible, unrolled");
    BENCH_NAME(7, "dark2 visible, unrolled");
    fill(); BENCH_BEGIN(5); dark1_ptr();    BENCH_END(5); BENCH_VALUE(sum());
    fill(); BENCH_BEGIN(6); dark1_unroll(); BENCH_END(6); BENCH_VALUE(sum());
    fill(); BENCH_BEGIN(7); dark2_unroll(); BENCH_END(7); BENCH_VALUE(sum());
}
