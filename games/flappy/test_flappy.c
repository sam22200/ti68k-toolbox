// Flappy unit and integration tests: sw_step + asserts, no window.
#include <stdio.h>
#include <string.h>
#include "../../runtime/platform-sw/rt_sw.h"
#include "flappy.h"

static int fails;
#define SCRIPT_ROUNDS 3                // pinned result of the scripted run
#define SCRIPT_MODE S_PLAY
#define SCRIPT_Y (-2683)
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static u16 run_until(u8 mode, u16 max)         // frames with no key until st.mode == mode
{
    u16 k = 0;
    while (st.mode != mode && k < max) { sw_step(0); k++; }
    return k;
}

static void tap(void) { sw_step(K_A); sw_step(0); }   // press then release

static int pipes_ok(void)                      // ordering, spacing, opening range
{
    u16 i;
    for (i = 0; i < NPIPE; i++) {
        if (st.pipe[i].top < GAP_MIN || st.pipe[i].top >= GAP_MIN + GAP_RANGE) return 0;
        if (st.pipe[i].top + GAP >= GROUND_Y) return 0;
        if (i && st.pipe[i].x - st.pipe[i - 1].x != PIPE_DIST) return 0;
    }
    return 1;
}

int main(void)
{
    u16 k, n;
    s16 top;
    Flappy a;

    // states: title → ready → play, keys are presses (held keys do nothing)
    sw_init(0);
    CHECK(st.mode == S_TITLE);
    for (k = 0; k < 20; k++) sw_step(0);
    CHECK(st.mode == S_TITLE);
    sw_step(K_A);
    CHECK(st.mode == S_READY && st.score == 0 && st.y == START_Y << 8);
    sw_step(K_A);                              // still held: no start
    CHECK(st.mode == S_READY);
    sw_step(0); sw_step(K_UP);
    CHECK(st.mode == S_PLAY && st.vy == FLAP_VY && st.angle == ANG_FLAP);

    // flap arc: rises ~9.8 px (44.8 px upstream x 0.22), apex after ~9 frames
    top = st.y;
    for (k = 0, n = 0; k < 30; k++) { sw_step(0); if (st.y < top) { top = st.y; n = k + 1; } }
    CHECK((START_Y << 8) - top >= 9 << 8 && (START_Y << 8) - top <= 10 << 8);
    CHECK(n >= 8 && n <= 10);
    CHECK(st.angle == ANG_MAX);

    // free fall to the ground → dead → over, best score updated
    sw_init(1); tap(); tap();
    CHECK(run_until(S_DEAD, 100) < 40);
    sw_step(0);
    CHECK(sw_level(80, 50) == 0 && sw_level(5, 95) == 0);   // hit flash: white screen
    CHECK(run_until(S_OVER, 100) < 20);
    CHECK((st.y >> 8) + BIRD_H == GROUND_Y && st.angle == ANG_MAX);
    for (k = 0; k < (OVER_INPUT - 2) / 2; k++) tap();        // presses ignored during the show
    CHECK(st.mode == S_OVER);
    tap(); tap();
    CHECK(st.mode == S_READY);

    // scroll: 1.5 px per frame
    sw_init(2);
    a = st;
    sw_step(0); sw_step(0);
    CHECK(a.pipe[0].x - st.pipe[0].x == 3 && (u8)(st.land - a.land) == 3);

    // ceiling clamp (pipes moved away)
    sw_init(2);
    for (k = 0; k < NPIPE; k++) st.pipe[k].x = 1000;
    for (k = 0; k < 60; k++) flappy_play_step(1);
    CHECK(st.y == CEIL_FP && st.mode == S_PLAY);

    // scenario 2: centred in the opening, the autopilot passes pipe[0] and scores
    sw_init(2);
    st.auto_pilot = 1;
    for (k = 0; k < 40; k++) sw_step(0);
    CHECK(st.mode == S_PLAY && st.score == 1);

    // scenario 3: too high, hits pipe[0]; scenario 6: hits the ground
    sw_init(3);
    CHECK(run_until(S_DEAD, 60) < 30 && st.score == 0);
    sw_init(6);
    CHECK(run_until(S_DEAD, 60) < 8);

    // scenario 4: the 40th point gives platinum, and a new best
    CHECK(flappy_medal(9) == 0 && flappy_medal(10) == 1 && flappy_medal(19) == 1);
    CHECK(flappy_medal(20) == 2 && flappy_medal(30) == 3 && flappy_medal(40) == 4 && flappy_medal(999) == 4);
    sw_init(4);
    st.auto_pilot = 1;
    for (k = 0; k < 40; k++) sw_step(0);
    CHECK(st.score == 40);
    st.auto_pilot = 0;
    run_until(S_OVER, 300);
    CHECK(st.best == 40 && st.newbest && flappy_medal(st.score) == 4);

    // scenario 5: over screen, a press restarts and keeps the best score
    sw_init(5);
    tap();
    CHECK(st.mode == S_READY && st.score == 0 && st.best == 30);

    // integration: endless autopilot, invariants every frame, deterministic
    rt_seed = 1234;
    sw_init(7);
    for (k = 0, n = 0; k < 3000; k++) { sw_step(0); n += !pipes_ok() && st.mode == S_PLAY; }
    CHECK(n == 0);
    printf("autopilot: 3000 frames, score %u best %u mode %u\n", st.score, st.best, st.mode);
    CHECK(st.score >= 20);
    a = st;
    rt_seed = 1234;
    sw_init(7);
    for (k = 0; k < 3000; k++) sw_step(0);
    CHECK(memcmp(&a, &st, sizeof(st)) == 0);

    // integration: a human-like script from the title (tap every 14 frames)
    rt_seed = 99;
    sw_init(0);
    for (k = 0, n = 0; k < 600; k++) { u8 m = st.mode; sw_step(k % 12 == 0 ? K_A : 0); n += m != S_READY && st.mode == S_READY; }
    printf("script: 600 frames, %u rounds, mode %u y %d\n", n, st.mode, st.y);
    CHECK(n == SCRIPT_ROUNDS && st.mode == SCRIPT_MODE && st.y == SCRIPT_Y);   // pinned
    CHECK(sw_step(K_ESC) == 0);

    printf(fails ? "%d FAILED\n" : "all tests passed\n", fails);
    return fails != 0;
}
