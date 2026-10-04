// Voxel landscape (Comanche style), seen in rwill's "Just some small effects" (pouet 85624):
// a 128x128 height map, columns drawn front to back, one "highest row so far" per column so each
// pixel is written once, lighting baked into the map from the slope. Measured under ti-cycles.
// Differences from the demo, all to cut per-sample work:
// - height and shade in one word per map cell (one read instead of two);
// - screen y from a per-slice table ytab[s][h] (no muls per sample), rebuilt only when the
//   camera height changes;
// - drawn straight into the two planes (no 2-bpp buffer and no 145k conversion);
// - a rotating camera (the demo's camera only translates).
// Variants (-D): NC columns (160/NC pixels wide each), S depth slices.
//   voxel8033: NC=80 S=33 (the demo's sizes)   voxel4024: NC=40 S=24   voxel4016: NC=40 S=16
// -DMOVIE=N: N frames with their cycles and screens, for the comparison GIF.
// -DVOX_ASM: the slice loop in asm (voxasm.s, -Wa,--defsym,PH=2 or 4), same pixels.
// Zones: 1 clear, 2 terrain, 3 ytab rebuild (per camera-height change), 4 frame set-up.
#define USE_TI89
#include <tigcclib.h>
#include "../../tools/m68kbench/bench.h"
#include "trig.h"

#ifndef NC
#define NC 40
#endif
#ifndef S
#define S 24
#endif
#define CW (160 / NC)                               // column width in pixels: 2 or 4
#define PH (8 / CW)                                 // columns per byte
#define VH 100                                      // view height
#define HOR 15
#define K 48
#define NFR 8

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

static u8 *buf;                                     // light plane, dark plane at +0xF00
static u16 *map;                                    // 128x128: level * 4 << 8 | height
static u8 ytab[S][64];
static u8 zs[S];
static u16 kz[S];                                   // K * 256 / z
u8 vox_pat[PH][7][2][4];                            // phase, level, first row parity: l, d, l, d of 2 rows
u16 vox_rowo[VH + 1];                               // y * 30
static u8 top[NC];

#ifdef VOX_ASM                                      // voxasm.s: one slice
typedef struct { u32 U, V, dU, dV; const u8 *map, *yt; u8 *top, *buf; } VoxSlice;
void vox_slice(const VoxSlice *p asm("%a0")) __attribute__((__regparm__(1)));
void vox_clear(void *buf asm("%a0")) __attribute__((__regparm__(1)));
#endif

static inline long muls16(short a, short b)          // one muls.w (performance §4)
{ long r; asm("muls.w %2,%0" : "=d"(r) : "0"(a), "dmi"(b)); return r; }

#define SIN(a) ((short)sin_tab[(u8)(a)])
#define COS(a) ((short)sin_tab[(u8)((a) + 64)])

static u16 lf = 0xACE1;
static short rnd(short amp)                         // -amp..amp, LFSR
{
    lf = (lf >> 1) ^ (-(lf & 1) & 0xB400);
    return (short)((long)(lf & 0x7FFF) * (2 * amp + 1) >> 15) - amp;
}

