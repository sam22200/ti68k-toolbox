// Tests, headless: movement and jump against the target numbers (RE_NOTES.md § Numbers), the
// height map (floor under the foot box, landing, falling), then the screen.
//   ./alundra_test            every test
//   ./alundra_test --trace    z per step of a standing jump
//   ./alundra_test --play KEYS N [SCENARIO]   the position per step of a key script
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../../runtime/platform-sw/rt_sw.h"
#include "alundra.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)
#define CHECKV(a, b) do { long _a = (a), _b = (b); if (_a != _b) { printf("FAIL %s:%d: %s = %ld, want %ld\n", __FILE__, __LINE__, #a, _a, _b); fails++; } } while (0)

static void steps(u32 keys, int n) { while (n--) al_step(keys); }

// the player standing at pixel (px, py) on whatever floor is there
static void at(int px, int py)
{
    al_start();
    st.x = px * SUB;
    st.y = py * SUB;
    st.z = al_floor(st.x, st.y);
}

// ---------------------------------------------------------------- milestone 2: move and jump
static void test_walk(void)
{
    at(88, 72); steps(K_RIGHT, 10);                 // row 4: open ground from x 48 to 143
    CHECKV(st.x, 88 * SUB + 10 * SPEED_X);          // 2.375 px per step (Alundra's, scaled)
    at(24, 92); steps(K_UP, 10);                    // column 1, the corridor
    CHECKV(st.y, 92 * SUB - 10 * SPEED_Y);          // depth at 2/3 of that (Alundra)
    at(88, 76); steps(K_UP | K_RIGHT, 2);
    CHECKV(st.x - 88 * SUB, 2 * DIAG_X);            // x 0.707 per axis on a diagonal
    CHECKV(76 * SUB - st.y, 2 * DIAG_Y);
    at(88, 72); steps(K_LEFT | K_RIGHT, 5);         // opposite keys cancel
    CHECKV(st.x, 88 * SUB);
}

static void test_jump(void)
{
    s16 apex = 0;
    int n;
    at(88, 72);
    al_step(K_A);
    CHECK(!st.grounded);
    for (n = 1; n < 40 && !st.grounded; n++) {
        if (st.z > apex) apex = st.z;
        al_step(K_A);                               // held: no second jump
    }
    CHECKV(apex, 222);                              // 13.9 px: between 1 and 2 levels of 8 px
    CHECK(apex > LEVEL_Z && apex < 2 * LEVEL_Z);
    CHECKV(n, 12);                                  // 12 steps = 0.375 s (Alundra 0.35 s)
    CHECKV(st.z, 0);
    CHECKV(st.vz, 0);
    steps(K_A, 5);                                  // still held: stays on the ground
    CHECK(st.grounded);
    al_step(0); al_step(K_A);                       // a new press jumps again
    CHECK(!st.grounded);
}

static void test_air_control(void)
{
    at(100, 72);
    al_step(K_A | K_LEFT);
    steps(K_LEFT, 3);
    CHECKV(st.x, 100 * SUB - 4 * SPEED_X);          // moving in the air at full speed
    steps(K_RIGHT, 3);                              // reversed at once, no momentum
    CHECKV(st.x, 100 * SUB - SPEED_X);
    steps(0, 2);                                    // released: stops at once
    CHECKV(st.x, 100 * SUB - SPEED_X);
    CHECK(!st.grounded);
}

static void test_fall_speed(void)
{
    at(88, 72);
    st.grounded = 0; st.z = 120 * SUB; st.vz = 0;   // dropped from high: the speed caps
    steps(0, 12);
    CHECKV(st.vz, -FALL_MAX);
    steps(0, 30);
    CHECK(st.grounded);
    CHECKV(st.z, 0);
}

// ---------------------------------------------------------------- milestone 3: height map
static void test_floor(void)
{
    CHECKV(al_floor(40 * SUB, 28 * SUB), 0);        // open ground
    CHECKV(al_floor(72 * SUB, 28 * SUB), LEVEL_Z);           // level 1 (columns 4-5)
    CHECKV(al_floor(120 * SUB, 28 * SUB), 2 * LEVEL_Z);      // level 2 (columns 6-8)
    CHECKV(al_floor(66 * SUB, 28 * SUB), LEVEL_Z);   // box 61..70 straddles 0|1: the highest
    CHECKV(al_floor(58 * SUB, 28 * SUB), 0);        // box 53..62: all on 0
    CHECKV(al_floor(120 * SUB, 46 * SUB), 2 * LEVEL_Z);  // box rows 42..49 straddles 2|0
    CHECKV(al_floor(120 * SUB, 54 * SUB), 0);       // box 50..57: below the platform
    CHECKV(al_tile(-1, 10), WALL);                  // outside the room = wall
}

