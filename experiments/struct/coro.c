// Stackless coroutines (Tatham / protothreads: switch on __LINE__, 2 bytes of state) vs a
// hand-written state machine, for 50 entities with the same behaviour: walk len steps, wait,
// turn right, every full turn len grows (2..8). One update = one step or one wait frame.
// Checks both give the same trajectory, then cycles per entity update (call included).
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>

static volatile unsigned long ticks;
DEFINE_INT_HANDLER(tick_handler) { ticks++; }
volatile unsigned short sink;

#define NENT 50
#define FRAMES 1000
#define N ((unsigned long)NENT * FRAMES)

#define crBegin(s) switch (s) { case 0:
#define crYield(s) do { s = __LINE__; return; case __LINE__:; } while (0)
#define crEnd      }

typedef struct { short x, y; unsigned short st; unsigned char dir, len, wait, i; } Ent;   // 10 bytes
static const signed char dx[4] = { 1, 0, -1, 0 }, dy[4] = { 0, 1, 0, -1 };
static Ent co[NENT], fs[NENT];

static __attribute__((noinline)) void co_update(Ent *e)
{
    crBegin(e->st);
    for (;;) {
        for (e->i = e->len; e->i; e->i--) {
            e->x += dx[e->dir]; e->y += dy[e->dir];
            crYield(e->st);
        }
        for (e->i = e->wait; e->i; e->i--) crYield(e->st);
        e->dir = (e->dir + 1) & 3;
        if (!e->dir && ++e->len > 8) e->len = 2;
    }
    crEnd;
}

enum { S_START, S_WALK, S_WAIT };
static __attribute__((noinline)) void fsm_update(Ent *e)
{
    switch (e->st) {
    case S_WAIT:
        if (--e->i) return;
        e->dir = (e->dir + 1) & 3;
        if (!e->dir && ++e->len > 8) e->len = 2;
        goto walk;
    case S_WALK:
        if (--e->i) break;
        e->i = e->wait; e->st = S_WAIT;
        return;
    default:
    walk:
        e->i = e->len; e->st = S_WALK;
        break;
    }
    e->x += dx[e->dir]; e->y += dy[e->dir];
}

static __attribute__((noinline)) void empty_update(Ent *e) { sink = e->st; }

static void init(Ent *t)
{
    unsigned short k;
    for (k = 0; k < NENT; k++) {
        t[k].x = k * 3; t[k].y = k; t[k].st = 0; t[k].dir = k & 3;
        t[k].len = 2 + k % 5; t[k].wait = 1 + k % 3; t[k].i = 0;
    }
}

static void b_co(void)    { unsigned short f = FRAMES; while (f--) { Ent *e = co; unsigned short n = NENT; while (n--) co_update(e++); } }
static void b_fsm(void)   { unsigned short f = FRAMES; while (f--) { Ent *e = fs; unsigned short n = NENT; while (n--) fsm_update(e++); } }
static void b_empty(void) { unsigned short f = FRAMES; while (f--) { Ent *e = fs; unsigned short n = NENT; while (n--) empty_update(e++); } }

#define NB 3
static void (*const fn[NB])(void) = { b_empty, b_co, b_fsm };
static const char *const nm[NB] = { "loop+call", "coroutine", "state mach" };

void _main(void)
{
    INT_HANDLER o1 = GetIntVec(AUTO_INT_1), o5 = GetIntVec(AUTO_INT_5);
    unsigned long t[NB], s, sum = 0;
    unsigned short k, f, same = 1;
    init(co); init(fs);
    for (f = 0; f < 1000; f++)                        // 1000 frames, compare every entity every frame
        for (k = 0; k < NENT; k++) {
            co_update(&co[k]); fsm_update(&fs[k]);
            if (co[k].x != fs[k].x || co[k].y != fs[k].y || co[k].dir != fs[k].dir) same = 0;
            sum += (unsigned short)(co[k].x * 7 + co[k].y);
        }
    init(co); init(fs);
    ClrScr(); printf_xy(0, 0, "Benchmarking...");
    SetIntVec(AUTO_INT_5, DUMMY_HANDLER);
    SetIntVec(AUTO_INT_1, tick_handler);
    for (k = 0; k < NB; k++) {
        unsigned long best = 0xFFFFFFFF, r;
        short rep;
        for (rep = 0; rep < 2; rep++) {
            s = ticks; while (ticks == s); s = ticks;
            fn[k]();
            r = ticks - s; if (r < best) best = r;
        }
        t[k] = best;
    }
    SetIntVec(AUTO_INT_1, o1); SetIntVec(AUTO_INT_5, o5);
    ClrScr(); FontSetSys(F_4x6);
    printf_xy(0, 0, "cycles per entity update (%u ent x %u fr)", NENT, FRAMES);
    for (k = 0; k < NB; k++)
        printf_xy(0, 8 + 7 * k, "%-11s %4lu  (-call %4ld)  %lu t", nm[k], t[k] * 46875UL / N,
                  (long)(t[k] * 46875UL / N) - (long)(t[0] * 46875UL / N), t[k]);
    printf_xy(0, 34, "same trajectory 1000 fr: %s", same ? "yes" : "NO");
    printf_xy(0, 41, "checksum %lu", sum);
    printf_xy(0, 48, "sizeof(Ent) %u", (unsigned short)sizeof(Ent));
    GKeyFlush();
    ngetchx();
    FontSetSys(F_6x8);
}
