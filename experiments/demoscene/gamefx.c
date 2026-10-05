// Demoscene effects a game can use (game-techniques §14), one per scenario (--arg N), each a
// movie of MOVIE frames (per frame BENCH_VALUE of its cycles, then BENCH_SHOT) for movie.py:
//   1 torch: bump-mapped light over a brick wall with "YARONET" carved in it (jsseffec's bump,
//     drawn only inside the lit disc around the torch, the rest of the room dark)
//   2 lake: the bottom 40 rows mirror the scene, rippled by a per-row shift and darkened one grey
//   3 twister pillars: 4 faces per row from a sine table, shaded by their width (Der Rechner)
//   4 static: rolling noise band, full static, tune-in, CRT switch-off (TV Noise, xorshift)
//   5 plasma title: 80x50 cells of 2x2 pixels, 16 levels Bayer-dithered to 4 greys, two cells
//     per table lookup (Der Rechner's table c2p), the title on top with a white outline
// Zone N: the frame of scenario N; zone 9: set-up (pictures, tables).
#define USE_TI89
#include <tigcclib.h>
#include "../../tools/m68kbench/bench.h"
#include "trig.h"

#ifndef MOVIE
#define MOVIE 240
#endif

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

#define DK 0xF00                                    // dark plane offset, rows of 30 bytes
static u8 *buf, *pic;                               // the screen, a still picture
static u32 rng = 2463534242UL;

static u32 xorshift(void)
{
    rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5;
    return rng;
}

// ---- text: Y A R O N E T in a 5x7 font --------------------------------------------------------
static const u8 glyphs[7][7] = {
    {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04},     // Y
    {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},     // A
    {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11},     // R
    {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E},     // O
    {0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11},     // N
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F},     // E
    {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04},     // T
};

// is (x, y) inside the word drawn at (x0, y0), sc pixels per font dot, gap pixels between letters
static u16 in_text(short x, short y, short x0, short y0, short sc, short gap)
{
    short lw = 5 * sc + gap, k, c, r;
    x -= x0; y -= y0;
    if (x < 0 || y < 0 || y >= 7 * sc) return 0;
    k = x / lw; c = (x % lw) / sc; r = y / sc;
    if (k >= 7 || c >= 5) return 0;
    return glyphs[k][r] >> (4 - c) & 1;
}

static void put(u8 *p, u16 x, u16 y, u16 g)         // pixel of darkness g (0 white .. 3 black)
{
    u8 *q = p + y * 30 + (x >> 3), m = 0x80 >> (x & 7);
    if (g & 1) q[0] |= m; else q[0] &= ~m;
    if (g & 2) q[DK] |= m; else q[DK] &= ~m;
}

// ---- 1: torch --------------------------------------------------------------------------------
// Each pixel stores the byte offset of its bump (minus the height gradient) into a light table
// of LW x LH entries centred on the torch; the pixel reads the table at (pixel - torch + bump):
// a slope facing the torch reads nearer the centre, so it is brighter. An entry holds the pixel
// for both checkerboard parities (7 light levels dithered between the 4 greys), each as a word
// light bit << 0 | dark bit << 8: acc = 2 acc + entry over 8 pixels gives both plane bytes.
// The pixel's parity is folded into its offset (+2), so all pixels run the same code. Only the
// lit disc is drawn (per row the bytes within RY of the torch); of last frame's disc, only the
// bytes the new one does not cover go back to black.
#define R 34                                        // light radius
#define RY (R + 4)                                  // half height of the drawn disc
#define LW 120
#define LH 102
static short *bump;                                 // 160x100 byte offsets into ltab
static u8 *carve;                                   // 160x100 bits: engraved letter, one grey darker
#define TX 18                                       // the word: scale 3, 123x21 pixels
#define TY 40
static u32 *ltab;                                   // LW x LH: even pixel word, odd pixel word
static u8 span[RY + 1];                             // half width of the disc per row
static u8 pb0[2 * RY + 1], pb1[2 * RY + 1];         // last frame's bytes per row
static short py0 = -1;