static void test_land_on_heights(void)
{
    at(72, 28);                                     // standing on level 1
    CHECKV(st.z, LEVEL_Z);
    st.grounded = 0; st.z = 40 * SUB; st.vz = 0;   // dropped above level 1: lands on it
    steps(0, 20);
    CHECK(st.grounded);
    CHECKV(st.z, LEVEL_Z);
    at(120, 28);
    al_step(K_A);                                   // a jump on level 2 lands back on level 2
    steps(0, 20);
    CHECK(st.grounded);
    CHECKV(st.z, 2 * LEVEL_Z);
}

static void test_walk_off(void)
{
    int n;
    at(120, 40);                                    // level 2, front edge (box rows 36..43)
    CHECKV(st.z, 2 * LEVEL_Z);
    al_step(K_DOWN);                                // box 39..46: still partly over the platform
    CHECK(st.grounded);
    CHECKV(st.z, 2 * LEVEL_Z);
    for (n = 0; n < 10 && st.grounded; n++) al_step(K_DOWN);
    CHECK(!st.grounded);                            // the whole box past the edge: falling
    CHECK(st.y / SUB - FOOT_D / 2 >= 48);
    steps(0, 20);
    CHECK(st.grounded);
    CHECKV(st.z, 0);
}

// ---------------------------------------------------------------- milestone 4: ledges
// hold k for n steps, with a jump pressed on step j (-1: none)
static void hold(u32 k, int n, int j)
{
    int i;
    for (i = 0; i < n; i++) al_step(k | (i == j ? K_A : 0));
}

static void test_ledges(void)
{
    // 0 -> +1 (column 4 at x 64): walking stops flush (box 54..63), a jump goes up
    at(56, 24); hold(K_RIGHT, 8, -1);
    CHECKV(st.x / SUB, 59);
    CHECKV(st.z, 0);
    at(56, 24); hold(K_RIGHT, 2, 0);
    CHECKV(st.x / SUB, 59);                         // blocked while the feet are below 8.4 px
    hold(K_RIGHT, 2, -1);
    CHECK(st.x / SUB > 59);                         // the feet above the ledge: it moves on
    hold(K_RIGHT, 13, -1);
    CHECK(st.grounded);
    CHECKV(st.z, LEVEL_Z);                          // landed on level 1
    CHECKV(st.x / SUB, 91);                         // and stopped by the +1 step to level 2
    // +1 -> +2 by a jump
    at(84, 24); hold(K_RIGHT, 12, 0);
    CHECK(st.grounded);
    CHECKV(st.z, 2 * LEVEL_Z);
    // 0 -> +2 unreachable (column 8: row 3 at 0, row 2 at 2), walking and jumping
    at(128, 56); hold(K_UP, 8, -1);
    CHECKV(st.y / SUB, 52);
    at(128, 56); hold(K_UP, 16, 0);
    CHECKV(st.y / SUB, 52);
    CHECKV(st.z, 0);
    // the pillar at 2 from the ground: unreachable too
    at(100, 88); hold(K_RIGHT, 16, 0);
    CHECKV(st.x / SUB, 107);
    // walls block at any height of the jump
    at(136, 72); hold(K_RIGHT, 16, 0);
    CHECKV(st.x / SUB, 139);
}

static void test_drops(void)
{
    int n;
    at(72, 24); hold(K_LEFT, 14, -1);               // walk off level 1 to the left
    CHECK(st.grounded);
    CHECKV(st.z, 0);
    CHECK(st.x < 48 * SUB);
    at(120, 40);                                    // walk off level 2's front edge
    for (n = 0; n < 10 && st.grounded; n++) al_step(K_DOWN);
    CHECK(!st.grounded);
    hold(K_DOWN, 20, -1);
    CHECK(st.grounded);
    CHECKV(st.z, 0);
    CHECKV(st.y / SUB, 76);                         // stopped by the pillar (2) in row 5
    at(132, 24); hold(K_DOWN, 12, 0);               // a jump down from level 2 to the ground
    hold(K_DOWN, 20, -1);
    CHECK(st.grounded);
    CHECKV(st.z, 0);
}

