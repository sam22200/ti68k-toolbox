// Frame limiter under grayscale: busy wait vs sleeping the CPU until the next interrupt with
// pokeIO(0x600005, mask) (bits 0-4 = auto-ints 1-5 that wake it; 0x1D skips int 2, the keyboard).
// 32 fps target (8 int-1 ticks at 256 Hz). Phase B lasts 6 s so a screenshot can be taken in it.
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>
#include "extgraph.h"

static volatile unsigned short ticks;
DEFINE_INT_HANDLER(tick_handler) { ticks++; }

static void draw(void *l, void *d, unsigned short f)
{
    unsigned short x = f & 127;
    FastClearScreen_R(l);
    FastClearScreen_R(d);
    GrayFastFillRect_R(l, d, x, 10, x + 30, 40, COLOR_DARKGRAY);
    GrayFastFillRect_R(l, d, 20, 50, 60, 80, COLOR_LIGHTGRAY);
    GrayFastFillRect_R(l, d, 100, 50, 140, 80, COLOR_BLACK);
}

static unsigned short run(short sleep, unsigned short len, unsigned long *spins, unsigned short *sw)
{
    unsigned short f = 0, last;
    unsigned long n = 0, sw0 = GrayGetSwitchCount();
    ticks = 0; last = 0;
    while (ticks < len) {
        draw(GrayDBufGetHiddenPlane(LIGHT_PLANE), GrayDBufGetHiddenPlane(DARK_PLANE), f++);
        GrayDBufToggle();
        while ((unsigned short)(ticks - last) < 8) { if (sleep) pokeIO(0x600005, 0x1D); n++; }
        last += 8;
    }
    *spins = n; *sw = GrayGetSwitchCount() - sw0;
    return f;
}

void _main(void)
{
    INT_HANDLER old_int1 = GetIntVec(AUTO_INT_1);
    void *dbuf = malloc(GRAYDBUFFER_SIZE);
    unsigned short fa = 0, fb = 0, swa = 0, swb = 0;
    unsigned long sa = 0, sb = 0;
    if (!dbuf) return;
    SetIntVec(AUTO_INT_1, DUMMY_HANDLER);
    if (GrayOn()) {
        GraySetInt1Handler(tick_handler);
        GrayDBufInit(dbuf);
        fa = run(0, 768, &sa, &swa);
        fb = run(1, 1536, &sb, &swb);
        GraySetInt1Handler(DUMMY_HANDLER);
        GrayOff();
    }
    SetIntVec(AUTO_INT_1, old_int1);
    free(dbuf);
    ClrScr();
    printf_xy(0, 0, "HW%d  32 fps target", (short)HW_VERSION);
    printf_xy(0, 10, "busy : %u fps, %lu spins/s", fa / 3, sa / 3);
    printf_xy(0, 20, "sleep: %u fps, %lu spins/s", fb / 6, sb / 6);
    printf_xy(0, 30, "switches/s: %u / %u", swa / 3, swb / 6);
    GKeyFlush();
    ngetchx();
}
