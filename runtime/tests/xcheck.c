// Cross-check "game": draws NSC fixed scenes (rectangles, masked and opaque sprites 8/16/32,
// both fonts, tile maps, all clipped at every edge), takes a Fletcher-16 of the visible planes
// after each, then shows the sums as text. The PC unit test (test_runtime.c) and the calculator
// build must print the same numbers: the software renderer is then bit-exact with ExtGraph/AMS.
// TI: ti-cc -o xcheck xcheck.c ../core/rt_core.c ../platform-ti68k/rt_ti.c   (-DRT_MONO: mono)
#include "../core/rt.h"

#define NSC 8
u16 xcheck_sums[NSC];
static u8 done;

static const u8 s8_d[6] = { 0x3C, 0x42, 0xA5, 0x81, 0xDB, 0x7E };
static const u8 s8_l[6] = { 0x00, 0x3C, 0x5A, 0x7E, 0x24, 0x00 };
static const u8 s8_m[6] = { 0xC3, 0x81, 0x00, 0x00, 0x00, 0x81 };
static const u16 s16_d[10] = { 0xF00F, 0x8001, 0x9FF9, 0x9009, 0x93C9, 0x9249, 0x93C9, 0x9009, 0x9FF9, 0xF00F };
static const u16 s16_l[10] = { 0x0FF0, 0x7FFE, 0x6006, 0x6FF6, 0x6C36, 0x6DB6, 0x6C36, 0x6FF6, 0x6006, 0x0FF0 };
static const u16 s16_m[10] = { 0x0000, 0x0000, 0x0000, 0x0660, 0x0C30, 0x0000, 0x0C30, 0x0660, 0x0000, 0x0000 };
static const u32 s32_d[5] = { 0xFFFF0000UL, 0x0F0F0F0FUL, 0x80000001UL, 0x12345678UL, 0xC0FFEE00UL };
static const u32 s32_l[5] = { 0x0000FFFFUL, 0x33333333UL, 0x7FFFFFFEUL, 0x87654321UL, 0x00ABCDEFUL };
static const u32 s32_m[5] = { 0x00000000UL, 0x00FF00FFUL, 0x00000000UL, 0x0000FFFFUL, 0xFF000000UL };
static const RtSprite sp8 = { 8, 6, s8_l, s8_d, s8_m }, sp8o = { 8, 6, s8_l, s8_d, RT_NULL };
static const RtSprite sp16 = { 16, 10, s16_l, s16_d, s16_m }, sp16o = { 16, 10, s16_l, s16_d, RT_NULL };
static const RtSprite sp32 = { 32, 5, s32_l, s32_d, s32_m }, sp32o = { 32, 5, s32_l, s32_d, RT_NULL };

static const u16 tiles[4 * 32] = {   // (dark, light) row pairs
    // 0: checker light, empty dark
    0, 0xAAAA, 0, 0x5555, 0, 0xAAAA, 0, 0x5555, 0, 0xAAAA, 0, 0x5555, 0, 0xAAAA, 0, 0x5555,
    0, 0xAAAA, 0, 0x5555, 0, 0xAAAA, 0, 0x5555, 0, 0xAAAA, 0, 0x5555, 0, 0xAAAA, 0, 0x5555,
    // 1: frame in black, dark centre
    0xFFFF, 0xFFFF, 0x8001, 0x8001, 0xBFFD, 0x8001, 0xBFFD, 0x8001, 0xBFFD, 0x8001, 0xBFFD, 0x8001, 0xBFFD, 0x8001, 0xBFFD, 0x8001,
    0xBFFD, 0x8001, 0xBFFD, 0x8001, 0xBFFD, 0x8001, 0xBFFD, 0x8001, 0xBFFD, 0x8001, 0xBFFD, 0x8001, 0x8001, 0x8001, 0xFFFF, 0xFFFF,
    // 2: diagonal
    0x0001, 0x8000, 0x0002, 0x4000, 0x0004, 0x2000, 0x0008, 0x1000, 0x0010, 0x0800, 0x0020, 0x0400, 0x0040, 0x0200, 0x0080, 0x0100,
    0x0100, 0x0080, 0x0200, 0x0040, 0x0400, 0x0020, 0x0800, 0x0010, 0x1000, 0x0008, 0x2000, 0x0004, 0x4000, 0x0002, 0x8000, 0x0001,
    // 3: stripes
    0xF0F0, 0xFFFF, 0xF0F0, 0, 0xF0F0, 0xFFFF, 0xF0F0, 0, 0x0F0F, 0xFFFF, 0x0F0F, 0, 0x0F0F, 0xFFFF, 0x0F0F, 0,
    0xF0F0, 0xFFFF, 0xF0F0, 0, 0xF0F0, 0xFFFF, 0xF0F0, 0, 0x0F0F, 0xFFFF, 0x0F0F, 0, 0x0F0F, 0xFFFF, 0x0F0F, 0,
};
static const u8 map[12 * 8] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 0, 0, 2, 0, 0, 3, 0, 0, 2, 0, 1,
    1, 0, 3, 3, 3, 0, 0, 0, 2, 2, 0, 1,
    1, 2, 0, 0, 0, 1, 1, 0, 0, 0, 3, 1,
    1, 0, 0, 2, 0, 1, 1, 0, 3, 0, 0, 1,
    1, 3, 3, 0, 0, 0, 0, 2, 0, 0, 2, 1,
    1, 0, 0, 0, 2, 3, 0, 0, 0, 1, 0, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};
