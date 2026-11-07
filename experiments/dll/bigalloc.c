// A DLL called after big allocations (-DBIG=0..3 blocks of 50,000 bytes): on an unpatched
// Titanium the module lands low in RAM and its first call crashes (ti68k-c-patterns.md: DLLs).
#ifndef BIG
#define BIG 3
#endif
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>
#define SumFromDLL _DLL_call_attr(int,(int,int),__attribute__((stkparm)),1)
void _main(void)
{
    void *b[3] = {0, 0, 0}; short i, r;
    for (i = 0; i < BIG; i++) b[i] = malloc(50000);
    r = LoadDLL("mydll", 372377271, 2, 11);
    ClrScr();
    printf_xy(0, 0, "big%d LoadDLL=%d at %lx", BIG, r, r == DLL_OK ? (unsigned long)_DLL_entry(1) : 0UL);
    ngetchx();
    if (r == DLL_OK) { printf_xy(0, 10, "2+3=%d", SumFromDLL(2, 3)); ngetchx(); UnloadDLL(); }
    for (i = 0; i < 3; i++) if (b[i]) free(b[i]);
}
