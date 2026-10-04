// MGS milestone 1 tests, headless (sw_step, no window): the engine against the numbers measured
// on the ROM (README.md § Measured), then whole runs from key scripts (keys/*.txt).
//   ./mgs_test                     every test
//   ./mgs_test --trace SCRIPT N    state per frame for N frames (to write a key script)
//   ./mgs_test --hash SCRIPT N     the per-frame state hash (make tihash: TI = PC)
//   ./mgs_test --shot SCRIPT N F   the screen after N frames as F (art review, by hand)
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../../runtime/platform-sw/rt_sw.h"
#include "mgs.h"
#include "level.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)
#define CHECKV(a, b) do { long _a = (a), _b = (b); if (_a != _b) { printf("FAIL %s:%d: %s = %ld, want %ld\n", __FILE__, __LINE__, #a, _a, _b); fails++; } } while (0)

static void steps(u8 keys, int n) { while (n--) mgs_step(keys); }

// the guard out of the way: far below, waiting (vision tests set it themselves)
static void no_guard(void) { st.gy = 230; st.gstate = G_WAIT_BOTTOM; st.gtimer = 0; }

static void test_walls(void)
{
    // from the start, each direction held 100 GB frames: where the ROM stops Snake
    static const struct { u8 key; u8 x, y; } T[] = {
        { K_LEFT, 22, 192 }, { K_DOWN, 32, 201 }, { K_UP, 32, 180 }, { K_RIGHT, 91, 192 } };
    unsigned k;
    for (k = 0; k < 4; k++) {
        mgs_start(); no_guard();
        steps((u8)T[k].key, 100);
        CHECKV(st.sx, T[k].x);
        CHECKV(st.sy, T[k].y);
    }
}

static void test_speed(void)
{
    u8 x;
    int n;
    mgs_start(); no_guard();
    for (n = 0; n < 20 && st.sx == LV1_START_X; n++) mgs_step(K_RIGHT);
    CHECK(n >= 6 && n <= 8);            // ROM: the first pixel on the 8th frame (a 90° turn)
    x = st.sx;
    steps(K_RIGHT, 30);
    CHECKV(st.sx - x, 30);              // 1 px per frame
    // diagonal in the open: 2 px of 3 frames on both axes (ROM: 40,200 -> 64,182 region)
    mgs_start(); no_guard();
    st.sx = 40; st.sy = 200; st.sdir = 1;
    steps(K_UP | K_RIGHT, 3);
    x = st.sx;
    steps(K_UP | K_RIGHT, 12);
    CHECKV(st.sx - x, 8);
    // along a wall the free axis goes at full speed (ROM: y stuck at 182, x +1 per frame)
    mgs_start(); no_guard();
    st.sx = 40; st.sy = 181; st.sdir = 1;
    steps(K_UP | K_RIGHT, 2);
    x = st.sx;
    steps(K_UP | K_RIGHT, 10);
    CHECKV(st.sy, 180);
    CHECKV(st.sx - x, 10);
    // a 180° turn goes clockwise (ROM: up then down shows 1, 2, 3, 4)
    mgs_start(); no_guard();
    st.sdir = 0;
    steps(K_DOWN, 2);
    CHECKV(st.sdir, 1);
}

static void test_patrol(void)
{
    // the guard's timeline from the level start (ROM, 1,300 frames: README § The guard)
    static const struct { int step; u8 state, y; } T[] = {
        { 159, G_WAIT_TOP, 32 }, { 161, G_WALK_DOWN, 32 }, { 425, G_WAIT_BOTTOM, 120 },
        { 710, G_WAIT_BOTTOM, 120 }, { 714, G_TURN_BOTTOM, 120 }, { 740, G_WALK_UP, 119 },
        { 1001, G_WAIT_TOP, 32 }, { 1287, G_WAIT_TOP, 32 }, { 1290, G_WALK_DOWN, 32 } };
    unsigned k;
    int s = 0;
    mgs_start();
    st.sx = 20; st.sy = 200;            // the corner the extraction parked Snake in
    for (k = 0; k < sizeof T / sizeof T[0]; k++) {
        while (s < T[k].step) { mgs_step(0); s++; }
        CHECKV(st.mode, M_PLAY);
        CHECKV(st.gstate, T[k].state);
        CHECK(st.gy >= T[k].y - 1 && st.gy <= T[k].y + 1);
    }
}

