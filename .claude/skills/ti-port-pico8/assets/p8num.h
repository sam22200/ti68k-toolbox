// p8num.h: PICO-8 numbers in C, bit-exact with PICO-8 as z8lua implements it (16.16 fixed
// point in a s32, wrapping). Copy into games/<name>/ and include after rt.h. Verified against
// z8lua by .claude/skills/ti-port-pico8/scripts/test_p8num.sh (host build).
// Cost on the 68000 (ti-cycles, datasheet timings, loop overhead included): a 16.16 add ~36
// cycles, p8_mul ~380 (4 muls/mulu, no library call), p8_rndi(n) ~420 (one divu.w),
// p8_rnd(FIX(1)) ~200, p8_div a 64-bit library division (thousands): keep p8_mul/p8_div out of inner loops and use the
// constant forms (p8_muli, p8_div2k, p8_mod2k, FIXB products folded by hand) where you can.
#ifndef P8NUM_H
#define P8NUM_H

typedef s32 fix;

// Constants. Integers: FIX(n). Decimals: the bits z8lua prints (tostr(v, true)), because
// PICO-8 truncates decimal literals (0.6 = 0x0.9999, not 0x0.999a) and a negative literal is
// the negated truncated value (-0.21 = -0x0.35c2): FIXB(0x35c2) is 0.21, FIXB(-0x35c2) -0.21.
// FIXB pastes an L on the literal: GCC4TI's int is 16 bits, so a bare -0xb505 is computed
// in unsigned int (0x4afb) before any cast; every 32-bit literal in the port needs its L.
#define FIX(n) ((fix)((u32)(s32)(n) << 16))
#define FIXB(bits) ((fix)(bits##L))
#define FIX_INT(a) ((s16)((a) >> 16))           // = flr(a) as an integer (arithmetic shift)

static inline fix p8_flr(fix a) { return a & (fix)0xffff0000; }
static inline fix p8_ceil(fix a) { return (fix)(((u32)a + 0xffffUL) & 0xffff0000UL); }
static inline fix p8_sgn(fix a) { return a < 0 ? FIX(-1) : FIX(1); }        // sgn(0) = 1
static inline fix p8_abs(fix a) { return a >= 0 ? a : a == (fix)0x80000000UL ? 0x7fffffff : -a; }
static inline fix p8_min(fix a, fix b) { return a < b ? a : b; }
static inline fix p8_max(fix a, fix b) { return a > b ? a : b; }
static inline fix p8_mid(fix a, fix b, fix c)
{
    if (a > b) { fix t = a; a = b; b = t; }
    return c < a ? a : c > b ? b : c;
}

// 16x16 multiplies as single instructions (GCC4TI turns (s32)a * b into a __mulsi3 call)
#ifdef __m68k__
static inline s32 p8__muls(s16 a, s16 b) { s32 r; asm("muls.w %2,%0" : "=d"(r) : "0"(a), "dmi"(b)); return r; }
static inline u32 p8__mulu(u16 a, u16 b) { u32 r; asm("mulu.w %2,%0" : "=d"(r) : "0"(a), "dmi"(b)); return r; }
#else
#define p8__muls(a, b) ((s32)(s16)(a) * (s16)(b))
#define p8__mulu(a, b) ((u32)(u16)(a) * (u16)(b))
#endif
// signed x unsigned 16 bits: muls on b as signed, corrected when b's top bit is set
static inline __attribute__((always_inline)) s32 p8__mulsu(s16 a, u16 b)
{
    s32 r = p8__muls(a, (s16)b);
    if (b & 0x8000) r += (s32)((u32)(s32)a << 16);
    return r;
}

// a * b: floor of the 48-bit product >> 16, wrapped to 32 bits, from the 16-bit halves
// (al, bl unsigned): 4 multiplies, no 64-bit arithmetic.
static inline fix p8_mul(fix a, fix b)
{
    s16 ah = (s16)(a >> 16), bh = (s16)(b >> 16);
    u16 al = (u16)a, bl = (u16)b;
    u32 r = (u32)p8__muls(ah, bh) << 16;
    r += (u32)p8__mulsu(ah, bl) + (u32)p8__mulsu(bh, al);
    r += p8__mulu(al, bl) >> 16;
    return (fix)r;
}
// a * n for an integer n: exact and one multiply (or shifts when n is a constant)
#define p8_muli(a, n) ((fix)((a) * (s32)(n)))

