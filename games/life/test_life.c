// Life unit tests: the glider moves one cell diagonally every 4 generations.
#include <stdio.h>
#include <string.h>
#include "../../runtime/platform-sw/rt_sw.h"
#include "life.h"

static int fails;
#define CHECK(c) do { if (!(c)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

int main(void)
{
    LifeState a;
    u16 k, x, y, same = 1;
    sw_init(0);
    a = st;
    for (k = 0; k < 4; k++) life_step();
    for (y = 0; y < GH; y++)
        for (x = 0; x < GW; x++)
            same &= st.cell[(y + 1) % GH][(x + 1) % GW] == a.cell[y][x];
    CHECK(same && st.gen == 4);
    for (k = 0; k < 4 * GW * GH; k++) life_step();              // wraps: still 5 live cells
    for (k = 0, y = 0; y < GH; y++) for (x = 0; x < GW; x++) k += st.cell[y][x];
    CHECK(k == 5);
    sw_init(1);
    CHECK(sw_step(0) == 1 && sw_step(K_ESC) == 0);
    printf(fails ? "%d FAILED\n" : "all tests passed\n", fails);
    return fails != 0;
}
