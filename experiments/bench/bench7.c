// Ideas from the web research, measured: small PRNGs, Lemire's fastrange, alpha-max distance,
// and one cave cellular-automaton step (wall if >= 5 walls in the 3x3), per cell vs bit-sliced
// (32 cells per long word). Checks that both CA versions give the same grid.
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>

static volatile unsigned long ticks;
DEFINE_INT_HANDLER(tick_handler) { ticks++; }
volatile unsigned short sink;
#define N 30000

static unsigned short xs = 1;
static unsigned short xorshift(void) { xs ^= xs << 7; xs ^= xs >> 9; xs ^= xs << 8; return xs; }
static unsigned long mz = 362436069, mw = 521288629;
static unsigned short mwc(void)
{
    mz = 36969UL * (unsigned short)mz + (mz >> 16);
    mw = 18000UL * (unsigned short)mw + (mw >> 16);
    return (unsigned short)mz + (unsigned short)(mw >> 16);
}
static unsigned short wy = 1;
static unsigned short wyhash16(void)
{
    unsigned long h;
    wy += 0xfc15;
    h = (unsigned long)wy * 0x2ab;
    return (unsigned short)(h >> 16) ^ (unsigned short)h;
}
static unsigned short lf = 1;
static unsigned short lfsr(void) { lf = (lf >> 1) ^ (-(lf & 1) & 0xB400); return lf; }

static void b_empty(void)  { unsigned short n = N; while (n--) sink = n; }
static void b_random(void) { unsigned short n = N; while (n--) sink = random(100); }
static void b_xs(void)     { unsigned short n = N; while (n--) sink = xorshift(); }
static void b_mwc(void)    { unsigned short n = N; while (n--) sink = mwc(); }
static void b_wy(void)     { unsigned short n = N; while (n--) sink = wyhash16(); }
static void b_lfsr(void)   { unsigned short n = N; while (n--) sink = lfsr(); }
static void b_mod(void)    { unsigned short n = N; while (n--) sink = n % 100; }   // n is unsigned: GCC uses mulu
static volatile unsigned short dv = 100;
static void b_modv(void)   { unsigned short n = N; while (n--) sink = n % dv; }    // runtime divisor: divu
static void b_range(void)  { unsigned short n = N; while (n--) sink = ((unsigned long)n * dv) >> 16; }
static void b_dist(void)
{
    unsigned short n = N;
    while (n--) {
        unsigned short dx = n & 127, dy = (n >> 7) & 127, mx = dx > dy ? dx : dy, mn = dx ^ dy ^ mx;
        sink = mx + (mn >> 2) + (mn >> 3);
    }
}

// ---- cave CA on 160x100 ----
#define W 160
#define H 100
#define WL (W / 32)
static unsigned char *cg, *cg2;         // (W+2)*(H+2) bytes, border of walls
static unsigned long *bg, *bg2;         // H*WL longs, bit 31 = leftmost cell

static void ca_cells(void)
{
    short x, y;
    for (y = 1; y <= H; y++) {
        const unsigned char *u = cg + (y - 1) * (W + 2), *m = u + W + 2, *d = m + W + 2;
        unsigned char *o = cg2 + y * (W + 2) + 1;
        for (x = W - 1; x >= 0; x--, u++, m++, d++)
            *o++ = (u[0] + u[1] + u[2] + m[0] + m[1] + m[2] + d[0] + d[1] + d[2]) >= 5;
    }
}

// horizontal 3-sums of one row as two bit planes (s1 s0)
static void hsum(const unsigned long *r, unsigned long *s0, unsigned long *s1)
{
    unsigned long prev = 0xFFFFFFFF, c = r[0], next;
    short k;
    for (k = 0; k < WL; k++) {
        unsigned long L, R;
        next = k < WL - 1 ? r[k + 1] : 0xFFFFFFFF;
        L = (c >> 1) | (prev << 31);
        R = (c << 1) | (next >> 31);
        s0[k] = L ^ c ^ R;
        s1[k] = (L & c) | (R & (L ^ c));
        prev = c; c = next;
    }
}

