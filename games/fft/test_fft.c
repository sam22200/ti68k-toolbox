// Tests, headless: the four orientations, the map's heights, the battle's units and their stats
// (against the original's, oracle.h), reach and paths, the scene and the occlusion of units by
// the buildings in front of them, rotation and walking by keys.
//   ./fft_test            every test
//   ./fft_test --find     the tiles where a unit is hidden in one view and seen in another
#include <stdio.h>
#include <string.h>
#include "../../runtime/platform-sw/rt_sw.h"
#include "fft.h"
#include "oracle.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)
#define CHECKV(a, b) do { long _a = (a), _b = (b); if (_a != _b) { printf("FAIL %s:%d: %s = %ld, want %ld\n", __FILE__, __LINE__, #a, _a, _b); fails++; } } while (0)

static void steps(u32 keys, int n) { while (n--) sw_step(keys); }
static void press(u32 keys) { sw_step(keys); sw_step(0); }

// the scene buffer as a PGM (4 greys)
static void scene_pgm(const char *path)
{
    FILE *f = fopen(path, "wb");
    int x, y;
    if (!f) return;
    fprintf(f, "P5 %d %d 255\n", SC_W, SC_H);
    for (y = 0; y < SC_H; y++)
        for (x = 0; x < SC_W; x++) {
            int b = 0x80 >> (x & 7), i = y * SC_BYTES + (x >> 3);
            int g = ((scene_l[i] & b) ? 1 : 0) + ((scene_d[i] & b) ? 2 : 0);
            fputc(255 - g * 80, f);
        }
    fclose(f);
}

// ---------------------------------------------------------------- milestone 1: orientations
static void test_rotations(void)
{
    u8 r;
    s16 x, z, u, v, x2, z2;
    for (r = 0; r < 4; r++)
        for (z = 0; z < MAP_H; z++)
            for (x = 0; x < MAP_W; x++) {
                view_of(r, x, z, &u, &v);
                CHECK(u >= 0 && u < view_w(r) && v >= 0 && v < view_h(r));
                world_of(r, u, v, &x2, &z2);
                CHECK(x2 == x && z2 == z);
            }
    // the prompt's transforms: 90: (H-1-y, x), 180: (W-1-x, H-1-y), 270: (y, W-1-x)
    view_of(1, 2, 3, &u, &v); CHECK(u == MAP_H - 1 - 3 && v == 2);
    view_of(2, 2, 3, &u, &v); CHECK(u == MAP_W - 1 - 2 && v == MAP_H - 1 - 3);
    view_of(3, 2, 3, &u, &v); CHECK(u == 3 && v == MAP_W - 1 - 2);
    // a step +1 in the view of r is the same world step as +1 in r+1 turned once: the
    // world's +x axis is view +u at r 0, +v at r 1, -u at r 2, -v at r 3
    view_of(0, 3, 3, &u, &v); view_of(0, 4, 3, &x2, &z2); CHECK(x2 == u + 1 && z2 == v);
    view_of(1, 3, 3, &u, &v); view_of(1, 4, 3, &x2, &z2); CHECK(x2 == u && z2 == v + 1);
    view_of(2, 3, 3, &u, &v); view_of(2, 4, 3, &x2, &z2); CHECK(x2 == u - 1 && z2 == v);
    view_of(3, 3, 3, &u, &v); view_of(3, 4, 3, &x2, &z2); CHECK(x2 == u && z2 == v - 1);
}

