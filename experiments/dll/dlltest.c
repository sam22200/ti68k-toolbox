// Can a NOSTUB program load a TIGCC DLL (mydll, the GCC4TI example) on HW2 and on the Titanium?
// Built twice: dlla (plain) and dllg (-DGHOST: EXECUTE_IN_GHOST_SPACE). Prints LoadDLL's code:
// 0 DLL_OK, 1 NOTINGHOSTSPACE, 2 NOTFOUND, 3 LOCKFAILED, 4 OUTOFMEM, 5 ALREADYLOADED, 6 WRONGVERSION
#define USE_TI89
#define SAVE_SCREEN
#ifdef GHOST
#define EXECUTE_IN_GHOST_SPACE
#endif
#include <tigcclib.h>

#define SumFromDLL _DLL_call_attr(int,(int,int),__attribute__((stkparm)),1)

void _main(void)
{
    short r = LoadDLL("mydll", 372377271, 2, 11);
    ClrScr();
    printf_xy(0, 0, "HW%d LoadDLL = %d", (short)HW_VERSION, r);
    if (r == DLL_OK) {
        printf_xy(0, 10, "2+3 = %d", SumFromDLL(2, 3));
        UnloadDLL();
    }
    ngetchx();
}
