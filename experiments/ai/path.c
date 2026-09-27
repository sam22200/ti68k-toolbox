// Grid pathfinding ideas of KB §12, measured on one 40x25 map (random walls, 1-cell border):
// Two maps: 22 % random walls (maze-like) and 4 % (open), plus two long walls each.
// 1. Flow field: one 4-connected BFS from the goal (ring-buffer queue, a direction byte per
//    cell); cycles per full build, then cycles per enemy step (one table read).
// 2. A* vs Jump Point Search (Harabor & Grastien 2011), 8-connected, corner cutting allowed (a
//    diagonal step only needs its target free: the rule JPS's pruning assumes), costs 10 / 14,
//    octile heuristic, binary heap with lazy deletion. Same start/goal pairs: path costs must be
//    equal; reports expanded nodes, heap pushes, max heap size and cycles.
// Host: gcc -O2 -Wall -o path path.c && ./path      TI: ti-cc -o path path.c
#include "ai.h"

#define GW 42                                   // 40x25 plus a wall border
#define GH 27
#define NCELL (GW * GH)
static u8 wall[NCELL], cx[NCELL], cy[NCELL];

static void make_map(u16 density)
{
    u16 c;
    wy = 7;
    for (c = 0; c < NCELL; c++) {
        cx[c] = c % GW; cy[c] = c / GW;
        wall[c] = cx[c] == 0 || cy[c] == 0 || cx[c] == GW - 1 || cy[c] == GH - 1 || RND(100) < density;
    }
    for (c = 3; c < GH - 6; c++) wall[c * GW + 14] = wall[(GH - 1 - c) * GW + 28] = 1;   // two long walls
}

// ------------------------------------------------------------------ flow field (4-connected)
static u8 fdir[NCELL];                          // 0 = none, 1..4 = step to take
static u16 queue[NCELL];
static const s16 step4[5] = { 0, 1, -1, GW, -GW };
static void flow(u16 goal)
{
    u16 *head = queue, *tail = queue;
    memset(fdir, 0, NCELL);
    fdir[goal] = 1;                             // any non-zero: visited
    *tail++ = goal;
    while (head != tail) {
        u16 c = *head++;
        // a neighbour n reached from c must step back towards c: its direction is the opposite
        if (!wall[c + 1] && !fdir[c + 1]) { fdir[c + 1] = 2; *tail++ = c + 1; }
        if (!wall[c - 1] && !fdir[c - 1]) { fdir[c - 1] = 1; *tail++ = c - 1; }
        if (!wall[c + GW] && !fdir[c + GW]) { fdir[c + GW] = 4; *tail++ = c + GW; }
        if (!wall[c - GW] && !fdir[c - GW]) { fdir[c - GW] = 3; *tail++ = c - GW; }
    }
}

// ------------------------------------------------------------------ A* and JPS
static u16 g[NCELL], par[NCELL];
static u8 closed[NCELL];
static u16 heap[2048], hkey[2048], hn, hmax;
static u16 expanded, pushes;

static void hpush(u16 c, u16 f)
{
    u16 i = hn++;
    pushes++;
    if (hn > hmax) hmax = hn;
    while (i) {
        u16 p = (i - 1) >> 1;
        if (hkey[p] <= f) break;
        heap[i] = heap[p]; hkey[i] = hkey[p]; i = p;
    }
    heap[i] = c; hkey[i] = f;
}
static u16 hpop(void)
{
    u16 top = heap[0], c = heap[--hn], f = hkey[hn], i = 0;
    for (;;) {
        u16 k = 2 * i + 1;
        if (k >= hn) break;
        if (k + 1 < hn && hkey[k + 1] < hkey[k]) k++;
        if (hkey[k] >= f) break;
        heap[i] = heap[k]; hkey[i] = hkey[k]; i = k;
    }
    heap[i] = c; hkey[i] = f;
    return top;
}
static u16 octile(u16 a, u16 b)
{
    s16 dx = cx[a] - cx[b], dy = cy[a] - cy[b];
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;
    return dx > dy ? 10 * dx + 4 * dy : 10 * dy + 4 * dx;
}
static void reset(void)
{
    memset(g, 0xFF, sizeof(g)); memset(closed, 0, NCELL);
    hn = hmax = expanded = pushes = 0;
}
static void relax(u16 from, u16 to, u16 cost, u16 goal)
{
    u16 ng = g[from] + cost;
    if (ng < g[to]) { g[to] = ng; par[to] = from; hpush(to, ng + octile(to, goal)); }
}

