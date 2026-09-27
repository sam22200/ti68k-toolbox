// Shows int-1 and int-5 tick counters live, to time them against a wall clock (screenshots).
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>

static volatile unsigned long t1, t5;
DEFINE_INT_HANDLER(h1) { t1++; }
DEFINE_INT_HANDLER(h5) { t5++; }

void _main(void)
{
    INT_HANDLER o1 = GetIntVec(AUTO_INT_1), o5 = GetIntVec(AUTO_INT_5);
    ClrScr();
    SetIntVec(AUTO_INT_1, h1);
    SetIntVec(AUTO_INT_5, h5);
    while (!_keytest(RR_ESC))
        printf_xy(0, 20, "int1 %lu  int5 %lu   ", t1, t5);
    SetIntVec(AUTO_INT_5, o5);
    SetIntVec(AUTO_INT_1, o1);
    GKeyFlush();
}
