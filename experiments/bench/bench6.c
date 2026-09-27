// Does TiEmu count 68000 cycles correctly? Compares instruction loops with their datasheet cost
// (MC68000UM): movem.l (a0)+,12 regs = 12+8*12 = 108; move.l (a0)+,(a1)+ = 20; muls.w = 38..70;
// dbra taken = 10. 20000 iterations each.
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>

static volatile unsigned long ticks;
DEFINE_INT_HANDLER(tick_handler) { ticks++; }
unsigned long buf[64];            // global: referenced by name from the asm

static void t_movem(void) { asm volatile("move.w #19999,%%d7\n0: lea buf,%%a0\n movem.l (%%a0)+,%%d0-%%d6/%%a1-%%a5\n dbra %%d7,0b" : : : "d0","d1","d2","d3","d4","d5","d6","d7","a0","a1","a2","a3","a4","a5"); }
static void t_movel(void) { asm volatile("move.w #19999,%%d7\n0: lea buf,%%a0\n lea buf,%%a1\n move.l (%%a0)+,(%%a1)+\n dbra %%d7,0b" : : : "d7","a0","a1"); }
static void t_empty(void) { asm volatile("move.w #19999,%%d7\n0: lea buf,%%a0\n lea buf,%%a1\n dbra %%d7,0b" : : : "d7","a0","a1"); }
static void t_lsl(void)   { asm volatile("move.w #19999,%%d7\n0: lea buf,%%a0\n lea buf,%%a1\n lsl.l #8,%%d0\n dbra %%d7,0b" : : : "d0","d7","a0","a1"); }
static void t_divu(void)  { asm volatile("move.w #19999,%%d7\n0: lea buf,%%a0\n lea buf,%%a1\n move.l #100000,%%d0\n divu.w #7,%%d0\n dbra %%d7,0b" : : : "d0","d7","a0","a1"); }
static void t_movem4(void) { asm volatile("move.w #19999,%%d7\n0: lea buf,%%a0\n lea buf,%%a1\n movem.l (%%a0)+,%%d0-%%d3\n dbra %%d7,0b" : : : "d0","d1","d2","d3","d7","a0","a1"); }
static void t_muls(void)  { asm volatile("move.w #19999,%%d7\n moveq #-1,%%d1\n0: lea buf,%%a0\n lea buf,%%a1\n muls.w %%d1,%%d0\n dbra %%d7,0b" : : : "d0","d1","d7","a0","a1"); }

void _main(void)
{
    INT_HANDLER o1 = GetIntVec(AUTO_INT_1), o5 = GetIntVec(AUTO_INT_5);
    static const char *const nm[7] = { "lea+lea+dbra", "+move.l (a)+", "+movem.l 12r", "+muls $FFFF", "+lsl.l #8", "+move.l#,divu", "+movem.l 4r" };
    static const short want[7] = { 34, 20, 96, 70, 24, 152, 44 };
    unsigned long t[7], s;
    short k;
    ClrScr(); printf_xy(0, 0, "Benchmarking...");
    SetIntVec(AUTO_INT_5, DUMMY_HANDLER);
    SetIntVec(AUTO_INT_1, tick_handler);
    for (k = 0; k < 7; k++) {
        s = ticks; while (ticks == s); s = ticks;
        if (k == 0) t_empty(); else if (k == 1) t_movel(); else if (k == 2) t_movem(); else if (k == 3) t_muls(); else if (k == 4) t_lsl(); else if (k == 5) t_divu(); else t_movem4();
        t[k] = ticks - s;
    }
    SetIntVec(AUTO_INT_1, o1); SetIntVec(AUTO_INT_5, o5);
    ClrScr();
    printf_xy(0, 0, "cycles/iter @12MHz  (datasheet)");
    for (k = 0; k < 7; k++) {
        long c = (long)(t[k] * 46875UL / 20000);
        if (k) c -= (long)(t[0] * 46875UL / 20000);
        printf_xy(0, 10 + 10 * k, "%-13s %4ld (%d) %lut", nm[k], c, want[k], t[k]);
    }
    GKeyFlush();
    ngetchx();
}
