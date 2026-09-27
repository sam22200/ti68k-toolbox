// Conway's Game of Life on a 40x25 torus. Scenario 0 = glider (default), 1 = empty.
// ESC quits. PC: make pc && ./life_pc    tests: make test    TI: make ti → life(0)
#include <string.h>
#include "life.h"

LifeState st;
static u8 next[GH][GW];

void game_init(void)
{
    rt_state = &st;
    rt_state_size = sizeof(st);
}

void game_scenario(u16 n)
{
    static const u8 glider[5][2] = { {1, 0}, {2, 1}, {0, 2}, {1, 2}, {2, 2} };
    u16 i;
    memset(&st, 0, sizeof(st));
    if (n == 0)
        for (i = 0; i < 5; i++) st.cell[1 + glider[i][1]][1 + glider[i][0]] = 1;
}

void life_step(void)
{
    u16 x, y;
    for (y = 0; y < GH; y++) {
        const u8 *up = st.cell[y ? y - 1 : GH - 1], *me = st.cell[y], *dn = st.cell[y < GH - 1 ? y + 1 : 0];
        for (x = 0; x < GW; x++) {
            u16 l = x ? x - 1 : GW - 1, r = x < GW - 1 ? x + 1 : 0;
            u8 s = up[l] + up[x] + up[r] + me[l] + me[r] + dn[l] + dn[x] + dn[r];
            next[y][x] = s == 3 || (s == 2 && me[x]);
        }
    }
    memcpy(st.cell, next, sizeof(next));
    st.gen++;
}

u8 game_update(void)
{
    if (input_pressed(K_ESC)) return 0;
    if (++st.tick >= GEN_FRAMES) { st.tick = 0; life_step(); }
    return 1;
}

void game_render(void)
{
    u16 x, y;
    draw_clear();
    for (y = 0; y < GH; y++)
        for (x = 0; x < GW; x++)
            if (st.cell[y][x]) draw_rect(x * CELL, y * CELL, CELL - 1, CELL - 1, C_BLACK);
}
