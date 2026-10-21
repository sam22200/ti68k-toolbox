// Shared by the battle engine (fft.c) and the scene player (campaign/scene.c): plane words,
// big-endian reads, ZX0.
// LCD rows are 30 bytes: long accesses are only word-aligned on alternating rows.
// 68000 permits this. Tell the host compiler as well, without changing TI code generation.
#ifdef __m68k__
typedef u32 PlaneWord;
#else
typedef u32 PlaneWord __attribute__((aligned(2), may_alias));
#endif

// Planes are big-endian 16-bit words (MSB = leftmost pixel): native on the TI, composed on
// the PC. RMW16: word = word & ~k | v.
#ifdef __m68k__
#define RD16(p, i) (((const u16 *)(p))[i])
#define WR16(p, i, v) (((u16 *)(p))[i] = (v))
#define RD32(p, i) (*(const PlaneWord *)((const u8 *)(p) + 2 * (i)))
#define WR32(p, i, v) (*(PlaneWord *)((u8 *)(p) + 2 * (i)) = (v))
#define RMW16(p, k, v) (*(u16 *)(p) = (*(u16 *)(p) & ~(k)) | (v))
// 16 x 16 -> 32 multiply and 32 / 16 division in one instruction (ti68k-performance.md §4,
// verified): plain C widens shared shorts and calls __mulsi3 / __divsi3
static inline long muls16(short a, short b)
{ long r; asm("muls.w %2,%0" : "=d"(r) : "0"(a), "dmi"(b)); return r; }
static inline short divs32_16(long n, short d)
{ asm("divs.w %1,%0" : "+d"(n) : "dmi"(d)); return (short)n; }
#else
#define RD16(p, i) ((u16)((p)[2 * (i)] << 8 | (p)[2 * (i) + 1]))
#define WR16(p, i, v) ((p)[2 * (i)] = (u8)((v) >> 8), (p)[2 * (i) + 1] = (u8)(v))
#define RD32(p, i) ((u32)RD16(p, i) << 16 | RD16(p, (i) + 1))
#define WR32(p, i, v) (WR16(p, i, (u16)((v) >> 16)), WR16(p, (i) + 1, (u16)(v)))
#define RMW16(p, k, v) do { u16 o_ = (u16)((p)[0] << 8 | (p)[1]); o_ = (u16)((o_ & ~(k)) | (v)); \
                            (p)[0] = (u8)(o_ >> 8); (p)[1] = (u8)o_; } while (0)
static long muls16(short a, short b) { return (long)a * b; }
static short divs32_16(long n, short d) { return (short)(n / d); }
#endif

#ifdef __m68k__
#include "../../lib/unpack68k.h"         // zx0_asm: ~42-73 cycles per output byte
#define zx0_unpack zx0_asm
#else
// ZX0 v2 (einar-saukas/ZX0), the PC's copy of lib/unpack68k.s (experiments/compress/unpack.c)
static int zx0_bit(const u8 **s, u16 *bb)
{
    u16 t;
    if (*bb == 0x8000) *bb = (u16)(*(*s)++ << 8 | 0x80);
    t = *bb; *bb = (u16)(*bb << 1);
    return (t & 0x8000) != 0;
}
static u16 zx0_gamma(const u8 **s, u16 *bb)
{
    u16 x = 1;
    while (!zx0_bit(s, bb)) { x += x; if (zx0_bit(s, bb)) x++; }
    return x;
}
static u8 *zx0_unpack(const u8 *s, u8 *d)
{
    u16 dist = 1, n, bb = 0x8000;
    for (;;) {
        for (n = zx0_gamma(&s, &bb); n; n--) *d++ = *s++;     // literals
        if (!zx0_bit(&s, &bb)) {                                // repeat the last offset
            for (n = zx0_gamma(&s, &bb); n; n--, d++) *d = d[-dist];
            if (!zx0_bit(&s, &bb)) continue;
        }
        for (;;) {                                              // new offset
            u16 lo;
            n = 1;
            while (!zx0_bit(&s, &bb)) { n += n; if (!zx0_bit(&s, &bb)) n++; }
            if (n == 256) return d;
            lo = *s++;
            dist = (u16)((n << 7) - (lo >> 1));
            n = 1;
            if (!(lo & 1)) do { n += n; if (zx0_bit(&s, &bb)) n++; } while (!zx0_bit(&s, &bb));
            for (n++; n; n--, d++) *d = d[-dist];
            if (!zx0_bit(&s, &bb)) break;
        }
    }
}
#endif
