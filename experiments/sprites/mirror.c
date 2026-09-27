// ExtGraph mirror routines vs the hand-written rev8 version (ti68k-c-patterns.md §3).
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>
#include "extgraph.h"

static unsigned char rev8[256];
static void make_rev8(void) { unsigned short i; for (i = 0; i < 256; i++) { unsigned char b = i, r = 0, k;
    for (k = 0; k < 8; k++) { r = (r << 1) | (b & 1); b >>= 1; } rev8[i] = r; } }
static void hflip16(const unsigned short *s, unsigned short *d, unsigned short h, unsigned short w)
{ while (h--) { unsigned short r = *s++; *d++ = (unsigned short)((rev8[r & 0xFF] << 8) | rev8[r >> 8]) << (16 - w); } }
static void vflip(const unsigned short *s, unsigned short *d, unsigned short h)
{ const unsigned short *e = s + h; while (h--) *d++ = *--e; }

void _main(void)
{
    unsigned short src[16], a[16], b[16], k, okh = 1, okv = 1, okf = 1;
    make_rev8();
    for (k = 0; k < 16; k++) src[k] = k * 0x1357 + 0x0F01;
    hflip16(src, a, 16, 16);
    SpriteX8_MIRROR_H_R(16, (unsigned char *)src, 2, (unsigned char *)b);
    for (k = 0; k < 16; k++) if (a[k] != b[k]) okh = 0;
    FastSprite16_MIRROR_H_R(16, src, b);
    for (k = 0; k < 16; k++) if (a[k] != b[k]) okf = 0;
    vflip(src, a, 16);
    SpriteX8_MIRROR_V_R(16, (unsigned char *)src, 2, (unsigned char *)b);
    for (k = 0; k < 16; k++) if (a[k] != b[k]) okv = 0;
    clrscr();
    printf("MIRROR_H == hflip16: %d\nFast16_MIRROR_H: %d\nMIRROR_V == vflip: %d\n%04x -> %04x", okh, okf, okv, src[3], b[12]);
    ngetchx();
}
