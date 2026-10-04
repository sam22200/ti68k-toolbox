// Runtime unit tests on the PC (no window): renderer semantics pixel by pixel, input edges, input
// scripts, PRNG, state save/load, and the xcheck sums against the values printed by the
// calculator build of xcheck.c (ti_sums below, read on the Titanium, 2026-09-27).
// Run: make -C runtime test
#include <stdio.h>
#include <string.h>
#include "../platform-sw/rt_sw.h"

extern u16 xcheck_sums[8];
static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

#ifdef RT_MONO
static const u16 ti_sums[8] = { 0x2FD5, 0xA09B, 0x2526, 0x5EA2, 0x8775, 0x7145, 0x32A5, 0x2875 };   // Titanium
#else
static const u16 ti_sums[8] = { 0x749B, 0xAA9C, 0x093E, 0x87D0, 0x8404, 0x5017, 0x2970, 0x13BD };   // Titanium
#endif

static void test_render(void)
{
    static const u16 d[2] = { 0xF000, 0x8000 }, l[2] = { 0x0F00, 0x0000 }, m[2] = { 0x00FF, 0x7FFF };
    static const RtSprite s = { 16, 2, l, d, m };
    draw_clear();
    draw_rect(-10, -10, 12, 13, C_DGRAY);          // clipped to (0,0)-(1,2)
#ifdef RT_MONO
    CHECK(sw_level(0, 0) == 3 && sw_level(2, 2) == 0);   // dark grey -> black
#endif
#ifndef RT_MONO
    CHECK(sw_level(1, 2) == C_DGRAY && sw_level(2, 2) == C_WHITE && sw_level(1, 3) == C_WHITE);
    draw_rect(10, 10, 20, 2, C_LGRAY);
    draw_sprite(10, 10, &s);
    // row 0: px 0-3 dark only (mask 0) -> DGRAY; 4-7 light only -> LGRAY; 8-15 mask 1 keeps LGRAY
    CHECK(sw_level(10, 10) == C_DGRAY && sw_level(14, 10) == C_LGRAY && sw_level(18, 10) == C_LGRAY);
    // row 1: px 0 black? d=1 l=0 -> DGRAY; px 1-15 mask 1 -> background LGRAY kept
    CHECK(sw_level(10, 11) == C_DGRAY && sw_level(11, 11) == C_LGRAY);
    {
        s16 x, y, n = 0;
        draw_text(40, 40, "I", F_MEDIUM, C_BLACK);
        for (y = 40; y < 48; y++) for (x = 40; x < 46; x++) n += sw_level(x, y) == C_BLACK;
        CHECK(n >= 5 && sw_level(46, 40) == C_WHITE);
    }
#endif
    draw_sprite(-16, -2, &s);                      // fully off screen: no crash, nothing drawn
    draw_sprite(300, 300, &s);
}

static void test_input_and_script(void)
{
    static SwScript sc;
    FILE *f = fopen("/tmp/rt_test_keys.txt", "w");
    CHECK(sw_parse_keys("UP A 5 # comment") == (K_UP | K_A | K_DIGIT(5)));
    CHECK(K_DIGIT(1) == 0x10000UL && K_DIGIT(9) == 0x1000000UL);
    fputs("# test\n0 RIGHT\n10 RIGHT A\n12\n20 ESC\n", f);
    fclose(f);
    CHECK(sw_load_script(&sc, "/tmp/rt_test_keys.txt") == 0 && sc.n == 4);
    CHECK(sw_script_keys(&sc, 0) == K_RIGHT && sw_script_keys(&sc, 9) == K_RIGHT);
    CHECK(sw_script_keys(&sc, 10) == (K_RIGHT | K_A) && sw_script_keys(&sc, 15) == 0);
    CHECK(sw_script_keys(&sc, 25) == K_ESC);
    rt_keys = 0;
    rt_prev = rt_keys; rt_keys = K_A;
    CHECK(input_pressed(K_A) && input_held(K_A) && !input_released(K_A));
    rt_prev = rt_keys; rt_keys = K_A;
    CHECK(!input_pressed(K_A) && input_held(K_A));
    rt_prev = rt_keys; rt_keys = 0;
    CHECK(input_released(K_A));
}

static void test_rand(void)
{
    u16 a[4], k;
    rt_seed = 1234; for (k = 0; k < 4; k++) a[k] = rt_rand();
    rt_seed = 1234; for (k = 0; k < 4; k++) CHECK(rt_rand() == a[k]);
    CHECK(a[0] != a[1]);
}

int main(void)
{
    u16 k;
    sw_init(0);
    test_render();
    test_input_and_script();
    test_rand();

    sw_init(0);                                    // xcheck: one frame renders the scenes
    CHECK(sw_step(0) == 1);
    printf("xcheck sums:");
    for (k = 0; k < 8; k++) printf(" %u:%04X", k, xcheck_sums[k]);
    printf("\n");
    for (k = 0; k < 8; k++)
        if (ti_sums[k]) CHECK(xcheck_sums[k] == ti_sums[k]);
    CHECK(sw_save_state("/tmp/rt_test.state") == 0);
    CHECK(sw_step(K_ESC) == 0);                    // ESC press quits
    CHECK(sw_write_png("/tmp/rt_test.png", 2) == 0);
    printf(fails ? "%d FAILED\n" : "all tests passed\n", fails);
    return fails != 0;
}
