// How big can a NOSTUB program be? data.h (gen.py) holds DATA_N bytes; the program sums them.
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>
#include "data.h"

void _main(void)
{
    const volatile unsigned char *p = data;
    unsigned short s = 0, i;
    for (i = 0; i < DATA_N; i++) s += p[i];
    clrscr();
    printf("size %u\nsum %u want %u\n%s", (unsigned short)DATA_N, s, (unsigned short)DATA_SUM, s == (unsigned short)DATA_SUM ? "OK" : "BAD");
    ngetchx();
}