static const RtTilemap tm = { map, 12, 8, tiles, 4 };

static void scene(u16 k)
{
    s16 i;
    switch (k) {
    case 0:                                        // rectangles, every colour, clipped
        draw_rect(-5, -3, 30, 20, C_BLACK);
        draw_rect(20, 10, 45, 30, C_LGRAY);
        draw_rect(40, 25, 50, 40, C_DGRAY);
        draw_rect(60, 30, 20, 10, C_WHITE);
        draw_rect(150, 90, 30, 30, C_BLACK);
        draw_rect(3, 97, 200, 9, C_DGRAY);
        draw_rect(157, -4, 8, 60, C_LGRAY);
        draw_rect(81, 3, 1, 1, C_BLACK);
        break;
    case 1:                                        // 8-wide, masked over grey, all shifts
        draw_rect(0, 0, 160, 50, C_LGRAY);
        for (i = 0; i < 18; i++) draw_sprite(i * 9 - 3, (i & 3) * 12 - 2, i & 1 ? &sp8 : &sp8o);
        draw_sprite(157, 97, &sp8);
        break;
    case 2:                                        // 16-wide
        draw_rect(10, 10, 140, 80, C_DGRAY);
        for (i = 0; i < 12; i++) draw_sprite(i * 14 - 9, i * 9 - 4, i & 1 ? &sp16 : &sp16o);
        draw_sprite(155, 95, &sp16);
        draw_sprite(-15, 50, &sp16o);
        break;
    case 3:                                        // 32-wide
        draw_rect(0, 20, 160, 60, C_LGRAY);
        for (i = 0; i < 10; i++) draw_sprite(i * 19 - 20, i * 11 - 3, i & 1 ? &sp32 : &sp32o);
        draw_sprite(150, 98, &sp32);
        break;
    case 4:                                        // small font, every colour, over grey
        draw_rect(0, 0, 80, 100, C_LGRAY);
        draw_text(1, 1, "Hello gAy 42! {}|~", F_SMALL, C_BLACK);
        draw_text(3, 10, "The quick brown fox", F_SMALL, C_DGRAY);
        draw_text(5, 20, "jumps over 0123456789", F_SMALL, C_LGRAY);
        draw_text(130, 30, "right edge cut", F_SMALL, C_BLACK);
        draw_text(10, 96, "bottom", F_SMALL, C_BLACK);
        draw_text(40, 50, "white", F_SMALL, C_WHITE);
        break;
    case 5:                                        // medium font
        draw_rect(50, 0, 60, 100, C_DGRAY);
        draw_text(0, 0, "Score: 1234", F_MEDIUM, C_BLACK);
        draw_text(7, 20, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", F_MEDIUM, C_LGRAY);
        draw_text(13, 40, "abcdefghijklmnopqrstuvwxyz", F_MEDIUM, C_BLACK);
        draw_text(2, 94, "cut", F_MEDIUM, C_DGRAY);
        break;
    case 6:                                        // tile map, partial camera
        draw_tilemap(&tm, 5, 3);
        break;
    default:                                       // camera off the top-left, then past the end
        draw_rect(0, 0, 160, 100, C_DGRAY);
        draw_tilemap(&tm, -20, -10);
        draw_tilemap(&tm, 100, 60);
        break;
    }
}

static u16 planes_sum(void)                        // = sw_checksum; mono: dark plane as zeros
{
    u16 a = 0, b = 0, p, y, k;
    for (p = 0; p < 2; p++) {
        const u8 *q = p ? rt_dark : rt_light;
        for (y = 0; y < RT_H; y++)
            for (k = 0; k < RT_W / 8; k++) {
                a += q ? q[y * RT_PBYTES + k] : 0; if (a >= 255) a -= 255;
                b += a; if (b >= 255) b -= 255;
            }
    }
    return b << 8 | a;
}

static void hex4(char *s, u16 v)
{
    u16 k;
    for (k = 0; k < 4; k++, v <<= 4) s[k] = "0123456789ABCDEF"[v >> 12];
    s[4] = 0;
}

void game_init(void) { rt_state = &done; rt_state_size = sizeof(done); }
void game_scenario(u16 n) { (void)n; }
u8 game_update(void) { return !input_pressed(K_ESC | K_ENTER); }

void game_render(void)
{
    char line[40];
    u16 k;
    if (!done) {
        for (k = 0; k < NSC; k++) { draw_clear(); scene(k); xcheck_sums[k] = planes_sum(); }
        done = 1;
    }
    draw_clear();
#ifdef RT_MONO
    draw_text(0, 0, "xcheck mono", F_MEDIUM, C_BLACK);
#else
    draw_text(0, 0, "xcheck grey", F_MEDIUM, C_BLACK);
#endif
    for (k = 0; k < NSC; k++) {
        line[0] = '0' + k; line[1] = ':'; hex4(line + 2, xcheck_sums[k]);
        draw_text((k & 3) * 40, 12 + (k >> 2) * 10, line, F_MEDIUM, C_BLACK);
    }
}