// ---------------------------------------------------------------- milestone 5: collision
static void test_corners(void)
{
    // the corridor (column 1, rows 3-4) entered misaligned: flush against the wall in column
    // 2, nudged left 0.75 px per step until the box fits, then down to the room's bottom
    at(30, 40); hold(K_DOWN, 3, -1);
    CHECKV(st.y / SUB, 44);                         // flush: box bottom at 47
    hold(K_DOWN, 1, -1);
    CHECKV(st.x, 30 * SUB - NUDGE_X);               // nudged, not moved down
    CHECKV(st.y / SUB, 44);
    hold(K_DOWN, 40, -1);
    CHECKV(st.x / SUB, 27);                         // box 22..31: fits the corridor
    CHECKV(st.y / SUB, 92);
    // a flat wall (both front corners blocked): no nudge
    at(40, 24); hold(K_UP, 10, -1);
    CHECKV(st.x, 40 * SUB);
    CHECKV(st.y / SUB, 20);
    at(96, 90); hold(K_LEFT, 10, -1);               // the wall in row 5, column 4
    CHECKV(st.x / SUB, 85);
    CHECKV(st.y, 90 * SUB);
    // around an obstacle: the pillar (2) in row 5, one corner on it, on foot and in the air
    at(96, 78); hold(K_RIGHT, 30, -1);
    CHECKV(st.y / SUB, 76);
    CHECKV(st.x / SUB, 139);                        // on to the right wall
    at(96, 78); hold(K_RIGHT, 30, 0);
    CHECKV(st.y / SUB, 76);
    CHECKV(st.x / SUB, 139);
    CHECK(st.grounded);
}

static void test_air_collision(void)
{
    int n;
    // a jump that reaches the +1 ledge on its way down, feet below the top: it stays a wall,
    // the player slides down its face to the ground
    at(35, 24); hold(K_RIGHT, 16, 0);
    CHECKV(st.x / SUB, 59);
    CHECKV(st.z, 0);
    CHECK(st.grounded);
    // the same jump started closer reaches it on the way up and lands on it
    at(44, 24); hold(K_RIGHT, 16, 0);
    CHECKV(st.z, LEVEL_Z);
    // a fall from level 2 along the wall in column 9: x stays flush the whole way down
    at(139, 40);
    for (n = 0; n < 20; n++) {
        al_step(K_RIGHT | K_DOWN);
        CHECKV(st.x, 139 * SUB);
    }
    CHECK(st.grounded);
    CHECKV(st.z, 0);
}

// The whole room by key script (keys/route.txt): 0->1 and 1->2 by jumps, the drop from 2,
// a failed jump at the 0->2 ledge, row 4, a corner rounded into row 5, the corridor up to the
// back wall
static void test_route(void)
{
    static SwScript sc;
    static const struct { u16 f; u8 x, y, z; } C[] = {
        { 19, 71, 40, 8 }, { 35, 109, 40, 16 }, { 57, 119, 62, 0 }, { 71, 119, 52, 0 },
        { 111, 67, 68, 0 }, { 135, 53, 82, 0 }, { 199, 25, 20, 0 } };
    u16 f;
    u8 c = 0;
    CHECKV(sw_load_script(&sc, "keys/route.txt"), 0);
    sw_init(0);
    for (f = 0; f < 200; f++) {
        sw_step(sw_script_keys(&sc, f));
        if (c < sizeof C / sizeof C[0] && f == C[c].f) {
            CHECKV(st.x / SUB, C[c].x);
            CHECKV(st.y / SUB, C[c].y);
            CHECKV(st.z / SUB, C[c].z);
            c++;
        }
    }
    CHECKV(c, sizeof C / sizeof C[0]);
}

// ---------------------------------------------------------------- the screen


// the player's sprite pixel (sprite coordinates): its grey, or -1 where it is transparent
static int hero_px(int c, int r)
{
    const hero_row (*g)[HERO_SH] = hero_gfx[st.dir][al_frame()];
    hero_row b = (hero_row)1 << (HERO_SW - 1 - c);
    if (g[2][r] & b) return -1;
    return ((g[0][r] & b) ? 1 : 0) | ((g[1][r] & b) ? 2 : 0);
}

// how many of the sprite's opaque pixels are on screen as drawn (the player not hidden)
static int hero_shown(int *opaque)
{
    int sx = st.x / SUB - al_camx + hero_ox[st.dir][al_frame()], c, r, n = 0;
    int sy = world->top - al_camy + st.y / SUB - st.z / SUB - HERO_AY;
    *opaque = 0;
    for (r = 0; r < HERO_SH; r++)
        for (c = 0; c < HERO_SW; c++) {
            int g = hero_px(c, r);
            if (g < 0) continue;
            ++*opaque;
            n += sw_level(sx + c, sy + r) == g;
        }
    return n;
}

