// Raster wobble on a 2-plane 160x100 picture, from Der Rechner (NPLI, pouet 61671): each screen
// row is copied from a source row chosen by a sine table (the demo works on an 80x100 chunky
// picture through a c2p; here the picture is already planar, so a row is 20 bytes per plane).
// Zones: 1 vertical wobble (source row per screen row: heat haze, underwater, a hit),
//        2 horizontal wobble (each row shifted by -7..7 pixels: water reflection, warp),
//        3 plain copy of the picture (the reference cost).
// Measured under ti-cycles; BENCH_SHOT of each result (light plane, dark plane at +0xF00).
#define USE_TI89
#include <tigcclib.h>
#include "../../tools/m68kbench/bench.h"
#include "trig.h"

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
#define NFR 8

static u8 *src, *dst;                               // 30-byte rows, dark plane at +0xF00

static void make_picture(void)                      // concentric rings in 4 greys
{
    u16 x, y;
    memset(src, 0, 7680);
    for (y = 0; y < 100; y++)
        for (x = 0; x < 160; x++) {
            short dx = (short)x - 80, dy = (short)y - 50;
            u16 c = ((u16)(dx * dx + dy * dy * 3) >> 6) & 3;
            if (c & 1) src[y * 30 + (x >> 3)] |= 0x80 >> (x & 7);
            if (c & 2) src[0xF00 + y * 30 + (x >> 3)] |= 0x80 >> (x & 7);
        }
}

static void copy_row(u8 *d, const u8 *s)            // 20 bytes of both planes
{
    u32 *q = (u32 *)d;
    const u32 *p = (const u32 *)s;
    q[0] = p[0]; q[1] = p[1]; q[2] = p[2]; q[3] = p[3]; q[4] = p[4];
    q = (u32 *)(d + 0xF00); p = (const u32 *)(s + 0xF00);
    q[0] = p[0]; q[1] = p[1]; q[2] = p[2]; q[3] = p[3]; q[4] = p[4];
}

static void plain(void)
{
    u16 y;
    for (y = 0; y < 100; y++) copy_row(dst + y * 30, src + y * 30);
}

static void vwobble(u8 t)
{
    u16 y;
    u8 *d = dst;
    for (y = 0; y < 100; y++, d += 30) {
        short r = (short)y + (sin_tab[(u8)(y * 4 + t)] >> 4);   // -8..+7 rows
        r = r < 0 ? 0 : r > 99 ? 99 : r;
        copy_row(d, src + r * 30);
    }
}

// one plane row of 20 bytes shifted by sh pixels (-15..15, > 0 = right), zeros entering
static void shift_row(u16 *d, const u16 *s, short sh)
{
    u32 w;
    u16 n, k;
    if (sh >= 0) {                                  // pairs (previous word, word) >> sh
        k = sh; w = 0; n = 10;
        while (n--) { w = (w << 16) | *s++; *d++ = (u16)(w >> k); }
    } else {                                        // pairs (word, next word) >> (16 + sh)
        k = 16 + sh; w = *s++; n = 9;
        while (n--) { w = (w << 16) | *s++; *d++ = (u16)(w >> k); }
        *d = (u16)((w << 16) >> k);
    }
}

static void hwobble(u8 t)
{
    u16 y;
    const u8 *s = src;
    u8 *d = dst;
    for (y = 0; y < 100; y++, s += 30, d += 30) {
        short sh = sin_tab[(u8)(y * 6 + t)] >> 4;   // -8..7 pixels
        shift_row((u16 *)d, (const u16 *)s, sh);
        shift_row((u16 *)(d + 0xF00), (const u16 *)(s + 0xF00), sh);
    }
}

void _main(void)
{
    u16 f;
    BENCH_NAME(1, "vertical wobble");
    BENCH_NAME(2, "horizontal wobble");
    BENCH_NAME(3, "plain copy");
    src = malloc(7680);
    dst = malloc(7680);
    if (!src || !dst) { if (src) free(src); return; }
    make_picture();
    memset(dst, 0, 7680);
    for (f = 0; f < NFR; f++) { BENCH_BEGIN(3); plain(); BENCH_END(3); }
    BENCH_SHOT(dst);
    for (f = 0; f < NFR; f++) { BENCH_BEGIN(1); vwobble(f * 8); BENCH_END(1); }
    BENCH_SHOT(dst);
    for (f = 0; f < NFR; f++) { BENCH_BEGIN(2); hwobble(f * 8); BENCH_END(2); }
    BENCH_SHOT(dst);
    free(dst); free(src);
}
