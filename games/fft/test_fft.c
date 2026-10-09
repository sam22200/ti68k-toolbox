// Tests, headless: the four orientations, the map's heights, reach and paths, the scene and the
// occlusion of units by the buildings in front of them, rotation and walking by keys.
//   ./fft_test            every test
//   ./fft_test --find     the tiles where a unit is hidden in one view and seen in another
#include <stdio.h>
#include <string.h>
#include "../../runtime/platform-sw/rt_sw.h"
#include "fft.h"

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

// ---------------------------------------------------------------- reach and paths
static void test_reach(void)
{
    u8 i, n;
    sw_init(0);
    compute_reach(0);                                     // Ramza at (5, 5), h 2
    CHECKV(st.reach[5 * MAP_W + 5], 0);
    CHECKV(st.reach[5 * MAP_W + 6], 1);                   // the street next to him
    CHECKV(st.reach[4 * MAP_W + 9], 0xFF);                // a canal: never
    CHECKV(st.reach[3 * MAP_W + 4], 0xFF);                // a roof 6 units up: Jump 3
    CHECKV(st.reach[1 * MAP_W + 4], 0xFF);                // h 4 but behind the canal... >4 steps
    for (i = 0; i < MAP_W * MAP_H; i++)                   // never more than Move steps
        CHECK(st.reach[i] == 0xFF || st.reach[i] <= MOVE);
    n = make_path(0, 7, 7);
    CHECK(n >= 2);
    CHECKV(st.path[0][0], 5); CHECKV(st.path[0][1], 5);
    CHECKV(n, 5);                                         // 4 steps round the corner
    CHECKV(st.path[n - 1][0], 7); CHECKV(st.path[n - 1][1], 7);
    for (i = 1; i < n; i++) {                             // adjacent steps, Jump respected
        s16 dx = st.path[i][0] - st.path[i - 1][0], dz = st.path[i][1] - st.path[i - 1][1];
        s16 dh = tile_at(st.path[i][0], st.path[i][1])->stand - tile_at(st.path[i - 1][0], st.path[i - 1][1])->stand;
        CHECKV(dx * dx + dz * dz, 1);
        CHECK(dh <= 2 * JUMP && dh >= -2 * JUMP);
    }
    CHECKV(make_path(0, 9, 4), 0);                        // a canal tile: no path
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
    sw_init(1);                                           // the thief behind the house
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

// the diamond above enemies: the thief seen (west view) shows ~30 pixels more than as an ally
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
    CHECK(st.unit[0].x == 5 && st.unit[0].z == 5);        // units never move with the view
    // every orientation: the units stand on their own tiles (same world tile under the feet)
    for (i = 0; i < 4; i++) {
        st.rot = (u8)i; sw_step(0);
        CHECK(scene_rot == i);
    }
}

static void test_walk_keys(void)
{
    int k;
    sw_init(0);                                           // cursor on Ramza (5, 5)
    press(K_A);
    CHECKV(st.mode, M_TARGET);
    press(K_RIGHT); press(K_RIGHT);                       // view u = world x at r 0
    press(K_DOWN); press(K_DOWN);                         // view v = world z
    CHECK(st.cx == 7 && st.cz == 7);
    press(K_A);
    CHECKV(st.mode, M_WALK);
    for (k = 0; k < 40 && st.mode == M_WALK; k++) sw_step(0);
    CHECKV(st.mode, M_BROWSE);
    CHECK(st.unit[0].x == 7 && st.unit[0].z == 7);
    CHECK(st.cx == 7 && st.cz == 7);
    CHECKV(st.unit[0].face, 2);                           // facing its last step: (7, 6) -> (7, 7), +z
    // after a turn the arrows follow the view: at r 1, view +v is world +x
    press(K_F5); steps(0, TURN_FRAMES);
    press(K_DOWN);
    CHECK(st.cx == 8 && st.cz == 7);
    press(K_A); press(K_ESC);                             // select then cancel: no quit
    CHECKV(st.mode, M_BROWSE);
    CHECKV(sw_step(K_ESC), 0);                            // ESC in browse quits
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
    test_reach();
    test_occlusion();
    test_team();
    test_turn_keys();
    test_walk_keys();
    shots();
    printf(fails ? "%d FAILED\n" : "all tests passed\n", fails);
    return fails != 0;
}
