// XOR polygon fill (KB §12): each edge toggles one pixel per column it spans (at the first row
// whose centre is below the edge), then one pass per 32-pixel column does acc ^= *p; *p = acc.
// Compared with a C scanline triangle filler (spans with long mask tables) and with ExtGraph's
// FilledTriangle_R + DrawSpan_OR_R. Both C fillers use the pixel-centre rule (centre inside,
// top/left edges inclusive), so they must give the same pixels on disjoint triangles.
// Scene A: 10 triangles (5x2 cells of 32x50), scene B: 40 triangles (10x4 cells of 16x25).
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>
#include <extgraph.h>

static volatile unsigned long ticks;
DEFINE_INT_HANDLER(tick_handler) { ticks++; }
#define NF 100
#define H 100

static unsigned char *buf, *buf2;
static unsigned char tri[40][6];        // x0 y0 x1 y1 x2 y2, x in 0..160, y in 0..100
static unsigned short ntri;
static unsigned long lmask[32], rmask[32];

// ---- XOR fill ----
static void xor_edge(unsigned char *b, short xa, short ya, short xb, short yb)
{
    short dx, dy, D, n, q, e, q2, r2, step;
    unsigned char *p, m;
    if (xa == xb) return;
    if (xa > xb) { short t = xa; xa = xb; xb = t; t = ya; ya = yb; yb = t; }
    dx = xb - xa; dy = yb - ya; D = dx << 1;
    // toggle row r(x) = ceil(ye(x + 0.5) - 0.5), kept as r*D - e with 0 <= e < D
    n = dy - dx;
    q = n / D; e = n % D;
    if (e > 0) q++;                     // ceil
    e = q * D - n;
    q2 = (dy << 1) / D; r2 = (dy << 1) % D;
    if (r2 < 0) { r2 += D; q2--; }      // floor
    step = (q2 << 5) - (q2 << 1);
    p = b + (ya + q) * 30 + (xa >> 3);  // row 100 is a guard row: toggles there are never swept
    m = 0x80 >> (xa & 7);
    while (dx--) {
        *p ^= m;
        e -= r2;
        if (e < 0) { e += D; p += 30; }
        p += step;
        m >>= 1;
        if (!m) { m = 0x80; p++; }
    }
}

static void xor_sweep(unsigned char *b)
{
    short c;
    for (c = 0; c < 5; c++) {
        unsigned long acc = 0;
        unsigned char *p = b + (c << 2);
        short n = H / 4;
        while (n--) {                   // unrolled by 4
            acc ^= *(unsigned long *)p;        *(unsigned long *)p = acc;
            acc ^= *(unsigned long *)(p + 30); *(unsigned long *)(p + 30) = acc;
            acc ^= *(unsigned long *)(p + 60); *(unsigned long *)(p + 60) = acc;
            acc ^= *(unsigned long *)(p + 90); *(unsigned long *)(p + 90) = acc;
            p += 120;
        }
    }
}

static void draw_xor(unsigned char *b)
{
    unsigned short k;
    for (k = 0; k < ntri; k++) {
        const unsigned char *t = tri[k];
        xor_edge(b, t[0], t[1], t[2], t[3]);
        xor_edge(b, t[2], t[3], t[4], t[5]);
        xor_edge(b, t[4], t[5], t[0], t[1]);
    }
    xor_sweep(b);
}

// ---- scanline filler ----
typedef struct { short v, e, D, q2, r2; } Walk;   // v(r) = ceil(xe(r + 0.5) - 0.5)

static void walk_init(Walk *w, short xa, short ya, short xb, short yb)
{
    short dx = xb - xa, dy = yb - ya, n, q, e;    // dy > 0: y is the major axis here
    w->D = dy << 1;
    n = dx - dy;
    q = n / w->D; e = n % w->D;
    if (e > 0) q++;
    w->v = xa + q; w->e = q * w->D - n;
    w->q2 = (dx << 1) / w->D; w->r2 = (dx << 1) % w->D;
    if (w->r2 < 0) { w->r2 += w->D; w->q2--; }
}
static inline void walk_step(Walk *w)
{
    w->e -= w->r2;
    if (w->e < 0) { w->e += w->D; w->v++; }
    w->v += w->q2;
}

static void span(unsigned char *row, unsigned short x0, unsigned short x1)   // pixels x0..x1-1
{
    unsigned long *a = (unsigned long *)row + (x0 >> 5), *z = (unsigned long *)row + ((x1 - 1) >> 5);
    unsigned long l = lmask[x0 & 31], r = rmask[(x1 - 1) & 31];
    if (a == z) { *a |= l & r; return; }
    *a++ |= l;
    while (a < z) *a++ = 0xFFFFFFFFUL;
    *a |= r;
}

static void scan_tri(unsigned char *b, const unsigned char *t)
{
    short x0 = t[0], y0 = t[1], x1 = t[2], y1 = t[3], x2 = t[4], y2 = t[5], s, y;
    Walk L, S, *left, *right;
    unsigned char *row;
    if (y0 > y1) { s = x0; x0 = x1; x1 = s; s = y0; y0 = y1; y1 = s; }
    if (y1 > y2) { s = x1; x1 = x2; x2 = s; s = y1; y1 = y2; y2 = s; }
    if (y0 > y1) { s = x0; x0 = x1; x1 = s; s = y0; y0 = y1; y1 = s; }
    if (y0 == y2) return;
    walk_init(&L, x0, y0, x2, y2);
    if ((long)(x1 - x0) * (y2 - y0) > (long)(y1 - y0) * (x2 - x0)) { left = &L; right = &S; }
    else { left = &S; right = &L; }
    row = b + y0 * 30;
    if (y1 > y0) {
        walk_init(&S, x0, y0, x1, y1);
        for (y = y1 - y0; y--; row += 30) {
            if (left->v < right->v) span(row, left->v, right->v);
            walk_step(&L); walk_step(&S);
        }
    }
    if (y2 > y1) {
        walk_init(&S, x1, y1, x2, y2);
        for (y = y2 - y1; y--; row += 30) {
            if (left->v < right->v) span(row, left->v, right->v);
            walk_step(&L); walk_step(&S);
        }
    }
}

