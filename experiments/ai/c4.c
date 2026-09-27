// Connect-4 search ideas of KB §12, measured:
// 1. Alpha-beta upgrades at fixed depth from 3 positions: plain (columns 0..6), + centre-first
//    order, + 2 killers per ply and a history table, + transposition table in 4 KB (Breuker
//    two-level buckets: a depth-preferred slot and an always-replace slot), + PVS. All variants
//    must return the same score (a fixed-depth minimax value is unique).
// 2. Memory-bounded MCTS: a fixed pool of 10-byte nodes (u16 visits and wins), expansion stops
//    when the pool is full; UCT with log-bucket tables (4 mantissa bits) for w/n, sqrt(ln N) and
//    1/sqrt(n): only mulu.w, adds and shifts, no float, no division. Playouts per second, then
//    tactics (takes an immediate win, blocks an immediate loss). Strength games run on the host
//    (same integer code, same results): ./c4 games
// Board: 9 columns x 8 cells (border cells = 3), index (col + 1) * 8 + row + 1.
// Host: gcc -O2 -Wall -o c4 c4.c && ./c4 [games]      TI: ti-cc -o c4ab c4.c (a variable c4 may exist)
#include "ai.h"

#define WIN 10000
#define INF 32000
static u8 b[72], ht[7], turn;                   // turn: 1 or 2, the side to move
static s16 ev;                                  // static eval, player 1's point of view
static u32 hash, zob[2][72];
static u16 ply;
static u32 nodes;
static const s8 wgt[6][7] = { { 3, 4, 5, 7, 5, 4, 3 }, { 4, 6, 8, 10, 8, 6, 4 }, { 5, 8, 11, 13, 11, 8, 5 },
                              { 5, 8, 11, 13, 11, 8, 5 }, { 4, 6, 8, 10, 8, 6, 4 }, { 3, 4, 5, 7, 5, 4, 3 } };

static void reset_board(void)
{
    u16 i;
    for (i = 0; i < 72; i++) b[i] = 3;
    for (i = 0; i < 7; i++) { u16 r; for (r = 0; r < 6; r++) b[(i + 1) * 8 + r + 1] = 0; ht[i] = 0; }
    turn = 1; ev = 0; hash = 0; ply = 0;
}
static u16 play(u16 c)                          // returns the cell
{
    u16 p = (c + 1) * 8 + ++ht[c];
    b[p] = turn;
    hash ^= zob[turn - 1][p];
    ev += turn == 1 ? wgt[ht[c] - 1][c] : -wgt[ht[c] - 1][c];
    turn ^= 3; ply++;
    return p;
}
static void unplay(u16 c)
{
    u16 p = (c + 1) * 8 + ht[c];
    turn ^= 3; ply--;
    ev -= turn == 1 ? wgt[ht[c] - 1][c] : -wgt[ht[c] - 1][c];
    hash ^= zob[turn - 1][p];
    b[p] = 0; ht[c]--;
}
static u16 wins(u16 p)                          // does the piece at p make four?
{
    static const s8 dir[4] = { 1, 8, 9, 7 };
    u8 v = b[p];
    u16 k;
    for (k = 0; k < 4; k++) {
        s16 d = dir[k];
        u16 n = 1, q;
        for (q = p + d; b[q] == v; q += d) n++;
        for (q = p - d; b[q] == v; q -= d) n++;
        if (n >= 4) return 1;
    }
    return 0;
}

// ------------------------------------------------------------------ alpha-beta variants
enum { V_PLAIN, V_CENTRE, V_KILLER, V_TT, V_PVS, NV };
static u16 var;
static const u8 centre[7] = { 3, 2, 4, 1, 5, 0, 6 }, plain[7] = { 0, 1, 2, 3, 4, 5, 6 };
static u8 killer[43][2];
static u16 hist[2][7];

