// Print hardware/timer facts: HW version, AMS version, auto-int 5 (PRG) start value and rate.
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>

void _main(void)
{
    ClrScr();
    FontSetSys(F_6x8);
    printf_xy(0, 0, "HW_VERSION : %d", (int)HW_VERSION);
    printf_xy(0, 10, "AMS        : %s", (const char *)(AMS_1xx ? "1.xx" : AMS_2xx ? "2.xx" : "3.xx"));
    printf_xy(0, 20, "PRG start  : 0x%02X", (int)PRG_getStart());
    printf_xy(0, 30, "PRG rate   : %d", (int)PRG_getRate());
    ngetchx();
}