static void torch_setup(void)
{
    u8 *h = malloc(160 * 102);                      // height map, one guard row each side
    u8 *t = malloc(160 * 102);
    short x, y, i, j, pass;
    if (!h || !t) { if (h) free(h); return; }
    for (y = 0; y < 102; y++)                       // bricks 24x12, staggered, 2-pixel mortar
        for (x = 0; x < 160; x++) {
            short by = y % 12, bx = (x + (y / 12 & 1) * 12) % 24;
            u16 v = by < 2 || bx < 2 ? 2 : 14 + (xorshift() & 3);
            if (in_text(x, y - 1, TX, TY, 3, 3)) {   // letters carved in, darker stone
                v = 0;
                if (y >= 1 && y <= 100) carve[(y - 1) * 20 + (x >> 3)] |= 0x80 >> (x & 7);
            }
            h[y * 160 + x] = v;
        }
    for (pass = 0; pass < 1; pass++) {              // 3x3 box blur: bevelled edges
        for (y = 1; y < 101; y++)
            for (x = 0; x < 160; x++) {
                short x1 = x ? x - 1 : 0, x2 = x < 159 ? x + 1 : 159, s = 0;
                for (j = -1; j <= 1; j++)
                    s += h[(y + j) * 160 + x1] + h[(y + j) * 160 + x] + h[(y + j) * 160 + x2];
                t[y * 160 + x] = (u16)(s * 57) >> 9;    // /9
            }
        memcpy(h + 160, t + 160, 160 * 100);
    }
    for (y = 0; y < 100; y++)
        for (x = 0; x < 160; x++) {
            const u8 *p = h + (y + 1) * 160 + x;
            short nx = (x ? p[-1] : p[0]) - (x < 159 ? p[1] : p[0]);
            short ny = p[-160] - p[160];
            nx = nx > 12 ? 12 : nx < -12 ? -12 : nx;
            ny = ny > 12 ? 12 : ny < -12 ? -12 : ny;
            bump[y * 160 + x] = (nx + ny * LW) * 4 + ((x + y) & 1) * 2;
        }
    free(t); free(h);
    for (j = 0; j < LH; j++)                        // a cone: 7 (1 - d/R), so slopes show everywhere
        for (i = 0; i < LW; i++) {
            short di = abs(i - LW / 2), dj = abs(j - LH / 2), d = di > dj ? di : dj;
            short d2 = di * di + dj * dj, ga, gb, lv;
            while ((d + 1) * (d + 1) <= d2) d++;    // integer sqrt
            lv = d >= R ? 0 : (R - d) * 9 / R;
            lv = lv > 6 ? 6 : lv;
            ga = 3 - (lv >> 1); gb = 3 - ((lv + 1) >> 1);               // darkness
            ltab[j * LW + i] = (u32)((ga & 1) | (ga >> 1) << 8) << 16 | ((gb & 1) | (gb >> 1) << 8);
        }
    for (j = 0; j <= RY; j++) {                     // integer sqrt(RY^2 - j^2)
        i = 0;
        while ((i + 1) * (i + 1) <= RY * RY - j * j) i++;
        span[j] = i;
    }
}

// flame sprite: k black, d dark, l light, w white, space transparent
static const char *const flame[2][11] = {
    {"    k     ", "   kwk    ", "   kwlk   ", "  kwlwk   ", "  kllwlk  ", " klwwwlk  ",
     " klwwwlk  ", " kdlwwlk  ", "  kdllk   ", "   kddk   ", "    kk    "},
    {"     k    ", "    kwk   ", "   klwk   ", "   kwlwk  ", "  klwllk  ", " klwwwlk  ",
     " kllwwlk  ", " kdlwwdk  ", "  kdlldk  ", "   kddk   ", "    kk    "},
};

static u16 fl_m[2][11], fl_l[2][11], fl_d[2][11];   // opaque mask, light plane, dark plane

static void flame_setup(void)
{
    u16 f, r, c;
    for (f = 0; f < 2; f++)
        for (r = 0; r < 11; r++)
            for (c = 0; c < 10; c++) {
                char ch = flame[f][r][c];
                u16 g = ch == 'k' ? 3 : ch == 'd' ? 2 : ch == 'l' ? 1 : 0, b = 0x8000 >> c;
                if (ch == ' ') continue;
                fl_m[f][r] |= b;
                if (g & 1) fl_l[f][r] |= b;
                if (g & 2) fl_d[f][r] |= b;
            }
}

