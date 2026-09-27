// Three rendering ideas of KB §12, measured in one run:
// 1. Adaptive tile refresh (Commander Keen): 20x12 map of 1-bpp 8x8 tiles; full redraw vs
//    redrawing cells whose shadow differs (scan) vs a dirty list, with k cells changed per frame.
// 2. Pseudo-3D road (Lou's pseudo 3d): per-line tables built once, curve by dx += ddx, stripes
//    from the Z table; cycles per frame of 70 lines.
// 3. Rotation of a 1-bpp 64x64 canvas by 30 degrees: three shears (Paeth; x shear = row shifts,
//    y shear = column runs moved by whole rows) vs per-destination-pixel inverse mapping (8.8
//    fixed point, incremental). Reports the pixels where they differ.
// Keys after the results: road, rotation (naive | shears), tile screen.
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>

static volatile unsigned long ticks;
DEFINE_INT_HANDLER(tick_handler) { ticks++; }
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
static u8 *buf;                                     // 30-byte rows, like LCD_MEM

static u16 lf = 0xACE1;
static u16 lfsr(void) { lf = (lf >> 1) ^ (-(lf & 1) & 0xB400); return lf; }

// ------------------------------------------------------------------ 1. tile refresh
#define TW 20
#define TH 12
#define NC (TW * TH)
static u8 tmap[NC], shadow[NC], dirty[NC];
static u16 ndirty;
static u8 tiles[16][8];
static u16 coff[NC];                                // byte offset of each cell in buf

static void draw_tile(u16 c)
{
    const u8 *s = tiles[tmap[c]];
    u8 *d = buf + coff[c];
    d[0] = s[0]; d[30] = s[1]; d[60] = s[2]; d[90] = s[3];
    d[120] = s[4]; d[150] = s[5]; d[180] = s[6]; d[210] = s[7];
}
static void mutate(u16 k)                           // k random cells get a new tile
{
    while (k--) {
        u16 c = (u16)(((u32)lfsr() * NC) >> 16);
        tmap[c] = lfsr() & 15;
        dirty[ndirty++] = c;                        // a game would do this where it edits the map
    }
}
static void refresh_full(void) { u16 c; for (c = 0; c < NC; c++) draw_tile(c); ndirty = 0; }
static void refresh_scan(void)
{
    u16 c;
    for (c = 0; c < NC; c++)
        if (shadow[c] != tmap[c]) { shadow[c] = tmap[c]; draw_tile(c); }
    ndirty = 0;
}
static void refresh_list(void)
{
    u16 n = ndirty;
    const u8 *p = dirty;
    while (n--) { u16 c = *p++; if (shadow[c] != tmap[c]) { shadow[c] = tmap[c]; draw_tile(c); } }
    ndirty = 0;
}

#define NFR 256
static unsigned long run_tiles(void (*refresh)(void), u16 k)
{
    unsigned long s;
    u16 n = NFR;
    lf = 0xACE1; ndirty = 0;
    s = ticks; while (ticks == s); s = ticks;
    while (n--) { mutate(k); if (refresh) refresh(); else ndirty = 0; }
    return (ticks - s) * 46875UL / NFR;
}

// ------------------------------------------------------------------ 2. pseudo-3D road
#define HOR 29
#define NL 70                                       // lines HOR+1 .. HOR+70
static u16 zt[NL + 1];
static u8 hw[NL + 1];

static void span(u8 *row, short x0, short x1, u8 pat)
{
    short b0, b1;
    u8 m0, m1;
    if (x0 < 0) x0 = 0;
    if (x1 > 160) x1 = 160;
    if (x0 >= x1) return;
    b0 = x0 >> 3; b1 = (x1 - 1) >> 3;
    m0 = 0xFF >> (x0 & 7); m1 = 0xFF << (7 - ((x1 - 1) & 7));
    if (b0 == b1) { m0 &= m1; row[b0] = (row[b0] & ~m0) | (pat & m0); return; }
    row[b0] = (row[b0] & ~m0) | (pat & m0);
    row[b1] = (row[b1] & ~m1) | (pat & m1);
    for (b0++; b0 < b1; b0++) row[b0] = pat;
}

