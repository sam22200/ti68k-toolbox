// Code small, data big: reads a 60 KB data variable ("bigdt", made by ttbin2oth) in place,
// from RAM or straight from the archive (Flash), without copying it.
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>

void _main(void)
{
    SYM_ENTRY *e = SymFindPtr(SYMSTR("bigdt"), 0);
    const unsigned char *p;
    unsigned short s = 0, n, i;
    clrscr();
    if (!e) { printf("bigdt missing"); ngetchx(); return; }
    p = HeapDeref(e->handle);         // archived: a pointer into Flash, readable directly
    n = *(const unsigned short *)p;   // variable size word: data + 0 + "dat" + 0 + OTH tag
    p += 2;
    for (i = 0; i < 60000u; i++) s += p[i];
    printf("size word %u\narchived %d\naddr %lx\nsum %u want 47214", n, e->flags.bits.archived, (long)p, s);
    ngetchx();
}