static void draw_flame(short x, short y, u16 f)    // 16-pixel rows over 3 bytes
{
    u16 r, sh = 8 - (x & 7);
    u8 *d = buf + y * 30 + (x >> 3);
    for (r = 0; r < 11; r++, d += 30) {
        u32 m = ~((u32)fl_m[f][r] << sh), l = (u32)fl_l[f][r] << sh, k = (u32)fl_d[f][r] << sh;
        d[0] = (d[0] & (u8)(m >> 16)) | (u8)(l >> 16);
        d[1] = (d[1] & (u8)(m >> 8)) | (u8)(l >> 8);
        d[2] = (d[2] & (u8)m) | (u8)l;
        d[DK] = (d[DK] & (u8)(m >> 16)) | (u8)(k >> 16);
        d[DK + 1] = (d[DK + 1] & (u8)(m >> 8)) | (u8)(k >> 8);
        d[DK + 2] = (d[DK + 2] & (u8)m) | (u8)k;
    }
}

#define TORCH8                              /* 8 pixels, pixel i at entry i (+4 i) */ \
    u16 acc = *(const u16 *)(p + o[0]);                 \
    acc = (acc << 1) + *(const u16 *)(p + 4 + o[1]);    \
    acc = (acc << 1) + *(const u16 *)(p + 8 + o[2]);    \
    acc = (acc << 1) + *(const u16 *)(p + 12 + o[3]);   \
    acc = (acc << 1) + *(const u16 *)(p + 16 + o[4]);   \
    acc = (acc << 1) + *(const u16 *)(p + 20 + o[5]);   \
    acc = (acc << 1) + *(const u16 *)(p + 24 + o[6]);   \
    acc = (acc << 1) + *(const u16 *)(p + 28 + o[7]);   \
    p += 32; o += 8;

static void clear_bytes(u8 *d, short b0, short b1)  // bytes b0..b1 of a row to black
{
    for (; b0 <= b1; b0++) d[b0] = 0xFF, d[DK + b0] = 0xFF;
}

static void torch_frame(short lx, short ly)        // lx in RY..159-RY, ly in RY..99-RY
{
    static u8 nb0[2 * RY + 1], nb1[2 * RY + 1];
    short j, b, y0 = ly - RY;
    for (j = 0; j <= 2 * RY; j++) {
        short s = span[j < RY ? RY - j : j - RY];
        nb0[j] = (lx - s) >> 3; nb1[j] = (lx + s) >> 3;
    }
    BENCH_BEGIN(6);
    if (py0 >= 0)                                   // last frame's disc minus the new one
        for (j = 0; j <= 2 * RY; j++) {
            short y = py0 + j, k = y - y0;
            u8 *d = buf + y * 30;
            if (k < 0 || k > 2 * RY) clear_bytes(d, pb0[j], pb1[j]);
            else {
                clear_bytes(d, pb0[j], nb0[k] - 1);
                clear_bytes(d, nb1[k] + 1, pb1[j]);
            }
        }
    BENCH_END(6);
    py0 = y0;
    for (j = 0; j <= 2 * RY; j++) {
        short b0 = nb0[j], b1 = nb1[j], y = y0 + j;
        const char *p = (const char *)ltab + ((long)((j - RY + LH / 2) * LW + b0 * 8 - lx + LW / 2) << 2);
        const short *o = bump + y * 160 + b0 * 8;
        const u8 *c = carve + y * 20 + b0;
        u8 *d = buf + y * 30 + b0;
        pb0[j] = b0; pb1[j] = b1;
        b = b1 - b0;
        if (y >= TY && y < TY + 21)                 // rows with letters
            do {
                TORCH8
                {                                   // carved: g -> min(g + 1, 3)
                    u8 l = acc, k = acc >> 8, m = *c++;
                    d[DK] = k | (l & m);
                    *d++ = l ^ (m & ~k);
                }
            } while (--b >= 0);
        else
            do {
                TORCH8
                d[DK] = acc >> 8; *d++ = acc;
            } while (--b >= 0);
    }
}