// ---------------------------------------------------------------- milestone 2: heights
static void test_heights(void)
{
    u8 r, c, maxh = 0;
    s16 u, v;
    for (u = 0; u < MAP_W * MAP_H; u++)
        for (c = 0; c < 4; c++) if (map_tiles[u].c[c] > maxh) maxh = map_tiles[u].c[c];
    CHECKV(maxh, MAXH);                                   // the scene buffer is sized on it
    CHECKV(MAP_W, 10); CHECKV(MAP_H, 15);                 // Gariland, MAP022
    // a view tile's corners are the same world corners in every orientation: the corner
    // between view tiles (u, v) and (u + 1, v + 1) has one height seen from both
    for (r = 0; r < 4; r++)
        for (v = 0; v + 1 < view_h(r); v++)
            for (u = 0; u + 1 < view_w(r); u++) {
                s16 x, z;
                world_of(r, u, v, &x, &z);
                // view corner 2 of (u, v) is world corner (2 - r) & 3 of (x, z): same height
                CHECKV(corner_h(r, u, v, 2), map_tiles[z * MAP_W + x].c[(2 - r) & 3]);
            }
    // Gariland's measured heights (RE_NOTES.md): street 2, canal 0, house roofs 6-8, chimney 10
    CHECKV(tile_at(5, 5)->stand, 4);                      // stone street, h 2 (half units)
    CHECK(!tile_at(4, 0)->walk);                          // canal
    CHECKV(tile_at(4, 3)->c[0], 8);                       // roof ridge
    CHECKV(tile_at(1, 13)->c[0], 10);                     // the chimney
    CHECK(tile_at(4, 2)->c[0] == 6 && tile_at(4, 2)->c[3] == 8);   // roof slope rising to +z
}

// ---------------------------------------------------------------- milestone 14: the units
// the original's units at Ramza's first turn (oracle.h: their raw stats and equipment read from
// its RAM): our stats from them equal the original's, every one
static void test_stats(void)
{
    u8 i, k;
    for (i = 0; i < NUNIT; i++) {
        Unit u;
        memset(&u, 0, sizeof u);
        u.job = oracle[i].job; u.level = oracle[i].level;
        for (k = 0; k < 5; k++) { u.raw[k] = oracle[i].raw[k]; u.eq[k] = oracle[i].eq[k]; }
        unit_stats(&u);
        CHECKV(u.max_hp, oracle[i].want[0]); CHECKV(u.max_mp, oracle[i].want[1]);
        CHECKV(u.sp, oracle[i].want[2]); CHECKV(u.pa, oracle[i].want[3]); CHECKV(u.ma, oracle[i].want[4]);
        CHECKV(u.move, oracle[i].want[5]); CHECKV(u.jump, oracle[i].want[6]);
        CHECKV(u.job, unit_defs[i].job);                  // the same jobs in the same order
        CHECKV(u.hp, u.max_hp);
    }
}

// the draw at each battle: every value in FFT's range, each random item drawn sometimes
static void test_draw(void)
{
    u16 seed, i, k, lo_hp = 999, hi_hp = 0, delita_eq[2] = { 0, 0 };
    for (seed = 1; seed <= 300; seed++) {
        sw_init(seed + 100);                              // scenarios above 7: Ramza's first turn
        CHECKV(st.cx, st.unit[U_RAMZA].x);
        for (i = 0; i < NUNIT; i++) {
            const Unit *u = &st.unit[i];
            const UnitDef *d = &unit_defs[i];
            CHECK(u->x == d->x && u->z == d->z && u->gfx == d->gfx && u->team == d->team);
            for (k = 0; k < 5; k++) {
                u32 b = (u32)gen_base[d->type][k] << 14;
                CHECK(u->raw[k] >= b && u->raw[k] <= b + 32767UL * gen_var[d->type][k] / 2);
            }
            CHECK(d->brave ? u->brave == d->brave : u->brave >= 45 && u->brave <= 74);
            CHECK(d->faith ? u->faith == d->faith : u->faith >= 45 && u->faith <= 74);
            CHECK(u->zodiac < 12);
            for (k = 0; k < EQ_N; k++) CHECK(u->eq[k] == d->eq[k][0] || u->eq[k] == d->eq[k][1]);
            CHECK(u->hp == u->max_hp && u->mp == u->max_mp && u->dc == 3);
            if (d->job == unit_defs[1].job) {             // the enemy squires: HP 34-44 seen
                if (u->max_hp < lo_hp) lo_hp = u->max_hp;
                if (u->max_hp > hi_hp) hi_hp = u->max_hp;
            }
        }
        delita_eq[st.unit[0].eq[EQ_WEAPON] == unit_defs[0].eq[EQ_WEAPON][1]]++;
    }
    printf("draw: squires' HP %d-%d, Delita's weapon %d / %d\n", lo_hp, hi_hp, delita_eq[0], delita_eq[1]);
    CHECK(lo_hp >= 30 && hi_hp <= 47 && hi_hp - lo_hp >= 5);
    CHECK(delita_eq[0] > 100 && delita_eq[1] > 100);      // Dagger or Broad Sword, as FFT draws
    CHECK(st.unit[U_RAMZA].brave == 70 && st.unit[U_RAMZA].move == 5);   // Battle Boots: Move +1
}

