// Measures the auto-int 5 rate for several PRG start values, against auto-int 1 as reference
// (256 Hz on HW2/HW3 if TiEmu and the docs are right; the total int-1 count is printed so the
// run can also be timed with a wall clock).
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>

static volatile unsigned short t1, t5;
DEFINE_INT_HANDLER(h1) { t1++; }
DEFINE_INT_HANDLER(h5) { t5++; }

static const unsigned char starts[] = {0, 0xB2, 0xCC, 0xF2, 0xF7, 0xFC};   // 0 = AMS default
#define N (sizeof(starts))

void _main(void)
{
    INT_HANDLER o1 = GetIntVec(AUTO_INT_1), o5 = GetIntVec(AUTO_INT_5);
    unsigned char def = PRG_getStart();
    unsigned short res[N];
    unsigned short total = 0;
    short k;

    ClrScr();
    printf_xy(0, 0, "Measuring, ~12 s...");
    SetIntVec(AUTO_INT_1, h1);
    SetIntVec(AUTO_INT_5, h5);
    for (k = 0; k < (short)N; k++) {
        PRG_setStart(starts[k] ? starts[k] : def);
        t1 = 0; while (t1 < 16);          // settle
        t1 = 0; t5 = 0;
        while (t1 < 512);                 // 2 s at 256 Hz
        res[k] = t5;
        total += 512 + 16;
    }
    PRG_setStart(def);
    SetIntVec(AUTO_INT_5, o5);
    SetIntVec(AUTO_INT_1, o1);

    ClrScr();
    printf_xy(0, 0, "HW%d default 0x%02X", (short)HW_VERSION, (short)def);
    printf_xy(0, 8, "int5 ticks / 512 int1 ticks:");
    for (k = 0; k < (short)N; k++)
        printf_xy(0, 18 + 8 * k, "0x%02X: %u  (%u.%u Hz)", (short)(starts[k] ? starts[k] : def),
                  res[k], res[k] / 2, (res[k] & 1) * 5);
    printf_xy(0, 70, "int1 total %u. DONE", total);
    ngetchx();
}
