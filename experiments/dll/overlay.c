// Code copied at run time into a zone reserved inside the program (an overlay area) and
// called, after big allocations (as a runtime game does): does it run on an unpatched
// Titanium, where a DLL loaded low in RAM crashes (bigalloc.c)?
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>
static unsigned short area[64] = { 1 };     // in the program's own block (initialised data)
void _main(void)
{
    static const unsigned short code[] = { 0x7007, 0x4E75 };   // moveq #7,%d0 ; rts
    void *b[3]; short i, r;
    for (i = 0; i < 3; i++) b[i] = malloc(50000);
    memcpy(area, code, sizeof code);
    r = ((short (*)(void))area)();
    ClrScr();
    printf_xy(0, 0, "area at %lx returned %d", (unsigned long)area, r);
    ngetchx();
    for (i = 0; i < 3; i++) if (b[i]) free(b[i]);
}