typedef struct { u16 chk; s16 val; u8 depth, fm; } TTE;     // fm: flag << 4 | move (7 = none)
#define NBUCK 341                               // 341 x 2 x 6 bytes = 4,092 bytes
static TTE *tt;
enum { F_EXACT = 1, F_LOWER, F_UPPER };

static s16 search(u16 depth, s16 alpha, s16 beta)
{
    u8 order[7], n = 0, best_m = 7, ttm = 7;
    s16 best = -INF, a0 = alpha;
    TTE *e = 0;
    u16 k;
    nodes++;
    if (depth == 0) return turn == 1 ? ev : -ev;
    if (var >= V_TT) {
        u16 i = (u16)(((u32)(u16)hash * NBUCK) >> 16) * 2;
        TTE *s0 = &tt[i], *s1 = s0 + 1;
        u16 chk = hash >> 16;
        e = s0->chk == chk && s0->depth == depth ? s0 : s1->chk == chk && s1->depth == depth ? s1 : 0;
        if (e) {
            u16 f = e->fm >> 4;
            ttm = e->fm & 15;
            if (f == F_EXACT) return e->val;
            if (f == F_LOWER && e->val > alpha) alpha = e->val;
            if (f == F_UPPER && e->val < beta) beta = e->val;
            if (alpha >= beta) return e->val;
        }
    }
    // move order
    {
        const u8 *base = var == V_PLAIN ? plain : centre;
        if (ttm < 7 && ht[ttm] < 6) order[n++] = ttm;
        if (var >= V_KILLER) {
            for (k = 0; k < 2; k++) {
                u8 m = killer[ply][k];
                if (m < 7 && m != ttm && ht[m] < 6) order[n++] = m;
            }
        }
        {
            u8 first = n;
            for (k = 0; k < 7; k++) {
                u8 m = base[k], j;
                if (ht[m] >= 6) continue;
                for (j = 0; j < first; j++) if (order[j] == m) break;
                if (j < first) continue;
                if (var >= V_KILLER) {          // insertion by history, stable
                    u16 h = hist[turn - 1][m];
                    for (j = n; j > first && hist[turn - 1][order[j - 1]] < h; j--) order[j] = order[j - 1];
                    order[j] = m; n++;
                } else order[n++] = m;
            }
        }
    }
    if (!n) return 0;                           // board full: draw
    for (k = 0; k < n; k++) {
        u8 m = order[k];
        s16 v;
        u16 p = play(m);
        if (wins(p)) v = WIN - ply;
        else if (var >= V_PVS && k) {
            v = -search(depth - 1, -alpha - 1, -alpha);
            if (v > alpha && v < beta) v = -search(depth - 1, -beta, -alpha);
        } else v = -search(depth - 1, -beta, -alpha);
        unplay(m);
        if (v > best) { best = v; best_m = m; }
        if (v > alpha) alpha = v;
        if (alpha >= beta) {
            if (var >= V_KILLER && killer[ply][0] != m) { killer[ply][1] = killer[ply][0]; killer[ply][0] = m; }
            if (var >= V_KILLER) hist[turn - 1][m] += depth * depth;
            break;
        }
    }
    if (var >= V_TT) {
        u16 i = (u16)(((u32)(u16)hash * NBUCK) >> 16) * 2;
        TTE *s = tt[i].depth <= depth ? &tt[i] : &tt[i + 1];   // depth-preferred, else always-replace
        s->chk = hash >> 16; s->val = best; s->depth = depth;
        s->fm = (best <= a0 ? F_UPPER : best >= beta ? F_LOWER : F_EXACT) << 4 | best_m;
    }
    return best;
}
static s16 root(u16 v, u16 depth)
{
    var = v;
    memset(killer, 7, sizeof(killer));
    memset(hist, 0, sizeof(hist));
    if (tt) memset(tt, 0, NBUCK * 2 * sizeof(TTE));
    return search(depth, -INF, INF);
}
static u16 ab_move(u16 depth)                   // best column for the side to move (for games)
{
    s16 best = -INF;
    u16 c, bm = 3;
    var = V_PVS;
    if (tt) memset(tt, 0, NBUCK * 2 * sizeof(TTE));
    for (c = 0; c < 7; c++) {
        u8 m = centre[c];
        s16 v;
        u16 p;
        if (ht[m] >= 6) continue;
        p = play(m);
        v = wins(p) ? WIN : -search(depth - 1, -INF, -best);
        unplay(m);
        if (v > best) { best = v; bm = m; }
    }
    return bm;
}

