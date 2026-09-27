// Grayscale double buffering: GrayDBuf (pointer swap, synced to the plane switch) vs drawing into
// two RAM planes and copying them with FastCopyScreen_R. Counts frames drawn in 3 s of int-1 ticks
// (a counting handler chained by the grayscale code through GraySetInt1Handler).
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>
#include "extgraph.h"

static volatile unsigned short ticks, t5;
DEFINE_INT_HANDLER(tick_handler) { ticks++; }
DEFINE_INT_HANDLER(t5_handler) { t5++; }   // independent clock (int 5, AMS default rate)

static void draw(void *l, void *d, unsigned short f)
{
    unsigned short x = f & 127, y = (f >> 1) & 63;
    FastClearScreen_R(l);
    FastClearScreen_R(d);
    GrayFastFillRect_R(l, d, x, 10, x + 30, 40, COLOR_DARKGRAY);
    GrayFastFillRect_R(l, d, 20, y, 60, y + 30, COLOR_LIGHTGRAY);
}

void _main(void)
{
    INT_HANDLER old_int1 = GetIntVec(AUTO_INT_1), old_int5 = GetIntVec(AUTO_INT_5);
    void *dbuf = malloc(GRAYDBUFFER_SIZE);
    unsigned char *ram = malloc(2 * LCD_SIZE);
    unsigned short f_dbuf = 0, f_copy = 0, sw_dbuf = 0, t5_copy = 0, t5_dbuf = 0;
    unsigned long sw0;

    if (!dbuf || !ram) goto out;
    SetIntVec(AUTO_INT_1, DUMMY_HANDLER);
    SetIntVec(AUTO_INT_5, t5_handler);
    if (!GrayOn()) goto out;
    GraySetInt1Handler(tick_handler);

    // A: RAM planes + 2 copies (no sync)
    ticks = 0; t5 = 0;
    while (ticks < 768) {
        draw(ram, ram + LCD_SIZE, f_copy++);
        FastCopyScreen_R(ram, GrayGetPlane(LIGHT_PLANE));
        FastCopyScreen_R(ram + LCD_SIZE, GrayGetPlane(DARK_PLANE));
    }
    t5_copy = t5;
    // B: GrayDBuf
    GrayDBufInit(dbuf);
    ticks = 0; t5 = 0; sw0 = GrayGetSwitchCount();
    while (ticks < 768) {
        draw(GrayDBufGetHiddenPlane(LIGHT_PLANE), GrayDBufGetHiddenPlane(DARK_PLANE), f_dbuf++);
        GrayDBufToggleSync();
    }
    t5_dbuf = t5;
    sw_dbuf = GrayGetSwitchCount() - sw0;
    GraySetInt1Handler(DUMMY_HANDLER);
    GrayOff();
out:
    SetIntVec(AUTO_INT_5, old_int5);
    SetIntVec(AUTO_INT_1, old_int1);
    free(ram); free(dbuf);
    ClrScr();
    printf_xy(0, 0, "HW%d, frames in 3 s:", (short)HW_VERSION);
    printf_xy(0, 10, "RAM+2 copies: %u (%u fps)", f_copy, f_copy / 3);
    printf_xy(0, 20, "GrayDBuf    : %u (%u fps)", f_dbuf, f_dbuf / 3);
    printf_xy(0, 30, "plane switches: %u (%u/s)", sw_dbuf, sw_dbuf / 3);
    printf_xy(0, 40, "int5 ticks: %u / %u (19.3/s)", t5_copy, t5_dbuf);
    GKeyFlush();
    ngetchx();
}