// tileable diamond-square fractal (heights 0..63), shade from the slope towards (+1, +1)
static void make_map(void)
{
    static short h[128][128];
    u16 x, y, st;
    short amp = 48, lo = 32767, hi = -32768;
    h[0][0] = 0;
    for (st = 128; st > 1; st >>= 1, amp = amp * 5 / 8) {
        u16 hs = st / 2;
        for (y = 0; y < 128; y += st)               // diamond: square centres
            for (x = 0; x < 128; x += st)
                h[y + hs][x + hs] = (h[y][x] + h[y][(x + st) & 127] + h[(y + st) & 127][x]
                                     + h[(y + st) & 127][(x + st) & 127]) / 4 + rnd(amp);
        for (y = 0; y < 128; y += hs)               // square: edge midpoints
            for (x = (y + hs) % st; x < 128; x += st)
                h[y][x] = (h[(y - hs) & 127][x] + h[(y + hs) & 127][x] + h[y][(x - hs) & 127]
                           + h[y][(x + hs) & 127]) / 4 + rnd(amp);
    }
    for (y = 0; y < 128; y++)
        for (x = 0; x < 128; x++) { if (h[y][x] < lo) lo = h[y][x]; if (h[y][x] > hi) hi = h[y][x]; }
    for (y = 0; y < 128; y++)
        for (x = 0; x < 128; x++) {
            short v = (short)((long)(h[y][x] - lo) * 63 / (hi - lo));
            h[y][x] = v < 14 ? 14 : v;              // flat valleys: water
        }
    for (y = 0; y < 128; y++)
        for (x = 0; x < 128; x++) {
            short d = h[y][x] - h[(y + 1) & 127][(x + 1) & 127];
            short l = h[y][x] == 14 ? 1 : 3 + d;
            l = l < 0 ? 0 : l > 6 ? 6 : l;
            map[y * 128 + x] = (u16)(l * 4) << 8 | h[y][x];
        }
}

static void make_patterns(void)
{
    u16 k, l, r, x;
    for (k = 0; k < PH; k++) {
        u8 m = (u8)(((1 << CW) - 1) << (8 - CW * (k + 1)));
        for (l = 0; l < 7; l++)
            for (r = 0; r < 4; r++) {               // rows of parity r & 1 (r: 0, 1, then 2 = 0, 3 = 1)
                u8 pl = 0, pd = 0;
                for (x = 0; x < 8; x++) {           // level l: grey l/2, odd = checker with the next
                    u16 c = l / 2 + ((l & 1) && ((x + r) & 1));
                    pl |= (c & 1) << (7 - x);
                    pd |= (c >> 1) << (7 - x);
                }
                if (r < 2) { vox_pat[k][l][r][0] = pl & m; vox_pat[k][l][r][1] = pd & m; }
                else { vox_pat[k][l][(r - 2) ^ 1][2] = pl & m; vox_pat[k][l][(r - 2) ^ 1][3] = pd & m; }
            }
    }
}

static void build_ytab(short ch)
{
    u16 s, h;
    for (s = 0; s < S; s++) {
        long acc = (long)HOR * 256 + (long)ch * kz[s];
        u8 *t = ytab[s];
        for (h = 0; h < 64; h++) {
            short y = (short)(acc >> 8);
            t[h] = y < 0 ? 0 : y > VH ? VH : y;
            acc -= kz[s];
        }
    }
}

static void clear_view(void)
{
#ifdef VOX_ASM
    vox_clear(buf);
#else
    u32 *p = (u32 *)buf, *q = (u32 *)(buf + 0xF00);
    u16 n = VH;
    while (n--) {                                   // 20 bytes of each 30-byte row
        p[0] = 0; p[1] = 0; p[2] = 0; p[3] = 0; p[4] = 0;
        q[0] = 0; q[1] = 0; q[2] = 0; q[3] = 0; q[4] = 0;
        p = (u32 *)((u8 *)p + 30); q = (u32 *)((u8 *)q + 30);
    }
#endif
}

#ifndef VOX_ASM
// rows of one column: 2 rows (both planes) per pass, patterns for even and odd rows
static __attribute__((noinline)) void fill(u8 *p, const u8 *q, u16 n)
{
    u8 a = q[0], b = q[1], c = q[2], d = q[3];
    u16 m = n >> 1;
    while (m--) { p[0] |= a; p[0xF00] |= b; p[30] |= c; p[0xF1E] |= d; p += 60; }
    if (n & 1) { p[0] |= a; p[0xF00] |= b; }
}
#endif

// one column: sample the map at (U, V), fill the rows it uncovers. The map offset is
// row * 256 + column * 2: V's high word holds row << 8, U's high word column * 2.
#define SAMPLE(k)                                                                        \
    {                                                                                    \
        const u8 *mp = mapb + (short)(((u16)(V >> 16) & 0x7F00) | ((u16)(U >> 16) & 0xFE)); \
        u8 y = yt[mp[1]];                                                                \
        u8 t = *tp;                                                                      \
        if (y < t) {                                                                     \
            *tp = y;                                                                     \
            fill(cb + vox_rowo[y], &vox_pat[k][0][0][0] + mp[0] * 2 + ((y & 1) << 2), t - y);    \
        }                                                                                \
        tp++; U += dU; V += dV;                                                          \
    }