// a / b: truncated toward zero (unlike a * 0.5, which floors: -0x0.0001 / 2 = 0 but
// -0x0.0001 * 0.5 = -0x0.0001), x / 0 = 0x7fff.ffff (x >= 0) or 0x8000.0001, overflow
// saturates the same way.
static inline fix p8_div(fix a, fix b)
{
    long long r;
    if (b == FIX(1)) return a;
    if (b == 0) return a >= 0 ? 0x7fffffff : (fix)0x80000001UL;
    r = ((long long)a << 16) / b;
    if (r > 0x7fffffffLL || r < -0x7fffffffLL) return (r > 0) ? 0x7fffffff : (fix)0x80000001UL;
    return (fix)r;
}
// a / 2^k with PICO-8's truncation (a plain >> k floors: wrong by one bit for negative a)
#define p8_div2k(a, k) ((fix)(((a) + (((a) >> 31) & ((1L << (k)) - 1))) >> (k)))
// a \ b (integer division) = flr(a / b)
static inline fix p8_idiv(fix a, fix b) { return p8_flr(p8_div(a, b)); }
// a % b: result in [0, |b|), x % 0 = 0. For b = 2^k integer: p8_mod2k(a, k) (one AND).
static inline fix p8_mod(fix a, fix b)
{
    fix r;
    b = p8_abs(b);                      // abs(0x8000) = 0x7fff.ffff, as PICO-8
    if (b == 0) return 0;
    r = a % b;
    return r >= 0 ? r : r + b;
}
#define p8_mod2k(a, k) ((fix)((a) & ((FIX(1) << (k)) - 1)))

// shr = arithmetic, lshr = logical, shl, rotl/rotr: on the 32 bits, as PICO-8
#define p8_shr(a, n) ((fix)((a) >> (n)))
#define p8_lshr(a, n) ((fix)((u32)(a) >> (n)))
#define p8_shl(a, n) ((fix)((u32)(a) << (n)))

// rnd/srand: PICO-8's own generator (decompiled in zepto8, same in ccleste and p8shim.lua):
// the port draws the same numbers as PICO-8 and as p8trace.py for the same seed. Start state:
// p8_srand(0) in game_init (PICO-8 seeds at random; the reference uses srand(0) too).
extern u32 p8_ra, p8_rb;               // define once: u32 p8_ra, p8_rb;
static inline u32 p8_step(void)
{
    p8_ra = ((p8_ra >> 16) | (p8_ra << 16)) + p8_rb;   // the 68000 does the rotation with swap
    p8_rb += p8_ra;
    return p8_ra;
}
static inline void p8_srand(fix x)
{
    u16 i;
    p8_rb = (u32)x & 0x7fffffffUL;
    if (!p8_rb) p8_rb = 0xdeadbeefUL;
    p8_ra = p8_rb ^ 0xbead29baUL;
    for (i = 0; i < 32; i++) p8_step();
}
// rnd(n) for an integer 0 < n < 32768: a % (n << 16) = ((a >> 16) % n) << 16 | (a & 0xffff),
// one divu.w (~150 cycles) instead of a 32-bit modulo
static inline fix p8_rndi(u16 n)
{
    u32 a = p8_step();
#ifdef __m68k__
    u32 q = a >> 16;
    asm("divu.w %1,%0" : "+d"(q) : "dmi"(n));          // remainder in the high word
    return (fix)((q & 0xffff0000UL) | (a & 0xffffUL));
#else
    return (fix)(((a >> 16) % n) << 16 | (a & 0xffffUL));
#endif
}
// rnd(x) for any x > 0: integer x takes p8_rndi, a fraction needs a 32-bit modulo (library
// call on the 68000: cosmetic use only); rnd() is p8_rnd(FIX(1)) = a & 0xffff.
static inline fix p8_rnd(fix x)
{
    if (x == FIX(1)) return (fix)(p8_step() & 0xffffUL);
    if (!(x & 0xffff) && x > 0) return p8_rndi((u16)(x >> 16));
    if (x == 0) return 0;
    return (fix)(p8_step() % (u32)x);
}

// Trace format of p8trace.py: tostr(v, true) = "0xhhhh.llll"
#ifndef __m68k__
#include <stdio.h>
static inline void p8_hex(char *buf, fix v)
{
    sprintf(buf, "0x%04x.%04x", (unsigned)((u32)v >> 16), (unsigned)((u32)v & 0xffff));
}
#endif

#endif
