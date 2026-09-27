// Can a program run code it wrote into a heap block? -DGHOST jumps through the +0x40000 mirror
// (HW2 execution-protection bypass, GCC4TI faq_49 / gb68k).
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>

void _main(void)
{
    unsigned short *code = HeapAllocPtr(8);
    short (*f)(void);
    short r;
    if (!code) return;
    code[0] = 0x702A;                 // moveq #42,d0
    code[1] = 0x4E75;                 // rts
#ifdef GHOST
    f = (void *)((char *)code + 0x40000);
#else
    f = (void *)code;
#endif
    clrscr();
    printf("HW %d addr %lx\ncalling...\n", HW_VERSION, (long)f);
    r = f();
    printf("returned %d", r);
    HeapFreePtr(code);
    ngetchx();
}
