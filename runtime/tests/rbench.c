// Cost of each runtime primitive on the calculator: build with make bench (NAME=rbench), run
// rbenchb(N): N = 1 full tilemap (11x7 visible), 2 draw_clear, 3 draw_text small 6 chars,
// 4 draw_text medium 6 chars, 5 ten masked 16x16 sprites, 6 draw_rect 160x100, 7 ten masked 8x8.
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
static const u8 d8[8] = { 0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF }, m8[8] = { 0 };
static const RtSprite sp8 = { 8, 8, d8, d8, m8 };

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
    }
}
