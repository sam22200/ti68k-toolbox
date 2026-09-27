// Demo unit tests: drive frames with sw_step and check the state, no window, no screenshot.
#include <stdio.h>
#include "../platform-sw/rt_sw.h"
#include "demo.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

int main(void)
{
    u16 k;
    sw_init(0);
    CHECK(st.x == 48 && st.y == 48 && st.steps == 0);
    for (k = 0; k < 10; k++) sw_step(K_RIGHT);
    CHECK(st.x == 68 && st.steps == 10);
    for (k = 0; k < 10; k++) sw_step(K_RIGHT | K_A);             // 2nd = double speed
    CHECK(st.x == 108);
    for (k = 0; k < 200; k++) sw_step(K_LEFT | K_UP);            // clamped inside the walls
    CHECK(st.x == MIN_X && st.y == MIN_Y);

    sw_step(K_DIGIT(9));                                         // teleport: bottom-right zone
    CHECK(st.x == MAX_X && st.y == MAX_Y);
    sw_step(K_DIGIT(9));                                         // held: no second jump
    sw_step(K_DIGIT(5));
    CHECK(st.x == (MIN_X + MAX_X) / 2 && st.y == (MIN_Y + MAX_Y) / 2);

    sw_init(1);                                                  // injection door
    CHECK(st.x == MAX_X && st.y == MAX_Y && st.steps == 0);
    CHECK(demo_cam(st.x, RT_W, MAP_W * 16) == MAP_W * 16 - RT_W);
    CHECK(demo_cam(0, RT_W, MAP_W * 16) == 0);
    sw_init(2);
    CHECK(st.steps == 500);
    CHECK(sw_step(0) == 1 && sw_step(K_ESC) == 0);

    printf(fails ? "%d FAILED\n" : "all tests passed\n", fails);
    return fails != 0;
}
