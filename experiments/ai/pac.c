// Pac-Man target-tile ghost AI (The Pac-Man Dossier), measured: no pathfinding, at each
// intersection a ghost drops the reverse direction and takes the exit whose next tile is closest
// (squared distance) to its target tile. Blinky targets Pac-Man, Pinky 4 tiles ahead (with the
// arcade "up" overflow bug), Inky the doubled vector Blinky -> 2 tiles ahead, Clyde Pac-Man when
// farther than 8 tiles, else his corner. Chase/scatter alternate on a step counter.
// Pac-Man does a random walk. Same results on host and TI (checksum).
// Host: gcc -O2 -Wall -o pac pac.c && ./pac      TI: ti-cc -o pac pac.c
#include "ai.h"

#define MW 28
#define MH 31
static const char maze[MH][MW + 1] = {
    "############################", "#............##............#", "#.####.#####.##.#####.####.#",
    "#.####.#####.##.#####.####.#", "#.####.#####.##.#####.####.#", "#..........................#",
    "#.####.##.########.##.####.#", "#.####.##.########.##.####.#", "#......##....##....##......#",
    "######.#####.##.#####.######", "######.#####.##.#####.######", "######.##..........##.######",
    "######.##.###--###.##.######", "######.##.#......#.##.######", "#.........#......#.........#",
    "######.##.#......#.##.######", "######.##.########.##.######", "######.##..........##.######",
    "######.##.########.##.######", "######.##.########.##.######", "#............##............#",
    "#.####.#####.##.#####.####.#", "#.####.#####.##.#####.####.#", "#...##................##...#",
    "###.##.##.########.##.##.###", "###.##.##.########.##.##.###", "#......##....##....##......#",
    "#.##########.##.##########.#", "#.##########.##.##########.#", "#..........................#",
    "############################" };

// Directions in the arcade tie-break order: up, left, down, right. Reverse = d ^ 2.
static const s8 ddx[4] = { 0, -1, 0, 1 }, ddy[4] = { -1, 0, 1, 0 };
static u8 exits[MW * MH];        // bit d set = the neighbour in direction d is free
static u16 sq[128];              // squares for |d| < 128
static const u8 firstbit[16] = { 0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0 };

typedef struct { u8 x, y, dir, pad; } Actor;
static Actor pac, gh[4];
static const s8 cornx[4] = { 25, 2, 27, 0 }, corny[4] = { -4, -4, 34, 34 };
static u16 step_no;
static u32 ndecide, ncorr, ncatch;

static void maze_init(void)
{
    short x, y, d;
    for (y = 0; y < MH; y++)
        for (x = 0; x < MW; x++) {
            u8 m = 0;
            if (maze[y][x] != '#' && maze[y][x] != '-')
                for (d = 0; d < 4; d++) {
                    short nx = x + ddx[d], ny = y + ddy[d];
                    if (nx >= 0 && nx < MW && ny >= 0 && ny < MH && maze[ny][nx] != '#' && maze[ny][nx] != '-')
                        m |= 1 << d;
                }
            exits[y * MW + x] = m;
        }
    for (x = 0; x < 128; x++) sq[x] = x * x;
}

static short iabs(short v) { return v < 0 ? -v : v; }

// Target tile of ghost g (chase or scatter).
static void target(short g, short *tx, short *ty)
{
    short px = pac.x, py = pac.y, d = pac.dir;
    if ((step_no & 127) < 28) { *tx = cornx[g]; *ty = corny[g]; return; }   // scatter
    switch (g) {
    case 0: *tx = px; *ty = py; break;
    case 1:
        *tx = px + (ddx[d] << 2); *ty = py + (ddy[d] << 2);
        if (d == 0) *tx -= 4;                                                 // arcade bug
        break;
    case 2: {
        short ax = px + (ddx[d] << 1), ay = py + (ddy[d] << 1);
        if (d == 0) ax -= 2;
        *tx = (ax << 1) - gh[0].x; *ty = (ay << 1) - gh[0].y;
        break;
    }
    default: {
        short dx = gh[3].x - px, dy = gh[3].y - py;
        if (dx * dx + dy * dy > 64) { *tx = px; *ty = py; }
        else { *tx = cornx[3]; *ty = corny[3]; }
    }
    }
}