static void draw_scan(unsigned char *b)
{
    unsigned short k;
    for (k = 0; k < ntri; k++) scan_tri(b, tri[k]);
}

static void draw_eg(unsigned char *b)
{
    unsigned short k;
    for (k = 0; k < ntri; k++) {
        const unsigned char *t = tri[k];
        FilledTriangle_R(t[0], t[1], t[2], t[3], t[4], t[5], b, DrawSpan_OR_R);
    }
}

// ---- scenes ----
static unsigned short rs = 7;
static unsigned short rnd(unsigned short n) { rs ^= rs << 7; rs ^= rs >> 9; rs ^= rs << 8; return ((unsigned long)rs * n) >> 16; }

static void scene(unsigned short cols, unsigned short rows)
{
    unsigned short cw = 160 / cols, ch = H / rows, i, j, k;
    ntri = 0;
    for (j = 0; j < rows; j++)
        for (i = 0; i < cols; i++) {
            unsigned char *t = tri[ntri++];
            for (k = 0; k < 3; k++) {
                t[2 * k] = i * cw + rnd(cw + 1);
                t[2 * k + 1] = j * ch + rnd(ch + 1);
            }
        }
}

static unsigned short diffbits(void)
{
    unsigned short k, n = 0;
    for (k = 0; k < H * 30; k++) {
        unsigned char d = buf[k] ^ buf2[k];
        while (d) { n += d & 1; d >>= 1; }
    }
    return n;
}
static unsigned short setbits(unsigned char *b)
{
    unsigned short k, n = 0;
    for (k = 0; k < H * 30; k++) { unsigned char d = b[k]; while (d) { n += d & 1; d >>= 1; } }
    return n;
}

static unsigned long timeit(void (*f)(unsigned char *))
{
    unsigned long s = ticks;
    unsigned short n = NF;
    while (ticks == s);
    s = ticks;
    while (n--) f(buf);
    return (ticks - s) * 46875UL / NF;
}
static void t_clear(unsigned char *b) { memset(b, 0, H * 30); }

static void show(unsigned char *b)
{
    memcpy(LCD_MEM, b, H * 30);
    GKeyFlush(); ngetchx();
}

void _main(void)
{
    INT_HANDLER o1 = GetIntVec(AUTO_INT_1), o5 = GetIntVec(AUTO_INT_5);
    unsigned long t[2][4];
    unsigned short diff[2], cover[2], k;
    buf = malloc(LCD_SIZE); buf2 = malloc(LCD_SIZE);
    if (!buf || !buf2) goto out;
    for (k = 0; k < 32; k++) { lmask[k] = 0xFFFFFFFFUL >> k; rmask[k] = ~(0x7FFFFFFFUL >> k); }
    for (k = 0; k < 2; k++) {
        scene(k ? 10 : 5, k ? 4 : 2);
        memset(buf, 0, LCD_SIZE); draw_xor(buf);
        memset(buf2, 0, LCD_SIZE); draw_scan(buf2);
        diff[k] = diffbits(); cover[k] = setbits(buf2);
        ClrScr(); printf_xy(0, 0, "Benchmarking...");
        SetIntVec(AUTO_INT_5, DUMMY_HANDLER);
        SetIntVec(AUTO_INT_1, tick_handler);
        t[k][0] = timeit(t_clear);
        t[k][1] = timeit(draw_xor);
        t[k][2] = timeit(draw_scan);
        t[k][3] = timeit(draw_eg);
        SetIntVec(AUTO_INT_1, o1); SetIntVec(AUTO_INT_5, o5);
    }
    ClrScr(); FontSetSys(F_4x6);
    printf_xy(0, 0, "cycles/frame     10 tri   40 tri");
    printf_xy(0, 8, "clear 3000 B   %7lu  %7lu", t[0][0], t[1][0]);
    printf_xy(0, 16, "XOR fill       %7lu  %7lu", t[0][1], t[1][1]);
    printf_xy(0, 24, "C scanline     %7lu  %7lu", t[0][2], t[1][2]);
    printf_xy(0, 32, "ExtGraph tri   %7lu  %7lu", t[0][3], t[1][3]);
    printf_xy(0, 40, "pixels set     %7u  %7u", cover[0], cover[1]);
    printf_xy(0, 48, "XOR vs scan    %5u px %5u px", diff[0], diff[1]);
    printf_xy(0, 56, "differ. Keys: XOR A, scan A, XOR B");
    GKeyFlush(); ngetchx();
    FontSetSys(F_6x8);
    scene(5, 2);
    memset(buf, 0, LCD_SIZE); draw_xor(buf); show(buf);
    memset(buf, 0, LCD_SIZE); draw_scan(buf); show(buf);
    scene(10, 4);
    memset(buf, 0, LCD_SIZE); draw_xor(buf); show(buf);
out:
    free(buf2); free(buf);
}
