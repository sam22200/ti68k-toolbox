// Save file written with stdio (fopen "wb"), as Xchange does: does fclose write the size word, so
// that the result is the same layout as savetest.c's manual SymAdd version?
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>

typedef struct { unsigned long magic; unsigned short version, runs; } SAVE;

void _main(void)
{
    SAVE s = {0x54534156UL, 1, 42};
    FILE *f = fopen("fsdat", "wb");
    SYM_ENTRY *se;
    const unsigned char *p;
    short n, i;
    ClrScr();
    if (!f) { printf_xy(0, 0, "fopen failed"); ngetchx(); return; }
    fwrite(&s, sizeof s, 1, f);
    fputc(0, f); fputs("sav", f); fputc(0, f); fputc(OTH_TAG, f);
    fclose(f);
    se = SymFindPtr(SYMSTR("fsdat"), 0);
    p = HeapDeref(se->handle);
    n = *(const unsigned short *)p;
    printf_xy(0, 0, "size word %d (want %d)", n, (short)(sizeof s + 6));
    for (i = 0; i < n + 2 && i < 16; i++) printf_xy((i & 7) * 20, 10 + (i >> 3) * 10, "%02X", p[i]);
    ngetchx();
}