static const s16 d8[8] = { 1, -1, GW, -GW, GW + 1, GW - 1, -GW + 1, -GW - 1 };
static u16 astar(u16 s, u16 goal)
{
    reset();
    g[s] = 0; hpush(s, octile(s, goal));
    while (hn) {
        u16 c = hpop(), k;
        if (closed[c]) continue;
        closed[c] = 1; expanded++;
        if (c == goal) return g[c];
        for (k = 0; k < 8; k++) {
            u16 n = c + d8[k];
            if (!wall[n] && !closed[n]) relax(c, n, k < 4 ? 10 : 14, goal);
        }
    }
    return 0xFFFF;
}

#define W_(c) wall[c]
static u16 jgoal;
// Walks from c in direction (dx, dy) and returns the first jump point, or 0 (cell 0 is a wall).
static u16 jump(u16 c, s16 dx, s16 dy)
{
    s16 d = dx + dy * GW;
    for (;;) {
        c += d;
        if (W_(c)) return 0;
        if (c == jgoal) return c;
        if (dx && dy) {
            if ((W_(c - dx) && !W_(c - dx + dy * GW)) || (W_(c - dy * GW) && !W_(c + dx - dy * GW))) return c;
            if (jump(c, dx, 0) || jump(c, 0, dy)) return c;
        } else if (dx) {
            if ((W_(c + GW) && !W_(c + dx + GW)) || (W_(c - GW) && !W_(c + dx - GW))) return c;
        } else {
            if ((W_(c + 1) && !W_(c + 1 + dy * GW)) || (W_(c - 1) && !W_(c - 1 + dy * GW))) return c;
        }
    }
}
static void jsucc(u16 c, s16 dx, s16 dy)
{
    u16 j = jump(c, dx, dy);
    if (j && !closed[j]) relax(c, j, octile(c, j), jgoal);
}
static u16 jps(u16 s, u16 goal)
{
    reset();
    jgoal = goal;
    g[s] = 0; par[s] = s; hpush(s, octile(s, goal));
    while (hn) {
        u16 c = hpop();
        s16 dx, dy;
        if (closed[c]) continue;
        closed[c] = 1; expanded++;
        if (c == goal) return g[c];
        if (c == s) {
            s16 a, b;
            for (b = -1; b <= 1; b++) for (a = -1; a <= 1; a++) if (a || b) jsucc(c, a, b);
            continue;
        }
        dx = cx[c] > cx[par[c]] ? 1 : cx[c] < cx[par[c]] ? -1 : 0;
        dy = cy[c] > cy[par[c]] ? 1 : cy[c] < cy[par[c]] ? -1 : 0;
        if (dx && dy) {
            jsucc(c, dx, 0); jsucc(c, 0, dy); jsucc(c, dx, dy);
            if (W_(c - dx)) jsucc(c, -dx, dy);
            if (W_(c - dy * GW)) jsucc(c, dx, -dy);
        } else if (dx) {
            jsucc(c, dx, 0);
            if (W_(c + GW)) jsucc(c, dx, 1);
            if (W_(c - GW)) jsucc(c, dx, -1);
        } else {
            jsucc(c, 0, dy);
            if (W_(c + 1)) jsucc(c, 1, dy);
            if (W_(c - 1)) jsucc(c, -1, dy);
        }
    }
    return 0xFFFF;
}

// ------------------------------------------------------------------ driver
#define NP 12
static u16 ps[NP], pg[NP];
static u16 free_cell(void) { u16 c; do c = RND(NCELL); while (wall[c]); return c; }

