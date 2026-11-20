// Cost of each runtime primitive on the calculator: build with make bench (NAME=rbench), run
// rbenchb(N): N = 1 full tilemap (11x7 visible), 2 draw_clear, 3 draw_text small 6 chars,
// 4 draw_text medium 6 chars, 5 ten masked 16x16 sprites, 6 draw_rect 160x100, 7 ten masked 8x8,
// 8 thirteen masked 16x28 at an odd x, 9 thirteen masked 32x28, 10 twenty-six masked 16x28
// (what one wide monster body costs today against what one 32-wide call would cost),
// 11 thirteen 32x28 bodies already shifted into aligned 48-bit rows, blitted by hand,
// 12 the same with the first two words as one long, 13 the same again with an opaque middle.
#include "../core/rt.h"

static u16 which;
static const u16 tiles[32] = { 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555,
                               0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555 };
static u8 map[16 * 16];
static const RtTilemap tm = { map, 16, 16, tiles, 1 };
static const u16 d16[16] = { 0xFFFF, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001,
                             0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0xFFFF };
static const u16 m16[16] = { 0 };
static const RtSprite sp16 = { 16, 16, d16, d16, m16 };
static const u16 d28[28] = { 0xFFFF, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001,
                             0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001,
                             0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001, 0x8001,
                             0x8001, 0x8001, 0x8001, 0xFFFF };
static const u16 m28[28] = { 0 };
static const RtSprite sp16x28 = { 16, 28, d28, d28, m28 };
static const u32 d32[28] = { 0xFFFFFFFFUL, 0x80000001UL, 0x80000001UL, 0x80000001UL,
    0x80000001UL, 0x80000001UL, 0x80000001UL, 0x80000001UL, 0x80000001UL, 0x80000001UL,
    0x80000001UL, 0x80000001UL, 0x80000001UL, 0x80000001UL, 0x80000001UL, 0x80000001UL,
    0x80000001UL, 0x80000001UL, 0x80000001UL, 0x80000001UL, 0x80000001UL, 0x80000001UL,
    0x80000001UL, 0x80000001UL, 0x80000001UL, 0x80000001UL, 0x80000001UL, 0xFFFFFFFFUL };
static const u32 m32[28] = { 0 };
static const RtSprite sp32x28 = { 32, 28, d32, d32, m32 };
static const u8 d8[8] = { 0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF }, m8[8] = { 0 };
static const RtSprite sp8 = { 8, 8, d8, d8, m8 };

// A 32-wide sprite at an arbitrary x spans three words once shifted. Keeping the
// shifted rows and blitting them on word boundaries trades the per-row shift, which
// dominates ExtGraph's clipped routines, for one read-mask-or per word per plane.
static u16 pre[3][28][3];                      // light, dark, mask (1 = transparent)
static void blit_shifted(s16 x, s16 y, u8 h)
{
    u16 *l = (u16 *)((u8 *)rt_light + y * RT_PBYTES + ((x >> 3) & ~1));
    u16 *d = (u16 *)((u8 *)rt_dark  + y * RT_PBYTES + ((x >> 3) & ~1));
    u8 r;
    for (r = 0; r < h; r++) {
        u16 m0 = pre[2][r][0], m1 = pre[2][r][1], m2 = pre[2][r][2];
        l[0] = (u16)((l[0] & m0) | pre[0][r][0]); l[1] = (u16)((l[1] & m1) | pre[0][r][1]);
        l[2] = (u16)((l[2] & m2) | pre[0][r][2]);
        d[0] = (u16)((d[0] & m0) | pre[1][r][0]); d[1] = (u16)((d[1] & m1) | pre[1][r][1]);
        d[2] = (u16)((d[2] & m2) | pre[1][r][2]);
        l += RT_PBYTES / 2; d += RT_PBYTES / 2;
    }
}

// The three words are a long and a word: one 32-bit read-mask-write instead of two.
static u32 prel[3][28][2];                     // [plane][row] = { long, word in the high half }
static void blit_shifted_long(s16 x, s16 y, u8 h)
{
    u8 *l = (u8 *)rt_light + y * RT_PBYTES + ((x >> 3) & ~1);
    u8 *d = (u8 *)rt_dark  + y * RT_PBYTES + ((x >> 3) & ~1);
    u8 r;
    for (r = 0; r < h; r++) {
        u32 ml = prel[2][r][0];u16 mw = (u16)prel[2][r][1];
        *(u32 *)l = (*(u32 *)l & ml) | prel[0][r][0];
        *(u16 *)(l + 4) = (u16)((*(u16 *)(l + 4) & mw) | (u16)prel[0][r][1]);
        *(u32 *)d = (*(u32 *)d & ml) | prel[1][r][0];
        *(u16 *)(d + 4) = (u16)((*(u16 *)(d + 4) & mw) | (u16)prel[1][r][1]);
        l += RT_PBYTES; d += RT_PBYTES;
    }
}
// A body's middle long is solid for most of its rows: write it, never read it.
static void blit_shifted_solid(s16 x, s16 y, u8 h)
{
    u8 *l = (u8 *)rt_light + y * RT_PBYTES + ((x >> 3) & ~1);
    u8 *d = (u8 *)rt_dark  + y * RT_PBYTES + ((x >> 3) & ~1);
    u8 r;
    for (r = 0; r < h; r++) {
        u16 mw = (u16)prel[2][r][1];
        *(u32 *)l = prel[0][r][0];
        *(u16 *)(l + 4) = (u16)((*(u16 *)(l + 4) & mw) | (u16)prel[0][r][1]);
        *(u32 *)d = prel[1][r][0];
        *(u16 *)(d + 4) = (u16)((*(u16 *)(d + 4) & mw) | (u16)prel[1][r][1]);
        l += RT_PBYTES; d += RT_PBYTES;
    }
}

void game_init(void) { }
void game_scenario(u16 n) { which = n; }
u8 game_update(void) { return 1; }

void game_render(void)
{
    s16 k;
    switch (which) {
    case 1: draw_tilemap(&tm, 5, 3); break;
    case 2: draw_clear(); break;
    case 3: draw_text(3, 3, "123456", F_SMALL, C_BLACK); break;
    case 4: draw_text(3, 3, "123456", F_MEDIUM, C_BLACK); break;
    case 5: for (k = 0; k < 10; k++) draw_sprite(k * 13 + 3, k * 7, &sp16); break;
    case 6: draw_rect(0, 0, 160, 100, C_DGRAY); break;
    case 7: for (k = 0; k < 10; k++) draw_sprite(k * 13 + 3, k * 7, &sp8); break;
    case 8: for (k = 0; k < 13; k++) draw_sprite(k * 11 + 3, k * 5, &sp16x28); break;
    case 9: for (k = 0; k < 13; k++) draw_sprite(k * 11 + 3, k * 5, &sp32x28); break;
    case 10: for (k = 0; k < 13; k++) { draw_sprite(k * 11 + 3, k * 5, &sp16x28);
                                        draw_sprite(k * 11 + 19, k * 5, &sp16x28); } break;
    case 11: for (k = 0; k < 13; k++) blit_shifted(k * 11 + 3, k * 5, 28); break;
    case 12: for (k = 0; k < 13; k++) blit_shifted_long(k * 11 + 3, k * 5, 28); break;
    case 13: for (k = 0; k < 13; k++) blit_shifted_solid(k * 11 + 3, k * 5, 28); break;
    }
}