static int grey_in(int x, int y, int a, int b) { int g = sw_level(x, y); return g == a || g == b; }

static void test_screen(void)
{
    u16 a;
    int n, all;
    sw_init(0);                                     // start: (24, 40) on open ground
    sw_step(0);
    n = hero_shown(&all);
    CHECK(all > 150);
    CHECKV(n, all);                                 // the whole sprite (white outline included)
    sw_step(K_C);                                   // overlay off: the textures alone
    CHECK(grey_in(50, ROOM_Y + 30, C_LGRAY, C_DGRAY));      // open ground (level 0)
    CHECK(grey_in(66, ROOM_Y + 18 - LEVEL_PX(1), C_WHITE, C_LGRAY));  // level 1 top, raised 8 px
    for (n = 44; n < 51; n++) CHECK(grey_in(66, n, C_DGRAY, C_BLACK));  // its front face
    CHECKV(sw_level(66, 51), C_BLACK);              // ending on a black line
    CHECK(grey_in(66, 52, C_LGRAY, C_DGRAY));       // then the ground of row 3
    CHECKV(sw_level(96, ROOM_Y + 16 - LEVEL_PX(2)), C_LGRAY);  // level 2: framed top
    CHECK(grey_in(40, 35, C_DGRAY, C_BLACK));       // the wall at column 2: top faces
    CHECK(grey_in(40, 62, C_BLACK, C_DGRAY));       // and its front face below row 4
    sw_init(1);                                     // in the air
    sw_step(0);
    CHECKV(al_frame(), 8);
    n = hero_shown(&all);
    CHECKV(n, all);                                 // the jump image, raised by z
    sw_init(5);                                     // (128, 56): row 3, the level-2 platform's
    sw_step(0);                                     // front face is behind, the player in front
    n = hero_shown(&all);
    CHECKV(n, all);
    sw_init(0);
    sw_step(0);
    a = sw_checksum();
    sw_step(K_C);                                   // overlay off
    CHECK(sw_checksum() != a);
}

// The shadow: every pixel of its mask around the feet, on the floor, that the sprite does not
// cover is two greys darker than the background (light floor) or black ringed with light grey
// (dark floor); returns how many, -1 on a wrong pixel
static int shadow_ok(const u16 *mask, int w, int h)
{
    static u8 ref[RT_PH][RT_PW];
    State keep = st;
    int x0, y0, sx, sy, c, r, n = 0, all = 0, sum = 0, dark;
    st.x = 136 * SUB; st.y = 20 * SUB; st.z = 2 * LEVEL_Z;  // far away: the background
    st.grounded = 1;
    sw_step(0);
    for (r = 0; r < RT_PH; r++)
        for (c = 0; c < RT_PW; c++) ref[r][c] = sw_level(c, r);
    st = keep;
    sw_step(0);                                     // one step on: the frame shows this state
    x0 = st.x / SUB - w / 2; y0 = ROOM_Y + st.y / SUB - al_floor(st.x, st.y) / SUB - SHADOW_DY - h / 2;
    sx = st.x / SUB + hero_ox[st.dir][al_frame()]; sy = ROOM_Y + st.y / SUB - st.z / SUB - HERO_AY;
    for (r = 0; r < h; r++)                         // the floor's mean grey picks the mode
        for (c = 0; c < w; c++)
            if (mask[r] & (0x8000 >> c)) { all++; sum += ref[y0 + r][x0 + c]; }
    dark = 2 * sum >= 3 * all;
    for (r = 0; r < h; r++)
        for (c = 0; c < w; c++) {
            int x = x0 + c, y = y0 + r, g = ref[y][x], want;
            u16 b = 0x8000 >> c, up = r ? mask[r - 1] : 0, dn = r + 1 < h ? mask[r + 1] : 0;
            int inner = (up & b) && (dn & b) && (mask[r] & (b << 1)) && (mask[r] & (b >> 1));
            if (!(mask[r] & b)) continue;
            if (x - sx >= 0 && x - sx < HERO_SW && y - sy >= 0 && y - sy < HERO_SH &&
                hero_px(x - sx, y - sy) >= 0) continue;
            want = dark ? (inner ? C_BLACK : C_LGRAY) : (g < 2 ? g + 2 : 3);
            if (sw_level(x, y) != want) return -1;
            n++;
        }
    return n;
}

