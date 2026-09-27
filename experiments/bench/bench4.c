// Interpreter / state-machine dispatch: switch vs function-pointer table vs computed goto.
// A toy VM with 8 opcodes runs a 64-op program in a loop; cycles per executed opcode.
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>

static volatile unsigned long ticks;
DEFINE_INT_HANDLER(tick_handler) { ticks++; }

static unsigned char prog[64];
static short acc, rb;
#define NOPS 60000L

static void op0(void) { acc++; }          static void op1(void) { acc--; }
static void op2(void) { acc += rb; }      static void op3(void) { rb ^= acc; }
static void op4(void) { acc <<= 1; }      static void op5(void) { acc >>= 1; }
static void op6(void) { rb++; }           static void op7(void) { acc = rb; }
static void (*const optab[8])(void) = { op0, op1, op2, op3, op4, op5, op6, op7 };

static void run_switch(long n)
{
    const unsigned char *pc = prog;
    short a = acc, r = rb;
    for (; n > 0; n--) {
        switch (*pc++) {
        case 0: a++; break;      case 1: a--; break;      case 2: a += r; break;  case 3: r ^= a; break;
        case 4: a <<= 1; break;  case 5: a >>= 1; break;  case 6: r++; break;     case 7: a = r; break;
        }
        if (pc == prog + 64) pc = prog;
    }
    acc = a; rb = r;
}

static void run_table(long n)
{
    const unsigned char *pc = prog;
    for (; n > 0; n--) {
        optab[*pc++]();
        if (pc == prog + 64) pc = prog;
    }
}

static void run_goto(long n)
{
    static void *const lbl[8] = { &&l0, &&l1, &&l2, &&l3, &&l4, &&l5, &&l6, &&l7 };
    const unsigned char *pc = prog;
    short a = acc, r = rb;
#define NEXT do { if (--n <= 0) goto done; if (pc == prog + 64) pc = prog; goto *lbl[*pc++]; } while (0)
    goto *lbl[*pc++];
l0: a++; NEXT;      l1: a--; NEXT;      l2: a += r; NEXT;  l3: r ^= a; NEXT;
l4: a <<= 1; NEXT;  l5: a >>= 1; NEXT;  l6: r++; NEXT;     l7: a = r; NEXT;
done:
    acc = a; rb = r;
#undef NEXT
}

static unsigned long timeit(short k)
{
    unsigned long s = ticks;
    while (ticks == s);
    s = ticks;
    if (k == 0) run_switch(NOPS); else if (k == 1) run_table(NOPS); else run_goto(NOPS);
    return ticks - s;
}

void _main(void)
{
    INT_HANDLER o1 = GetIntVec(AUTO_INT_1), o5 = GetIntVec(AUTO_INT_5);
    static const char *const nm[3] = { "switch", "fn table", "comp. goto" };
    unsigned long t[3];
    short k, chk[3];
    for (k = 0; k < 64; k++) prog[k] = (k * 5 + (k >> 3)) & 7;
    for (k = 0; k < 3; k++) {                       // same final state for all three
        acc = 1; rb = 2;
        if (k == 0) run_switch(1000); else if (k == 1) run_table(1000); else run_goto(1000);
        chk[k] = acc ^ rb;
    }
    ClrScr(); printf_xy(0, 0, "Benchmarking...");
    SetIntVec(AUTO_INT_5, DUMMY_HANDLER);
    SetIntVec(AUTO_INT_1, tick_handler);
    for (k = 0; k < 3; k++) { unsigned long a = timeit(k), b = timeit(k); t[k] = a < b ? a : b; }
    SetIntVec(AUTO_INT_1, o1); SetIntVec(AUTO_INT_5, o5);
    ClrScr();
    printf_xy(0, 0, "cycles per opcode @12MHz:");
    for (k = 0; k < 3; k++)
        printf_xy(0, 10 + 10 * k, "%-10s %4ld  (%lu t)", nm[k], (long)(t[k] * 46875UL / NOPS), t[k]);
    printf_xy(0, 45, "same result: %s", (chk[0] == chk[1] && chk[1] == chk[2]) ? "yes" : "NO");
    GKeyFlush();
    ngetchx();
}
