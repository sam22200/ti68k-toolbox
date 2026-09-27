// Third benchmark set: 3D vertex transform variants, perspective projection, Mode 7 row rendering.
// Same harness as bench.c. Per iteration: one vertex (tests 1-5) or one 40-pixel Mode 7 row (6-7).
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>
#include "extgraph.h"
#include "../tables/tables.h"          // sin_tab[256] (signed char, x127), atan_tab

#define SIN(a) sin_tab[(unsigned char)(a)]
// One muls.w, always. GCC turns (long)a * b into a __mulsi3 call when an operand is shared by
// several products (it widens it to long once): see the knowledge base.
static inline long muls16(short a, short b) { long r; asm("muls.w %2,%0" : "=d"(r) : "0"(a), "dmi"(b)); return r; }
// 32/16 signed division in one divs.w (quotient must fit 16 bits, else the result is garbage)
static inline short divs32_16(long n, short d) { asm("divs.w %1,%0" : "+d"(n) : "dmi"(d)); return (short)n; }
#define COS(a) sin_tab[(unsigned char)((a) + 64)]

static volatile unsigned long ticks;
DEFINE_INT_HANDLER(tick_handler) { ticks++; }

volatile long sink;
volatile short vx = 20, vy = -13, vz = 7;
volatile unsigned char va = 37, vb = 91, vc = 150;   // volatile: no constant folding of the angles
static short m[9];                                   // 3x3 rotation matrix, x127
static unsigned short recip[256];                    // recip[z] = 2^15 / z
unsigned char m7map[64 * 64];                        // Mode 7 texture, 64x64 (global: used by m7row.s)
#define map m7map
void m7row_asm(unsigned char *out asm("%a0"), const short *horz asm("%a1"), short cx asm("%d0"),
               short vrow asm("%d1")) __attribute__((__regparm__(4)));
static short horz[40], hstep[40];                    // grid table, incremental step
static unsigned char row[40];

typedef struct { const char *name; long n; } TEST;
static const TEST tests[] = {
    {"empty loop", 100000L}, {"3 axis rot", 20000L}, {"3x3 muls16", 20000L},
    {"long matrix", 5000L},  {"proj divs", 30000L},  {"proj recip", 30000L},
    {"m7 row incr", 3000L},  {"m7 row grid", 3000L},  {"proj C shared", 10000L}, {"3x3 16-bit", 20000L}, {"m7 row ASM", 3000L},
    {"proj long/long", 10000L}, {"proj divs32_16", 30000L},
};
#define NTESTS (sizeof(tests) / sizeof(tests[0]))
static unsigned long t_ticks[NTESTS];
static short asm_ok;