static void render(u16 cx, u16 cy, u8 ang)
{
    short dx = COS(ang) * 2, dy = SIN(ang) * 2;     // 8.8 per unit of z
    short px = -dy, py = dx, px3 = px * 3, py3 = py * 3;
    short sx = (short)(((long)px * 3 * 256) / (2 * NC)), sy = (short)(((long)py * 3 * 256) / (2 * NC));
    const u8 *mapb = (const u8 *)map;
    u16 s, b, c;
    BENCH_BEGIN(4);
    for (c = 0; c < NC; c++) top[c] = VH;
    BENCH_END(4);
    BENCH_BEGIN(2);
    for (s = 0; s < S; s++) {
        short z = zs[s];
        u32 U = (u32)(u16)(cx + muls16(z, dx) - (muls16(z, px3) >> 2)) << 9;
        u32 V = (u32)(u16)(cy + muls16(z, dy) - (muls16(z, py3) >> 2)) << 16;
        u32 dU = muls16(z, sx) << 1, dV = muls16(z, sy) << 8;
#ifdef VOX_ASM
        VoxSlice p;
        p.U = U; p.V = V; p.dU = dU; p.dV = dV;
        p.map = mapb; p.yt = ytab[s]; p.top = top; p.buf = buf;
        vox_slice(&p);
        (void)b;
#else
        const u8 *yt = ytab[s];
        u8 *tp = top, *cb = buf;
        for (b = 0; b < 20; b++, cb++) {
            SAMPLE(0)
            SAMPLE(1)
#if PH == 4
            SAMPLE(2)
            SAMPLE(3)
#endif
        }
#endif
    }
    BENCH_END(2);
}

void _main(void)
{
    u16 f, s, z = 3;
    u16 cx = 20 << 8, cy = 30 << 8;
    u8 ang = 10;
    BENCH_NAME(1, "clear view");
    BENCH_NAME(2, "terrain");
    BENCH_NAME(3, "ytab rebuild");
    BENCH_NAME(4, "frame set-up");
    for (s = 0; s <= VH; s++) vox_rowo[s] = s * 30;
    buf = malloc(7680);
    map = malloc(128UL * 128 * 2);
    if (!buf || !map) { if (buf) free(buf); return; }
    make_map();
    make_patterns();
    for (s = 0; s < S; s++) {
        zs[s] = z; kz[s] = (u16)((long)K * 256 / z);
#if S >= 32
        z += 1 + (z >> 4);
#elif S >= 24
        z += 1 + (z >> 3);
#else
        z += 1 + (z >> 2);
#endif
    }
    BENCH_VALUE(z);                                 // depth reached
#ifdef MOVIE
    // a movie for the side-by-side GIF: MOVIE frames, a fixed camera height (the y table is built
    // once), per frame its cycles (BENCH_VALUE) and its screen
    build_ytab(70);
    for (f = 0; f < MOVIE; f++) {
        unsigned long c0 = BENCH_CYCLES;
        clear_view();
        render(cx, cy, ang);
        BENCH_VALUE(BENCH_CYCLES - c0);
        BENCH_SHOT(buf);
        cx += COS(ang) / 2; cy += SIN(ang) / 2;
        ang += (f / 60) & 1 ? 2 : -1;
    }
#else
    for (f = 0; f < NFR; f++) {
        BENCH_BEGIN(3); build_ytab(70); BENCH_END(3);
        BENCH_BEGIN(1); clear_view(); BENCH_END(1);
        render(cx, cy, ang);
        cx += COS(ang) / 2; cy += SIN(ang) / 2; ang += 3;
    }
    BENCH_SHOT(buf);
#endif
    free(map); free(buf);
}
