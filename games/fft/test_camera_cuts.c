// Combat must stay isometric even when the old cinematic support is linked.
#include <stdio.h>
#include "fft.c"
#include "../../runtime/platform-sw/rt_sw.h"
static unsigned failures;
#define CHECK(v) do { if (!(v)) { printf("FAIL %d: %s\n", __LINE__, #v); failures++; } } while (0)
static void press(u32 keys) { CHECK(sw_step(keys)); CHECK(sw_step(0)); }
int main(void)
{
    Unit expected[NUNIT]; u32 seed; unsigned frame;
    sw_init(65); CHECK(!st.cenabled);
    st.acted = 1; attack(U_RAMZA, 1);
    for (frame=0; st.mode == M_ACT && frame<160; frame++) attack_step();
    CHECK(frame<160); memcpy(expected, st.unit, sizeof expected); seed = rng32;
    sw_init(65); game_render();
    press(K_DIGIT(7)); CHECK(!st.cenabled);
    press(K_A); press(K_A); CHECK(st.mode == M_AIM);
    st.cx = 6; st.cz = 5; press(K_A); CHECK(st.mode == M_ACT);
    for (frame=0; st.mode == M_ACT && frame<160; frame++) {
        CHECK(!st.perspective && !st.ptrans && !st.cphase);
        CHECK(sw_step(0));
    }
    CHECK(frame<160 && st.mode == M_MENU && !st.cphase && !st.perspective);
    CHECK(!memcmp(expected, st.unit, sizeof expected) && rng32 == seed);
    printf("camera cuts: %s; [7] cannot enable cinematics; real Attack stays ISO, exact units/rewards/RNG\n", failures ? "FAILED" : "passed");
    return failures != 0;
}