static void fx_torch(void)
{
    u16 f;
    bump = malloc(160 * 100 * 2);
    ltab = malloc((long)LW * LH * 4);
    carve = calloc(20 * 100, 1);
    if (!bump || !ltab || !carve) goto out;
    BENCH_BEGIN(9); torch_setup(); flame_setup(); BENCH_END(9);
    memset(buf, 0xFF, 7680);
    for (f = 0; f < MOVIE; f++) {
        u32 c0 = BENCH_CYCLES;
        u32 r = xorshift();
        short lx = 80 + (sin_tab[(u8)(f * 2)] * 38 >> 7) + (r & 1);      // flicker: +-1 pixel
        short ly = 50 + (sin_tab[(u8)(f * 5 + 64)] * 10 >> 7) + (r >> 1 & 1);
        BENCH_BEGIN(1);
        torch_frame(lx, ly);
        BENCH_BEGIN(7); draw_flame(lx - 4, ly - 6, r >> 2 & 1); BENCH_END(7);
        BENCH_END(1);
        BENCH_VALUE(BENCH_CYCLES - c0);
        BENCH_SHOT(buf);
    }
out:
    if (carve) free(carve);
    if (ltab) free(ltab);
    if (bump) free(bump);
}

// ---- 2: lake ---------------------------------------------------------------------------------
// one plane row of 20 bytes shifted by sh pixels (-15..15, > 0 = right), from wobble.c
static void shift_row(u16 *d, const u16 *s, short sh)
{
    u32 w;
    u16 n, k;
    if (sh >= 0) {
        k = sh; w = 0; n = 10;
        while (n--) { w = (w << 16) | *s++; *d++ = (u16)(w >> k); }
    } else {
        k = 16 + sh; w = *s++; n = 9;
        while (n--) { w = (w << 16) | *s++; *d++ = (u16)(w >> k); }
        *d = (u16)((w << 16) >> k);
    }
}

// rows 60..99 of dst: rows 59..20 of src mirrored, shifted by ripples of amplitude amp (0: none)
// growing towards the viewer, and one grey darker (g -> min(g + 1, 3) is 2 bitwise ops)
static void reflect(u8 *dst, const u8 *src, u16 f, u16 amp)
{
    u16 y, i;
    for (y = 60; y < 100; y++) {
        const u8 *s = src + (119 - y) * 30;
        u8 *d = dst + y * 30;
        short sh = amp ? sin_tab[(u8)(y * 11 - f * 9)] * (short)(amp + ((y - 60) >> 4)) >> 7 : 0;
        shift_row((u16 *)d, (const u16 *)s, sh);
        shift_row((u16 *)(d + DK), (const u16 *)(s + DK), sh);
        for (i = 0; i < 20; i += 2) {
            u16 l = *(u16 *)(d + i), k = *(u16 *)(d + DK + i);
            *(u16 *)(d + i) = ~l | k;
            *(u16 *)(d + DK + i) = l | k;
        }
    }
}

// the still picture: sky, sun, two mountain ranges, a shore with fir trees, its reflection
static void make_scene(u8 *p)
{
    short x, y, i;
    memset(p, 0, 7680);
    for (y = 0; y < 60; y++)
        for (x = 0; x < 160; x++) {
            short far = 30 + (sin_tab[(u8)(x * 3)] >> 4) + (sin_tab[(u8)(x * 7 + 40)] >> 5);
            short nr = 42 + (sin_tab[(u8)(x * 2 + 100)] >> 4) + (sin_tab[(u8)(x * 9)] >> 6);
            short dx = x - 112, dy = y - 18;
            u16 g = y < 8 ? ((x ^ y) & 1 ? 2 : 1) : y < 16 ? 1 : y < 22 ? (x ^ y) & 1 : 0;  // sky
            if (dx * dx + dy * dy < 81) g = 0;                                  // sun
            if (y >= far) g = y < far + 3 ? 0 : 1;                              // snowy far range
            if (y >= nr) g = 2;
            if (y >= 57) g = 3;                                                 // shore
            put(p, x, y, g);
        }
    for (i = 0; i < 12; i++) {                      // fir trees on the shore
        short xt = 6 + i * 13 + (i * 7) % 5, ht = 7 + (i * 5) % 6;
        for (y = 0; y < ht; y++)
            for (x = -(y + 1) / 2; x <= (y + 1) / 2; x++) put(p, xt + x, 57 - ht + y, 3);
    }
    reflect(p, p, 0, 0);
}

