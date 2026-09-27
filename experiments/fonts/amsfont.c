// Read AMS font bitmaps in place through OO_CondGetAttr (AMS >= 2.00), as faststr.h does, and draw
// text with them next to DrawStr for a visual check. F_6x8: 8 bytes/char; F_4x6: [width][5 rows].
#define USE_TI89
#define MIN_AMS 200
#define SAVE_SCREEN
#include <tigcclib.h>

static const unsigned char *font[3];

static short get_fonts(void)
{
    pFrame fr = 0xFF000000UL;                      // no running app: the system frame
    short app = EV_runningApp, k;
    if (app) fr = *(pFrame *)((char *)HeapDeref(app) + 20);   // ACB.pFrame
    for (k = 0; k < 3; k++)
        if (!OO_CondGetAttr(fr, OO_SFONT + k, (void **)&font[k])) return 0;
    return 1;
}

// OR one glyph row by row at (x, y) straight into LCD memory (30-byte rows)
static short put(short x, short y, unsigned char c, short f)
{
    const unsigned char *g;
    short rows, adv, r;
    if (f == 0) { g = font[0] + c * 6; adv = *g++; rows = 5; }
    else if (f == 1) { g = font[1] + c * 8; adv = 6; rows = 8; }
    else { g = font[2] + c * 10; adv = 8; rows = 10; }
    for (r = 0; r < rows; r++) {
        unsigned char *p = (unsigned char *)LCD_MEM + (y + r) * 30 + (x >> 3);
        unsigned short w = (unsigned short)g[r] << (8 - (x & 7));
        p[0] |= w >> 8; p[1] |= w;
    }
    return adv;
}

void _main(void)
{
    static const char txt[] = "Hello gAy 42!";
    short k, f, x;
    ClrScr();
    if (!get_fonts()) { DrawStr(0, 0, "OO_CondGetAttr failed", A_NORMAL); ngetchx(); return; }
    for (f = 0; f < 3; f++) {
        FontSetSys(f);
        DrawStr(0, f * 22, txt, A_NORMAL);                       // AMS
        for (k = 0, x = 3; txt[k]; k++) x += put(x, f * 22 + 11, txt[k], f);   // ours, 3 px right
    }
    FontSetSys(F_4x6);
    printf_xy(0, 70, "fonts %lx %lx %lx", (long)font[0], (long)font[1], (long)font[2]);
    printf_xy(0, 78, "HW%d", (short)HW_VERSION);
    ngetchx();
}
