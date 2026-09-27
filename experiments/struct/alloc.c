// Allocation patterns of KB §12, measured: AMS malloc/free vs an arena (one malloc, bump pointer
// rounded to even, mark/release) vs an object pool (free list kept inside dead objects) with
// byte handles plus a generation counter (stale handles are detected).
// 100 allocations of mixed sizes (4..130 bytes) then all freed, 20 rounds; the pool uses
// 16-byte objects. Cycles per operation (loop included).
#define USE_TI89
#define SAVE_SCREEN
#include <tigcclib.h>

static volatile unsigned long ticks;
DEFINE_INT_HANDLER(tick_handler) { ticks++; }
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

#define NA 100
#define ROUNDS 20
static void *ptr[NA];
static u16 size[NA];

// ---- arena
static u8 *arena, *top, *arena_end;
static void *a_alloc(u16 n)
{
    u8 *p = top;
    n = (n + 1) & ~1;                               // keep words and longs at even addresses
    if ((u16)(arena_end - p) < n) return 0;
    top = p + n;
    return p;
}

// ---- pool of 16-byte objects, handles = index | generation << 8
typedef struct OBJ { union { struct OBJ *next; u8 data[14]; } u; u8 gen; } OBJ;   // 16 bytes
#define NPOOL 128
static OBJ *pool, *freel;
static void p_init(void)
{
    u16 k;
    freel = 0;
    for (k = NPOOL; k--;) { pool[k].u.next = freel; freel = &pool[k]; }
}
static u16 p_alloc(void)
{
    OBJ *o = freel;
    if (!o) return 0xFFFF;
    freel = o->u.next;
    return (u16)(o - pool) | (u16)o->gen << 8;      // o - pool: a shift (sizeof 16), no divide
}
static void p_free(u16 h)
{
    OBJ *o = &pool[h & 0xFF];
    o->gen++;                                       // every old handle to it is now stale
    o->u.next = freel; freel = o;
}
static OBJ *p_get(u16 h)
{
    OBJ *o = &pool[h & 0xFF];
    return o->gen == h >> 8 ? o : 0;
}

static unsigned long t_start(void) { unsigned long s = ticks; while (ticks == s); return ticks; }

void _main(void)
{
    INT_HANDLER o1 = GetIntVec(AUTO_INT_1), o5 = GetIntVec(AUTO_INT_5);
    unsigned long t[8], s;
    u16 k, r, h[NA], stale_ok = 1, ok = 1;
    volatile u16 sink = 0;
    arena = malloc(8192); pool = malloc(NPOOL * sizeof(OBJ));
    if (!arena || !pool) goto out;
    arena_end = arena + 8192;
    for (k = 0; k < NA; k++) size[k] = 4 + (k * 37) % 127;
    ClrScr(); printf_xy(0, 0, "Benchmarking...");
    SetIntVec(AUTO_INT_5, DUMMY_HANDLER);
    SetIntVec(AUTO_INT_1, tick_handler);

    s = t_start();                                  // loop overhead
    for (r = 0; r < ROUNDS; r++) for (k = 0; k < NA; k++) ptr[k] = (void *)(u32)size[k];
    t[0] = ticks - s;
    t[1] = t[2] = 0;
    for (r = 0; r < ROUNDS; r++) {                  // AMS malloc / free (free in allocation order)
        s = t_start();
        for (k = 0; k < NA; k++) ptr[k] = malloc(size[k]);
        t[1] += ticks - s;
        for (k = 0; k < NA; k++) if (!ptr[k]) ok = 0;
        s = t_start();
        for (k = 0; k < NA; k++) free(ptr[k]);
        t[2] += ticks - s;
    }
    s = t_start();                                  // arena: allocate, then release to the mark
    for (r = 0; r < ROUNDS; r++) {
        u8 *mark = top = arena;
        for (k = 0; k < NA; k++) ptr[k] = a_alloc(size[k]);
        top = mark;
    }
    t[3] = ticks - s;
    for (k = 0; k < NA; k++) if (!ptr[k] || ((u16)(u32)ptr[k] & 1)) ok = 0;
    p_init();
    s = t_start();                                  // pool: alloc 100, free 100
    for (r = 0; r < ROUNDS; r++) {
        for (k = 0; k < NA; k++) h[k] = p_alloc();
        for (k = 0; k < NA; k++) p_free(h[k]);
    }
    t[4] = ticks - s;
    for (k = 0; k < NA; k++) h[k] = p_alloc();
    s = t_start();                                  // handle lookups
    for (r = 0; r < ROUNDS; r++) for (k = 0; k < NA; k++) sink += p_get(h[k]) != 0;
    t[5] = ticks - s;
    p_free(h[0]);
    if (p_get(h[0])) stale_ok = 0;                  // a freed handle must not resolve
    if (!p_get(h[1])) stale_ok = 0;
    SetIntVec(AUTO_INT_1, o1); SetIntVec(AUTO_INT_5, o5);

    ClrScr(); FontSetSys(F_4x6);
#define C(x) ((x) * 46875UL / (ROUNDS * NA))
    printf_xy(0, 0, "cycles per operation (%u x %u)", ROUNDS, NA);
    printf_xy(0, 8, "loop only        %5lu", C(t[0]));
    printf_xy(0, 15, "AMS malloc       %5lu", C(t[1]));
    printf_xy(0, 22, "AMS free         %5lu", C(t[2]));
    printf_xy(0, 29, "arena alloc      %5lu (release: 1 store)", C(t[3]));
    printf_xy(0, 36, "pool alloc+free  %5lu", C(t[4]));
    printf_xy(0, 43, "handle lookup    %5lu", C(t[5]));
    printf_xy(0, 53, "all allocations ok: %s, stale handle caught: %s", ok ? "yes" : "NO", stale_ok ? "yes" : "NO");
    printf_xy(0, 60, "sizeof(OBJ) %u, arena %u B used", (u16)sizeof(OBJ), (u16)(ptr[NA - 1] - (void *)arena) + size[NA - 1]);
    GKeyFlush(); ngetchx();
    FontSetSys(F_6x8);
out:
    free(pool); free(arena);
}