// ---------------------------------------------------------------- reach and paths
static void test_reach(void)
{
    u8 i, n;
    const Unit *r = &st.unit[U_RAMZA];
    sw_init(0);
    compute_reach(U_RAMZA);                               // Ramza at (4, 11), h 2, Move 5 Jump 3
    CHECKV(st.reach[11 * MAP_W + 4], 0);
    CHECKV(st.reach[10 * MAP_W + 4], 1);                  // the street in front of him
    CHECKV(st.reach[11 * MAP_W + 5], 0xFF);               // the canal: never
    CHECKV(st.reach[11 * MAP_W + 2], 2);                  // through an ally (the squire at 3, 11)
    CHECKV(st.reach[11 * MAP_W + 3], 0xFF);               // but not onto it
    CHECKV(st.reach[7 * MAP_W + 3], 5);                   // a roof, up from the next one
    for (i = 0; i < MAP_W * MAP_H; i++)                   // never more than Move steps
        CHECK(st.reach[i] == 0xFF || st.reach[i] <= r->move);
    n = make_path(U_RAMZA, 4, 8);
    CHECKV(n, 4);                                         // 3 steps up the street
    CHECKV(st.path[0][0], 4); CHECKV(st.path[0][1], 11);
    CHECKV(st.path[n - 1][0], 4); CHECKV(st.path[n - 1][1], 8);
    for (i = 1; i < n; i++) {                             // adjacent steps, Jump respected
        s16 dx = st.path[i][0] - st.path[i - 1][0], dz = st.path[i][1] - st.path[i - 1][1];
        s16 dh = tile_at(st.path[i][0], st.path[i][1])->stand - tile_at(st.path[i - 1][0], st.path[i - 1][1])->stand;
        CHECKV(dx * dx + dz * dz, 1);
        CHECK(dh <= 2 * r->jump && dh >= -2 * r->jump);
    }
    CHECKV(make_path(U_RAMZA, 5, 11), 0);                 // a canal tile: no path
    compute_reach(U_HIDE);                                // an enemy: the player's units block it
    CHECKV(st.reach[11 * MAP_W + 3], 0xFF);
}

// ---------------------------------------------------------------- milestone 3-4: occlusion
// pixels of unit i that show on screen: the frame with it against the frame without it
static int mid;                                           // of those, in a mid grey (not the contour)

static int shown(u8 i)
{
    static u8 with[RT_H][RT_W];
    int x, y, n = 0;
    u8 ux = st.unit[i].x;
    State save = st;
    sw_step(0);
    for (y = 0; y < RT_H; y++) for (x = 0; x < RT_W; x++) with[y][x] = sw_level(x, y);
    st = save;
    st.unit[i].x = 200;                                   // out of the map: not drawn
    sw_step(0);
    mid = 0;
    for (y = 0; y < RT_H - 8; y++)
        for (x = 0; x < RT_W; x++)
            if (with[y][x] != sw_level(x, y)) { n++; mid += with[y][x] == 1 || with[y][x] == 2; }
    st = save;
    st.unit[i].x = ux;
    return n;
}

static int alone;                                         // the unit's pixels with nothing in front

static void at_view(u8 r, u8 x, u8 z)
{
    st.rot = r; st.turn = 0; st.mode = M_BROWSE;
    st.unit[2].x = x; st.unit[2].z = z;
    st.cx = x; st.cz = z;
    st.camx = 0; st.camy = 0;
    for (int k = 0; k < 40; k++) sw_step(0);              // let the camera settle on the cursor
}

static void find_spots(void)
{
    s16 x, z;
    u8 r;
    sw_init(0);
    for (z = 0; z < MAP_H; z++)
        for (x = 0; x < MAP_W; x++) {
            int s[4], lo = 1000, hi = 0;
            if (!tile_at(x, z)->walk || unit_at(x, z)) continue;
            for (r = 0; r < 4; r++) {
                at_view(r, (u8)x, (u8)z);
                s[r] = shown(2);
                if (s[r] < lo) lo = s[r];
                if (s[r] > hi) hi = s[r];
            }
            if (lo * 4 < hi) printf("(%d, %d) shown %d %d %d %d\n", x, z, s[0], s[1], s[2], s[3]);
        }
}

