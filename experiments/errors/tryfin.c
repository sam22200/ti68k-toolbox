// Does TRY/FINALLY restore grayscale and vectors when an AMS error is thrown mid-game?
// -DODD: provoke a CPU Address Error (word write at an odd address) instead of an AMS error.
#define USE_TI89
#define SAVE_SCREEN
#define ENABLE_ERROR_RETURN   // the error rethrown by ENDFINAL reaches AMS (error dialog)
#include <tigcclib.h>

static volatile short *odd;

void _main(void)
{
    INT_HANDLER old1 = GetIntVec(AUTO_INT_1);
    SetIntVec(AUTO_INT_1, DUMMY_HANDLER);
    TRY
        GrayOn();
        memset(GetPlane(DARK_PLANE), 0x55, LCD_SIZE);
        { unsigned short t = 0; while (++t < 30000) ; }     // (loop kept: volatile-free, just a pause)
#ifdef ODD
        odd = (volatile short *)((char *)GetPlane(LIGHT_PLANE) + 1);
        *odd = -1;                                        // Address Error
#else
        ER_throw(ER_MEMORY);                              // what a failing ROM call does
#endif
    FINALLY
        GrayOff();
        SetIntVec(AUTO_INT_1, old1);
    ENDFINAL
}