static void test_vision(void)
{
    // facing down at (80, 92), on screen: boxes measured with Snake poked on a 2-pixel grid
    static const struct { s8 dx, dy; u8 seen; } D[] = {
        { 0, 0, 1 }, { 0, -2, 0 }, { -4, 6, 1 }, { 4, 6, 0 }, { -12, 8, 1 }, { 12, 8, 0 },
        { -20, 24, 1 }, { 18, 46, 1 }, { 20, 30, 0 }, { 0, 54, 1 }, { 0, 56, 0 }, { 14, 50, 0 } };
    // facing right at (80, 32): the measured bands (contact row, +-8, +-16, the top tip)
    static const struct { s8 dx, dy; u8 seen; } R[] = {
        { -4, 0, 1 }, { -6, 0, 0 }, { 50, 0, 1 }, { 52, 0, 0 }, { 4, -12, 1 }, { 2, -10, 0 },
        { 20, 18, 1 }, { 18, 18, 0 }, { 28, -28, 1 }, { 28, 20, 0 }, { 44, -14, 0 } };
    unsigned k;
    for (k = 0; k < sizeof D / sizeof D[0]; k++) {
        mgs_start();
        st.gx = 80; st.gy = 92; st.gdir = 4; st.camy = 60;
        st.sx = 80 + D[k].dx; st.sy = 92 + D[k].dy;
        if (mgs_sees() != D[k].seen) { printf("FAIL vision down %d,%d\n", D[k].dx, D[k].dy); fails++; }
        st.gdir = 0; st.sy = 92 - D[k].dy - 1;      // up mirrors down
        if (mgs_sees() != D[k].seen) { printf("FAIL vision up %d,%d\n", D[k].dx, D[k].dy); fails++; }
    }
    for (k = 0; k < sizeof R / sizeof R[0]; k++) {
        mgs_start();
        st.gx = 80; st.gy = 32; st.gdir = 2; st.camy = 0;
        st.sx = 80 + R[k].dx; st.sy = 32 + R[k].dy;
        if (mgs_sees() != R[k].seen) { printf("FAIL vision right %d,%d\n", R[k].dx, R[k].dy); fails++; }
    }
    // off screen, a guard sees nothing (ROM: detection starts when he scrolls in)
    mgs_start();
    st.gx = 80; st.gy = 40; st.gdir = 4; st.camy = 100;
    st.sx = 80; st.sy = 60;
    CHECKV(mgs_sees(), 0);
}

static void test_goal_and_alert(void)
{
    int n;
    sw_init(1);                         // next to the goal: walk right
    for (n = 0; n < 40 && st.mode == M_PLAY; n++) sw_step(K_RIGHT);
    CHECKV(st.mode, M_CLEAR);
    CHECK(st.sx >= LV1_GOAL_X && st.sx < LV1_GOAL_X + 20);
    sw_init(2);                         // the guard walks down at Snake in the corridor
    for (n = 0; n < 100 && st.mode == M_PLAY; n++) sw_step(0);
    CHECKV(st.mode, M_SPOTTED);
    for (n = 0; n < 40; n++) sw_step(0);
    CHECKV(st.mode, M_FAILED);
    sw_step(K_A); sw_step(0);
    for (n = 0; n < 20; n++) sw_step(0);
    sw_step(K_A);
    CHECKV(st.mode, M_PLAY);            // try again: the level from its start
    CHECKV(st.sx, LV1_START_X);
    sw_init(3);                         // hidden in the side room: the guard walks past
    for (n = 0; n < 200; n++) sw_step(0);
    CHECKV(st.mode, M_PLAY);
}

// a key script played from scenario 0; returns the frame its run ends on (mode leaves PLAY)
static int play(const char *path, int frames, u8 *mode)
{
    static SwScript s;
    int f;
    if (sw_load_script(&s, path)) { printf("FAIL cannot read %s\n", path); fails++; return -1; }
    sw_init(0);
    for (f = 0; f < frames && st.mode == M_PLAY; f++) sw_step(sw_script_keys(&s, (u16)f));
    *mode = st.mode;
    return f;
}

static void test_scripts(void)
{
    u8 mode;
    int f = play("keys/win.txt", 2000, &mode);
    CHECKV(mode, M_CLEAR);
    printf("keys/win.txt: clear at frame %d (%d GB frames)\n", f, st.steps);
    f = play("keys/spotted.txt", 2000, &mode);
    CHECKV(mode, M_SPOTTED);
    printf("keys/spotted.txt: spotted at frame %d\n", f);
}

static void test_render(void)
{
    // the data file is there and the level draws: the floor under Snake is light grey
    sw_init(0);
    sw_step(0);
    CHECK(mgs_data != RT_NULL);
    CHECKV(sw_level(40, 200 - st.camy), 1);
}

int main(int argc, char **argv)
{
    if (argc >= 4 && (!strcmp(argv[1], "--trace") || !strcmp(argv[1], "--hash") || !strcmp(argv[1], "--shot"))) {
        static SwScript s;
        int f, n = atoi(argv[3]);
        if (sw_load_script(&s, argv[2])) { fprintf(stderr, "cannot read %s\n", argv[2]); return 1; }
        sw_init(0);
        for (f = 0; f < n; f++) {
            sw_step(sw_script_keys(&s, (u16)f));
            if (!strcmp(argv[1], "--trace"))
                printf("%d mode %d snake %d,%d dir %d guard %d,%d dir %d state %d cam %d\n", f, st.mode,
                       st.sx, st.sy, st.sdir, st.gx, st.gy, st.gdir, st.gstate, st.camy);
            if (!strcmp(argv[1], "--hash")) printf("(0x%lx)\n", (unsigned long)mgs_hash());   /* ti-cycles format */
        }
        if (!strcmp(argv[1], "--shot")) return sw_write_png(argc > 4 ? argv[4] : "shot.png", 3);
        return 0;
    }
    test_walls();
    test_speed();
    test_patrol();
    test_vision();
    test_goal_and_alert();
    test_scripts();
    test_render();
    printf(fails ? "%d FAILED\n" : "all tests passed\n", fails);
    return fails != 0;
}
