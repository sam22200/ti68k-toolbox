// Compiled sprites (KB §12): a 16x16 masked sprite turned by gen_sprites.py into one C function
// per shift (andi/ori/move with immediates on d16(a0), no data fetch, no shift, transparent words
// skipped), against ExtGraph's Sprite16_MASK_R / ClipSprite16_MASK_R / GraySprite16_MASK_R.
// Measures cycles per draw (address computation included) over 64 positions covering all 16
// shifts, and checks that both give byte-identical buffers (B/W and gray).
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>
#include <extgraph.h>
#include "sprgen.h"

static volatile unsigned long ticks;
DEFINE_INT_HANDLER(tick_handler) { ticks++; }
volatile unsigned short sink;
#define N 4000

static unsigned char px[64], py[64];
static unsigned char *b0, *b1;

static void b_empty(void)
{
    unsigned short n = N;
    while (n--) { unsigned short i = n & 63; sink = px[i] + py[i]; }
}
static void b_eg(void)
{
    unsigned short n = N;
    while (n--) { unsigned short i = n & 63; Sprite16_MASK_R(px[i], py[i], 16, spr_bw, spr_mask, b0); }
}
static void b_egc(void)
{
    unsigned short n = N;
    while (n--) { unsigned short i = n & 63; ClipSprite16_MASK_R(px[i], py[i], 16, spr_bw, spr_mask, b0); }
}
static inline unsigned short rowoff(unsigned short x, unsigned short y)
{
    return (y << 5) - (y << 1) + ((x >> 3) & ~1);
}
static void b_cs(void)
{
    unsigned short n = N;
    while (n--) {
        unsigned short i = n & 63, x = px[i];
        spr_bw_tab[x & 15](b0 + rowoff(x, py[i]));
    }
}
static void b_egg(void)
{
    unsigned short n = N;
    while (n--) {
        unsigned short i = n & 63;
        GraySprite16_MASK_R(px[i], py[i], 16, spr_light, spr_dark, spr_mask, spr_mask, b0, b1);
    }
}
static void b_csg(void)
{
    unsigned short n = N;
    while (n--) {
        unsigned short i = n & 63, x = px[i], o = rowoff(x, py[i]);
        spr_gr_tab[x & 15](b0 + o, b1 + o);
    }
}

static void fillpat(unsigned char *b)
{
    unsigned short k;
    for (k = 0; k < LCD_SIZE; k++) b[k] = (k / 30) & 2 ? 0xCC : 0x33;
}

// 1 if ExtGraph and compiled sprites give identical buffers at all 64 positions
static short check(void)
{
    unsigned char *c0 = malloc(LCD_SIZE), *c1 = malloc(LCD_SIZE);
    short i, ok = 0;
    if (!c0 || !c1) goto done;
    fillpat(b0); fillpat(c0);
    for (i = 0; i < 64; i++) {
        Sprite16_MASK_R(px[i], py[i], 16, spr_bw, spr_mask, b0);
        spr_bw_tab[px[i] & 15](c0 + rowoff(px[i], py[i]));
    }
    ok = !memcmp(b0, c0, LCD_SIZE);
    fillpat(b0); fillpat(b1); fillpat(c0); fillpat(c1);
    for (i = 0; i < 64; i++) {
        unsigned short o = rowoff(px[i], py[i]);
        GraySprite16_MASK_R(px[i], py[i], 16, spr_light, spr_dark, spr_mask, spr_mask, b0, b1);
        spr_gr_tab[px[i] & 15](c0 + o, c1 + o);
    }
    ok += !memcmp(b0, c0, LCD_SIZE) && !memcmp(b1, c1, LCD_SIZE) ? 2 : 0;
done:
    free(c1); free(c0);
    return ok;
}

#define NB 6
static void (*const fn[NB])(void) = { b_empty, b_eg, b_egc, b_cs, b_egg, b_csg };
static const char *const nm[NB] = { "loop", "Sprite16_MASK_R", "ClipSprite16_MASK_R", "compiled B/W",
                                    "GraySprite16_MASK_R", "compiled gray" };

void _main(void)
{
    INT_HANDLER o1 = GetIntVec(AUTO_INT_1), o5 = GetIntVec(AUTO_INT_5);
    unsigned long t[NB];
    short k, ok;
    b0 = malloc(LCD_SIZE); b1 = malloc(LCD_SIZE);
    if (!b0 || !b1) goto out;
    for (k = 0; k < 64; k++) { px[k] = (k * 37 + k / 16) % 144; py[k] = (k * 23) % 85; }
    ok = check();
    ClrScr(); printf_xy(0, 0, "Benchmarking...");
    SetIntVec(AUTO_INT_5, DUMMY_HANDLER);
    SetIntVec(AUTO_INT_1, tick_handler);
    for (k = 0; k < NB; k++) {
        unsigned long s = ticks; while (ticks == s); s = ticks;
        fn[k]();
        t[k] = ticks - s;
    }
    SetIntVec(AUTO_INT_1, o1); SetIntVec(AUTO_INT_5, o5);
    // screen: ExtGraph (top row) and compiled (second row) over a pattern, shifts 0..8
    fillpat(LCD_MEM);
    memset(LCD_MEM + 38 * 30, 0, LCD_SIZE - 38 * 30);
    for (k = 0; k < 9; k++) {
        Sprite16_MASK_R(k * 17, 2, 16, spr_bw, spr_mask, LCD_MEM);
        spr_bw_tab[(k * 17) & 15]((unsigned char *)LCD_MEM + rowoff(k * 17, 20));
    }
    FontSetSys(F_4x6);
    for (k = 0; k < NB; k++) {
        long c = (long)(t[k] * 46875UL / N) - (k ? (long)(t[0] * 46875UL / N) : 0);
        printf_xy(0, 40 + 6 * k, "%-20s %5ld cyc/draw", nm[k], c);
    }
    printf_xy(0, 78, "identical: B/W %s, gray %s", ok & 1 ? "yes" : "NO", ok & 2 ? "yes" : "NO");
    GKeyFlush();
    ngetchx();
    FontSetSys(F_6x8);
out:
    free(b1); free(b0);
}