static void road(u16 pos, short curve)              // curve: ddx in 1/256 pixel per line^2
{
    short i, x = 80 << 8, dx = 0;
    u8 *row = buf + (HOR + NL) * 30;
    memset(buf, 0, (HOR + 1) * 30);                 // sky
    for (i = NL; i >= 1; i--, row -= 30) {          // bottom (near) to top (far)
        u16 st = ((zt[i] + pos) >> 6) & 1;
        short c = x >> 8, w = hw[i], r = (w >> 3) + 1;
        memset(row, st ? ((i & 1) ? 0xAA : 0x55) : 0x00, 20);   // grass: dithered / light
        span(row, c - w - r, c + w + r, st ? 0xFF : 0x00);      // rumble strips
        span(row, c - w, c + w, 0x00);                          // asphalt
        if (st) span(row, c - (r >> 1), c + (r >> 1) + 1, 0xFF);   // centre line
        dx += curve; x += dx;
    }
}
// Same picture, rows written as longs: grass as 5 long stores, then each span masks only its two
// end longs (tables) and stores whole longs in between.
static u32 lm[33], rm[33];                          // lm[k] = pixels k..31, rm[k] = pixels 0..k-1
static void span32(u32 *row, short x0, short x1, u32 pat)
{
    short k0, k1;
    u32 m;
    if (x0 < 0) x0 = 0;
    if (x1 > 160) x1 = 160;
    if (x0 >= x1) return;
    k0 = x0 >> 5; k1 = (x1 - 1) >> 5;
    m = lm[x0 & 31];
    if (k0 == k1) { m &= rm[((x1 - 1) & 31) + 1]; row[k0] = (row[k0] & ~m) | (pat & m); return; }
    row[k0] = (row[k0] & ~m) | (pat & m);
    m = rm[((x1 - 1) & 31) + 1];
    row[k1] = (row[k1] & ~m) | (pat & m);
    for (k0++; k0 < k1; k0++) row[k0] = pat;
}
static void road_l(u16 pos, short curve)
{
    short i, x = 80 << 8, dx = 0;
    u32 *row = (u32 *)(buf + (HOR + NL) * 30);      // rows start at even addresses: long access ok
    memset(buf, 0, (HOR + 1) * 30);
    for (i = NL; i >= 1; i--, row = (u32 *)((u8 *)row - 30)) {
        u16 st = ((zt[i] + pos) >> 6) & 1;
        short c = x >> 8, w = hw[i], r = (w >> 3) + 1;
        u32 g = st ? ((i & 1) ? 0xAAAAAAAAUL : 0x55555555UL) : 0;
        row[0] = g; row[1] = g; row[2] = g; row[3] = g; row[4] = g;
        span32(row, c - w - r, c + w + r, st ? 0xFFFFFFFFUL : 0);
        span32(row, c - w, c + w, 0);
        if (st) span32(row, c - (r >> 1), c + (r >> 1) + 1, 0xFFFFFFFFUL);
        dx += curve; x += dx;
    }
}

static unsigned long run_road(void (*f)(u16, short))
{
    unsigned long s;
    u16 n = 64, pos = 0;
    s = ticks; while (ticks == s); s = ticks;
    while (n--) { f(pos, n & 32 ? 6 : -6); pos += 9; }
    return (ticks - s) * 46875UL / 64;
}

// ------------------------------------------------------------------ 3. rotation
static u32 src[64][2], tmp[64][2], dst[64][2], nav[64][2];
static short xo[64], yo[64];
static u32 bm[32];                                  // 0x80000000 >> k: no variable long shift
#define COS30 222                                   // 8.8
#define SIN30 128
#define TAN15 69

