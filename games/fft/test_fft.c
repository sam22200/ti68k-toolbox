// Tests, headless: the four orientations, the map's heights, the battle's units and their stats
// (against the original's, oracle.h), reach and paths, the scene and the occlusion of units by
// the buildings in front of them, rotation and walking by keys, the turn menu, attacks (against
// the original's previews and attacks, oracle.h), abilities (against the original's, oracle.h).
//   ./fft_test            every test
//   ./fft_test --find     the tiles where a unit is hidden in one view and seen in another
//   ./fft_test --ai       every test, and each AI turn of the original's that ours plays otherwise
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
// a player's unit waits, facing as it does: Wait in its menu, then the facing confirmed
static void wait_turn(void) { press(K_DOWN); press(K_DOWN); press(K_A); press(K_A); }

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
    CHECK(tile_at(4, 0)->water && tile_at(4, 0)->walk);   // canal: waded
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
        sw_init(seed + 100);                              // scenarios above 9: Ramza's first turn
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
static void ai_load(u16 r);

static void test_reach(void)
{
    u8 i, k, n, diff = 0;
    const Unit *r = &st.unit[U_RAMZA];
    sw_init(0);
    compute_reach(U_RAMZA);                               // Ramza at (4, 11), h 2, Move 5 Jump 3
    CHECKV(st.reach[11 * MAP_W + 4], 0);
    CHECKV(st.reach[10 * MAP_W + 4], 1);                  // the street in front of him
    CHECKV(st.reach[11 * MAP_W + 5], 1);                  // into the canal: 1
    CHECKV(st.reach[11 * MAP_W + 6], 3);                  // out of it: 2
    CHECKV(st.reach[11 * MAP_W + 2], 2);                  // through an ally (the squire at 3, 11)
    CHECKV(st.reach[11 * MAP_W + 3], 0xFF);               // but not onto it
    for (i = 0; i < MAP_W * MAP_H; i++)                   // never more than Move
        CHECK(st.reach[i] == 0xFF || st.reach[i] <= r->move);
    n = make_path(U_RAMZA, 4, 8);
    CHECKV(n, 4);                                         // 3 steps up the street
    CHECKV(st.path[0][0], 4); CHECKV(st.path[0][1], 11);
    CHECKV(st.path[n - 1][0], 4); CHECKV(st.path[n - 1][1], 8);
    for (i = 1; i < n; i++) {                             // steps (or a jump over one tile), Jump
        s16 dx = st.path[i][0] - st.path[i - 1][0], dz = st.path[i][1] - st.path[i - 1][1];
        s16 dh = tile_at(st.path[i][0], st.path[i][1])->stand - tile_at(st.path[i - 1][0], st.path[i - 1][1])->stand;
        CHECK(dx * dx + dz * dz == 1 || dx * dx + dz * dz == 4);
        CHECK(dh <= 2 * r->jump + 2 && dh >= -2 * r->jump - 2);
    }
    compute_reach(U_HIDE);                                // an enemy: the player's units block it
    CHECKV(st.reach[11 * MAP_W + 3], 0xFF);
    // the original's move ranges (Ramza at his first turn, the recruit after him)
    for (k = 0; k < 2; k++) {
        ai_load(0);
        compute_reach(oracle_reach[k].unit);
        for (i = 0; i < MAP_W * MAP_H; i++)
            if ((st.reach[i] != 0xFF && st.reach[i]) != (oracle_reach[k].row[i / MAP_W] >> (i % MAP_W) & 1)) {
                printf("  reach u%d (%d, %d): ours %d\n", oracle_reach[k].unit, i % MAP_W, i / MAP_W, st.reach[i]);
                diff++;
            }
    }
    printf("reach: Ramza's and a recruit's move ranges as the original's but %d tiles\n", diff);
    CHECK(diff <= 4);
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
    for (y = 8; y < RT_H; y++)                            // exclude the height and bottom bands
        for (x = 0; x < RT_W; x++)
            if ((y < 71 && (y >= 17 || x < 128)) && with[y][x] != sw_level(x, y)) {
                n++; mid += with[y][x] == 1 || with[y][x] == 2;
            }
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

static int verbose_hides;

// the AI's camera: the rows of a unit's sprite hidden in each view, from the map's heights,
// against the view's depth: more than half hidden or not alike (unit 2 on every free tile)
static void test_hides(void)
{
    s16 x, z;
    u8 r;
    int n = 0, bad = 0;
    sw_init(0);
    for (r = 0; r < 4; r++) {
        st.rot = r; sw_step(0);
        for (z = 0; z < MAP_H; z++)
            for (x = 0; x < MAP_W; x++) {
                int e, h;
                if (!tile_at(x, z)->walk || unit_at(x, z)) continue;
                st.unit[2].x = (u8)x; st.unit[2].z = (u8)z;
                e = view_hides(r, 2); h = view_hides_now(2);
                n++;
                if ((e > 44) != (h > 44)) {
                    bad++;
                    if (verbose_hides) printf("(%d, %d) view %d: depth %d, heights %d\n", x, z, r, h, e);
                }
            }
    }
    printf("hides: %d of %d tiles and views alike on half hidden\n", n - bad, n);
    CHECK(bad * 100 <= n);
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
    steps(0, 80);                                         // all cached HUD parts settled
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
    sw_init(10);                                          // Ramza's turn: nothing moves alone
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

// ---------------------------------------------------------------- milestone 24: the reach tiles
static int scene_whites(void)
{
    int i, n = 0;
    for (i = 0; i < SC_BYTES * SC_H; i++) n += __builtin_popcount((u8)~(scene_l[i] | scene_d[i]));
    return n;
}

// a Move's reach all drawn the frame it opens (view E: tiles behind the houses); [MODE] draws
// them over the scenery in front (more white), again as in FFT (the same scene)
static void test_reach_tiles(void)
{
    u16 s1;
    int w0, w1, w2;
    sw_init(10);
    press(K_F5); steps(0, TURN_FRAMES + 2);
    w0 = scene_whites();
    press(K_A);
    CHECKV(st.mode, M_TARGET);
    s1 = scene_checksum(); w1 = scene_whites();
    steps(0, 10);
    CHECKV(scene_checksum(), s1);
    CHECK(w1 > w0 + 2000);
    press(K_MODE);
    CHECKV(st.over, 1);
    steps(0, 10);
    w2 = scene_whites();
    printf("reach tiles: %d white pixels in the scene, %d with the reach, %d over the scenery\n", w0, w1, w2);
    CHECK(w2 > w1 + 500);
    press(K_MODE);
    CHECKV(st.over, 0);
    steps(0, 10);
    CHECKV(scene_checksum(), s1);
}

static void test_walk_keys(void)
{
    int k;
    sw_init(10);                                          // Ramza's turn, cursor on him (4, 11)
    CHECKV(st.act, U_RAMZA);
    CHECKV(st.mode, M_MENU);
    press(K_A);                                           // Move
    CHECKV(st.mode, M_TARGET);
    press(K_UP); press(K_UP);                             // view v = world z at r 0
    press(K_RIGHT); press(K_LEFT);                        // view u = world x
    CHECK(st.cx == 4 && st.cz == 9);
    press(K_A);
    CHECKV(st.mode, M_WALK);
    for (k = 0; k < 40 && st.mode == M_WALK; k++) sw_step(0);
    CHECKV(st.mode, M_MENU);                              // back to the menu, on Wait
    CHECKV(st.menu, MN_WAIT);
    CHECK(st.unit[U_RAMZA].x == 4 && st.unit[U_RAMZA].z == 9);
    CHECKV(st.unit[U_RAMZA].face, 3);                     // facing its last step: -z
    press(K_A);                                           // Wait: the facing
    CHECKV(st.mode, M_FACE);
    press(K_RIGHT);                                       // view +u = world +x at r 0
    CHECKV(st.unit[U_RAMZA].face, 0);
    press(K_A);
    CHECKV(st.unit[U_RAMZA].ct, 22);                      // moved, no Act: 2 + 20
    CHECKV(st.unit[U_RAMZA].face, 0);
    CHECKV(st.act, U_RAMZA + 1);                          // the next recruit's turn, cursor on it
    CHECK(st.cx == 3 && st.cz == 11);
    CHECKV(st.mode, M_MENU);
    press(K_B);                                           // looking around
    CHECKV(st.mode, M_BROWSE);
    // after a turn the arrows follow the view: at r 1, view +v is world +x
    press(K_F5); steps(0, TURN_FRAMES);
    press(K_DOWN);
    CHECK(st.cx == 4 && st.cz == 11);
    press(K_A);                                           // no unit there: back to the menu
    CHECKV(st.mode, M_MENU);
    CHECK(st.cx == 3 && st.cz == 11);
    press(K_ESC);
    CHECKV(st.mode, M_BROWSE);
    press(K_ESC);                                         // ESC while looking around: "Quit?"
    CHECK(st.mode == M_QUIT && st.quit == 1);             // No by default
    press(K_ENTER);
    CHECKV(st.mode, M_BROWSE);
    press(K_ESC); press(K_ESC);                           // ESC again: No
    CHECKV(st.mode, M_BROWSE);
    press(K_ESC); press(K_UP);
    CHECKV(st.quit, 0);
    CHECKV(sw_step(K_ENTER), 0);                          // Yes quits
}

// ---------------------------------------------------------------- milestone 22: small fixes
// [APPS] opens the turn order over any input mode, any key closes it; [2nd] on a tile drawn on
// the same pixels as a higher one picks that one (Ramza in the canal at (1, 5), a Squire on the
// street at (0, 5): in view E its tile hides the canal tile (1, 4), in reach but empty)
static int hud_shown(void)                    // the active unit's panel edge, the height's text
{
    int x, y, ink = 0;
    for (y = 1; y < 9; y++) for (x = 128; x < 155; x++) ink += sw_level(x, y) == 3;
    return sw_level(35, 74) == C_DGRAY && ink > 4;
}

static void test_fixes(void)
{
    u8 c, t, i, n = 0;
    u16 hp;
    sw_init(10);
    steps(0, 20);
    CHECK(hud_shown());
    press(K_APPS);
    CHECK(st.mode == M_ORDER && st.back == M_MENU);
    steps(0, 12);
    press(K_LEFT);
    CHECKV(st.mode, M_MENU);
    CHECKV(st.menu, MN_MOVE);                             // the key only closed it
    press(K_A);
    CHECKV(st.mode, M_TARGET);
    press(K_APPS); press(K_B);
    CHECKV(st.mode, M_TARGET);
    press(K_B);
    st.unit[U_RAMZA].x = 1; st.unit[U_RAMZA].z = 5; st.unit[1].x = 0; st.unit[1].z = 5;
    st.cx = 1; st.cz = 5; st.rot = 3;
    steps(0, 30);
    press(K_DOWN); press(K_A); press(K_A);                // Act, Attack
    CHECKV(st.mode, M_AIM);
    CHECK(st.reach[4 * MAP_W + 1] == 1 && st.reach[5 * MAP_W] == 1);
    st.cx = 1; st.cz = 4; hp = st.unit[1].hp;
    press(K_A);
    CHECKV(st.mode, M_ACT);
    CHECK(st.tgt == 1 && st.cx == 0 && st.cz == 5);
    steps(0, 60);
    CHECK(st.unit[1].hp < hp || !st.hit);
    // Move: every unreachable tile whose twin is reachable walks to the twin
    sw_init(10);
    press(K_A);
    CHECKV(st.mode, M_TARGET);
    for (c = 0; c < MAP_W * MAP_H; c++) {
        if (st.reach[c] != 0xFF && st.reach[c]) continue;
        for (t = 0; t < MAP_W * MAP_H; t++) {
            s16 u, v, x0, y0, x1, y1;
            if (t == c || st.reach[t] == 0xFF || !st.reach[t]) continue;
            view_of(st.rot, c % MAP_W, c / MAP_W, &u, &v); x0 = scene_x(st.rot, u, v); y0 = scene_y(u, v) - map_tiles[c].stand * (HU / 2);
            view_of(st.rot, t % MAP_W, t / MAP_W, &u, &v); x1 = scene_x(st.rot, u, v); y1 = scene_y(u, v) - map_tiles[t].stand * (HU / 2);
            if (x0 == x1 && y0 == y1) break;
        }
        if (t == MAP_W * MAP_H) continue;
        st.cx = c % MAP_W; st.cz = c / MAP_W;
        press(K_A);
        CHECKV(st.mode, M_WALK);
        CHECK(st.path[st.path_n - 1][0] == t % MAP_W && st.path[st.path_n - 1][1] == t / MAP_W);
        for (i = 0; i < 60 && st.mode == M_WALK; i++) sw_step(0);
        press(K_B); press(K_A);                           // undone, Move again
        n++;
    }
    CHECK(n > 0);
    printf("fixes: [APPS] order, \"Quit?\", twin picks: an attack, %d moves\n", n);
}

// ---------------------------------------------------------------- milestone 16: the turn menu
// a whole player turn by keys: Act not yet there, Status, a move undone, the facing chosen
// then cancelled, Wait; a unit's numbers while looking around
static void test_menu(void)
{
    int k;
    sw_init(10);
    press(K_DOWN);
    CHECKV(st.menu, MN_ACT);
    press(K_A);                                           // Act: its list (test_skills)
    CHECKV(st.mode, M_ACTS);
    press(K_B);
    CHECKV(st.mode, M_MENU);
    press(K_DOWN); press(K_DOWN);
    CHECKV(st.menu, MN_STATUS);
    press(K_A);
    CHECKV(st.mode, M_STATUS); CHECKV(st.sel, U_RAMZA);
    steps(0, 4);
    press(K_B);
    CHECKV(st.mode, M_MENU);
    press(K_DOWN);                                        // the entries wrap
    CHECKV(st.menu, MN_MOVE);
    press(K_A); press(K_UP); press(K_UP); press(K_A);     // Move to (4, 9)
    for (k = 0; k < 40 && st.mode == M_WALK; k++) sw_step(0);
    CHECK(st.unit[U_RAMZA].z == 9 && st.moved);
    press(K_UP); press(K_UP);                             // Move is greyed: nothing
    CHECKV(st.menu, MN_MOVE);
    press(K_A);
    CHECKV(st.mode, M_MENU);
    press(K_B);                                           // the move undone
    CHECKV(st.mode, M_MENU);
    CHECK(st.unit[U_RAMZA].x == 4 && st.unit[U_RAMZA].z == 11 && !st.moved);
    CHECKV(st.unit[U_RAMZA].face, unit_defs[U_RAMZA].face);
    CHECK(st.cx == 4 && st.cz == 11);
    press(K_A);                                           // Move again
    CHECKV(st.mode, M_TARGET);
    press(K_B);
    CHECKV(st.mode, M_MENU);
    press(K_DOWN); press(K_DOWN); press(K_A);             // Wait
    CHECKV(st.mode, M_FACE);
    press(K_LEFT);                                        // view -u = world -x
    CHECKV(st.unit[U_RAMZA].face, 1);
    press(K_B);                                           // cancelled: the facing back
    CHECKV(st.mode, M_MENU);
    CHECKV(st.unit[U_RAMZA].face, unit_defs[U_RAMZA].face);
    press(K_A); press(K_DOWN); press(K_A);                // Wait, facing +z
    CHECKV(st.unit[U_RAMZA].face, 2);
    CHECKV(st.unit[U_RAMZA].ct, 42);                      // no move, no Act: 2 + 40
    CHECKV(st.act, U_RAMZA + 1);
    press(K_B);                                           // an enemy's numbers
    st.cx = st.unit[1].x; st.cz = st.unit[1].z;
    press(K_A);
    CHECKV(st.mode, M_STATUS); CHECKV(st.sel, 1);
    press(K_B);
    CHECKV(st.mode, M_BROWSE);
}

// ---------------------------------------------------------------- milestone 17: attack
// The original's previews (oracle.h: 40 cases poked into Ramza and the recruit south of him)
// equal ours; its 400 attacks executed (misses, criticals seen, damage) against 4000 of ours;
// the reach's height limits, the sides, an attack played by keys, a knockback, a KO.
static void test_attack(void)
{
    u16 i, n, miss = 0, crit = 0, push = 0;
    u32 sum = 0;
    double om, oc, od, sd;
    u8 x, z, d;
    Unit *ra, *t;
    sw_init(0);
    ra = &st.unit[U_RAMZA]; t = &st.unit[10];             // (4, 11), (4, 12)
    for (i = 0; i < ORACLE_ATTACKS; i++) {
        const u8 *c = oracle_attacks[i];
        ra->pa = c[0]; ra->sp = c[1]; ra->brave = c[2]; ra->eq[EQ_WEAPON] = c[3];
        ra->zodiac = c[4]; ra->female = c[5]; t->zodiac = c[6]; t->female = c[7]; t->face = c[8];
        if (hit_chance(U_RAMZA, 10) != c[9] || attack_damage(U_RAMZA, 10, 0, 0) != c[10]) {
            printf("attack case %d: ", i);
            CHECKV(hit_chance(U_RAMZA, 10), c[9]); CHECKV(attack_damage(U_RAMZA, 10, 0, 0), c[10]);
        }
    }
    sw_init(0);                                           // the first case as the battle has it:
    ra = &st.unit[U_RAMZA]; t = &st.unit[10];             // the oracle's signs (RAM +0x09:
    ra->zodiac = 9; t->zodiac = 3; ra->female = t->female = 0;   // Capricorn, Cancer), both male
    CHECKV(hit_chance(U_RAMZA, 10), 95);
    CHECKV(attack_damage(U_RAMZA, 10, 0, 0), oracle_trials[0]);
    for (n = 0; n < 4000; n++) {
        attack(U_RAMZA, 10);
        miss += !st.hit; crit += st.dmg > oracle_trials[0]; push += st.push; sum += st.dmg;
    }
    om = oracle_trials[2] / (double)oracle_trials[1];      // the original's rates, and ours
    oc = oracle_trials[3] / (double)oracle_trials[1];
    od = oracle_trials[4] / (double)oracle_trials[1];
    printf("attack: %d previews as the original's; misses %.3f (original %.3f), criticals seen %.3f (%.3f), "
           "damage %.2f (%.2f), pushes %d (%d)\n", ORACLE_ATTACKS, miss / 4000.0, om, crit / 4000.0, oc,
           sum / 4000.0, od, push, oracle_trials[5]);
    sd = 3 * 0.011 + 0.01;                               // 3 sigma of 400 trials at 5 %, ours
    CHECK(miss / 4000.0 > om - sd && miss / 4000.0 < om + sd);
    CHECK(crit / 4000.0 > oc - sd && crit / 4000.0 < oc + sd);
    CHECK(sum / 4000.0 > od - 0.6 && sum / 4000.0 < od + 0.6);
    // the sides: the target faces +z; from +z the front, -z the back, +-x a side
    t->face = 2;
    ra->x = 4; ra->z = 13; CHECKV(attack_side(U_RAMZA, 10), SIDE_FRONT); CHECKV(hit_chance(U_RAMZA, 10), 95);
    ra->z = 11; CHECKV(attack_side(U_RAMZA, 10), SIDE_BACK); CHECKV(hit_chance(U_RAMZA, 10), 100);
    ra->x = 5; ra->z = 12; CHECKV(attack_side(U_RAMZA, 10), SIDE_SIDE);
    ra->x = 3; CHECKV(attack_side(U_RAMZA, 10), SIDE_SIDE);
    // the reach: the four neighbours whose height is within [h - 6, h + 5] half units
    for (n = 0, z = 0; z < MAP_H; z++)
        for (x = 0; x < MAP_W; x++) {
            if (!tile_at(x, z)->walk) continue;
            ra->x = x; ra->z = z;
            attack_range(U_RAMZA);
            for (d = 0; d < 4; d++) {
                s16 nx = x + (d == 0) - (d == 1), nz = z + (d == 2) - (d == 3);
                const Tile *a = tile_at(nx, nz);
                s16 dh = a ? (s16)a->stand - tile_at(x, z)->stand : 0;
                if (!a) continue;
                CHECKV(st.reach[nz * MAP_W + nx], a->walk && dh >= -6 && dh <= 5 ? 1 : 0xFF);
                n += a->walk && (dh == -6 || dh == 5 || dh == -7 || dh == 6);
            }
        }
    printf("attack: reach on every tile, %d neighbours at a height limit\n", n);
    // by keys: Act, the enemy in front, the attack played, the menu back with Act and the
    // move's undo gone
    sw_init(10);
    st.unit[1].x = 4; st.unit[1].z = 10; st.unit[1].face = 2;
    n = st.unit[1].hp;
    press(K_DOWN); press(K_A); press(K_A);                // Act, Attack
    CHECKV(st.mode, M_AIM);
    CHECKV(st.reach[10 * MAP_W + 4], 1);
    press(K_A);                                           // the cursor on Ramza: nothing
    CHECKV(st.mode, M_AIM);
    press(K_B);
    CHECKV(st.mode, M_ACTS);
    press(K_A);
    st.cx = 4; st.cz = 10;
    press(K_A);
    CHECKV(st.mode, M_ACT);
    CHECK(st.acted && st.unit[U_RAMZA].face == 3);
    for (i = 0; i < ACT_GAIN_END && st.mode == M_ACT; i++) sw_step(0);
    CHECKV(st.mode, M_MENU);
    CHECKV(st.menu, MN_MOVE);
    CHECKV(st.unit[1].hp, st.hit ? (st.dmg >= n ? 0 : n - st.dmg) : n);
    press(K_DOWN); press(K_A);                            // Act is greyed
    CHECKV(st.mode, M_MENU);
    // knockback: none onto a higher tile (behind the enemy of scenario 8: (4, 9), h 8 over 4;
    // the original's 400 attacks pushed none either: (4, 13) is higher too); on the flat, from
    // (7, 5) onto (7, 7); from a roof (3, 2) h 16 down to (2, 2) h 4: 6 levels, 3 over Jump
    sw_init(8);
    for (i = 0; i < 2000; i++) { attack(U_RAMZA, 1); CHECK(!st.push); }
    sw_init(10);
    st.unit[U_RAMZA].x = 7; st.unit[U_RAMZA].z = 5; st.unit[1].x = 7; st.unit[1].z = 6;
    for (i = 0; i < 2000 && !st.push; i++) attack(U_RAMZA, 1);
    CHECK(st.crit && st.push && st.kx == 7 && st.kz == 7);
    n = st.dmg;
    while (st.act_t <= ACT_HIT) sw_step(0);
    CHECK(st.unit[1].x == 7 && st.unit[1].z == 7 && st.unit[1].hp == st.unit[1].max_hp - n);
    sw_init(10);
    st.unit[U_RAMZA].x = 4; st.unit[U_RAMZA].z = 2; st.unit[1].x = 3; st.unit[1].z = 2;
    for (i = 0; i < 2000 && !st.push; i++) attack(U_RAMZA, 1);
    CHECK(st.push && st.kx == 2 && st.kz == 2);
    CHECK(st.dmg > 3 * st.unit[1].max_hp / 10);
    // a KO: no knockback, out of the turn order
    sw_init(8);
    st.unit[1].hp = 1;
    for (i = 0; i < 100 && !st.hit; i++) attack(U_RAMZA, 1);
    CHECK(st.hit && !st.push);
    while (st.mode == M_ACT) sw_step(0);
    CHECKV(st.unit[1].hp, 0);
    for (i = 1; i < ORDER_N; i++) CHECK(st.turns[i] != 1);
}

// ---------------------------------------------------------------- milestone 18: abilities
// What a unit knows: the original's units are outcomes of FFT's draw, and our draws learn each
// ability as often as the rule's exact odds (JP 100..199, each ability of the set in order
// bought if its learn rate roll passes and the JP left pay for it)
static void learn_odds(u8 set, double *p)
{
    const u8 *a = skill_sets[set].abil;
    u8 n = 0, k;
    u16 jp, m;
    while (n < SET_N && a[n] != 0xFF) n++;
    for (k = 0; k < n; k++) p[k] = 0;
    for (jp = 100; jp < 200; jp++)
        for (m = 0; m < 1u << n; m++) {             // m: the rolls that pass
            double w = 0.01;
            u16 left = jp;
            for (k = 0; k < n; k++) w *= m >> k & 1 ? abilities[a[k]].rate / 100.0 : 1 - abilities[a[k]].rate / 100.0;
            for (k = 0; k < n; k++)
                if (m >> k & 1 && left >= abilities[a[k]].jp) { left -= abilities[a[k]].jp; p[k] += w; }
        }
}

static u8 abil_of(const char *name)
{
    u8 i;
    for (i = 0; i < NABIL && strcmp(abilities[i].name, name); i++) ;
    return i;
}

static void test_skills(void)
{
    u16 i, n, k, h[2][5] = { { 0 } }, push[2] = { 0 };
    u32 cnt[NSET][SET_N] = { { 0 } }, units[NSET] = { 0 };
    double p[SET_N], worst = 0;
    Unit *ra, *t;
    u8 dash = abil_of("Dash"), stone = abil_of("Throw Stone"), potion = abil_of("Potion"),
       phoenix = abil_of("Phoenix Down"), wish = abil_of("Wish"), heal = abil_of("Heal");
    sw_init(0);
    for (i = 0; i < NUNIT; i++) {                         // the original's: possible outcomes
        const u8 *a = skill_sets[unit_defs[i].set].abil;
        u16 jp = 0;
        for (k = 0; k < SET_N; k++) if (oracle_learned[i] >> k & 1) { CHECK(a[k] != 0xFF); jp += abilities[a[k]].jp; }
        if (unit_defs[i].learned == 0xFF) CHECK(jp <= 199);
        else CHECKV(oracle_learned[i], unit_defs[i].learned);
    }
    for (n = 100; n < 3100; n++) {                        // ours: 3000 battles
        game_scenario(n);
        for (i = 0; i < NUNIT; i++) {
            if (unit_defs[i].learned != 0xFF) { CHECKV(st.unit[i].learned, unit_defs[i].learned); continue; }
            units[st.unit[i].set]++;
            for (k = 0; k < SET_N; k++) cnt[st.unit[i].set][k] += st.unit[i].learned >> k & 1;
        }
    }
    for (i = 0; i < NSET; i++) {
        if (!units[i]) continue;
        learn_odds((u8)i, p);
        for (k = 0; k < SET_N && skill_sets[i].abil[k] != 0xFF; k++) {
            double d = cnt[i][k] / (double)units[i] - p[k];
            if (d < 0) d = -d;
            if (d > worst) worst = d;
            CHECK(d < 0.03);
        }
    }
    printf("skills: the original's %d units possible draws; ours learn as the rule's odds (worst %.3f)\n", NUNIT, worst);

    // the original's cases: Ramza on the recruit south of him, the attack's game case
    for (i = 0; i < ORACLE_SKILLS; i++) {
        u8 ab = oracle_skills[i].abil;
        sw_init(0);
        ra = &st.unit[U_RAMZA]; t = &st.unit[10];
        ra->zodiac = 9; t->zodiac = 3; ra->female = t->female = 0;
        t->hp = oracle_skills[i].hp; t->status = oracle_skills[i].status;
        if (ab == 0xFF) {                                 // Attack on a Defending target
            CHECKV(hit_chance(U_RAMZA, 10), oracle_skills[i].hit);
            continue;
        }
        if (skill_hit(U_RAMZA, 10, ab) != oracle_skills[i].hit) {
            printf("skill case %d: ", i); CHECKV(skill_hit(U_RAMZA, 10, ab), oracle_skills[i].hit);
        }
        if (oracle_skills[i].hit && abilities[ab].kind != AB_CURE) {   // FFT's preview: one roll
            u16 lo = skill_amount(U_RAMZA, 10, ab, 0), hi = skill_amount(U_RAMZA, 10, ab, 32767);
            CHECK(oracle_skills[i].amount >= lo && oracle_skills[i].amount <= hi);
            if (abilities[ab].kind == AB_DAMAGE) CHECKV(oracle_skills[i].amount % lo, 0);
        }
        do skill(U_RAMZA, 10, ab); while (oracle_skills[i].hit && oracle_skills[i].hit < 100 && !st.hit);
        while (st.mode == M_ACT) sw_step(0);
        if (oracle_skills[i].hit && (abilities[ab].kind == AB_DAMAGE || abilities[ab].kind == AB_REVIVE)) {
            u16 lo = skill_amount(U_RAMZA, 10, ab, 0), d = abilities[ab].kind == AB_DAMAGE   // a roll
                ? oracle_skills[i].hp - oracle_skills[i].hp2 : oracle_skills[i].hp2;
            if (d < lo || d > skill_amount(U_RAMZA, 10, ab, 32767)) {
                printf("skill case %d: ", i); CHECK(d >= lo && d <= skill_amount(U_RAMZA, 10, ab, 32767));
            }
        } else CHECKV(t->hp, oracle_skills[i].hp2);
        CHECKV(t->status, oracle_skills[i].status2);
        CHECKV(ra->hp, oracle_skills[i].ramza);
        for (k = 0; k < NBAG; k++) CHECKV(st.bag[k], oracle_skills[i].bag[k]);
    }
    // Dash and Throw Stone executed 4000 times: how often each multiple of XA, pushes
    sw_init(0);
    ra = &st.unit[U_RAMZA]; t = &st.unit[10];
    ra->zodiac = 9; t->zodiac = 3; ra->female = t->female = 0;
    for (k = 0; k < 2; k++)
        for (n = 0; n < 4000; n++) {
            skill(U_RAMZA, 10, k ? stone : dash);
            h[k][st.hit ? st.dmg / 3 : 0]++; push[k] += st.push;
        }
    for (k = 0; k < 2; k++) {
        const u16 *o = oracle_skill_trials[k];
        printf("skills: %s x0..4 ", k ? "Throw Stone" : "Dash");
        for (i = 0; i < 5; i++) {
            double ours = h[k][i] / 4000.0, theirs = o[2 + i] / (double)o[0];
            printf("%.3f (%.3f) ", ours, theirs);
            CHECK(ours > theirs - 0.09 && ours < theirs + 0.09);   // 3 sigma of 300 at 50 %
        }
        printf("pushes %d (%d)\n", push[k], o[1]);
        CHECK(!push[k] == !o[1]);
    }
    // a push by Brave: on the flat, from (7, 5) onto (7, 7), Dash
    sw_init(10);
    st.unit[U_RAMZA].x = 7; st.unit[U_RAMZA].z = 5; st.unit[1].x = 7; st.unit[1].z = 6;
    for (n = 0, i = 0; i < 1000; i++) { skill(U_RAMZA, 1, dash); n += st.push; }
    printf("skills: Dash pushed %d of 1000 (Brave %d against %d)\n", n, st.unit[U_RAMZA].brave, st.unit[1].brave);
    CHECK(n > 100 && n < 900);
    // the reach: Throw Stone 4 steps, not the user; Potion 4 with the user; Dash 1 step within
    // 1 level (2 half units); Heal 1 step and the user, within 2 levels
    sw_init(0);
    ra = &st.unit[U_RAMZA];
    for (k = 0; k < 4; k++) {
        static const char *const nm[4] = { "Throw Stone", "Potion", "Dash", "Heal" };
        u8 ab = abil_of(nm[k]);
        s16 x, z, hr = tile_at(ra->x, ra->z)->stand;
        skill_range(U_RAMZA, ab);
        for (z = 0; z < MAP_H; z++)
            for (x = 0; x < MAP_W; x++) {
                const Tile *tl = tile_at(x, z);
                s16 d = (x > ra->x ? x - ra->x : ra->x - x) + (z > ra->z ? z - ra->z : ra->z - z);
                s16 dh = tl->stand > hr ? tl->stand - hr : hr - tl->stand;
                u8 want = tl->walk && d <= abilities[ab].range && (d || abilities[ab].flags & AF_SELF)
                    && (abilities[ab].vert == 0xFF || dh <= abilities[ab].vert);
                CHECKV(st.reach[z * MAP_W + x], want ? 1 : 0xFF);
            }
    }
    CHECK(abilities[stone].range == 4 && !(abilities[stone].flags & AF_SELF) && abilities[stone].vert == 0xFF);
    CHECK(abilities[potion].range == 4 && abilities[potion].flags & AF_SELF);
    CHECK(abilities[dash].range == 1 && abilities[dash].vert == 2 && abilities[heal].vert == 4);
    // by keys: Ramza's Act, Guts, Wish on the recruit (10 HP): +20, Ramza -10
    sw_init(10);
    st.unit[10].hp = 10;
    press(K_DOWN); press(K_A); CHECKV(st.mode, M_ACTS);
    press(K_DOWN); press(K_A); CHECKV(st.mode, M_SKILLS);
    CHECKV(known(U_RAMZA, 0), wish); CHECKV(known(U_RAMZA, 1), 0xFF);
    press(K_A); CHECKV(st.mode, M_AIM); CHECKV(st.abil, wish);
    CHECKV(st.reach[11 * MAP_W + 4], 0xFF);               // not himself
    press(K_B); CHECKV(st.mode, M_SKILLS);
    press(K_B); CHECKV(st.mode, M_ACTS); CHECKV(st.sub, 1);
    press(K_A); press(K_A);
    st.cx = 4; st.cz = 12;
    press(K_A); CHECKV(st.mode, M_ACT);
    while (st.mode == M_ACT) sw_step(0);
    CHECKV(st.unit[10].hp, 30); CHECKV(st.unit[U_RAMZA].hp, st.unit[U_RAMZA].max_hp - st.unit[U_RAMZA].max_hp / 5);
    CHECK(st.mode == M_MENU && st.acted);
    // the Chemist's items: Potion on Ramza (the bag 5 -> 4), an empty Phoenix Down greyed, then
    // one revives a KO unit
    sw_init(11);
    CHECKV(st.mode, M_SKILLS);
    CHECKV(known(U_CHEMIST, 0), potion);
    press(K_A); CHECKV(st.mode, M_AIM);
    st.cx = 4; st.cz = 11;
    press(K_A); CHECKV(st.mode, M_ACT);
    while (st.mode == M_ACT) sw_step(0);
    CHECKV(st.unit[U_RAMZA].hp, 50); CHECKV(st.bag[abilities[potion].bag], 4);
    st.bag[abilities[potion].bag] = 0;
    CHECK(!can_use(U_CHEMIST, potion)); CHECK(can_use(1, potion));   // the enemies' never run out
    sw_init(10);
    st.unit[1].x = 4; st.unit[1].z = 10; st.unit[1].hp = 0;
    CHECKV(skill_hit(U_RAMZA, 1, phoenix), 100); CHECKV(skill_hit(U_RAMZA, 10, phoenix), 0);
    skill(U_RAMZA, 1, phoenix);
    while (st.mode == M_ACT) sw_step(0);
    CHECK(st.unit[1].hp >= 1 && st.unit[1].hp <= 20 && st.bag[abilities[phoenix].bag] == 1);
    // Wish when it leaves Ramza at 0 HP: the turn ends; a Defending unit stops at its turn
    sw_init(10);
    st.unit[U_RAMZA].hp = 5; st.unit[1].status = ST_DEFEND;
    skill(U_RAMZA, 10, wish);
    while (st.act == U_RAMZA) sw_step(0);
    CHECKV(st.unit[U_RAMZA].hp, 0);
    for (i = 0; i < 2000 && st.act != 1; i++)            // the player's units wait
        if (st.unit[st.act].team == TEAM_PLAYER && st.mode == M_MENU) wait_turn(); else sw_step(0);
    CHECKV(st.act, 1); CHECKV(st.unit[1].status, 0);
}

// ---------------------------------------------------------------- milestone 19: KO and the end
// The original's rewards (oracle.h: Ramza acting on the recruit south of him), the death counter
// and what a KO unit becomes, picking a crystal or a chest up, a battle won and one lost by keys.
static void test_end(void)
{
    u16 i, k, n, jp[NUNIT], cr[2] = { 0 };
    Unit *ra, *u;
    for (i = 0; i < ORACLE_REWARDS; i++) {
        sw_init(0);
        ra = &st.unit[U_RAMZA];
        memcpy(ra->raw, oracle[U_RAMZA].raw, sizeof ra->raw);
        unit_stats(ra);
        ra->exp = oracle_rewards[i].exp; ra->tjp = oracle_rewards[i].tjp;
        for (k = 0; k < NUNIT; k++) jp[k] = st.unit[k].jp;
        if (oracle_rewards[i].kill) { st.unit[10].hp = 0; st.unit[10].kos = 1; }
        if (oracle_rewards[i].hit) rewards(U_RAMZA, 10, oracle_rewards[i].kill);
        CHECKV(ra->exp, oracle_rewards[i].exp2); CHECKV(ra->level, oracle_rewards[i].level2);
        CHECKV(ra->max_hp, oracle_rewards[i].max_hp2); CHECKV(ra->hp, oracle_rewards[i].hp2);
        for (k = 0; k < NUNIT; k++)                       // FFT gives the others' share to their
            if (jobs[st.unit[k].job].row == jobs[ra->job].row)   // Squire JP: kept for the
                CHECKV(st.unit[k].jp - jp[k], oracle_rewards[i].jp[k]);   // Squires only
    }
    printf("end: %d actions' EXP, JP and level up as the original's\n", ORACLE_REWARDS);
    // a second KO of the same unit: 20 / 2 = 10; a lower target: at least 1; a job level up at
    // 200 JP (8 + 2 x 2)
    sw_init(0);
    ra = &st.unit[U_RAMZA];
    st.unit[10].kos = 2; rewards(U_RAMZA, 10, 1); CHECKV(st.gain, 10);
    ra->level = 20; rewards(U_RAMZA, 10, 0); CHECKV(st.gain, 1);
    ra->level = 1; ra->tjp = 200; n = ra->jp; rewards(U_RAMZA, 10, 0); CHECKV(ra->jp - n, 12);
    // the death counter: 3, 2, 1, 0 at the KO unit's picks, then a crystal or a chest; the guest
    // never turns
    sw_init(0);
    st.unit[1].hp = 0; st.unit[0].hp = 0;
    for (n = 3, i = 0; i < 400 && !DC_OBJ(st.unit[1].dc); i++) {
        u8 a = ct_next();
        CHECK(st.unit[1].dc == n || st.unit[1].dc == n - 1 || DC_OBJ(st.unit[1].dc));
        if (st.unit[1].dc <= 3) n = st.unit[1].dc;
        ct_end(a, 1, 1);
    }
    CHECK(DC_OBJ(st.unit[1].dc) && n == 0); CHECKV(st.unit[0].dc, 3);
    for (i = 0; i < 4000; i++) {                          // an enemy 1 in 2, a player's 3 in 4
        for (k = 0; k < 2; k++) {
            u = &st.unit[k ? 7 : 2];
            u->hp = 0; u->dc = 0; u->ct = 200;
            ct_next();
            cr[k] += u->dc == DC_CRYSTAL;
        }
    }
    printf("end: crystals %.3f of an enemy's, %.3f of a player's (FFT 0.5, 0.75)\n", cr[0] / 4000.0, cr[1] / 4000.0);
    CHECK(cr[0] > 1850 && cr[0] < 2150 && cr[1] > 2850 && cr[1] < 3150);
    // picked up by walking onto it: a crystal (HP, MP full), a chest (the dead unit's equipment);
    // no undo after, no target on it
    for (k = 0; k < 2; k++) {
        sw_init(10);
        ra = &st.unit[U_RAMZA]; u = &st.unit[1];
        u->x = 4; u->z = 9; u->hp = 0; u->dc = k ? DC_CHEST : DC_CRYSTAL;
        ra->hp = 10;
        CHECKV(unit_at(4, 9), 0);
        press(K_A); CHECKV(st.mode, M_TARGET);
        CHECK(st.reach[9 * MAP_W + 4] != 0xFF);
        st.cx = 4; st.cz = 9; press(K_A);
        for (i = 0; i < 100 && st.mode == M_WALK; i++) sw_step(0);
        CHECKV(st.mode, M_PICK); CHECKV(u->dc, DC_GONE);
        if (k) CHECK(st.pick == u->eq[EQ_HEAD] || st.pick == u->eq[EQ_BODY] || st.pick == u->eq[EQ_ACC]
                     || st.pick == u->eq[EQ_WEAPON] || st.pick == u->eq[EQ_SHIELD]);
        else CHECK(st.pick == 0xFF && ra->hp == ra->max_hp);
        steps(0, PICK_FRAMES);
        CHECKV(st.mode, M_MENU);
        press(K_B);
        CHECK(ra->x == 4 && ra->z == 9 && st.mode == M_BROWSE);
    }
    // won by keys (fft(14): the last enemy at 1 HP): the results, then a new battle
    sw_init(14);
    press(K_DOWN); press(K_A); press(K_A); press(K_UP); press(K_A);
    CHECKV(st.mode, M_ACT);
    for (i = 0; i < 100 && st.mode == M_ACT; i++) sw_step(0);
    CHECKV(st.mode, M_END); CHECKV(st.end, END_VICTORY);
    CHECKV(st.unit[U_RAMZA].gexp, 20); CHECKV(st.unit[U_RAMZA].gjp, 10);
    press(K_A);
    CHECK(st.mode != M_END && !st.end && st.unit[1].hp == st.unit[1].max_hp);
    // lost by keys (fft(15): Ramza's Wish knocks him out, the recruits KO): the enemies knock
    // Delita out, or Ramza's counter runs out (a crystal or a chest): Game Over
    sw_init(15);
    press(K_DOWN); press(K_A); press(K_DOWN); press(K_A); press(K_A); press(K_UP); press(K_A);
    CHECKV(st.mode, M_ACT);
    for (i = 0; i < 4000 && st.mode != M_END; i++) sw_step(0);
    CHECKV(st.end, END_DEFEAT); CHECK(DC_OBJ(st.unit[U_RAMZA].dc) || !st.unit[0].hp);
    printf("end: won by keys; lost after %d frames (%s)\n", i,
           DC_OBJ(st.unit[U_RAMZA].dc) ? "Ramza a crystal or a chest" : "Delita KO");
    // the guest's KO alone does not end it; every unit of the player's side KO does
    sw_init(0);
    st.unit[0].hp = 0; CHECKV(outcome(), END_NONE);
    for (i = 6; i < NUNIT; i++) st.unit[i].hp = 0;
    CHECKV(outcome(), END_DEFEAT);
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
    for (i = 0; i < NUNIT; i++) { ko += !st.unit[i].hp; gone += DC_OBJ(st.unit[i].dc); }
    printf("clock: %d turns as the original's, %d KO, %d crystal or chest\n", ORACLE_TURNS, ko, gone);
    CHECK(ko >= 3 && gone >= 1);
    // the same CT, the lower index first; the cap at 60
    st.unit[0].ct = 50; ct_end(0, 0, 0); CHECKV(st.unit[0].ct, 60);
}

// ---------------------------------------------------------------- milestone 20: the AI
// The original's AI turns (oracle.h): each situation put in our engine with the original's units
// (its stats, Brave, signs, what each knows), our plan against what its AI did.
static void ai_load(u16 r)
{
    u8 i;
    sw_init(10);
    for (i = 0; i < NUNIT; i++) {
        Unit *u = &st.unit[i];
        const u8 *v = oracle_ai[r].u[i];
        u8 k;
        u->job = oracle[i].job; u->level = oracle[i].level;
        for (k = 0; k < 5; k++) { u->raw[k] = oracle[i].raw[k]; u->eq[k] = oracle[i].eq[k]; }
        unit_stats(u);
        u->brave = oracle_bz[i][0]; u->faith = oracle_bz[i][1]; u->zodiac = oracle_bz[i][2]; u->female = oracle_bz[i][3];
        u->x = v[0]; u->z = v[1]; u->face = v[2]; u->hp = v[3]; u->ct = v[4]; u->dc = v[5];
        u->status = v[6]; u->learned = v[7]; u->defend = v[8];
    }
    st.act = oracle_ai[r].unit; st.moved = st.acted = 0; st.ai_nosup = 0;
}

static const char *const ai_cases[] = { "battle", "near", "critical", "far", "potion", "kill", "defend", "delita-low", "recruit" };
static int ai_diff;                                       // --ai: each turn that differs

// our plan against what the original's AI did, turn by turn: the action (and its target), whether
// FFT's is in our ranking (its draws on the hit chance take a lower one at times), the tile it
// ends on (acting from its own tile, where it goes after), its facing; in each built situation
// (replayed with other draws) FFT's most frequent choice must be our first
static void test_ai(void)
{
    u16 r, same = 0, inlist = 0, tile = 0, face = 0, cases = 0, firsts = 0;
    u8 c, pick[ORACLE_AI][2];
    for (r = 0; r < ORACLE_AI; r++) {
        const AiAct *k;
        u8 n, i, a = oracle_ai[r].unit, fab = oracle_ai[r].ab, ft = (u8)(oracle_ai[r].tz * MAP_W + oracle_ai[r].tx);
        u8 ab, o, t, x, z, f, hit = 0, ok;
        ai_load(r);
        ai_plan(a, 0);
        k = ai_ranked(&n);
        ab = n ? k[0].ab : st.unit[a].defend ? ABIL_DEFEND : AI_NONE;
        o = n ? k[0].o : ai_fallback(); t = n ? k[0].t : o;
        pick[r][0] = ab; pick[r][1] = ab >= AI_NONE ? 0 : t;
        for (i = 0; i < n; i++) if (k[i].ab == fab && k[i].t == ft) hit = 1;
        if (fab >= AI_NONE && !n && ab == fab) hit = 1;
        ok = ab == fab && (ab >= AI_NONE || t == ft);
        same += ok; inlist += hit;
        if (n && (k[0].f || o == (u8)(st.unit[a].z * MAP_W + st.unit[a].x))) {
            for (i = 0; i < NUNIT; i++) st.unit[i].hp = oracle_ai[r].u[i][9];
            ai_plan(a, 1);
            o = ai_fallback();
        }
        x = o % MAP_W; z = o / MAP_W;
        st.unit[a].x = x; st.unit[a].z = z;
        for (i = 0; i < NUNIT; i++) st.unit[i].hp = oracle_ai[r].u[i][9];
        f = ai_facing(a);
        tile += x == oracle_ai[r].x && z == oracle_ai[r].z;
        face += f == oracle_ai[r].face;
        if (ai_diff && (!ok || x != oracle_ai[r].x || z != oracle_ai[r].z || f != oracle_ai[r].face))
            printf("  ai%3d %-10s u%-2d ours %02X t%d,%d -> %d,%d f%d | FFT %02X t%d,%d -> %d,%d f%d%s\n",
                   r, ai_cases[oracle_ai[r].kase], a, ab, t % MAP_W, t / MAP_W, x, z, f, fab, oracle_ai[r].tx,
                   oracle_ai[r].tz, oracle_ai[r].x, oracle_ai[r].z, oracle_ai[r].face, hit ? " (in our ranking)" : "");
    }
    for (c = 1; c < sizeof ai_cases / sizeof *ai_cases; c++) {   // the built situations
        u16 best = 0, j, m;
        u8 bab = 0, bt = 0, first = 0xFF;
        for (r = 0; r < ORACLE_AI; r++) {
            u8 t = oracle_ai[r].ab >= AI_NONE ? 0 : (u8)(oracle_ai[r].tz * MAP_W + oracle_ai[r].tx);
            if (oracle_ai[r].kase != c) continue;
            if (first == 0xFF) first = (u8)r;
            for (j = m = 0; j < ORACLE_AI; j++)
                m += oracle_ai[j].kase == c && oracle_ai[j].ab == oracle_ai[r].ab
                     && (oracle_ai[j].ab >= AI_NONE ? 0 : (u8)(oracle_ai[j].tz * MAP_W + oracle_ai[j].tx)) == t;
            if (m > best) { best = m; bab = oracle_ai[r].ab; bt = t; }
        }
        if (first == 0xFF) continue;
        cases++;
        firsts += pick[first][0] == bab && pick[first][1] == bt;
    }
    printf("ai: %d turns of the original's: the same action %d, in our ranking %d, the same tile %d, facing %d;"
           " FFT's most frequent choice our first in %d of %d situations\n", ORACLE_AI, same, inlist, tile, face, firsts, cases);
    CHECK(same >= 129 && inlist >= 134 && tile >= 125 && face >= 128);
    CHECKV(firsts, cases);
}

// the AI on both sides (fft(16)): a whole battle plays itself to its end; an AI turn's thinking
// waits for the picture to stand still, then takes a few frames
static void test_ai_battle(void)
{
    u32 i, think = 0, frozen = 0, turns = 0, most = 0, run = 0, fmost = 0, frun = 0;
    sw_init(16);
    for (i = 0; i < 20000 && st.mode != M_END; i++) {
        u8 a = st.ai == AI_THINK, t = st.act;
        if (a && ai_frozen()) { frozen++; if (++frun > fmost) fmost = frun; } else if (!a) frun = 0;
        sw_step(0);
        if (a) { think++; run++; if (run > most) most = run; } else run = 0;
        if (st.act != t) turns++;
    }
    printf("ai battle: %s after %lu frames, %lu turns; thinking %.1f frames a turn (%lu at most): the camera on"
           " the unit, then %.1f with the picture still (%lu at most)\n", st.end == END_VICTORY ? "won" : "lost",
           (unsigned long)i, (unsigned long)turns, (double)think / turns, (unsigned long)most, (double)frozen / turns,
           (unsigned long)fmost);
    CHECK(st.mode == M_END && fmost <= 15);
    printf("redraw: %lu frames from the screen's copy of the scene so far, %lu of them not the view's\n",
           (unsigned long)bg_used, (unsigned long)bg_bad);
    CHECK(bg_used > 1000 && bg_bad == 0);
}

// milestone 23, the original's pace (RE_NOTES.md § Pace): a walked step WALK_FRAMES frames, a
// climb, a drop or a jump CLIMB_FRAMES; an attack's menu back at ACT_GAIN_END after a hit (the
// EXP and the JP shown), at ACT_END after a miss; on the AI's turns the cursor gliding a tile
// every AI_GLIDE frames to its target, the preview AI_PREVIEW frames (an ability's name AI_NAME
// more), the view turned when the actor or its target is more than half hidden and the player's
// view back on the player's turn
static void test_pace(void)
{
    State s0;
    u16 c;
    u32 i, f, flat = 0, climb = 0, glides = 0, aims = 0, names = 0, turned = 0, bad = 0;
    u8 k, p, cx, cz, back = 0;
    sw_init(10);
    compute_reach(U_RAMZA);
    s0 = st;
    for (c = 0; c < MAP_W * MAP_H && (flat < 8 || climb < 4); c++) {
        if (s0.reach[c] == 0xFF || !s0.reach[c]) continue;
        st = s0; st.sel = U_RAMZA;
        if (!make_path(U_RAMZA, (u8)(c % MAP_W), (u8)(c / MAP_W))) continue;
        st.mode = M_WALK; st.path_i = 0; st.walk_t = 0;
        while (st.mode == M_WALK) {
            const u8 *a = st.path[st.path_i], *b = st.path[st.path_i + 1];
            u8 up = tile_at(a[0], a[1])->stand != tile_at(b[0], b[1])->stand || (a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) > 1;
            for (p = st.path_i, f = 0; st.path_i == p && f < 20; f++) sw_step(0);
            CHECKV(f, up ? CLIMB_FRAMES : WALK_FRAMES);
            if (up) climb++; else flat++;
        }
    }
    CHECK(flat >= 8 && climb >= 4);
    sw_init(10);                                          // an attack that hits, one that misses
    st.unit[1].x = 4; st.unit[1].z = 10; st.unit[1].face = 2;
    s0 = st;
    for (k = 0; k < 2; k++) {
        for (i = 0; i < 1000; i++) {
            st = s0; attack(U_RAMZA, 1);
            if (st.hit == !k) break;
        }
        for (f = 0; st.mode == M_ACT && f < 200; f++) sw_step(0);
        CHECKV(f, k ? ACT_END : ACT_GAIN_END);
        CHECKV(st.mode, M_MENU);
    }
    sw_init(16);                                          // the AI plays the whole battle
    for (i = 0; i < 30000 && st.mode != M_END; i++) {
        u8 r = st.rot, aim = st.ai == AI_AIM && st.mode == M_AIM && !st.turn, at = st.ai_t;
        cx = st.cx; cz = st.cz;
        sw_step(0);
        if (st.rot != r) turned++;
        if (!aim) continue;
        if (st.cx != cx || st.cz != cz) {                // a glide step: one tile, AI_GLIDE frames
            glides++;
            bad += (st.cx - cx) * (st.cx - cx) + (st.cz - cz) * (st.cz - cz) != 1 || st.ai_t;
        }
        if (st.mode == M_ACT) {                          // the preview, an ability's name
            aims++;
            names += st.abil != NO_ABIL;
            bad += at + 1 != (st.abil == NO_ABIL ? AI_PREVIEW : AI_PREVIEW + AI_NAME);
        }
    }
    CHECK(st.mode == M_END);
    CHECKV(bad, 0);
    printf("pace: %lu walked steps and %lu climbs or jumps as timed; the AI's cursor glided %lu tiles to %lu"
           " targets (%lu abilities named); the view turned %lu times\n", (unsigned long)flat, (unsigned long)climb,
           (unsigned long)glides, (unsigned long)aims, (unsigned long)names, (unsigned long)turned);
    CHECK(glides && names && turned);
    sw_init(0);                                           // the player's view back after an AI turned it
    for (i = 0; i < 30000 && st.mode != M_END && !back; i++) {
        if (!st.ai && st.mode == M_MENU && !st.turn && st.want == 0xFF) {
            if (st.prot != 0xFF) break;
            turn_end(0, 0);
        }
        p = st.prot;
        sw_step(0);
        if (p != 0xFF && st.prot == 0xFF && !st.ai) {    // a player's turn: turning back
            for (f = 0; f < 4 * TURN_FRAMES && st.want != 0xFF; f++) sw_step(0);
            for (f = 0; f < 4 * TURN_FRAMES && st.turn; f++) sw_step(0);
            CHECKV(st.rot, p);
            back = 1;
        }
    }
    CHECK(back);
}

// The flash changes visible flesh/clothes and keeps the black contour. Compare pixels
// in the actual hit pose, with the recoil's screen displacement removed; HP/tiles stay put.
static void test_blows(void)
{
    u8 normal[UNIT_SH][16], light[UNIT_SH][16];
    const u8 *data;
    u16 size, lit = 0, edges = 0;
    s16 u, v, sx, sy, x, y;
    u8 k;
    State before;
    sw_init(17);
    for (k = 0; k < NUNIT; k++) if (k != 1) st.unit[k].x = 200;
    st.unit[1].x = 9; st.unit[1].z = 14; st.unit[1].face = 2;
    st.unit[st.atk].face = 3; st.rot = 0; st.abil = NO_ABIL; st.hit = 1;
    st.unit[1].hp = st.unit[1].max_hp;
    view_of(0, 9, 14, &u, &v);
    sx = scene_x(0, u, v); sy = scene_y(u, v) + HH - map_tiles[149].stand * (HU / 2);
    st.camx = sx - 80; st.camy = sy - 65; sx = 80; sy = 65;
    data = rt_file("fftu", &size) + (st.unit[1].gfx * 2 * UNIT_FRAMES + P_HIT) * UNIT_FB;
    st.act_t = ACT_HIT + FLASH_FRAMES;
    game_render(); game_render();
    for (y = 0; y < UNIT_SH; y++) for (x = 0; x < 16; x++) normal[y][x] = sw_level(sx - 8 + x, sy - UNIT_FOOT + y);
    before = st;
    st.act_t = ACT_HIT + 10;
    game_render();
    for (y = 0; y < UNIT_SH; y++) for (x = 0; x < 16; x++) light[y][x] = sw_level(sx - 8 + x, sy - UNIT_FOOT + y);
    st.act_t = ACT_HIT + 8;
    game_render();
    for (y = 2; y < UNIT_FOOT - 1; y++) for (x = 0; x < 16; x++) {
        u16 bit = (u16)(0x8000 >> x), in = (u16)(data[2 * (3 * UNIT_SH + y)] << 8 | data[2 * (3 * UNIT_SH + y) + 1]);
        u16 mask = (u16)(data[2 * (2 * UNIT_SH + y)] << 8 | data[2 * (2 * UNIT_SH + y) + 1]);
        u8 white = sw_level(sx - 8 + x + 1, sy - UNIT_FOOT + y - 1);
        if (in & bit) { CHECKV(light[y][x], C_LGRAY); CHECKV(white, C_WHITE); lit++; }
        else if (!(mask & bit) && normal[y][x] == C_BLACK) { CHECKV(light[y][x], C_BLACK); CHECKV(white, C_BLACK); edges++; }
    }
    CHECK(lit >= 20 && edges >= 10);
    CHECKV(st.unit[1].x, before.unit[1].x); CHECKV(st.unit[1].z, before.unit[1].z); CHECKV(st.unit[1].hp, before.unit[1].hp);
    // Misses, healing and Defend keep the normal idle sprite, even during the flash window.
    st.mode = M_BROWSE;
    game_render(); game_render();
    for (y = 0; y < UNIT_SH; y++) for (x = 0; x < 16; x++) normal[y][x] = sw_level(sx - 8 + x, sy - UNIT_FOOT + y);
    for (k = 0; k < NABIL && abilities[k].kind != AB_HEAL; k++) ;
    CHECK(k < NABIL);
    for (u8 which = 0; which < 3; which++) {
        st.mode = M_ACT; st.act_t = ACT_HIT + 10; st.hit = which != 0;
        st.abil = which == 0 ? NO_ABIL : which == 1 ? k : ABIL_DEFEND;
        game_render();
        for (y = 0; y < UNIT_SH; y++) for (x = 0; x < 16; x++) CHECKV(sw_level(sx - 8 + x, sy - UNIT_FOOT + y), normal[y][x]);
        CHECKV(st.unit[1].hp, before.unit[1].hp);
    }
    for (k = 17; k <= 25; k++) { sw_init(k); steps(0, BLADE_END + 1); CHECKV(st.mode, M_ACT); }
    printf("blows: %u interior pixels light/white, %u black contour pixels kept; nine armed/unarmed doors\n", lit, edges);
}

// Defend: drawn as FFT draws a support ability; a command after the skill set, Defending until
// the unit's next turn
static void test_defend(void)
{
    u16 n, d[2] = { 0 }, all[2] = { 0 };
    u8 i;
    for (n = 1; n <= 500; n++) {
        sw_init(n);
        for (i = 0; i < NUNIT; i++)
            if (i != U_RAMZA) { all[jobs[st.unit[i].job].row]++; d[jobs[st.unit[i].job].row] += st.unit[i].defend; }
        CHECKV(st.unit[U_RAMZA].defend, 0);
    }
    printf("defend: %.3f of the Squires, %.3f of the Chemists\n", (double)d[0] / all[0], (double)d[1] / all[1]);
    CHECK(d[0] && d[1] && d[0] * 4 < all[0] && d[1] * 4 < all[1]);
    sw_init(10);
    st.unit[U_RAMZA].defend = 1;
    press(K_DOWN); press(K_A); CHECKV(st.mode, M_ACTS);   // Act: Attack, Guts, Defend
    press(K_DOWN); press(K_DOWN); CHECKV(st.sub, 2);
    press(K_A); CHECKV(st.mode, M_ACT);
    while (st.mode == M_ACT) sw_step(0);
    CHECK(st.unit[U_RAMZA].status & ST_DEFEND && st.acted && st.mode == M_MENU);
    turn_end(0, 1);
    while (st.act != U_RAMZA) turn_end(0, 0);
    CHECKV(st.unit[U_RAMZA].status & ST_DEFEND, 0);       // gone at its next turn
}

// the battle from its start: Delita and the five enemies end their turns without moving or
// acting, then Ramza's; the order shown is FFT's AT list
static void test_turns(void)
{
    u8 i;
    sw_init(0);
    CHECKV(st.act, 0);                                    // all at CT 102: Delita, index 0
    CHECKV(st.ai, AI_THINK);                              // the guest's AI
    for (i = 0; i < ORDER_N; i++) CHECKV(st.turns[i], i);
    for (i = 1; i <= 5; i++) {
        turn_end(0, 0);
        CHECKV(st.act, i);
        CHECK(st.cx == st.unit[i].x && st.cz == st.unit[i].z);
        CHECKV(st.unit[i - 1].ct, 42);                    // waited: 2 + 40
    }
    turn_end(0, 0);
    CHECKV(st.act, U_RAMZA);
    CHECKV(st.ai, AI_OFF);
    CHECKV(st.turns[1], U_RAMZA + 1);                     // the recruits, still at 102
    steps(0, 60);
    CHECKV(st.act, U_RAMZA);                              // the player's turn waits for keys
    wait_turn();
    CHECKV(st.unit[U_RAMZA].ct, 42);
    CHECKV(st.act, U_RAMZA + 1);
    // after the recruits, the next tick: everyone at 42 + 60 = 102 again, Delita first
    for (i = 0; i < 4; i++) wait_turn();
    CHECKV(st.act, 0);
    CHECKV(st.unit[0].ct, 2);
}

// Milestone 26: names are stable by seed and unique by gender, two reversible Status
// pages, horizontal bands hidden during action/motion, live HP/CT and a canal depth label.
static void test_interface(void)
{
    State original;
    char names[NUNIT][12];
    u32 page0, page1, old;
    u8 i, j;
    int x, y, ink0 = 0, ink1 = 0;
    sw_init(26);
    original = st;
    for (i = 0; i < NUNIT; i++) {
        CHECK(strlen(unit_name(i)) > 0 && strlen(unit_name(i)) <= 11);
        strcpy(names[i], unit_name(i));
        for (j = 0; j < i; j++) if (st.unit[i].gfx > UG_DELITA && st.unit[j].gfx > UG_DELITA
            && st.unit[i].female == st.unit[j].female) CHECK(strcmp(names[j], names[i]));
    }
    CHECK(!strcmp(names[U_RAMZA], "Ramza") && !strcmp(names[0], "Delita"));
    sw_init(26); CHECK(!memcmp(&original, &st, sizeof st));
    for (i = 0; i < NUNIT; i++) CHECK(!strcmp(names[i], unit_name(i)));
    sw_init(27); CHECK(strcmp(names[1], unit_name(1)));
    sw_init(26); steps(0, 30); CHECK(hud_shown());
    old = sw_checksum(); st.unit[st.act].hp--; st.unit[st.act].ct++;
    steps(0, 30); CHECK(sw_checksum() != old); CHECK(hud_shown());
    sw_init(31); steps(0, 12); original = st; page0 = sw_checksum();
    CHECK(st.page == 0 && st.mode == M_STATUS);
    press(K_RIGHT); steps(0, 12); page1 = sw_checksum();
    CHECK(st.page == 1 && st.mode == M_STATUS && page0 != page1);
    CHECK(!memcmp(original.unit, st.unit, sizeof st.unit));
    press(K_LEFT); steps(0, 12); CHECKV(sw_checksum(), page0);
    press(K_ESC); CHECKV(st.mode, M_MENU);
    st.menu = MN_STATUS; press(K_A); steps(0, 12); CHECKV(sw_checksum(), page0);
    // Portrait, numerical fields and equipment are inside the screen; the 2h top-right
    // height changes to a depth-labelled 0h in water, even when no unit occupies the tile.
    sw_init(34); steps(0, 30);
    for (y = 11; y < 16; y++) for (x = 128; x < 160; x++) ink0 += sw_level(x, y) == C_BLACK;
    st.cx = 4; st.cz = 11; steps(0, 30);
    for (y = 11; y < 17; y++) for (x = 128; x < 160; x++) {
        u16 o = (st.camy + y) * SC_BYTES + ((st.camx + x) >> 3);
        u8 b = (u8)(0x80 >> ((st.camx + x) & 7));
        u8 background = (scene_l[o] & b ? 1 : 0) | (scene_d[o] & b ? 2 : 0);
        ink1 += sw_level(x, y) != background;   // old depth text clears back to the scenery
    }
    CHECK(ink0 > 10 && ink1 == 0);
    sw_init(36); steps(0, 5); CHECKV(st.mode, M_WALK); CHECK(!hud_shown());
    sw_init(37); steps(0, 18); CHECKV(st.mode, M_ACT); CHECK(!hud_shown());
    printf("interface: 510 disc names, deterministic unique draws, live bands, two Status pages, depth label, hidden during motion/blows\n");
}

static void test_water(void)
{
    u8 i, r, count = 0, base[UNIT_SH][16];
    s16 u, v, sx, sy, x, y;
    int upper = 0, edge = 0;
    // Every actual canal is grounded water in every view; terrain and stats
    // remain intact. Object markers are excluded from human immersion.
    for (r = 0; r < 4; r++) {
        sw_init((u16)(41 + r));
        for (i = 0; i < MAP_W * MAP_H; i++) if (map_tiles[i].water) {
            st.unit[U_RAMZA].x = i % MAP_W; st.unit[U_RAMZA].z = i / MAP_W;
            CHECK(unit_in_water(U_RAMZA)); CHECKV(map_tiles[i].stand, 0);
            if (!r) count++;
        }
    }
    CHECKV(count, 38);
    sw_init(50); CHECK(!unit_in_water(1)); CHECK(unit_in_water(U_RAMZA));
    // A crop reveals the unmodified scene beneath it, including the old shadow.
    // Freeze clocks/camera, suppress action UI and isolate one standing unit.
    for (r = 0; r < 4; r++) {
        sw_init((u16)(41 + r)); steps(0, 30);
        for (i = 0; i < NUNIT; i++) st.unit[i].x = 200;
        st.mode = M_ACT; st.atk = 1; st.tgt = 2; st.act_t = 0;
        view_of(r, 5, 11, &u, &v);
        sx = scene_x(r, u, v) - st.camx - 8;
        sy = scene_y(u, v) + HH - st.camy - UNIT_FOOT;
        game_render();
        for (y = 0; y < UNIT_SH; y++) for (x = 0; x < 16; x++) base[y][x] = sw_level(sx + x, sy + y);
        st.unit[U_RAMZA].x = 5; st.unit[U_RAMZA].z = 11;
        game_render();
        for (y = 0; y < UNIT_SH; y++) for (x = 0; x < 16; x++) {
            u8 pix = sw_level(sx + x, sy + y);
            if (y >= WATER_ROWS) CHECKV(pix, base[y][x]);
            else if (pix != base[y][x]) {
                if (y == WATER_ROWS - 1 && pix == C_LGRAY) edge++;
                else upper++;
            }
        }
    }
    CHECK(upper > 30 && edge >= 2);
    // Actual entry, wade and exit paths: a hop stays whole; the floor landing
    // changes the pose without altering the movement's destination or duration.
    sw_init(45); CHECKV(st.mode, M_WALK); CHECK(!unit_in_water(U_RAMZA));
    steps(0, CLIMB_FRAMES - 1); CHECK(!unit_in_water(U_RAMZA));
    steps(0, 1); CHECKV(st.mode, M_MENU); CHECK(unit_in_water(U_RAMZA));
    CHECKV(st.unit[U_RAMZA].x, 5); CHECKV(st.unit[U_RAMZA].z, 11);
    sw_init(46);
    for (i = 0; i < WALK_FRAMES; i++) { CHECK(unit_in_water(U_RAMZA)); sw_step(0); }
    CHECKV(st.mode, M_MENU); CHECK(unit_in_water(U_RAMZA));
    sw_init(47); CHECK(unit_in_water(U_RAMZA));
    sw_step(0); CHECK(!unit_in_water(U_RAMZA));
    steps(0, CLIMB_FRAMES - 1); CHECKV(st.mode, M_MENU); CHECK(!unit_in_water(U_RAMZA));
    CHECKV(st.unit[U_RAMZA].x, 4); CHECKV(st.unit[U_RAMZA].z, 11);
    sw_init(48); steps(0, ACT_HIT + 1); CHECKV(st.mode, M_ACT); CHECK(unit_in_water(U_RAMZA));
    CHECK(st.unit[1].hp < st.unit[1].max_hp);
    printf("water: 38 canals x four views; scene exposed below cut, light edge, entry/wade/exit, wet attack and object graphics retained\n");
}

// The displayed prediction against complete clock advances, including ties,
// every supported Speed and crossings just short of the 100 CT threshold.
static void test_turn_prediction(void)
{
    u8 speed, i, predicted[ORDER_N];
    for (speed = 1; speed <= 50; speed++) {
        sw_init(10);
        for (i = 0; i < NUNIT; i++) {
            st.unit[i].sp = (u8)(1 + (speed + 7 * i) % 50);
            st.unit[i].ct = (u8)((17 * speed + 29 * i) % 100);
        }
        st.act = ct_next();
        turn_order(); memcpy(predicted, st.turns, sizeof predicted);
        CHECKV(predicted[0], st.act);
        for (i = 1; i < ORDER_N; i++) {
            ct_end(st.act, 1, 1);
            st.act = ct_next(); CHECKV(predicted[i], st.act);
        }
    }
    printf("turn prediction: 300 displayed entries equal complete CT clock advances, Speed 1..50\n");
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
    sw_init(10);                                          // the turn menu, Status, the facing,
    steps(0, 12); sw_write_png("x/m16_menu.png", 4);      // an enemy's panel
    press(K_UP); press(K_A); steps(0, 6); sw_write_png("x/m16_status.png", 4);
    press(K_B); press(K_UP); press(K_A); steps(0, 4); sw_write_png("x/m16_face.png", 4);
    press(K_B); press(K_B);
    st.cx = st.unit[7].x; st.cz = st.unit[7].z;
    steps(0, 30); sw_write_png("x/m16_browse.png", 4);
    sw_init(8); steps(0, 12); sw_write_png("x/m17_aim.png", 4);   // an attack: aimed, the
    sw_init(9); steps(0, 6); sw_write_png("x/m17_hit.png", 4);    // number, a miss, a KO
    sw_init(8);
    do attack(U_RAMZA, 1); while (st.hit);
    while (st.act_t < ACT_HIT + 6) sw_step(0);
    sw_write_png("x/m17_miss.png", 4);
    sw_init(8); st.unit[1].hp = 1;
    do attack(U_RAMZA, 1); while (!st.hit);
    while (st.act_t < ACT_HIT + HIT_FRAMES + 6) sw_step(0);
    sw_write_png("x/m17_ko.png", 4);
    sw_init(10); press(K_DOWN); press(K_A); steps(0, 6); sw_write_png("x/m18_acts.png", 4);   // Act's
    sw_init(11); steps(0, 10); sw_write_png("x/m18_items.png", 4);   // list, the items, a Potion
    sw_init(12); steps(0, 10); sw_write_png("x/m18_potion.png", 4);  // aimed, Wish landing
    sw_init(13); steps(0, 4); sw_write_png("x/m18_wish.png", 4);
    sw_init(14);                                          // EXP+20 rising, the results; a
    press(K_DOWN); press(K_A); press(K_A); press(K_UP); press(K_A);   // death counter, a crystal
    steps(0, ACT_HIT + GAIN_T + 4); sw_write_png("x/m19_exp.png", 4);  // and a chest beside the KO
    steps(0, 40); sw_write_png("x/m19_victory.png", 4);
    sw_init(10);
    st.unit[1].x = 4; st.unit[1].z = 10; st.unit[1].hp = 0; st.unit[1].dc = 2;
    st.unit[2].x = 3; st.unit[2].z = 10; st.unit[2].hp = 0; st.unit[2].dc = DC_CRYSTAL;
    st.unit[3].x = 5; st.unit[3].z = 10; st.unit[3].hp = 0; st.unit[3].dc = DC_CHEST;
    steps(0, 4); sw_write_png("x/m19_ko.png", 4);
    {                                                     // an AI turn (fft(16)): the first that
        u16 f, fm = 0, fa = 0, fh = 0, ff = 0, n;         // moves and acts: its range shown, the
        sw_init(16);                                      // target marked, the blow, the facing
        for (f = 0; f < 3000 && !ff; f++) {
            sw_step(0);
            if (st.ai == AI_MOVE && st.ai_t == 5) fm = f;
            if (st.ai == AI_AIM && st.ai_t == 5 && fm) fa = f;
            if (fa && st.mode == M_ACT && st.act_t == ACT_HIT + 3) fh = f;
            if (fh && st.ai == AI_FACE && st.ai_t == 2) ff = f;
        }
        sw_init(16);
        for (n = 0; n <= ff; n++) {
            sw_step(0);
            if (n == fm) sw_write_png("x/m20_move.png", 4);
            if (n == fa) sw_write_png("x/m20_aim.png", 4);
            if (n == fh) sw_write_png("x/m20_hit.png", 4);
            if (n == ff) sw_write_png("x/m20_face.png", 4);
        }
    }
    {                                                     // milestone 21: the throw pose and a
        u16 f, ft = 0, fs = 0, n, i;                      // hit's burst (fft(16)), the reach and
        sw_init(16);                                      // the cursor on a sloped tile (fft(5))
        for (f = 0; f < 6000 && !(ft && fs); f++) {
            sw_step(0);
            if (!ft && st.mode == M_ACT && st.abil != NO_ABIL && st.abil != ABIL_DEFEND
                && abilities[st.abil].flags & AF_EVADE && st.act_t == ACT_BLOW + 1) ft = f;
            if (ft && !fs && st.mode == M_ACT && st.hit && st.act_t == ACT_HIT + 1) fs = f;
        }
        sw_init(16);
        for (n = 0; n <= fs; n++) {
            sw_step(0);
            if (n == ft) sw_write_png("x/m21_throw.png", 4);
            if (n == fs) sw_write_png("x/m21_spark.png", 4);
        }
        sw_init(5);
        steps(0, 40);
        for (i = 0; i < MAP_W * MAP_H; i++) {
            const Tile *t = &map_tiles[i];
            if (st.reach[i] != 0xFF && (t->c[0] != t->c[1] || t->c[1] != t->c[2] || t->c[2] != t->c[3])) break;
        }
        CHECK(i < MAP_W * MAP_H);
        st.cx = (u8)(i % MAP_W); st.cz = (u8)(i / MAP_W);
        steps(0, 24); sw_write_png("x/m21_slope.png", 4);
    }
}

int main(int argc, char **argv)
{
    if (argc > 1 && !strcmp(argv[1], "--find")) { find_spots(); return 0; }
    if (argc > 1 && !strcmp(argv[1], "--hides")) { verbose_hides = 1; test_hides(); return 0; }
    ai_diff = argc > 1 && !strcmp(argv[1], "--ai");
    test_rotations();
    test_heights();
    test_stats();
    test_draw();
    test_reach();
    test_occlusion();
    test_hides();
    test_team();
    test_turn_keys();
    test_reach_tiles();
    test_walk_keys();
    test_menu();
    test_fixes();
    test_attack();
    test_skills();
    test_end();
    test_clock();
    test_turns();
    test_ai();
    test_ai_battle();
    test_pace();
    test_blows();
    test_defend();
    test_interface();
    test_water();
    test_turn_prediction();
    shots();
    printf(fails ? "%d FAILED\n" : "all tests passed\n", fails);
    return fails != 0;
}
