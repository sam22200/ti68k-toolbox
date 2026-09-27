// Runtime demo: a masked 16x16 sprite walks over a 24x14 tile map with a clamped camera.
// Arrows move (2nd = faster), 1..9 teleport to a 3x3 grid of map zones, ESC quits.
// Injection door: scenario 1 = bottom-right corner, scenario 2 = centre with 500 steps.
// PC: make pc && ./demo_pc [--scenario 1]    tests: make test    TI: make ti → demo.89z
#include "../core/rt.h"
#include "demo.h"

DemoState st;

static const u16 tiles[3 * 32] = {   // (dark, light) row pairs
    // 0: floor, light dots
    0, 0x0000, 0, 0x0000, 0, 0x0000, 0, 0x0100, 0, 0x0000, 0, 0x0000, 0, 0x0000, 0, 0x0000,
    0, 0x0000, 0, 0x0000, 0, 0x0000, 0, 0x1000, 0, 0x0000, 0, 0x0000, 0, 0x0000, 0, 0x0000,
    // 1: wall, black bricks
    0xFFFF, 0xFFFF, 0x8080, 0x8080, 0x8080, 0x8080, 0x8080, 0x8080, 0xFFFF, 0xFFFF, 0x0808, 0x0808, 0x0808, 0x0808, 0x0808, 0x0808,
    0xFFFF, 0xFFFF, 0x8080, 0x8080, 0x8080, 0x8080, 0x8080, 0x8080, 0xFFFF, 0xFFFF, 0x0808, 0x0808, 0x0808, 0x0808, 0x0808, 0x0808,
    // 2: grass, light grey
    0x0000, 0xFFFF, 0x2004, 0xFFFF, 0x0000, 0xFFFF, 0x0240, 0xFFFF, 0x0000, 0xFFFF, 0x4002, 0xFFFF, 0x0000, 0xFFFF, 0x0000, 0xFFFF,
    0x0420, 0xFFFF, 0x0000, 0xFFFF, 0x0000, 0xFFFF, 0x2004, 0xFFFF, 0x0000, 0xFFFF, 0x0240, 0xFFFF, 0x0000, 0xFFFF, 0x0000, 0xFFFF,
};
static u8 map[MAP_W * MAP_H];
static const RtTilemap tm = { map, MAP_W, MAP_H, tiles, 3 };

static const u16 hero_d[16] = { 0x07E0, 0x0FF0, 0x1818, 0x1BD8, 0x1818, 0x0FF0, 0x07E0, 0x3FFC,
                                0x7FFE, 0x6FF6, 0x6FF6, 0x0FF0, 0x0E70, 0x0C30, 0x0C30, 0x1C38 };
static const u16 hero_l[16] = { 0x0000, 0x0000, 0x07E0, 0x0420, 0x07E0, 0x0000, 0x0000, 0x0000,
                                0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 };
static const u16 hero_m[16] = { 0xF81F, 0xF00F, 0xE007, 0xE007, 0xE007, 0xF00F, 0xF81F, 0xC003,
                                0x8001, 0x8001, 0x8001, 0xF00F, 0xF00F, 0xF3CF, 0xF3CF, 0xE3C7 };
static const RtSprite hero = { 16, 16, hero_l, hero_d, hero_m };

void game_init(void)
{
    u16 x, y;
    for (y = 0; y < MAP_H; y++)
        for (x = 0; x < MAP_W; x++)
            map[y * MAP_W + x] = x == 0 || y == 0 || x == MAP_W - 1 || y == MAP_H - 1 ? 1 : (x ^ y) % 5 == 0 ? 2 : 0;
    rt_state = &st;
    rt_state_size = sizeof(st);
}

void game_scenario(u16 n)
{
    st.steps = 0;
    st.x = 3 * 16; st.y = 3 * 16;
    if (n == 1) { st.x = MAX_X; st.y = MAX_Y; }
    if (n == 2) { st.x = (MAP_W * 16 - 16) / 2; st.y = (MAP_H * 16 - 16) / 2; st.steps = 500; }
}

u8 game_update(void)
{
    s16 v = input_held(K_A) ? 4 : 2, ox = st.x, oy = st.y;
    u32 d = rt_keys & K_DIGITS;
    if (input_pressed(K_ESC)) return 0;
    if (input_held(K_LEFT)) st.x -= v;
    if (input_held(K_RIGHT)) st.x += v;
    if (input_held(K_UP)) st.y -= v;
    if (input_held(K_DOWN)) st.y += v;
    if (st.x < MIN_X) st.x = MIN_X; else if (st.x > MAX_X) st.x = MAX_X;
    if (st.y < MIN_Y) st.y = MIN_Y; else if (st.y > MAX_Y) st.y = MAX_Y;
    if (d & ~(rt_prev & K_DIGITS)) {               // digit n: zone (n-1)%3, (n-1)/3 of the map
        u16 n = 1;
        while (!(d & K_DIGIT(n))) n++;
        st.x = MIN_X + (MAX_X - MIN_X) * ((n - 1) % 3) / 2;
        st.y = MIN_Y + (MAX_Y - MIN_Y) * ((n - 1) / 3) / 2;
    }
    if (st.x != ox || st.y != oy) st.steps++;
    return 1;
}

s16 demo_cam(s16 p, s16 view, s16 size)        // centre on p, clamped to the map
{
    s16 c = p + 8 - view / 2;
    if (c > size - view) c = size - view;
    if (c < 0) c = 0;
    return c;
}

void game_render(void)
{
    s16 cx = demo_cam(st.x, RT_W, MAP_W * 16), cy = demo_cam(st.y, RT_H, MAP_H * 16);
    char s[16];
    u16 v = st.steps, k = 6;
    draw_tilemap(&tm, cx, cy);
    draw_sprite(st.x - cx, st.y - cy, &hero);
    s[k] = 0;
    do s[--k] = '0' + v % 10; while ((v /= 10) && k);
    draw_rect(0, 0, 30, 7, C_WHITE);
    draw_text(1, 1, s + k, F_SMALL, C_BLACK);
}