static void test_occlusion(void)
{
    int s[4], m[4], inside;
    u8 r;
    sw_init(1);                                           // an enemy behind the house
    for (r = 0; r < 4; r++) {
        at_view(r, st.unit[2].x, st.unit[2].z);
        s[r] = shown(2); m[r] = mid;
    }
    printf("occlusion: shown %d %d %d %d, mid greys %d %d %d %d\n", s[0], s[1], s[2], s[3],
           m[0], m[1], m[2], m[3]);
    alone = s[0]; inside = m[0];
    for (r = 1; r < 4; r++) { if (s[r] > alone) alone = s[r]; if (m[r] > inside) inside = m[r]; }
    CHECK(alone > 80);                                    // ~100 pixels with the outline
    CHECK(m[0] * 5 < inside);                             // hidden from the south...
    CHECK(s[0] > alone / 3);                              // ...but its contour drawn over the house
    CHECK(s[1] > alone * 9 / 10);                         // seen once turned with F5
}

// the diamond above enemies: the enemy seen (west view) shows ~30 pixels more than as an ally
static void test_team(void)
{
    int foe, ally;
    sw_init(2);
    foe = shown(2);
    st.unit[2].team = TEAM_PLAYER;
    ally = shown(2);
    printf("team: enemy %d pixels, ally %d\n", foe, ally);
    CHECK(foe - ally >= 20 && foe - ally <= 40);
}

// ---------------------------------------------------------------- rotation and walking by keys
static void test_turn_keys(void)
{
    u16 scene0, i;
    u8 cx, cz;
    sw_init(0);
    sw_step(0);
    scene0 = scene_checksum();
    cx = st.cx; cz = st.cz;
    press(K_F5);
    CHECKV(st.turn, 1);
    steps(0, TURN_FRAMES);
    CHECKV(st.rot, 1); CHECKV(st.turn, 0);
    CHECK(st.cx == cx && st.cz == cz);                    // the cursor stays on its tile
    CHECK(scene_checksum() != scene0);
    for (i = 0; i < 3; i++) { press(K_F5); steps(0, TURN_FRAMES); }
    CHECKV(st.rot, 0);
    CHECKV(scene_checksum(), scene0);                     // four turns: the same scene
    press(K_F1); steps(0, TURN_FRAMES);
    CHECKV(st.rot, 3);
    CHECK(st.unit[U_RAMZA].x == 4 && st.unit[U_RAMZA].z == 11);   // units never move with the view
    // every orientation: the units stand on their own tiles (same world tile under the feet)
    for (i = 0; i < 4; i++) {
        st.rot = (u8)i; sw_step(0);
        CHECK(scene_rot == i);
    }
}

static void test_walk_keys(void)
{
    int k;
    sw_init(8);                                           // Ramza's turn, cursor on him (4, 11)
    CHECKV(st.act, U_RAMZA);
    press(K_A);
    CHECKV(st.mode, M_TARGET);
    press(K_UP); press(K_UP);                             // view v = world z at r 0
    press(K_RIGHT); press(K_LEFT);                        // view u = world x
    CHECK(st.cx == 4 && st.cz == 9);
    press(K_A);
    CHECKV(st.mode, M_WALK);
    for (k = 0; k < 40 && st.mode == M_WALK; k++) sw_step(0);
    CHECKV(st.mode, M_BROWSE);
    CHECK(st.unit[U_RAMZA].x == 4 && st.unit[U_RAMZA].z == 9);
    CHECKV(st.unit[U_RAMZA].face, 3);                     // facing its last step: -z
    CHECKV(st.unit[U_RAMZA].ct, 22);                      // moved, no Act: 2 + 20
    CHECKV(st.act, U_RAMZA + 1);                          // the next recruit's turn, cursor on it
    CHECK(st.cx == 3 && st.cz == 11);
    press(K_A); press(K_ESC);                             // select then cancel: no quit
    CHECKV(st.mode, M_BROWSE);
    // after a turn the arrows follow the view: at r 1, view +v is world +x
    press(K_F5); steps(0, TURN_FRAMES);
    press(K_DOWN);
    CHECK(st.cx == 4 && st.cz == 11);
    press(K_A);                                           // not the active unit: nothing
    CHECKV(st.mode, M_BROWSE);
    CHECKV(sw_step(K_ESC), 0);                            // ESC in browse quits
}