// ------------------------------------------------------------------ MCTS
typedef struct { u16 n, w, child, parent; u8 nch, move; } NODE;   // w: 2 per win, 1 per draw
#define POOL 1500
static NODE *pool;
static u16 used;
static u16 R16[16] = { 63550, 59919, 56680, 53773, 51150, 48771, 46603, 44620, 42799, 41121, 39569, 38130, 36792, 35545, 34380, 33288 };
static u16 RS16[32] = { 32268, 31332, 30474, 29682, 28949, 28268, 27632, 27038, 26481, 25956, 25462, 24994, 24552, 24132, 23733, 23354,
                        22817, 22155, 21548, 20988, 20470, 19988, 19539, 19119, 18725, 18354, 18004, 17674, 17361, 17064, 16782, 16514 };
static const u16 LOG2M[16] = { 182, 530, 858, 1169, 1465, 1746, 2015, 2272, 2518, 2754, 2982, 3200, 3412, 3615, 3812, 4003 };
static u16 SL[256];                             // sqrt(ln N) in q12, per bucket
static u8 blen[256];                            // bit length of a byte
static u16 bucket(u16 n)                        // n >= 1 -> e * 16 + 4 mantissa bits
{
    u16 e = n >> 8 ? 8 + blen[n >> 8] - 1 : blen[n] - 1;
    return e * 16 + (((n << (15 - e)) >> 11) & 15);
}
static u32 isqrt(u32 x)
{
    u32 r = 0, bit = 1UL << 30;
    while (bit > x) bit >>= 2;
    while (bit) { if (x >= r + bit) { x -= r + bit; r = (r >> 1) + bit; } else r >>= 1; bit >>= 2; }
    return r;
}
static void mcts_tables(void)
{
    u16 i;
    for (i = 1; i < 256; i++) blen[i] = blen[i >> 1] + 1;
    for (i = 0; i < 256; i++) {
        u32 ln24 = (u32)((i >> 4) * 4096UL + LOG2M[i & 15]) * 2839UL;   // ln N = log2 N * ln 2, q24
        SL[i] = (u16)isqrt(ln24);
    }
}
static u16 uct_pick(NODE *nd)
{
    u16 bi = bucket(nd->n), k, best = 0;
    u16 sl = SL[bi];
    u32 bv = 0;
    NODE *c = &pool[nd->child];
    for (k = 0; k < nd->nch; k++, c++) {
        u16 cb, e;
        u32 v;
        if (!c->n) return nd->child + k;        // unvisited first
        cb = bucket(c->n); e = cb >> 4;
        v = ((u32)c->w * R16[cb & 15]) >> (e + 2);                              // w/n, q15
        v += ((u32)sl * RS16[((e & 1) << 4) | (cb & 15)]) >> (12 + (e >> 1));        // sqrt(ln N / n), q15
        if (v > bv) { bv = v; best = k; }
    }
    return nd->child + best;
}
static u8 mb[72], mht[7], mturn;                // saved position of the root
static u16 playout(void)                        // returns the winner (0 draw) from the current position
{
    while (ply < 42) {
        u16 c = RND(7), p;
        while (ht[c] >= 6) c = c == 6 ? 0 : c + 1;
        p = play(c);
        if (wins(p)) return b[p];
    }
    return 0;
}
static u32 playouts;
static u16 mcts(u16 iters)                      // returns the column for the side to move
{
    u16 it, k, start_ply = ply, path[43];
    NODE *r = pool;
    memcpy(mb, b, 72); memcpy(mht, ht, 7); mturn = turn;
    used = 1; r->n = r->w = 0; r->nch = 0; r->child = 0; r->parent = 0xFFFF; r->move = 7;
    for (it = 0; it < iters; it++) {
        u16 cur = 0, winner = 0xFFFF, d = 0;
        NODE *nd;
        memcpy(b, mb, 72); memcpy(ht, mht, 7); turn = mturn; ply = start_ply;
        path[0] = 0;
        for (;;) {                              // selection, expansion
            nd = &pool[cur];
            if (nd->child == 0xFFFF) { winner = turn ^ 3; break; }   // terminal: the last mover won
            if (!nd->nch) {
                if (!nd->n || used + 7 > POOL || ply >= 42) break;   // leaf: play out from here
                nd->child = used;
                for (k = 0; k < 7; k++)
                    if (ht[k] < 6) {
                        NODE *c = &pool[used++];
                        c->n = c->w = 0; c->nch = 0; c->child = 0; c->parent = cur; c->move = k;
                    }
                nd->nch = used - nd->child;
            }
            cur = uct_pick(nd);
            path[++d] = cur;
            if (wins(play(pool[cur].move))) { pool[cur].child = 0xFFFF; winner = turn ^ 3; break; }
        }
        if (winner == 0xFFFF) winner = playout();
        playouts++;
        for (k = 0; k <= d; k++) {             // backpropagation: node k was entered by mover(k)
            NODE *c = &pool[path[k]];
            u16 mover = k & 1 ? mturn : mturn ^ 3;
            c->n++;
            c->w += winner == mover ? 2 : winner == 0 ? 1 : 0;
        }
    }
    memcpy(b, mb, 72); memcpy(ht, mht, 7); turn = mturn; ply = start_ply;
    {
        u16 best = 3, bn = 0;
        NODE *c = &pool[r->child];
        for (k = 0; k < r->nch; k++, c++) if (c->n > bn) { bn = c->n; best = c->move; }
        return best;
    }
}