static void fx_lake(void)
{
    u16 f;
    BENCH_BEGIN(9); make_scene(pic); memcpy(buf, pic, 7680); BENCH_END(9);
    for (f = 0; f < MOVIE; f++) {
        u32 c0 = BENCH_CYCLES;
        BENCH_BEGIN(2);
        reflect(buf, pic, f, 1);
        BENCH_END(2);
        BENCH_VALUE(BENCH_CYCLES - c0);
        BENCH_SHOT(buf);
    }
}

// ---- 3: twister pillars ----------------------------------------------------------------------
// A pillar row depends only on its angle: 256 precomputed 32-pixel rows per row parity (the wall
// pattern behind it alternates), so a frame is one table read and two long writes per pillar row.
static void fx_twister(void)
{
    static const u8 shade[18] = {2, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0};  // by face width (<= 17)
    u32 *tw = malloc(2 * 256 * 8);
    u16 f, y, k, i, a, par;
    if (!tw) return;
    BENCH_BEGIN(9);
    for (par = 0; par < 2; par++)
        for (a = 0; a < 256; a++) {
            u32 bgl = par ? 0x88888888UL : 0x22222222UL;   // dark grey wall, black dots
            u32 pm = (0xFFFFFFFFUL >> 4) & ~(0xFFFFFFFFUL >> 29), L = bgl & ~pm, D = ~pm;
            short xs[5];
            for (i = 0; i < 4; i++) xs[i] = 16 + (sin_tab[(u8)(a + 64 * i + 64)] * 12 >> 7);
            xs[4] = xs[0];
            for (i = 0; i < 4; i++)
                if (xs[i + 1] > xs[i]) {
                    u32 m = (0xFFFFFFFFUL >> xs[i]) & ~(0xFFFFFFFFUL >> xs[i + 1]);
                    u32 e = 0x80000000UL >> xs[i];
                    u16 g = (i & 1) + shade[xs[i + 1] - xs[i]];   // faces alternate
                    g = g > 3 ? 3 : g;
                    if (g & 1) L |= m;
                    if (g & 2) D |= m;
                    L |= e; D |= e;                 // black edge between faces
                }
            tw[(par * 256 + a) * 2] = L;
            tw[(par * 256 + a) * 2 + 1] = D;
        }
    for (y = 0; y < 100; y++) {                     // the wall, once
        memset(buf + y * 30, y & 1 ? 0x88 : 0x22, 20);
        memset(buf + DK + y * 30, 0xFF, 20);
    }
    BENCH_END(9);
    for (f = 0; f < MOVIE; f++) {
        u32 c0 = BENCH_CYCLES;
        BENCH_BEGIN(3);
        for (k = 0; k < 3; k++) {                   // pillars at x = 16, 64, 112
            u8 base = f * (k == 1 ? 3 : 2) + k * 40, ph = f * 3 + k * 85;
            u8 *row = buf + 2 + k * 6;
            for (y = 0; y < 100; y++, row += 30, ph += 2) {
                const u32 *e = tw + ((y & 1) * 256 + (u8)(base + (sin_tab[ph] >> 2))) * 2;
                *(u32 *)row = e[0];
                *(u32 *)(row + DK) = e[1];
            }
        }
        BENCH_END(3);
        BENCH_VALUE(BENCH_CYCLES - c0);
        BENCH_SHOT(buf);
    }
    free(tw);
}

// ---- 4: TV static ----------------------------------------------------------------------------
static u8 noise[25 * 40];                           // 25 rows of both planes, reused 4 times

static void noise_rows(u16 y0, u16 y1)
{
    u32 *q = (u32 *)noise;
    u16 i, y, s = xorshift() % 25;
    for (i = 0; i < 25 * 10; i++) *q++ = xorshift();
    for (y = y0; y < y1; y++) {
        const u32 *n = (const u32 *)(noise + ((y + s) % 25) * 40);
        u32 *a = (u32 *)(buf + y * 30), *b = (u32 *)(buf + DK + y * 30);
        a[0] = n[0]; a[1] = n[1]; a[2] = n[2]; a[3] = n[3]; a[4] = n[4];
        b[0] = n[5]; b[1] = n[6]; b[2] = n[7]; b[3] = n[8]; b[4] = n[9];
    }
}