static void test_shadow(void)
{
    // in the air, high: the small shadow, wholly visible below the player
    sw_init(1);
    CHECK(shadow_ok(shadow1, SHADOW1_W, SHADOW1_H) > 50);     // nearly all of it
    // on the ground: always there, around the feet (partly under the sprite)
    sw_init(0);
    CHECK(shadow_ok(shadow0, SHADOW0_W, SHADOW0_H) > 10);
    // low in a jump: the big one; on level 1 it lies on the platform, not on the ground
    at(56, 24); hold(K_RIGHT, 9, 0);
    CHECK(!st.grounded);
    CHECKV(al_floor(st.x, st.y), LEVEL_Z);
    CHECK(shadow_ok(shadow0, SHADOW0_W, SHADOW0_H) > 10);
}

static void test_animation(void)
{
    int k;
    at(88, 72);
    CHECKV(al_frame(), 0);                          // standing, facing down
    al_step(K_LEFT);
    CHECKV(st.dir, 2);
    CHECKV(al_frame(), 1);
    for (k = 0; k < 5; k++) al_step(K_LEFT);
    CHECKV(al_frame(), 2);                          // a walk image every 5 steps
    for (k = 0; k < 25; k++) al_step(K_LEFT);
    CHECKV(al_frame(), 1);                          // six images, then again
    al_step(K_DOWN | K_RIGHT);
    CHECKV(st.dir, 0);                              // a diagonal faces up or down (Alundra)
    al_step(K_UP | K_LEFT);
    CHECKV(st.dir, 1);
    al_step(0);
    CHECKV(al_frame(), 0);
    CHECKV(st.dir, 1);                              // a stop keeps the facing
    for (k = 2; k < IDLE_STAND; k++) al_step(0);  // (one stop step above)
    CHECKV(al_frame(), 0);                          // the stand image 21 steps (Alundra 40 frames)
    al_step(0);
    CHECKV(al_frame(), 9);                          // breathing in, 5 steps
    for (k = 0; k < IDLE_IN; k++) al_step(0);
    CHECKV(al_frame(), 10);                         // out, 2 steps
    for (k = 0; k < IDLE_OUT; k++) al_step(0);
    CHECKV(al_frame(), 0);                          // and again
    al_step(K_A);
    CHECKV(al_frame(), 7);                          // take-off
    al_step(K_A);
    CHECKV(al_frame(), 8);                          // in the air
}

static void test_hidden_behind(void)
{
    // the wall at column 4 row 5 (3 levels): top face on screen rows 59..74, front face below;
    // a player in row 4 right behind it (feet y 78) is hidden by it: the wall's pixels are the
    // same as without the player
    static u8 ref[16][36];
    int x, y, diff = 0, n, all, w0 = ROOM_Y + 5 * TILE - LEVEL_PX(3);
    sw_init(0);
    st.x = 24 * SUB; st.y = 40 * SUB;
    sw_step(0);
    for (y = 0; y < 36; y++)
        for (x = 0; x < 16; x++) ref[x][y] = sw_level(64 + x, w0 + y);
    st.x = 72 * SUB; st.y = 78 * SUB; st.z = 0;
    sw_step(0);
    for (y = 0; y < 36; y++)
        for (x = 0; x < 16; x++) diff += ref[x][y] != sw_level(64 + x, w0 + y);
    CHECKV(diff, 0);
    n = hero_shown(&all);
    CHECK(n > 0 && n < all / 2);                    // only the head shows above the wall
    st.y = 54 * SUB;                                // further back (sprite above row 60): all shows
    sw_step(0);
    n = hero_shown(&all);
    CHECKV(n, all);
}

