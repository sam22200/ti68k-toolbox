// Second micro-benchmark set: competing techniques found in old sources (same harness as bench.c).
// Pixel plotting (shift mask vs mask table vs ExtGraph EXT_SETPIX bset), text (AMS DrawStr vs a
// pre-rendered font drawn with one long XOR per row), full-screen scrolls (ExtGraph).
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>
#include "extgraph.h"

static volatile unsigned long ticks;
DEFINE_INT_HANDLER(tick_handler) { ticks++; }

volatile long sink;
static unsigned char buf[LCD_SIZE];
static unsigned char font[256 * 8];                 // F_6x8 glyphs, 1 byte per row, left-aligned
static const unsigned char mask_tab[8] = {0x80, 0x40, 0x20, 0x10, 8, 4, 2, 1};
static const char str20[] = "SCORE 0012345 LIVES3";

typedef struct { const char *name; long n; } TEST;
static const TEST tests[] = {
    {"empty loop", 150000L}, {"pix 80>>x&7", 100000L}, {"pix masktab", 100000L},
    {"EXT_SETPIX", 100000L}, {"DrawStr 20c", 1000L},   {"font XOR 20", 1000L},
    {"ScrollUp160", 300L},   {"ScrollLeft160", 300L},
};
#define NTESTS (sizeof(tests) / sizeof(tests[0]))
static unsigned long t_ticks[NTESTS];

// Draws a string with the pre-rendered font: one aligned long XOR per glyph row (any x, no split).
static void font_xor(unsigned char *plane, unsigned short x, unsigned short y, const char *s)
{
    unsigned char *row = plane + (y << 5) - (y << 1);
    unsigned char c;
    while ((c = *s++)) {
        const unsigned char *g = font + (c << 3);
        unsigned long *p = (unsigned long *)(row + ((x >> 3) & ~1));
        short sh = 24 - (x & 15);
        short i;
        for (i = 7; i >= 0; i--) {
            *p ^= (unsigned long)*g++ << sh;
            p = (unsigned long *)((unsigned char *)p + 30);
        }
        x += 6;
    }
}

static unsigned long run(short which, long n)
{
    long i;
    long acc = 0;
    unsigned long start;

    start = ticks;
    while (ticks == start);
    start = ticks;
    switch (which) {
    case 0: for (i = 0; i < n; i++) acc += i; break;
    case 1: for (i = 0; i < n; i++) { unsigned short x = (unsigned short)i & 127, y = ((unsigned short)i >> 7) & 63;
              buf[(y << 5) - (y << 1) + (x >> 3)] |= 0x80 >> (x & 7); } break;
    case 2: for (i = 0; i < n; i++) { unsigned short x = (unsigned short)i & 127, y = ((unsigned short)i >> 7) & 63;
              buf[(y << 5) - (y << 1) + (x >> 3)] |= mask_tab[x & 7]; } break;
    case 3: for (i = 0; i < n; i++) { unsigned short x = (unsigned short)i & 127, y = ((unsigned short)i >> 7) & 63;
              EXT_SETPIX(buf, x, y); } break;
    case 4: PortSet(buf, 239, 127);
            for (i = 0; i < n; i++) DrawStr(0, 40, str20, A_XOR);
            PortRestore(); break;
    case 5: for (i = 0; i < n; i++) font_xor(buf, 0, 40, str20); break;
    case 6: for (i = 0; i < n; i++) ScrollUp160_R((unsigned short *)buf, 100); break;
    case 7: for (i = 0; i < n; i++) ScrollLeft160_R((unsigned short *)buf, 100); break;
    }
    sink = acc + buf[1234];
    return ticks - start;
}

void _main(void)
{
    INT_HANDLER old_int1 = GetIntVec(AUTO_INT_1), old_int2 = GetIntVec(AUTO_INT_2),
                old_int5 = GetIntVec(AUTO_INT_5);
    short k, line;
    long empty_cyc_x100;

    FontSetSys(F_6x8);                              // pre-render the font once, with AMS
    PortSet(font, 7, 256 * 8 - 1);
    ClrScr();
    for (k = 0; k < 256; k++) DrawChar(0, k << 3, k, A_REPLACE);
    PortRestore();

    ClrScr();
    printf_xy(0, 0, "Benchmarking...");
    while (_rowread(0)) ;
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

    empty_cyc_x100 = (long)(t_ticks[0] * 4687500ULL / tests[0].n);
    ClrScr();
    FontSetSys(F_4x6);
    printf_xy(0, 0, "~cycles/op @12MHz (loop cost removed)");
    for (k = 0, line = 7; k < (short)NTESTS; k++, line += 6) {
        long c100 = (long)(t_ticks[k] * 4687500ULL / tests[k].n);
        if (k > 0) c100 -= empty_cyc_x100;
        printf_xy(0, line, "%-13s %7ld  (%lu t)", tests[k].name, c100 / 100, t_ticks[k]);
    }
    font_xor(LCD_MEM, 0, 60, "font_xor OK");      // visual check of the pre-rendered font
    GKeyFlush();
    ngetchx();
}