// Choose the next direction at tile (x, y) coming in direction dir. Squares from a table.
static u8 choose_tab(short x, short y, u8 dir, short tx, short ty)
{
    u8 m = exits[y * MW + x] & ~(1 << (dir ^ 2)), best = 0, d;
    u16 bd = 0xFFFF;
    if (!(m & (m - 1))) return m ? firstbit[m] : dir ^ 2;   // corridor or dead end
    for (d = 0; d < 4; d++)
        if (m & (1 << d)) {
            u16 dist = sq[iabs(x + ddx[d] - tx)] + sq[iabs(y + ddy[d] - ty)];
            if (dist < bd) { bd = dist; best = d; }
        }
    return best;
}

// Same with 16-bit multiplies (one muls.w each).
static u8 choose_mul(short x, short y, u8 dir, short tx, short ty)
{
    u8 m = exits[y * MW + x] & ~(1 << (dir ^ 2)), best = 0, d;
    u16 bd = 0xFFFF;
    if (!(m & (m - 1))) return m ? firstbit[m] : dir ^ 2;
    for (d = 0; d < 4; d++)
        if (m & (1 << d)) {
            short dx = x + ddx[d] - tx, dy = y + ddy[d] - ty;
            u16 dist = dx * dx + dy * dy;
            if (dist < bd) { bd = dist; best = d; }
        }
    return best;
}

// Unrolled: the 4 candidate tiles differ from (x, y) by one, so |dx| and |dy| are computed once.
#define SQ(v) sq[(v) < 0 ? -(v) : (v)]
static u8 choose_unr(short x, short y, u8 dir, short tx, short ty)
{
    u8 m = exits[y * MW + x] & ~(1 << (dir ^ 2)), best = 0;
    short dx = x - tx, dy = y - ty;
    u16 bd = 0xFFFF, d, sx = SQ(dx), sy = SQ(dy);
    if (!(m & (m - 1))) return m ? firstbit[m] : dir ^ 2;
    if (m & 1) { bd = sx + SQ(dy - 1); }
    if (m & 2) { d = SQ(dx - 1) + sy; if (d < bd) { bd = d; best = 1; } }
    if (m & 4) { d = sx + SQ(dy + 1); if (d < bd) { bd = d; best = 2; } }
    if (m & 8) { d = SQ(dx + 1) + sy; if (d < bd) best = 3; }
    return best;
}

static void move(Actor *a) { a->x += ddx[a->dir]; a->y += ddy[a->dir]; }

static void pac_step(void)
{
    u8 m = exits[pac.y * MW + pac.x] & ~(1 << (pac.dir ^ 2)), k, d;
    if (!m) m = 1 << (pac.dir ^ 2);
    k = RND((u8)((m & 1) + ((m >> 1) & 1) + ((m >> 2) & 1) + (m >> 3)));
    for (d = 0; d < 4; d++)
        if ((m & (1 << d)) && !k--) break;
    pac.dir = d;
    move(&pac);
}

static void ghosts_step(void)
{
    short g, tx, ty;
    for (g = 0; g < 4; g++) {
        Actor *a = gh + g;
        u8 m = exits[a->y * MW + a->x] & ~(1 << (a->dir ^ 2));
        if (m & (m - 1)) ndecide++; else ncorr++;
        target(g, &tx, &ty);
        a->dir = choose_tab(a->x, a->y, a->dir, tx, ty);
        move(a);
#ifndef __m68k__
        if (maze[a->y][a->x] == '#' || maze[a->y][a->x] == '-') { printf("ghost %d in a wall!\n", g); exit(1); }
#endif
        if (a->x == pac.x && a->y == pac.y) ncatch++;
    }
}

static void sim_init(void)
{
    short g;
    wy = 7; step_no = 0; ndecide = ncorr = ncatch = 0;
    pac.x = 13; pac.y = 23; pac.dir = 1;
    for (g = 0; g < 4; g++) { gh[g].x = 13 + (g & 1); gh[g].y = 11; gh[g].dir = 1 + ((g & 1) << 1); }
}

#define NSTEP 2000
static void sim(void) { u16 n; for (n = 0; n < NSTEP; n++) { pac_step(); ghosts_step(); step_no++; } }
static u16 checksum(void)
{
    u16 c = step_no, g;
    for (g = 0; g < 4; g++) c = c * 31 + gh[g].x * 7 + gh[g].y + gh[g].dir;
    return c + pac.x + pac.y;
}

