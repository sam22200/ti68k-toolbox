// GCC 4.1.2 m68k code-generation traps reported for newer GCCs: do they happen in GCC4TI -Os?
// Build: tigcc -Os -mregparm=5 -fomit-frame-pointer -S traps.c, then read traps.s.
#define USE_TI89
#include <tigcclib.h>

// 1. byte loads summed into a short: andi #255 per iteration?
unsigned short sum_bytes(const unsigned char *p, unsigned short n)
{ unsigned short s = 0; do s += *p++; while (--n); return s; }
// 2. (x & 0xFF) * 320 : mulu.w or __mulsi3?
unsigned long row_off(unsigned short x) { return (unsigned long)(x & 0xFF) * 320; }
unsigned short row_off16(unsigned short x) { return (x & 0xFF) * 320; }
// 3. 16-bit quotient and remainder together: one divu or two library calls?
unsigned short qr(unsigned short a, unsigned short b, unsigned short *r) { *r = a % b; return a / b; }
unsigned short qr32(unsigned long a, unsigned short b, unsigned short *r) { *r = a % b; return a / b; }
// 4. up-counting loop: dbra?
void clr_up(unsigned char *p) { unsigned short i; for (i = 0; i < 100; i++) p[i] = 0; }
void clr_down(unsigned char *p) { unsigned short n = 100; do *p++ = 0; while (--n); }
// 5. bitfield vs mask
struct bf { unsigned alive:1, type:3, hp:4; };
short bf_test(const struct bf *e) { return e->alive && e->hp > 3; }
#define F_ALIVE 0x80
short mask_test(const unsigned char *f, const unsigned char *hp) { return (*f & F_ALIVE) && *hp > 3; }
// 6. struct return
typedef struct { short x, y; } V2;
V2 v2add(V2 a, V2 b) { V2 r; r.x = a.x + b.x; r.y = a.y + b.y; return r; }
// 7. Lemire fastrange: one mulu?
unsigned short fastrange(unsigned short r, unsigned short n) { return ((unsigned long)r * n) >> 16; }
// 8. Marsaglia MWC 16-bit step
unsigned long z = 362436069;
unsigned short mwc(void) { z = 36969UL * (unsigned short)z + (z >> 16); return z; }
unsigned short mwc2(void) { z = (unsigned long)(unsigned short)z * 36969u + (z >> 16); return z; }
// 9. xorshift 7,9,8
unsigned short xs = 1;
unsigned short xorshift(void) { xs ^= xs << 7; xs ^= xs >> 9; xs ^= xs << 8; return xs; }
// 10. null pointer test
short isnull(const void *p) { return p == 0; }