static void make_src(void)
{
    short y, x;
    memset(src, 0, sizeof(src));
    for (y = 10; y < 54; y++)
        for (x = 10; x < 54; x++) {
            short on = y < 12 || y > 51 || x < 12 || x > 51                // frame
                    || (x > 16 && x < 24 && y > 16 && y < 44)                // F stem
                    || (y > 16 && y < 22 && x > 16 && x < 46)                // F top bar
                    || (y > 28 && y < 33 && x > 16 && x < 38)                // F middle bar
                    || ((x - 42) * (x - 42) + (y - 42) * (y - 42) < 30);     // dot
            if (on) src[y][x >> 5] |= 0x80000000UL >> (x & 31);
        }
}

static void offsets(void)                           // per-angle tables (a game stores them)
{
    short k;
    for (k = 0; k < 64; k++) {
        short d = 2 * k - 63;                       // 2 * (k + 0.5 - 32)
        short a = -TAN15 * d, b = SIN30 * d;       // round(-tan(t/2) * (k - 31.5)), round(sin t * ...)
        xo[k] = (a + (a < 0 ? -256 : 256)) / 512;
        yo[k] = (b + (b < 0 ? -256 : 256)) / 512;
    }
}

static void xshear(u32 (*s)[2], u32 (*d)[2])
{
    short y;
    for (y = 0; y < 64; y++) {
        short o = xo[y];
        u32 h = s[y][0], l = s[y][1];
        if (o > 0) { l = (l >> o) | (h << (32 - o)); h >>= o; }
        else if (o < 0) { o = -o; h = (h << o) | (l >> (32 - o)); l <<= o; }
        d[y][0] = h; d[y][1] = l;
    }
}

static void yshear(u32 (*s)[2], u32 (*d)[2])       // runs of columns with one offset move by rows
{
    short half, x0;
    memset(d, 0, 64 * 8);
    for (half = 0; half < 2; half++)
        for (x0 = 0; x0 < 32;) {
            short o = yo[half * 32 + x0], x1 = x0 + 1, y, ya, yb;
            u32 m;
            while (x1 < 32 && yo[half * 32 + x1] == o) x1++;
            m = (0xFFFFFFFFUL >> x0) & ~(x1 == 32 ? 0 : 0xFFFFFFFFUL >> x1);
            ya = o > 0 ? 0 : -o; yb = o > 0 ? 64 - o : 64;
            for (y = ya; y < yb; y++) d[y + o][half] |= s[y][half] & m;
            x0 = x1;
        }
}

static void rot_shear(void) { xshear(src, tmp); yshear(tmp, dst); xshear(dst, dst); }

static void rot_naive(void)                         // dst(x, y) = src(R^-1 (x, y))
{
    short x, y;
    for (y = 0; y < 64; y++) {
        short u = COS30 * -32 + SIN30 * (y - 32) + (32 << 8) + 128;   // +0.5: pixel centres
        short v = -SIN30 * -32 + COS30 * (y - 32) + (32 << 8) + 128;
        u32 *o = nav[y];
        for (x = 0; x < 64; x += 32) {
            u32 acc = 0, bit = 0x80000000UL;
            do {
                u16 su = (u16)u >> 8, sv = (u16)v >> 8;
                if (su < 64 && sv < 64 && (src[sv][su >> 5] & bm[su & 31])) acc |= bit;
                u += COS30; v -= SIN30;
            } while (bit >>= 1);
            *o++ = acc;
        }
    }
}

static unsigned long run_rot(void (*f)(void))
{
    unsigned long s;
    u16 n = 16;
    s = ticks; while (ticks == s); s = ticks;
    while (n--) f();
    return (ticks - s) * 46875UL / 16;
}

static void blit64(u32 (*b)[2], short x)            // x multiple of 8
{
    short y;
    for (y = 0; y < 64; y++) {
        u8 *d = (u8 *)LCD_MEM + (y + 18) * 30 + (x >> 3), *s = (u8 *)b[y];
        short k;
        for (k = 0; k < 8; k++) d[k] = s[k];
    }
}

static void wait_key(void) { GKeyFlush(); ngetchx(); }

