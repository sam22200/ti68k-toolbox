// Code in a heap block placed low in RAM, after big allocations (what a game's planes do first).
// Unpatched Titanium: freezes at such addresses (heapx/heapp). With HW3Patch it must return 42.
// Reads without screenshots: `ti-mem log` shows "call ADDR" then "ret 42" (tilog.h).
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>
#include "../../lib/tilog.h"

void _main(void)
{
    void *big[3]; unsigned short *code; short (*f)(void); short r; unsigned char i;
    for (i = 0; i < 3; i++) big[i] = HeapAllocPtr(50000);
    code = HeapAllocPtr(8650);                       // an FFT module zone's size
    if (!code) { tilog("heaplow: no block"); goto out; }
    code[0] = 0x702A;                                // moveq #42,d0
    code[1] = 0x4E75;                                // rts
    f = (void *)code;
    tilog("heaplow: HW %d call %lx", HW_VERSION, (long)f);
    r = f();
    tilog("heaplow: ret %d", r);
    HeapFreePtr(code);
out:
    for (i = 3; i-- > 0;) if (big[i]) HeapFreePtr(big[i]);
}