// ---------------------------------------------------------------- milestone 15: turn order
// FFT's clock replayed on the original's 46 first turns (oracle.h: the player's units played by
// its AI): each turn's unit, every unit's CT when it starts, with the original's CT bonus at the
// end of each turn and its KOs (a unit at 0 HP keeps its clock, its death counter runs down).
static void test_clock(void)
{
    u16 t, i, ko = 0, gone = 0;
    sw_init(0);
    for (i = 0; i < NUNIT; i++) { st.unit[i].ct = 0; st.unit[i].sp = (u8)oracle[i].want[2]; }
    for (t = 0; t < ORACLE_TURNS; t++) {
        u8 u = ct_next(), b = oracle_turns[t].bonus;
        CHECKV(u, oracle_turns[t].unit);
        for (i = 0; i < NUNIT; i++)
            if (oracle_turns[t].ct[i] != 0xFF && st.unit[i].ct != oracle_turns[t].ct[i]) {
                printf("turn %d unit %d: ", t, i); CHECKV(st.unit[i].ct, oracle_turns[t].ct[i]);
            }
        ct_end(u, b < 40, b == 0);
        for (i = 0; i < NUNIT; i++)
            if (oracle_turns[t].dead >> i & 1) st.unit[i].hp = 0;
    }
    for (i = 0; i < NUNIT; i++) { ko += !st.unit[i].hp; gone += st.unit[i].dc == DC_GONE; }
    printf("clock: %d turns as the original's, %d KO, %d crystal or chest\n", ORACLE_TURNS, ko, gone);
    CHECK(ko >= 3 && gone >= 1);
    // the same CT, the lower index first; the cap at 60
    st.unit[0].ct = 50; ct_end(0, 0, 0); CHECKV(st.unit[0].ct, 60);
}

// the battle from its start: Delita and the five enemies wait their turns, then Ramza's; the
// order shown is FFT's AT list
static void test_turns(void)
{
    u8 i;
    sw_init(0);
    CHECKV(st.act, 0);                                    // all at CT 102: Delita, index 0
    for (i = 0; i < ORDER_N; i++) CHECKV(st.turns[i], i);
    for (i = 1; i <= 5; i++) {
        steps(0, AI_WAIT);
        CHECKV(st.act, i);
        CHECK(st.cx == st.unit[i].x && st.cz == st.unit[i].z);
        CHECKV(st.unit[i - 1].ct, 42);                    // waited: 2 + 40
    }
    steps(0, AI_WAIT);
    CHECKV(st.act, U_RAMZA);
    CHECKV(st.turns[1], U_RAMZA + 1);                     // the recruits, still at 102
    steps(0, 60);
    CHECKV(st.act, U_RAMZA);                              // the player's turn waits for keys
    press(K_C);                                           // Wait
    CHECKV(st.unit[U_RAMZA].ct, 42);
    CHECKV(st.act, U_RAMZA + 1);
    // after the recruits, the next tick: everyone at 42 + 60 = 102 again, Delita first
    for (i = 0; i < 4; i++) press(K_C);
    CHECKV(st.act, 0);
    CHECKV(st.unit[0].ct, 2);
}

static void shots(void)
{
    u8 n;
    char name[32];
    for (n = 0; n <= 7; n++) {
        sw_init(n);
        sw_step(0);
        snprintf(name, sizeof name, "x/s%d.png", n);
        sw_write_png(name, 4);
    }
    sw_init(0);
    sw_step(0);
    scene_pgm("x/scene0.pgm");
}

int main(int argc, char **argv)
{
    if (argc > 1 && !strcmp(argv[1], "--find")) { find_spots(); return 0; }
    test_rotations();
    test_heights();
    test_stats();
    test_draw();
    test_reach();
    test_occlusion();
    test_team();
    test_turn_keys();
    test_walk_keys();
    test_clock();
    test_turns();
    shots();
    printf(fails ? "%d FAILED\n" : "all tests passed\n", fails);
    return fails != 0;
}