static unsigned long run(short which, long n)
{
    long i;
    long acc = 0;
    unsigned long start;
    short x = vx, y = vy, z = vz;
    unsigned char a = va, b = vb, c = vc;

    start = ticks;
    while (ticks == start);
    start = ticks;
    switch (which) {
    case 0: for (i = 0; i < n; i++) acc += i; break;
    case 1: for (i = 0; i < n; i++) {                     // chained per-axis rotations (3D tut ex5)
              short px = x ^ ((short)i & 31), py = y + ((short)i & 15), pz = z - ((short)i & 7), t;
              t  = (py * COS(a) - pz * SIN(a)) >> 7; pz = (py * SIN(a) + pz * COS(a)) >> 7; py = t;
              t  = (px * COS(b) + pz * SIN(b)) >> 7; pz = (pz * COS(b) - px * SIN(b)) >> 7; px = t;
              t  = (px * COS(c) - py * SIN(c)) >> 7; py = (px * SIN(c) + py * COS(c)) >> 7; px = t;
              acc += px + py + pz; } break;
    case 2: for (i = 0; i < n; i++) {                     // one matrix per frame, 9 muls per vertex
              short px = x ^ ((short)i & 31), py = y + ((short)i & 15), pz = z - ((short)i & 7);
              acc += ((muls16(px, m[0]) + muls16(py, m[1]) + muls16(pz, m[2])) >> 7)
                   + ((muls16(px, m[3]) + muls16(py, m[4]) + muls16(pz, m[5])) >> 7)
                   + ((muls16(px, m[6]) + muls16(py, m[7]) + muls16(pz, m[8])) >> 7); } break;
    case 9: for (i = 0; i < n; i++) {                     // same matrix, 16-bit products (range allows)
              short px = x ^ ((short)i & 31), py = y + ((short)i & 15), pz = z - ((short)i & 7);
              acc += ((short)(px * m[0] + py * m[1] + pz * m[2]) >> 7) + ((short)(px * m[3] + py * m[4] + pz * m[5]) >> 7)
                   + ((short)(px * m[6] + py * m[7] + pz * m[8]) >> 7); } break;
    case 3: for (i = 0; i < n; i++) {                     // edit3d style: long operands
              long px = x ^ ((short)i & 31), py = y + ((short)i & 15), pz = z - ((short)i & 7);
              acc += ((px * m[0] + py * m[1] + pz * m[2]) >> 7) + ((px * m[3] + py * m[4] + pz * m[5]) >> 7)
                   + ((px * m[6] + py * m[7] + pz * m[8]) >> 7); } break;
    case 4: for (i = 0; i < n; i++) {                     // perspective: 2 divisions
              short pz = 64 + ((short)i & 127), px = x ^ (short)i;
              acc += (px << 7) / pz + ((y + ((short)i & 15)) << 7) / pz; } break;
    case 5: for (i = 0; i < n; i++) {                     // perspective: reciprocal table, 2 mulu
              short pz = 64 + ((short)i & 127), px = x ^ (short)i;
              short r = recip[pz];                            // fits a short
              acc += (short)(muls16(px, r) >> 8) + (short)(muls16(y + ((short)i & 15), r) >> 8); } break;
    case 6: for (i = 0; i < n; i++) {                     // Mode 7 row, incremental u/v in 8.8
              unsigned short u = (unsigned short)i << 4, v = 0x1234, du = 300, dv = 77;
              unsigned char *o = row; short k;
              for (k = 39; k >= 0; k--) { *o++ = map[((v >> 8) & 63) << 6 | ((u >> 8) & 63)]; u += du; v += dv; }
              acc += row[5]; } break;
    case 7: for (i = 0; i < n; i++) {                     // Mode 7 row, precomputed grid (engine style)
              short cx = (short)i << 4, vrow = 0x12;
              const short *h = horz; unsigned char *o = row; short k;
              for (k = 39; k >= 0; k--) { *o++ = map[(vrow & 63) << 6 | (((cx + *h++) >> 8) & 63)]; }
              acc += row[5]; } break;
    case 8: for (i = 0; i < n; i++) {                     // same in plain C: r shared -> __mulsi3
              short pz = 64 + ((short)i & 127), px = x ^ (short)i;
              short r = recip[pz];
              acc += (short)(((long)px * r) >> 8) + (short)(((long)(y + ((short)i & 15)) * r) >> 8); } break;
    case 10: for (i = 0; i < n; i++) {                    // case 7 hand-written in asm (m7row.s)
              m7row_asm(row, horz, (short)i << 4, 0x12);
              acc += row[5]; } break;
    case 11: for (i = 0; i < n; i++) {                   // X3D style: (long)x * scale / z -> __divsi3
              short pz = 64 + ((short)i & 127), px = x ^ (short)i, py = y + ((short)i & 15);
              acc += (short)(((long)px * 120) / pz) + (short)(((long)py * 120) / pz); } break;
    case 12: for (i = 0; i < n; i++) {                   // same with one divs.w each
              short pz = 64 + ((short)i & 127), px = x ^ (short)i, py = y + ((short)i & 15);
              acc += divs32_16(muls16(px, 120), pz) + divs32_16(muls16(py, 120), pz); } break;
    }
    sink = acc;
    return ticks - start;
}

void _main(void)
{
    INT_HANDLER old_int1 = GetIntVec(AUTO_INT_1), old_int2 = GetIntVec(AUTO_INT_2),
                old_int5 = GetIntVec(AUTO_INT_5);
    short k, line;
    long empty_cyc_x100;

    for (k = 0; k < 9; k++) m[k] = SIN(k * 23);
    for (k = 1; k < 256; k++) recip[k] = 32768U / k;
    for (k = 0; k < 64 * 64; k++) map[k] = k * 7;
    for (k = 0; k < 40; k++) { horz[k] = k * 300; hstep[k] = 300; }

    {                                                   // the asm row must equal the C row
        unsigned char ref[40]; short j, ok = 1;
        for (j = 0; j < 40; j++) ref[j] = map[(0x12 & 63) << 6 | (((1234 + horz[j]) >> 8) & 63)];
        m7row_asm(row, horz, 1234, 0x12);
        for (j = 0; j < 40; j++) if (row[j] != ref[j]) ok = 0;
        asm_ok = ok;
    }
    ClrScr();
    printf_xy(0, 0, "Benchmarking...");
    while (_rowread(0)) ;
    SetIntVec(AUTO_INT_2, DUMMY_HANDLER);
    SetIntVec(AUTO_INT_5, DUMMY_HANDLER);
    SetIntVec(AUTO_INT_1, tick_handler);
    for (k = 0; k < (short)NTESTS; k++) {
        unsigned long t1 = run(k, tests[k].n), t2 = run(k, tests[k].n);
        t_ticks[k] = t1 < t2 ? t1 : t2;
    }
    SetIntVec(AUTO_INT_1, old_int1);
    SetIntVec(AUTO_INT_5, old_int5);
    SetIntVec(AUTO_INT_2, old_int2);

    empty_cyc_x100 = (long)(t_ticks[0] * 4687500ULL / tests[0].n);
    ClrScr();
    FontSetSys(F_4x6);
    printf_xy(0, 0, "~cycles/op @12MHz (loop cost removed)");
    for (k = 0, line = 7; k < (short)NTESTS; k++, line += 6) {
        long c100 = (long)(t_ticks[k] * 4687500ULL / tests[k].n);
        if (k > 0) c100 -= empty_cyc_x100;
        printf_xy(0, line, "%-12s %7ld  (%lu t)", tests[k].name, c100 / 100, t_ticks[k]);
    }
    printf_xy(0, line + 1, "asm row == C row: %s", asm_ok ? "yes" : "NO");
    GKeyFlush();
    ngetchx();
}