void _main(void)
{
    INT_HANDLER o1 = GetIntVec(AUTO_INT_1), o5 = GetIntVec(AUTO_INT_5);
    unsigned long tt[4][3], tr, trl, tn, ts;
    static const u16 ks[3] = { 5, 24, 120 };        // 2 %, 10 %, 50 % of 240 cells
    u16 i, j, diff = 0, on = 0, same;
    buf = malloc(2 * LCD_SIZE);
    if (!buf) return;
    memset(buf, 0, LCD_SIZE);
    for (i = 0; i < 16; i++) for (j = 0; j < 8; j++) tiles[i][j] = (i * 37 + j * 11) ^ (j & 1 ? 0x55 : 0xAA);
    for (i = 0; i < NC; i++) { tmap[i] = i & 15; coff[i] = (i / TW) * (8 * 30) + i % TW; }
    for (i = 0; i < 32; i++) bm[i] = 0x80000000UL >> i;
    for (i = 0; i <= 32; i++) { lm[i] = i == 32 ? 0 : 0xFFFFFFFFUL >> i; rm[i] = i == 32 ? 0xFFFFFFFFUL : ~(0xFFFFFFFFUL >> i); }
    road(100, 6); memcpy(buf + 100 * 30, buf, 100 * 30);          // both road versions must match
    road_l(100, 6); same = !memcmp(buf + 100 * 30, buf, 100 * 30);
    for (i = 1; i <= NL; i++) { zt[i] = 16384U / i; hw[i] = 1 + (i * 70) / NL; }
    make_src(); offsets();
    rot_naive(); rot_shear();
    for (i = 0; i < 64; i++) for (j = 0; j < 2; j++) {
        u32 x = nav[i][j] ^ dst[i][j], a = nav[i][j];
        while (x) { diff += x & 1; x >>= 1; }
        while (a) { on += a & 1; a >>= 1; }
    }

    ClrScr(); printf_xy(0, 0, "Benchmarking...");
    SetIntVec(AUTO_INT_5, DUMMY_HANDLER);
    SetIntVec(AUTO_INT_1, tick_handler);
    for (j = 0; j < 3; j++) {
        tt[0][j] = run_tiles(0, ks[j]);
        tt[1][j] = run_tiles(refresh_full, ks[j]);
        refresh_full(); memcpy(shadow, tmap, NC);
        tt[2][j] = run_tiles(refresh_scan, ks[j]);
        tt[3][j] = run_tiles(refresh_list, ks[j]);
    }
    tr = run_road(road);
    trl = run_road(road_l);
    tn = run_rot(rot_naive);
    ts = run_rot(rot_shear);
    SetIntVec(AUTO_INT_1, o1); SetIntVec(AUTO_INT_5, o5);

    ClrScr(); FontSetSys(F_4x6);
    printf_xy(0, 0, "tile refresh 20x12, cyc/frame (-mutate)");
    printf_xy(0, 7, "changed/frame   5     24    120");
    for (i = 1; i < 4; i++)
        printf_xy(0, 7 + 7 * i, "%-10s %6lu %6lu %6lu", i == 1 ? "full" : i == 2 ? "shadow scan" : "dirty list",
                  tt[i][0] - tt[0][0], tt[i][1] - tt[0][1], tt[i][2] - tt[0][2]);
    printf_xy(0, 35, "mutate only %lu %lu %lu", tt[0][0], tt[0][1], tt[0][2]);
    printf_xy(0, 45, "road 70 lines: bytes %lu, longs %lu %s", tr, trl, same ? "same" : "DIFF");
    printf_xy(0, 55, "rotate 64x64 30deg: naive %lu", tn);
    printf_xy(0, 62, "  3 shears %lu cyc", ts);
    printf_xy(0, 69, "  differ %u of %u px", diff, on);
    printf_xy(0, 79, "keys: road, rotation, tiles");
    wait_key();
    FontSetSys(F_6x8);
    road(100, 6); memcpy(LCD_MEM, buf, 100 * 30); wait_key();
    ClrScr(); blit64(nav, 8); blit64(dst, 88);
    printf_xy(8, 4, "naive"); printf_xy(88, 4, "3 shears"); wait_key();
    memset(buf, 0, LCD_SIZE); refresh_full(); memcpy(LCD_MEM, buf, 100 * 30); wait_key();
    free(buf);
}