static void ca_bits(void)
{
    static const unsigned long wall[WL] = { ~0UL, ~0UL, ~0UL, ~0UL, ~0UL };
    unsigned long a0[WL], a1[WL], b0[WL], b1[WL], c0[WL], c1[WL];
    unsigned long *p0 = a0, *p1 = a1, *q0 = b0, *q1 = b1, *r0 = c0, *r1 = c1, *t;
    short y, k;
    hsum(wall, p0, p1);
    hsum(bg, q0, q1);
    for (y = 0; y < H; y++) {
        unsigned long *o = bg2 + y * WL;
        hsum(y < H - 1 ? bg + (y + 1) * WL : wall, r0, r1);
        for (k = 0; k < WL; k++) {
            unsigned long x0 = p0[k], y0 = q0[k], z0 = r0[k];
            unsigned long t0 = x0 ^ y0 ^ z0, cy = (x0 & y0) | (z0 & (x0 ^ y0));
            unsigned long p = p1[k], q = q1[k], r = r1[k];
            unsigned long u = p | q, v = p & q, w = r | cy, x = r & cy;
            unsigned long ge2 = v | x | (u & w), ge3 = (v & w) | (x & u);
            o[k] = ge3 | (ge2 & t0);
        }
        t = p0; p0 = q0; q0 = r0; r0 = t;
        t = p1; p1 = q1; q1 = r1; r1 = t;
    }
}

static void ca_init(void)
{
    short x, y;
    memset(cg, 1, (W + 2) * (H + 2));
    memset(cg2, 1, (W + 2) * (H + 2));
    xs = 12345;
    for (y = 0; y < H; y++)
        for (x = 0; x < W; x++) {
            unsigned char v = (unsigned short)(((unsigned long)xorshift() * 100) >> 16) < 45;
            cg[(y + 1) * (W + 2) + x + 1] = v;
            if (v) bg[y * WL + (x >> 5)] |= 0x80000000UL >> (x & 31);
            else   bg[y * WL + (x >> 5)] &= ~(0x80000000UL >> (x & 31));
        }
}

static short ca_same(void)
{
    short x, y;
    for (y = 0; y < H; y++)
        for (x = 0; x < W; x++)
            if (cg2[(y + 1) * (W + 2) + x + 1] != ((bg2[y * WL + (x >> 5)] >> (31 - (x & 31))) & 1)) return 0;
    return 1;
}

static void b_cells(void) { ca_cells(); }
static void b_bits(void)  { short i; for (i = 0; i < 10; i++) ca_bits(); }

#define NB 12
static void (*const fn[NB])(void) = { b_empty, b_random, b_xs, b_mwc, b_wy, b_lfsr, b_mod, b_modv, b_range, b_dist, b_cells, b_bits };
static const char *const nm[NB] = { "loop+store", "random(100)", "xorshift16", "MWC 2x16", "wyhash16", "LFSR16",
                                    "n%100", "n%var", "fastrange", "alphamax", "CA per cell", "CA bitslice" };

void _main(void)
{
    INT_HANDLER o1 = GetIntVec(AUTO_INT_1), o5 = GetIntVec(AUTO_INT_5);
    unsigned long t[NB], s;
    short k, same;
    cg = malloc((W + 2) * (H + 2)); cg2 = malloc((W + 2) * (H + 2));
    bg = malloc(H * WL * 4); bg2 = malloc(H * WL * 4);
    if (!cg || !cg2 || !bg || !bg2) goto out;
    ca_init(); ca_cells(); ca_bits(); same = ca_same();
    ClrScr(); printf_xy(0, 0, "Benchmarking...");
    SetIntVec(AUTO_INT_5, DUMMY_HANDLER);
    SetIntVec(AUTO_INT_1, tick_handler);
    for (k = 0; k < NB; k++) {
        s = ticks; while (ticks == s); s = ticks;
        fn[k]();
        t[k] = ticks - s;
    }
    SetIntVec(AUTO_INT_1, o1); SetIntVec(AUTO_INT_5, o5);
    ClrScr(); FontSetSys(F_4x6);
    for (k = 0; k < 10; k++) {
        long c = (long)(t[k] * 46875UL / N) - (k ? (long)(t[0] * 46875UL / N) : 0);
        printf_xy(k < 5 ? 0 : 80, 6 * (k % 5), "%-11s%4ld", nm[k], c);
    }
    printf_xy(0, 32, "CA step per cell: %lu cyc", t[10] * 46875UL);
    printf_xy(0, 40, "CA step bitslice: %lu cyc", t[11] * 46875UL / 10);
    printf_xy(0, 48, "same grid: %s", same ? "yes" : "NO");
    GKeyFlush();
    ngetchx();
    FontSetSys(F_6x8);
out:
    free(bg2); free(bg); free(cg2); free(cg);
}