// Decision replay list for the micro-benchmarks
#define NL 256
static u8 lx[NL], ly[NL], ld[NL];
static s8 ltx[NL], lty[NL];
static void record(void)
{
    short n = 0, g, tx, ty;
    sim_init();
    while (n < NL) {
        pac_step();
        for (g = 0; g < 4 && n < NL; g++) {
            Actor *a = gh + g;
            u8 m;
            target(g, &tx, &ty);
            m = exits[a->y * MW + a->x] & ~(1 << (a->dir ^ 2));
            if (m & (m - 1)) { lx[n] = a->x; ly[n] = a->y; ld[n] = a->dir; ltx[n] = tx; lty[n] = ty; n++; }
            a->dir = choose_tab(a->x, a->y, a->dir, tx, ty);
            move(a);
        }
        step_no++;
    }
}

#ifdef __m68k__
volatile u16 sink;
#define NR 20000
static void b_empty(void) { u16 n; for (n = 0; n < NR; n++) { u8 i = (u8)n; sink = lx[i] + ly[i] + ld[i] + ltx[i] + lty[i]; } }
static void b_tab(void)   { u16 n; for (n = 0; n < NR; n++) { u8 i = (u8)n; sink = choose_tab(lx[i], ly[i], ld[i], ltx[i], lty[i]); } }
static void b_mul(void)   { u16 n; for (n = 0; n < NR; n++) { u8 i = (u8)n; sink = choose_mul(lx[i], ly[i], ld[i], ltx[i], lty[i]); } }
static void b_unr(void)   { u16 n; for (n = 0; n < NR; n++) { u8 i = (u8)n; sink = choose_unr(lx[i], ly[i], ld[i], ltx[i], lty[i]); } }
static void b_targ(void)  { u16 n; short tx, ty; for (n = 0; n < NR; n++) { target(n & 3, &tx, &ty); sink = tx + ty; } }

#endif

static short agree(void)
{
    short i;
    for (i = 0; i < NL; i++)
        if (choose_tab(lx[i], ly[i], ld[i], ltx[i], lty[i]) != choose_mul(lx[i], ly[i], ld[i], ltx[i], lty[i])
            || choose_tab(lx[i], ly[i], ld[i], ltx[i], lty[i]) != choose_unr(lx[i], ly[i], ld[i], ltx[i], lty[i])) return 0;
    return 1;
}

#ifdef __m68k__
void _main(void)
{
    unsigned long t[6];
    short ok;
    u16 cs;
    maze_init();
    record(); ok = agree();
    ClrScr(); printf_xy(0, 0, "Benchmarking...");
    timer_on();
    tstart(); b_empty(); t[0] = TICKS();
    tstart(); b_tab();   t[1] = TICKS();
    tstart(); b_mul();   t[2] = TICKS();
    tstart(); b_unr();   t[5] = TICKS();
    step_no = 100; // chase phase for the target benchmark
    tstart(); b_targ();  t[3] = TICKS();
    sim_init();
    tstart(); sim();     t[4] = TICKS();
    timer_off();
    cs = checksum();
    ClrScr(); FontSetSys(F_4x6);
    printf_xy(0, 0, "Pac-Man ghosts, cycles (NR=%u)", NR);
    printf_xy(0, 7, "decide, sq table : %lu", (CYC(t[1]) - CYC(t[0])) / NR);
    printf_xy(0, 14, "decide, muls.w   : %lu", (CYC(t[2]) - CYC(t[0])) / NR);
    printf_xy(80, 7, "unrolled: %lu", (CYC(t[5]) - CYC(t[0])) / NR);
    printf_xy(0, 21, "target (avg of 4): %lu", CYC(t[3]) / NR);
    printf_xy(0, 28, "sim step pac+4gh : %lu", CYC(t[4]) / NSTEP);
    printf_xy(0, 35, "decisions %lu corr %lu", ndecide, ncorr);
    printf_xy(0, 42, "catches %lu cksum %u", ncatch, cs);
    printf_xy(0, 49, "3 versions agree: %s  RAM %u B", ok ? "yes" : "NO", (u16)(sizeof exits + sizeof sq));
    GKeyFlush(); ngetchx();
    FontSetSys(F_6x8);
}
#else
int main(void)
{
    maze_init();
    record();
    printf("tab==mul==unr: %s\n", agree() ? "yes" : "NO");
    sim_init(); sim();
    printf("decisions %lu corr %lu catches %lu cksum %u\n", (unsigned long)ndecide, (unsigned long)ncorr,
           (unsigned long)ncatch, checksum());
    return 0;
}
#endif