static void pic_rows(u16 y0, u16 y1)
{
    u16 y;
    for (y = y0; y < y1; y++) {
        memcpy(buf + y * 30, pic + y * 30, 20);
        memcpy(buf + DK + y * 30, pic + DK + y * 30, 20);
    }
}

static void fx_static(void)
{
    u16 f, y;
    BENCH_BEGIN(9); make_scene(pic); BENCH_END(9);
    for (f = 0; f < MOVIE; f++) {
        u32 c0 = BENCH_CYCLES;
        u16 ph = f * 4 / MOVIE, k = f % (MOVIE / 4);   // 4 phases
        BENCH_BEGIN(4);
        if (ph == 0) {                              // a band of static rolls down
            short b = k * 4 % 116 - 16;
            pic_rows(0, 100);
            noise_rows(b < 0 ? 0 : b, b + 16 > 100 ? 100 : b + 16);
        } else if (ph == 1) {
            noise_rows(0, 100);
        } else if (ph == 2) {                       // tuning in from the top
            u16 t = k * 100 / (MOVIE / 4 - 4);
            t = t > 100 ? 100 : t;
            pic_rows(0, t);
            noise_rows(t, 100);
        } else {                                    // CRT off: squeeze to a line, then a dot
            u16 n = MOVIE / 4, hh = k < n / 2 ? 100 - k * 196 / n : 2;
            memset(buf, 0xFF, 7680);
            if (k < n / 2) {
                u16 step = (100 << 8) / hh, src = 0, y0 = 50 - hh / 2;
                for (y = y0; y < y0 + hh; y++, src += step) {
                    memcpy(buf + y * 30, pic + (src >> 8) * 30, 20);
                    memcpy(buf + DK + y * 30, pic + DK + (src >> 8) * 30, 20);
                }
            } else if (k < n - 2) {                 // a white line shrinking to the centre
                u16 w = (n - 2 - k) * 10 / (n / 2), x;
                for (y = 49; y < 51; y++)
                    for (x = 80 - w * 8; x < 80 + w * 8 + 2; x++) put(buf, x, y, 0);
            }
        }
        BENCH_END(4);
        BENCH_VALUE(BENCH_CYCLES - c0);
        BENCH_SHOT(buf);
    }
}

// ---- 5: plasma title -------------------------------------------------------------------------
// level(x, y) = ax[x] + by[y] + dg[x + y] (0..14). Two cells side by side form one index
// level0 << 4 | level1, built from per-frame pair arrays (already x4 for long tables); one long
// read gives their 4 pixels on both planes and both rows of the cell (the Bayer row parities):
// light even row << 24 | light odd row << 16 | dark even << 8 | dark odd.
static u32 pt_hi[256], pt_lo[256];                  // [level a << 4 | level b], a in the high nibble
static short l5h[256], l5l[256], l6h[256], l6l[256], l6b[256];   // levels << 6, << 2, x 68
static short axp[40], dgp[130], byp[50];
static u16 tmask[21][10], tglyph[21][10];          // title rows: outline mask, glyph pixels

