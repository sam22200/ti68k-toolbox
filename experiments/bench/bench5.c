// Re-measure full-screen copies/clears over 1000 calls (bench.c used 100: too few ticks).
// A 68000 needs >= 4 cycles per bus word: a 3840-byte copy cannot take less than 15,360 cycles.
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>
#include "extgraph.h"

static volatile unsigned long ticks;
DEFINE_INT_HANDLER(tick_handler) { ticks++; }

void _main(void)
{
    INT_HANDLER o1 = GetIntVec(AUTO_INT_1), o5 = GetIntVec(AUTO_INT_5);
    static const char *const nm[5] = { "memcpy", "FastCopy_R", "memset", "FastClear_R", "long loop" };
    unsigned long t[5], s;
    unsigned char *buf = malloc(LCD_SIZE);
    short k, i;
    if (!buf) return;
    ClrScr(); printf_xy(0, 0, "Benchmarking...");
    SetIntVec(AUTO_INT_5, DUMMY_HANDLER);
    SetIntVec(AUTO_INT_1, tick_handler);
    for (k = 0; k < 5; k++) {
        s = ticks; while (ticks == s); s = ticks;
        for (i = 0; i < 1000; i++) {
            if (k == 0) memcpy(buf, LCD_MEM, LCD_SIZE);
            else if (k == 1) FastCopyScreen_R(LCD_MEM, buf);
            else if (k == 2) memset(buf, 0, LCD_SIZE);
            else if (k == 3) FastClearScreen_R(buf);
            else { const unsigned long *a = (const unsigned long *)LCD_MEM; unsigned long *b = (unsigned long *)buf; short j;
                   for (j = LCD_SIZE / 4 - 1; j >= 0; j--) *b++ = *a++; }
        }
        t[k] = ticks - s;
    }
    SetIntVec(AUTO_INT_1, o1); SetIntVec(AUTO_INT_5, o5);
    free(buf);
    ClrScr();
    printf_xy(0, 0, "cycles per call @12MHz (1000 calls)");
    for (k = 0; k < 5; k++) printf_xy(0, 10 + 10 * k, "%-11s %6lu (%lu t)", nm[k], t[k] * 46875UL / 1000, t[k]);
    GKeyFlush();
    ngetchx();
}
