// Micro-benchmarks of common operations on the TI-89 (68000).
// Timing: auto-int 1 (fixed 256 Hz on HW2/HW3) is replaced by a tick counter; auto-ints 2 (key
// press) and 5 (AMS timers) are silenced during the run, after waiting for all keys to be released.
// Each test runs n iterations, twice, and the best time is kept; the empty-loop cost per iteration is subtracted and the result is printed as
// approximate CPU cycles per operation, assuming a 12 MHz clock. Treat the numbers as relative
// costs. Inputs come from volatile globals and results go to a volatile sink, so the compiler
// cannot fold the work away.

#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>
#include "extgraph.h"

static volatile unsigned long ticks;
DEFINE_INT_HANDLER(tick_handler) { ticks++; }

volatile short vs = 7, vs2 = 3;
volatile long vl = 123457L, vl2 = 11L;
volatile float vf = 1.5f;
volatile long sink;

static unsigned char grid[16][30];
static unsigned char buf[LCD_SIZE];

typedef struct { const char *name; long n; } TEST;
static const TEST tests[] = {
    {"empty loop", 150000L}, {"mul 16x16", 150000L}, {"mul 32x32", 30000L},
    {"div 16/16", 50000L},  {"div 32/32", 15000L},  {"shift >>4", 150000L},
    {"float *,+", 1500L},    {"a[y][x]", 150000L},   {"*p++", 150000L},
    {"DrawPix", 15000L},     {"pixel dir.", 150000L},{"memcpy LCD", 100L},
    {"FastCopy_R", 100L},    {"memset LCD", 100L},   {"FastClr_R", 100L},
};
#define NTESTS (sizeof(tests) / sizeof(tests[0]))
static unsigned long t_ticks[NTESTS];

static unsigned long run(short which, long n)
{
    long i;
    long acc = 0;
    short a = vs, b = vs2;
    long la = vl, lb = vl2;
    float f = vf, g = 0;
    unsigned long start;

    start = ticks;
    while (ticks == start);             // align on a tick edge
    start = ticks;
    switch (which) {
    case 0:  for (i = 0; i < n; i++) acc += i; break;
    case 1:  for (i = 0; i < n; i++) acc += (long)(short)((short)i ^ a) * b; break;   // XOR: no strength reduction
    case 2:  for (i = 0; i < n; i++) acc += (la ^ i) * lb; break;
    case 3:  for (i = 0; i < n; i++) acc += (unsigned short)((short)i | 1024) / (unsigned short)b; break;
    case 4:  for (i = 0; i < n; i++) acc += (la ^ i) / lb; break;
    case 5:  for (i = 0; i < n; i++) acc += (la ^ i) >> 4; break;
    case 6:  for (i = 0; i < n; i++) g = g * f + f; acc = (long)g; break;   // loop-carried: no hoisting
    case 7:  for (i = 0; i < n; i++) acc += grid[(short)i & 15][((short)i >> 4) & 15]; break;
    case 8:  { unsigned char *p = &grid[0][0];
               for (i = 0; i < n; i++) { acc += *p++; if (p == &grid[16][0]) p = &grid[0][0]; } } break;
    case 9:  for (i = 0; i < n; i++) DrawPix((short)i & 127, (short)i & 63, A_XOR); break;
    case 10: for (i = 0; i < n; i++) { short x = (short)i & 127, y = (short)i & 63;
               ((unsigned char *)LCD_MEM)[y * 30 + (x >> 3)] ^= 0x80 >> (x & 7); } break;
    case 11: for (i = 0; i < n; i++) memcpy(buf, LCD_MEM, LCD_SIZE); break;
    case 12: for (i = 0; i < n; i++) FastCopyScreen_R(LCD_MEM, buf); break;
    case 13: for (i = 0; i < n; i++) memset(buf, 0, LCD_SIZE); break;
    case 14: for (i = 0; i < n; i++) FastClearScreen_R(buf); break;
    }
    sink = acc;
    return ticks - start;
}

void _main(void)
{
    INT_HANDLER old_int1 = GetIntVec(AUTO_INT_1);
    short k, line;
    long empty_cyc_x100;               // empty-loop cycles per iteration, x100

    INT_HANDLER old_int2 = GetIntVec(AUTO_INT_2), old_int5 = GetIntVec(AUTO_INT_5);

    ClrScr();
    printf_xy(0, 0, "Benchmarking, ~2 min...");
    while (_rowread(0)) ;               // wait until every key is released
    SetIntVec(AUTO_INT_2, DUMMY_HANDLER);
    SetIntVec(AUTO_INT_5, DUMMY_HANDLER);
    SetIntVec(AUTO_INT_1, tick_handler);
    for (k = 0; k < (short)NTESTS; k++) {
        unsigned long t1 = run(k, tests[k].n), t2 = run(k, tests[k].n);
        t_ticks[k] = t1 < t2 ? t1 : t2;
    }
    SetIntVec(AUTO_INT_1, old_int1);
    SetIntVec(AUTO_INT_5, old_int5);
    SetIntVec(AUTO_INT_2, old_int2);

    // cycles per iteration = ticks / 256 s * 12e6 / n = ticks * 46875 / n
    empty_cyc_x100 = (long)(t_ticks[0] * 4687500ULL / tests[0].n);
    ClrScr();
    FontSetSys(F_4x6);
    printf_xy(0, 0, "~cycles/op @12MHz (loop cost removed)");
    for (k = 0, line = 7; k < (short)NTESTS; k++, line += 6) {
        long c100 = (long)(t_ticks[k] * 4687500ULL / tests[k].n);
        if (k > 0 && k < 11) c100 -= empty_cyc_x100;
        printf_xy(0, line, "%-11s %7ld  (%lu t)", tests[k].name, c100 / 100, t_ticks[k]);
    }
    GKeyFlush();
    ngetchx();
}