typedef struct { u32 exp, push, t; u16 hmax; u32 cost; } STAT;

static void search(u16 (*f)(u16, u16), STAT *st, u16 *costs)
{
    u16 k;
    memset(st, 0, sizeof(*st));
#ifdef __m68k__
    tstart();
#endif
    for (k = 0; k < NP; k++) {
        costs[k] = f(ps[k], pg[k]);
        st->exp += expanded; st->push += pushes; st->cost += costs[k];
        if (hmax > st->hmax) st->hmax = hmax;
    }
#ifdef __m68k__
    st->t = TICKS();
#endif
}

#define NENEMY 32
static u16 en[NENEMY];
static void enemies_step(void)
{
    u16 k;
    for (k = 0; k < NENEMY; k++) en[k] += step4[fdir[en[k]]];
}

static u16 same, reach;
static STAT a[2], j[2];
static u32 tf, te, fsum;
static void run_map(u16 m)
{
    u16 ca[NP], cj[NP], k, goal;
    make_map(m ? 4 : 22);
    for (k = 0; k < NP; k++) { ps[k] = free_cell(); pg[k] = free_cell(); }
    goal = pg[0];
    for (k = 0; k < NENEMY; k++) en[k] = free_cell();
#ifdef __m68k__
    if (!m) {
        tstart();
        for (k = 0; k < 64; k++) flow(goal);
        tf = TICKS();
        tstart();
        for (k = 0; k < 1024; k++) enemies_step();
        te = TICKS();
    }
#endif
    flow(goal);
    if (!m) for (k = 0; k < NCELL; k++) fsum += fdir[k] * (k + 1);
    search(astar, &a[m], ca);
    search(jps, &j[m], cj);
    for (k = 0; k < NP; k++) { if (ca[k] != cj[k]) same = 0; if (ca[k] != 0xFFFF) reach++; }
}

#ifdef __m68k__
void _main(void)
#else
int main(void)
#endif
{
    u16 m;
    same = 1;
#ifdef __m68k__
    ClrScr(); printf_xy(0, 0, "Benchmarking...");
    timer_on();
#endif
    run_map(0); run_map(1);
#ifdef __m68k__
    timer_off();
    ClrScr(); FontSetSys(F_4x6);
    printf_xy(0, 0, "flow field 40x25 BFS: %lu cyc", CYC(tf) / 64);
    printf_xy(0, 7, "enemy step (table read): %lu cyc", CYC(te) / (1024UL * NENEMY));
    printf_xy(0, 14, "RAM: dir %u B + queue %u B, sum %lu", NCELL, NCELL * 2, fsum);
    printf_xy(0, 24, "2x%u pairs, %u reachable, costs %s", NP, reach, same ? "EQUAL" : "DIFFER");
    printf_xy(0, 31, "walls   expand push heapmax  cyc/path");
    for (m = 0; m < 2; m++) {
        printf_xy(0, 38 + 14 * m, "%2u%% A* %6lu %5lu %4u %9lu", m ? 4 : 22, a[m].exp, a[m].push, a[m].hmax, CYC(a[m].t) / NP);
        printf_xy(0, 45 + 14 * m, "   JPS %6lu %5lu %4u %9lu", j[m].exp, j[m].push, j[m].hmax, CYC(j[m].t) / NP);
    }
    GKeyFlush(); ngetchx();
    FontSetSys(F_6x8);
#else
    printf("flow sum %lu, 2x%u pairs, %u reachable, costs %s\n", (unsigned long)fsum, NP, reach, same ? "EQUAL" : "DIFFER");
    for (m = 0; m < 2; m++)
        printf("%u%%: A* exp %lu push %lu heapmax %u | JPS exp %lu push %lu heapmax %u\n", m ? 4 : 22,
               (unsigned long)a[m].exp, (unsigned long)a[m].push, a[m].hmax,
               (unsigned long)j[m].exp, (unsigned long)j[m].push, j[m].hmax);
    return 0;
#endif
}