static void plasma_setup(void)
{
    static const u8 bayer[2][2] = {{0, 2}, {3, 1}};
    u16 py, v, c, x, y;
    for (v = 0; v < 256; v++) {                     // sine levels 0..4 and 0..5, pre-shifted
        short a = (sin_tab[v] + 128) * 5 >> 8, b = (sin_tab[v] + 128) * 6 >> 8;
        l5h[v] = a << 6; l5l[v] = a << 2; l6h[v] = b << 6; l6l[v] = b << 2; l6b[v] = b * 68;
    }
    for (py = 0; py < 2; py++)
        for (v = 0; v < 256; v++) {                 // two cells = 4 pixels per plane
            u32 l = 0, d = 0;
            for (c = 0; c < 4; c++) {
                u16 lev = (c < 2 ? v >> 4 : v & 15) * 12 / 15;    // 0..12 brightness quarters
                u16 b = (lev >> 2) + ((lev & 3) > bayer[py][c & 1]);
                u16 g = 3 - (b > 3 ? 3 : b);
                l = l << 1 | (g & 1); d = d << 1 | g >> 1;
            }
            pt_hi[v] |= (l << 4 << (py ? 16 : 24)) | (d << 4 << (py ? 0 : 8));
            pt_lo[v] |= (l << (py ? 16 : 24)) | (d << (py ? 0 : 8));
        }
    for (y = 0; y < 21; y++)                        // title at scale 3, centred
        for (x = 0; x < 160; x++) {
            short dx, dy;
            u16 in = in_text(x, y, 19, 0, 3, 3), out = 0;
            for (dy = -1; dy <= 1; dy++)
                for (dx = -1; dx <= 1; dx++) out |= in_text(x + dx, y + dy, 19, 0, 3, 3);
            if (out) tmask[y][x >> 4] |= 0x8000 >> (x & 15);
            if (in) tglyph[y][x >> 4] |= 0x8000 >> (x & 15);
        }
}

static void fx_plasma(void)
{
    u16 f, x, y;
    BENCH_BEGIN(9); plasma_setup(); BENCH_END(9);
    for (f = 0; f < MOVIE; f++) {
        u32 c0 = BENCH_CYCLES;
        BENCH_BEGIN(5);
        BENCH_BEGIN(6);
        {
            u8 u = f * 3, v = -f * 2, w = f * 4;    // ax: phase u step 5, by: v step 7, dg: w step 3
            for (x = 0; x < 40; x++, u += 10) axp[x] = l5h[u] + l5l[(u8)(u + 5)];
            for (y = 0; y < 50; y++, v += 7) byp[y] = l6b[v];
            for (x = 0; x < 130; x++, w += 3) dgp[x] = l6h[w] + l6l[(u8)(w + 3)];
        }
        BENCH_END(6);
        for (y = 0; y < 50; y++) {
            u8 *r = buf + y * 60;
            const short *a = axp, *g = dgp + y;
            const char *ph = (const char *)pt_hi, *pl = (const char *)pt_lo;
            short b = byp[y];
            for (x = 0; x < 20; x++, a += 2, g += 4) {     // 4 cells = 8 pixels
                short i0 = a[0] + g[0] + b, i1 = a[1] + g[2] + b;
                u32 w = *(const u32 *)(ph + i0) | *(const u32 *)(pl + i1);
                r[DK + 30] = w;
                r[DK] = w >> 8;
                w >>= 16;
                r[30] = w;
                *r++ = w >> 8;
            }
        }
        BENCH_BEGIN(7);
        {                                           // the title: black glyphs, white outline
            const u16 *m = tmask[0], *t = tglyph[0];
            u16 *l = (u16 *)(buf + 40 * 30);
            for (y = 0; y < 21; y++, l += 5)
                for (x = 0; x < 10; x++, l++) {
                    u16 k = ~*m++, g = *t++;
                    l[0] = (l[0] & k) | g;
                    l[DK / 2] = (l[DK / 2] & k) | g;
                }
        }
        BENCH_END(7);
        BENCH_END(5);
        BENCH_VALUE(BENCH_CYCLES - c0);
        BENCH_SHOT(buf);
    }
}

void _main(void)
{
    BENCH_NAME(1, "torch");
    BENCH_NAME(2, "lake");
    BENCH_NAME(3, "twister");
    BENCH_NAME(4, "static");
    BENCH_NAME(5, "plasma");
    BENCH_NAME(6, "torch clear / plasma tables");
    BENCH_NAME(7, "flame / title");
    BENCH_NAME(9, "set-up");
    buf = malloc(7680);
    pic = malloc(7680);
    if (!buf || !pic) { if (buf) free(buf); return; }
    memset(buf, 0, 7680);
    switch (BENCH_ARG) {
        case 2: fx_lake(); break;
        case 3: fx_twister(); break;
        case 4: fx_static(); break;
        case 5: fx_plasma(); break;
        default: fx_torch();
    }
    free(pic); free(buf);
}
