// The town's screens in the source's own arrangement, for the user's visual decision
// (SHOP_SCENE.md). PC only: no shipping behaviour, no save write.
// The room is the disc's own picture (ffsk, tools/shop_scene.py): MENU/BK_SHOP.TIM and
// MENU/BK_HONE.TIM already contain the shopkeeper, the clerk and their customers, so nothing
// is placed on top of them. The town's own menu stays over the world map, as the source keeps
// it over its world map.
#include "worldmap.c"
#include "../portrait32.inc"
#include "../../../runtime/platform-sw/rt_sw.h"
#include "fixture_save.h"

static const u8 *sk;
static u8 *picture;                                  // 160x100 light plane then dark
static const u8 *sk_part(u8 k) { return sk + b16(sk + 4 + 2 * k); }
static void show_room(u8 which)
{
    u8 *p;
    s16 y;
    zx0_unpack(sk_part((u8)(which + 1)), picture);
    for (y = 0; y < 100; y++) {
        p = (u8 *)rt_light + (u16)y * RT_PBYTES;
        memcpy(p, picture + (u16)y * 20, 20);
        p = (u8 *)rt_dark + (u16)y * RT_PBYTES;
        memcpy(p, picture + 2000 + (u16)y * 20, 20);
    }
}
// The source's dialogue band: the speaker's portrait, his name, and what he says.
static void says(const char *first, const char *second)
{
    draw_rect(0, 0, 160, 33, C_WHITE);
    draw_rect(0, 32, 160, 1, C_BLACK);
    draw_rect(0, 0, 160, 1, C_BLACK);
    portrait32(sk_part(0), 2, 2);
    black(38, 3, "Shopkeeper");
    black(38, 13, first);
    if (second) black(38, 22, second);
}
// The source's War Funds, bottom right, over the picture.
static void war_funds(void)
{
    draw_rect(88, 78, 72, 20, C_WHITE);
    draw_rect(88, 78, 72, 1, C_BLACK); draw_rect(88, 78, 1, 20, C_BLACK);
    black(93, 80, "War Funds");
    right32(155, 89, gil());
}
static const ChoiceLayout store_menu =
    { 3, 36, 66, 50, 7, CHOICE_TAB_PLAIN, 3, 1, 22, 6, 12, 9, 158, -20, 9 };
static const ChoiceLayout town_menu =
    { 3, 36, 66, 41, 7, CHOICE_TAB_PLAIN, 3, 1, 22, 6, 12, 9, 158, -20, 9 };

static void screen(u8 which)
{
    static const char *const store[4] = { "Buy", "Sell", "Fitting room", "Leave store" };
    static const char *const office[4] = { "Hire", "Dismiss", "Rename", "Leave office" };
    static const char *const town[3] = { "Shop", "Soldier Office", "Leave" };
    u8 r;
    if (which == 2) {                       // the town's own menu, over the world map
        s16 cx = (s16)(wm.x - 80), cy = (s16)(wm.y - 50);
        rt_clamp_cam(&map, &cx, &cy);
        map_background(cx, cy, 1);
        ramza((s16)(wm.x - cx), (s16)(wm.y - cy));
        draw_rect(0, 0, 160, 11, C_WHITE); draw_rect(0, 11, 160, 1, C_BLACK);
        black(4, 2, loc_name(wm.at));
        choice_frame(&town_menu, "Town");
        for (r = 0; r < 3; r++) choice_row(&town_menu, r, town[r], C_BLACK);
        choice_cursor_animated(&town_menu, 0, rt_frame);
        war_funds();
        return;
    }
    show_room(which);
    says(which ? "Welcome." : "Welcome.",
         which ? "Looking for a soldier?" : "What can I do for you?");
    choice_frame(&store_menu, "Menu");
    for (r = 0; r < 4; r++)
        choice_row(&store_menu, r, (which ? office : store)[r], r == 2 ? C_DGRAY : C_BLACK);
    choice_cursor_animated(&store_menu, 0, rt_frame);
    war_funds();
}

int main(int argc, char **argv)
{
    static const char *const names[3] = { "shop-store", "shop-office", "shop-town" };
    char path[512];
    u8 v;
    u16 size;
    FILE *f;
    if (argc != 3) return 2;
    sw_init(0); if (!bank || !mgb || !shop) return 3;
    f = fopen(argv[2], "rb"); if (!f) return 4;
    if (!read_save_fixture(f, &save)) return 5;
    fclose(f);
    sk = rt_file("ffsk", &size); if (!sk || memcmp(sk, "FSK2", 4)) return 6;
    picture = malloc(4000); if (!picture) return 7;
    game_scenario(0);
    wm.at = 6; wm.x = loc_x(6); wm.y = loc_y(6); wm.tick = 4;
    for (v = 0; v < 3; v++) {
        draw_clear();
        screen(v);
        snprintf(path, sizeof path, "%s/%s.png", argv[1], names[v]);
        if (sw_write_png(path, 1)) return 8;
    }
    free(picture);
    return 0;
}
