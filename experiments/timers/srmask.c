// Does ngetchx() still work while OSSetSR(0x0400) masks interrupt levels 1-4?
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>

void _main(void)
{
    short old, key;
    ClrScr();
    printf_xy(0, 0, "SR masked, press a key");
    old = OSSetSR(0x0400);
    key = ngetchx();
    OSSetSR(old);
    printf_xy(0, 10, "got key %d, old SR %04X", key, (unsigned short)old);
    printf_xy(0, 20, "press again");
    ngetchx();
}