// ------------------------------------------------------------------ driver
static void setup(const char *moves)
{
    reset_board();
    while (*moves) play(*moves++ - '0');
}
static const char *const pos[3] = { "", "3322", "33224405" };
static const u16 depth[3] = { 8, 8, 8 };
static s16 score[3][NV];
static u32 vn[3][NV], vt[3][NV];
static const char *const vname[NV] = { "plain", "+centre", "+kill/hist", "+TT 4KB", "+PVS" };

// tactics: X to move can win at column 4 ("3304041" -> X has 3 in row 0: cols 0? ) built below
static u16 tactic(const char *moves, u16 iters) { setup(moves); return mcts(iters); }

static u16 game(u16 mcts_first, u16 opp, u16 iters)   // opp: 0 random, else alpha-beta depth
{
    u16 side_mcts = mcts_first ? 1 : 2;
    reset_board();
    while (ply < 42) {
        u16 c, p;
        if (turn == side_mcts) c = mcts(iters);
        else if (!opp) { c = RND(7); while (ht[c] >= 6) c = c == 6 ? 0 : c + 1; }
        else c = ab_move(opp);
        p = play(c);
        if (wins(p)) return b[p] == side_mcts ? 2 : 0;
    }
    return 1;
}

#ifdef __m68k__
void _main(void)
#else
int main(int argc, char **argv)
#endif
{
    u16 i, v, same = 1, twin, tblock;
    u32 tm = 0, pl;
    for (i = 0; i < 72; i++) { zob[0][i] = (u32)rnd16() << 16 | rnd16(); zob[1][i] = (u32)rnd16() << 16 | rnd16(); }
    tt = malloc(NBUCK * 2 * sizeof(TTE));
    pool = malloc(POOL * sizeof(NODE));
    if (!tt || !pool) goto out;
    mcts_tables();
#ifdef __m68k__
    ClrScr(); printf_xy(0, 0, "Benchmarking (~1 min)...");
    timer_on();
#endif
    for (i = 0; i < 3; i++)
        for (v = 0; v < NV; v++) {
            setup(pos[i]);
            nodes = 0;
#ifdef __m68k__
            tstart();
#endif
            score[i][v] = root(v, depth[i]);
#ifdef __m68k__
            vt[i][v] = TICKS();
#endif
            vn[i][v] = nodes;
            if (score[i][v] != score[i][0]) same = 0;
        }
    wy = 99; playouts = 0;
    setup("");
#ifdef __m68k__
    tstart();
#endif
    i = mcts(2000);
#ifdef __m68k__
    tm = TICKS();
#endif
    pl = playouts;
    // X (1) has 0,1,2 on the bottom row, O (2) stacked on column 6: X to move wins at column 3.
    twin = tactic("061626", 300);
    // O to move must block column 3 (X threatens 0,1,2 + 3).
    tblock = tactic("06162", 300);
#ifdef __m68k__
    timer_off();
    ClrScr(); FontSetSys(F_4x6);
    printf_xy(0, 0, "alpha-beta depth 8: nodes (kcycles)");
    printf_xy(0, 7, "            empty      3322      33224405");
    for (v = 0; v < NV; v++)
        printf_xy(0, 14 + 7 * v, "%-10s%6lu %5lu %6lu %5lu %6lu %5lu", vname[v],
                  vn[0][v], CYC(vt[0][v]) / 1000, vn[1][v], CYC(vt[1][v]) / 1000, vn[2][v], CYC(vt[2][v]) / 1000);
    printf_xy(0, 50, "scores %d %d %d, all variants %s", score[0][0], score[1][0], score[2][0], same ? "SAME" : "DIFFER");
    printf_xy(0, 60, "MCTS 2000 playouts: %lu cyc each, %lu/s", CYC(tm) / pl, pl * 256UL / tm);
    printf_xy(0, 67, "pool %u of %u nodes x %u B, move %u", used, POOL, (u16)sizeof(NODE), i);
    printf_xy(0, 74, "tactics: win col %u (3), block col %u (3)", twin, tblock);
    GKeyFlush(); ngetchx();
    FontSetSys(F_6x8);
#else
    for (i = 0; i < 3; i++) {
        printf("pos '%s' depth %u:", pos[i], depth[i]);
        for (v = 0; v < NV; v++) printf(" %s %lu (%d)", vname[v], (unsigned long)vn[i][v], score[i][v]);
        printf("\n");
    }
    printf("all variants %s; MCTS 2000 from empty: move %u, pool %u; tactics win %u block %u\n",
           same ? "SAME" : "DIFFER", i, used, twin, tblock);
    (void)tm; (void)pl;
    if (argc > 1) {
        static const u16 iters[2] = { 300, 1000 };
        u16 it, g, r[3];
        for (it = 0; it < 2; it++) {
            r[0] = r[1] = r[2] = 0;
            for (g = 0; g < 20; g++) r[game(g & 1, 0, iters[it])]++;
            printf("MCTS %u vs random, 20 games: %u wins %u draws %u losses\n", iters[it], r[2], r[1], r[0]);
            r[0] = r[1] = r[2] = 0;
            for (g = 0; g < 10; g++) r[game(g & 1, 4, iters[it])]++;
            printf("MCTS %u vs alpha-beta d4, 10 games: %u wins %u draws %u losses\n", iters[it], r[2], r[1], r[0]);
        }
    }
    (void)argv;
#endif
out:
    free(pool); free(tt);
#ifndef __m68k__
    return 0;
#endif
}
