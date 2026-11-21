// The town's screens in the source's own arrangement, for the user's visual decision
// (SHOP_SCENE.md). PC only: no shipping behaviour, no save write.
// The room is the disc's own picture (ffsk, tools/shop_scene.py): MENU/BK_SHOP.TIM and
// MENU/BK_HONE.TIM already contain the shopkeeper, the clerk and their customers, so nothing
// is placed on top of them, and only the 66 rows the dialogue box leaves visible are ripped.
// The dialogue box, the menu and the War Funds plate are the campaign's shared ones
// (dialogue_box.inc, choice_menu.inc, war_funds.inc).
#include "worldmap.c"
#define DLG_TEXT(x,y,s,c,l) text(x,y,s,c,l)
#include "../dialogue_box.inc"
#define WF_TEXT(x,y,s,c,l) text(x,y,s,c,l)
#define WF_WIDTH(s) map_menu_width(s)
#include "../war_funds.inc"
#include "../../../runtime/platform-sw/rt_sw.h"
#include "fixture_save.h"

enum { ROOM_W = 160, ROOM_TOP = 34 };            // the rows the dialogue box leaves visible
static const u8 *sk;
static u8 *picture;
static const u8 *sk_part(u8 k) { return sk + b16(sk + 5 + 2 * k); }
static void show_room(u8 which)
{
    const u8 *rec = sk_part((u8)(which + 1));
    u8 h = rec[0];
    s16 y;
    zx0_unpack(rec + 1, picture);
    for (y = 0; y < h && ROOM_TOP + y < 100; y++) {
        memcpy((u8 *)rt_light + (u16)(ROOM_TOP + y) * RT_PBYTES, picture + (u16)y * 20, 20);
        memcpy((u8 *)rt_dark + (u16)(ROOM_TOP + y) * RT_PBYTES, picture + (u16)h * 20 + (u16)y * 20, 20);
    }
}
// The shared dialogue look, at the top of the screen as the source puts it in a store.
static void says(const char *name, const char *first, const char *second)
{
    s16 x = DLG_FACE_W + 4;
    u8 w = (u8)(map_menu_width(second ? second : first) + 10);
    if (w < map_menu_width(first) + 10) w = (u8)(map_menu_width(first) + 10);
    dlg_name(x, 1, (u8)(map_menu_width(name) + 8), name, 0);
    dlg_frame(x, 13, w, (u8)(6 + 7 * (second ? 2 : 1)), 0);
    text(x + 4, 17, first, C_WHITE, 156);
    if (second) text(x + 4, 24, second, C_WHITE, 156);
    dlg_face(sk_part(0), 1, 1, 0);
}
static u8 rows_width(const char *const *labels, u8 n)
{
    u8 i, w = 0;
    for (i = 0; i < n; i++) { u8 k = map_menu_width(labels[i]); if (k > w) w = k; }
    return (u8)(w + 12);
}
// The shared menu, narrowed to its labels and pushed right so the glove is wholly on screen.
static void action_menu(const char *title, const char *const *labels, u8 n, u8 grey)
{
    ChoiceLayout p = { 24, 36, 0, 0, 7, CHOICE_TAB_PLAIN, 3, 1, 0, 6, 12, 9, 158, -16, 9 };
    u8 i;
    p.width = rows_width(labels, n);
    p.height = (u8)(10 + 9 * n);
    choice_frame(&p, title);
    for (i = 0; i < n; i++) choice_row(&p, i, labels[i], i == grey ? C_DGRAY : C_BLACK);
    choice_cursor_animated(&p, 0, rt_frame);
}

static void screen(u8 v)
{
    static const char *const store[4] = { "Buy", "Sell", "Fitting room", "Leave store" };
    static const char *const office[4] = { "Hire", "Dismiss", "Rename", "Leave office" };
    static const char *const town[3] = { "Shop", "Soldier Office", "Leave" };
    draw_clear();
    if (v == 6) {                               // the town's own menu, over the world map
        s16 cx = (s16)(wm.x - 80), cy = (s16)(wm.y - 50);
        rt_clamp_cam(&map, &cx, &cy);
        map_background(cx, cy, 1);
        ramza((s16)(wm.x - cx), (s16)(wm.y - cy));
        draw_rect(0, 0, 160, 11, C_WHITE); draw_rect(0, 11, 160, 1, C_BLACK);
        black(4, 2, loc_name(wm.at));
        action_menu("Town", town, 3, 255);
    } else {
        u8 office_room = v == 5;
        show_room(v);
        says("Shopkeeper", "Welcome.",
             office_room ? "Looking for a soldier?" : "What can I do for you?");
        action_menu("Menu", office_room ? office : store, 4, 2);
    }
    war_funds(gil());
}

int main(int argc, char **argv)
{
    char path[512];
    u8 v;
    u16 size;
    FILE *f;
    if (argc != 3) return 2;
    sw_init(0); if (!bank || !mgb || !shop) return 3;
    f = fopen(argv[2], "rb"); if (!f) return 4;
    if (!read_save_fixture(f, &save)) return 5;
    fclose(f);
    sk = rt_file("ffsk", &size); if (!sk || memcmp(sk, "FSK3", 4)) return 6;
    picture = malloc(2 * ROOM_W * 66 / 8); if (!picture) return 7;
    game_scenario(0);
    wm.at = 6; wm.x = loc_x(6); wm.y = loc_y(6); wm.tick = 4;
    for (v = 0; v < 7; v++) {
        screen(v);
        if (v < 5) snprintf(path, sizeof path, "%s/shop-grey-%u.png", argv[1], v + 1);
        else snprintf(path, sizeof path, "%s/shop-%s.png", argv[1], v == 5 ? "office" : "town");
        if (sw_write_png(path, 1)) return 8;
    }
    free(picture);
    return 0;
}
