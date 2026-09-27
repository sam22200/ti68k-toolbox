// Hello World for the TI-89 / TI-89 Titanium, NOSTUB mode (no kernel needed)

#define USE_TI89        // target: TI-89 and TI-89 Titanium
#define OPTIMIZE_ROM_CALLS
#define SAVE_SCREEN     // restore the screen on exit

#include <tigcclib.h>

void _main(void)
{
    ClrScr();
    FontSetSys(F_8x10);
    DrawStr(10, 30, "Hello, World!", A_NORMAL);
    FontSetSys(F_6x8);
    DrawStr(10, 60, "Press any key!", A_NORMAL);
    ngetchx();
}
