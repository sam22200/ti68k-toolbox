// Platform-independent part of the runtime: shared state, PRNG, rectangle clipping, tile map
// camera clamp. The platforms provide rt_fill (clipped rectangle) and the rest of rt.h.
#include "rt.h"

u32 rt_keys, rt_prev;
u16 rt_frame;
u16 rt_seed = 1;
void *rt_state;
u16 rt_state_size;
void *rt_light, *rt_dark;

void rt_fill(u16 x1, u16 y1, u16 x2, u16 y2, u8 color);   // inclusive, inside the screen

// wyhash16 (performance §4): one mulu.w
u16 rt_rand(void)
{
    u32 h;
    rt_seed += 0xfc15;
    h = (u32)rt_seed * 0x2ab;
    return (u16)(h >> 16) ^ (u16)h;
}

void draw_rect(s16 x, s16 y, s16 w, s16 h, u8 color)
{
    s16 x2 = x + w - 1, y2 = y + h - 1;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x2 > RT_W - 1) x2 = RT_W - 1;
    if (y2 > RT_H - 1) y2 = RT_H - 1;
    if (x > x2 || y > y2) return;
    rt_fill(x, y, x2, y2, color);
}

void rt_clamp_cam(const RtTilemap *m, s16 *cx, s16 *cy)
{
    s16 mx = (s16)(m->w << 4) - RT_W, my = (s16)(m->h << 4) - RT_H;
    if (*cx > mx) *cx = mx;
    if (*cx < 0) *cx = 0;
    if (*cy > my) *cy = my;
    if (*cy < 0) *cy = 0;
}