// ---------------------------------------------------------------- milestone 9: the village
// The whole village of Inoa (gfx.h, tools/extract.py), the game's image, the camera following.
// keys/village.txt from the start (scenario 7, the doorstep of the house Alundra leaves, level
// 9 = the game's 10 units): down the road, left, down the stairs (9 -> 8 -> 6) walking, then
// down a second flight (6 -> 4 -> 2), off a retaining wall (2 -> 0, a fall), and right along
// the lower street to a house
static void test_village(void)
{
    static SwScript sc;
    static const struct { u16 f; u16 x, y; u8 z; } C[] = {
        { 105, 233, 210, 75 }, { 115, 219, 217, 67 }, { 130, 183, 217, 50 }, { 170, 181, 280, 16 },
        { 185, 181, 300, 0 }, { 259, 299, 300, 0 } };
    u16 f;
    u8 c = 0, air = 0;
    int n, all;
    CHECKV(sw_load_script(&sc, "keys/village.txt"), 0);
    sw_init(7);
    CHECK(world == &worlds[W_VILLAGE]);
    CHECK(st.grounded);
    CHECKV(st.z, 9 * LEVEL_Z);                 // the upper road (the game's 10 units)
    CHECK(al_free(st.x, st.y, st.z));
    for (f = 0; f < 260; f++) {
        sw_step(sw_script_keys(&sc, f));
        if (f < 177) air += !st.grounded;          // stairs walked: never in the air (the fall: 177)
        if (c < sizeof C / sizeof C[0] && f == C[c].f) {
            CHECKV(st.x / SUB, C[c].x);
            CHECKV(st.y / SUB, C[c].y);
            CHECKV(st.z / SUB, C[c].z);
            c++;
        }
        if (f == 225) {                             // the lower street: scrolled, the player
            CHECK(al_camx > 0 && al_camy > 0);      // wholly in view
            n = hero_shown(&all);
            CHECKV(n, all);
        }
    }
    CHECKV(c, sizeof C / sizeof C[0]);
    CHECKV(air, 0);
    n = hero_shown(&all);                           // at the end, behind a house: hidden
    CHECK(n < all / 2);
    al_start();                                     // back to the test room within the run
    sw_step(0);
    CHECK(world == &worlds[W_TEST]);
    CHECKV(al_camx, 0);
    CHECKV(al_camy, 0);
}

// The houses' roofs are floors (the game: dropped on one, the player stands and walks on it):
// the roof of the house next to the start (level 14, its eaves at 9) is walked on, shows the
// player whole, cannot be walked onto from the road (+5) and is walked off (a fall to 9)
static void test_roof(void)
{
    int n, all;
    sw_init(7);
    st.x = (26 * 16 + 8) * SUB;                     // tile (26, 7), on the ridge
    st.y = (7 * 16 + 8) * SUB;
    st.z = al_floor(st.x, st.y);
    CHECKV(st.z, 14 * LEVEL_Z);
    CHECK(al_free(st.x, st.y, st.z));
    steps(K_DOWN, 12);                              // along the roof (tiles 26, 7-8)
    CHECKV(st.z, 14 * LEVEL_Z);
    CHECK(st.grounded);
    sw_step(0);
    n = hero_shown(&all);                           // on top: nothing of the house hides him
    CHECKV(n, all);
    steps(K_UP, 40);                                // off its back onto the road (row 5, level 9)
    CHECKV(st.z, 9 * LEVEL_Z);
    CHECK(st.grounded);
    steps(K_DOWN, 40);                              // the road to the roof: +5, blocked
    CHECKV(st.z, 9 * LEVEL_Z);
    CHECK(st.y / SUB < 6 * 16 + 8);
}

int main(int argc, char **argv)
{
    if (argc > 3 && !strcmp(argv[1], "--play")) {   // --play KEYS N [SCENARIO]: position every step
        static SwScript sc;
        int f;
        if (sw_load_script(&sc, argv[2])) return 1;
        sw_init(argc > 4 ? atoi(argv[4]) : 0);
        for (f = 0; f < atoi(argv[3]); f++) {
            sw_step(sw_script_keys(&sc, f));
            printf("%3d x%3d y%3d z%2d %c\n", f, st.x / SUB, st.y / SUB, st.z / SUB, st.grounded ? 'G' : 'A');
        }
        return 0;
    }
    if (argc > 1 && !strcmp(argv[1], "--trace")) {
        int n;
        at(88, 72);
        for (n = 0; n < 14; n++) {
            al_step(K_A);
            printf("%2d z=%5.2f vz=%4d %s\n", n + 1, st.z / 16.0, st.vz, st.grounded ? "ground" : "air");
        }
        return 0;
    }
    test_walk();
    test_jump();
    test_air_control();
    test_fall_speed();
    test_floor();
    test_land_on_heights();
    test_walk_off();
    test_ledges();
    test_drops();
    test_corners();
    test_air_collision();
    test_route();
    test_screen();
    test_animation();
    test_shadow();
    test_hidden_behind();
    test_village();
    test_roof();
    printf(fails ? "%d FAILED\n" : "alundra: all tests pass\n", fails);
    return fails != 0;
}
