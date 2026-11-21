// Town/shop layout candidates for the user's visual decision (SHOP_SCENE.md). PC only: no
// shipping behaviour, no save write. The art is the disc's own (ffsk, tools/shop_scene.py):
// the 40M townsperson's portrait and body for the shopkeeper, RAMUZA's body for Ramza.
#include "worldmap.c"
#include "../portrait32.inc"
#include "../../../runtime/platform-sw/rt_sw.h"
#include "fixture_save.h"

static const u8 *sk;
static const u8 *sk_part(u8 k) { return sk + b16(sk + 4 + 2 * k); }
static void body(u8 which, s16 x, s16 y)            // 0 Ramza, 1 the shopkeeper
{
    const u8 *p = sk_part((u8)(which + 1));
    u16 rows[3][26];
    u8 i, j;
    RtSprite sp = { 16, 26, rows[0], rows[1], rows[2] };
    for (i = 0; i < 3; i++) for (j = 0; j < 26; j++) rows[i][j] = b16(p + i * 52 + 2 * j);
    draw_sprite(x, y, &sp);
}
// A back wall of shelves and a counter, drawn natively: a readable suggestion of the source
// room, not a rip of it.
static void shelves(s16 x, s16 y, s16 w, s16 h)
{
    s16 r, c;
    draw_rect(x, y, w, h, C_WHITE);
    for (r = 0; r + 9 <= h; r += 9) {
        draw_rect(x, y + r + 8, w, 1, C_DGRAY);
        for (c = 0; c + 11 <= w; c += 11) draw_rect(x + 3 + c, y + r + 3, 4, 5, C_LGRAY);
    }
}
static void counter(s16 x, s16 y, s16 w)
{
    draw_rect(x, y, w, 10, C_LGRAY);
    draw_rect(x, y, w, 1, C_BLACK); draw_rect(x, y + 9, w, 1, C_BLACK);
    draw_rect(x, y + 1, w, 3, C_WHITE);
}
// The town seen from its gate: roofs, walls and windows, same native treatment.
static void town_view(s16 x, s16 y, s16 w, s16 h)
{
    s16 c, k = 0;
    draw_rect(x, y, w, h, C_WHITE);
    for (c = 0; c + 24 <= w; c += 24, k++) {
        s16 bh = (s16)(h - 4 - (k & 1 ? 8 : 0)), top = (s16)(y + h - bh);
        draw_rect(x + c + 2, top, 20, bh, C_LGRAY);
        draw_rect(x + c + 2, top, 20, 2, C_BLACK);
        draw_rect(x + c + 5, top + 6, 4, 5, C_DGRAY);
        draw_rect(x + c + 13, top + 6, 4, 5, C_DGRAY);
    }
    draw_rect(x, y + h - 1, w, 1, C_BLACK);
}
static void room(s16 y, s16 h, u8 town)       // the picture, with its two actors
{
    if (town) { town_view(0, y, 160, h); body(0, 70, (s16)(y + h - 26)); return; }
    shelves(0, y, 160, (s16)(h - 14));
    body(1, 120, (s16)(y + h - 42));
    counter(0, (s16)(y + h - 14), 160);
    body(0, 86, (s16)(y + h - 26));
}
static void funds(s16 y)
{
    black(92, y, "War Funds"); right32(156, y, gil());
}
// A shop has a speaker and his portrait; a town has none, only its name.
static void greeting(s16 x, s16 y, u8 town, const char *line)
{
    if (town) { black(x, y, loc_name(wm.at)); black(x, (s16)(y + 9), "Where to?"); return; }
    black(x, y, "Shopkeeper"); black(x, (s16)(y + 9), line);
}
static const char *const menu_rows[3] = { "Buy", "Sell", "Leave store" };
static const char *const town_rows[3] = { "Shop", "Soldier Office", "Leave" };
static void rows3(s16 x, s16 y, u8 town)
{
    const char *const *labels = town ? town_rows : menu_rows;
    u8 r;
    for (r = 0; r < 3; r++) black(x, (s16)(y + 9 * r), labels[r]);
    choice_hand_at((s16)(x - 26), (s16)(y - 3));
}
static void items(s16 x, s16 y, u8 n, s16 price_at)
{
    u8 r;
    for (r = 0; r < n; r++) {
        u8 id = shop_nth(r);
        black(x, (s16)(y + 9 * r), item_name(id));
        right(price_at, (s16)(y + 9 * r), item_price(id));
    }
    choice_hand_at((s16)(x - 26), (s16)(y - 3));
}
static void border(const char *title)
{
    draw_rect(0, 0, 160, 1, C_BLACK); draw_rect(0, 99, 160, 1, C_BLACK);
    draw_rect(0, 0, 1, 100, C_BLACK); draw_rect(159, 0, 1, 100, C_BLACK);
    black(6, 4, title); right32(154, 4, gil());
    draw_rect(4, 12, 152, 1, C_BLACK);
}

// ---------------------------------------------------------------- the five candidates
static void variant(u8 v, u8 town)
{
    draw_clear();
    switch (v) {
    case 0:                      // 1. the source's own: portrait and words, menu over the picture
        room(34, 50, town);
        draw_rect(0, 0, 160, 33, C_WHITE); draw_rect(0, 33, 160, 1, C_BLACK);
        if (!town) portrait32(sk_part(0), 2, 2);
        greeting(town ? 6 : 38, town ? 8 : 4, town, "What can I do for you?");
        panel(4, 40, 76, 40);
        rows3(32, 48, town);
        funds(90);
        break;
    case 1:                      // 2. a picture band over the list
        room(0, 38, town);
        draw_rect(0, 38, 160, 1, C_BLACK);
        if (town) rows3(44, 52, 1); else items(30, 44, 5, 140);
        funds(90);
        break;
    case 2:                      // 3. the list, with the counter kept in a side column
        border(town ? loc_name(wm.at) : "Shop - Buy");
        draw_rect(103, 13, 1, 86, C_BLACK);
        if (town) { town_view(104, 14, 55, 50); body(0, 124, 38); }
        else { shelves(104, 14, 55, 34); body(1, 136, 30); counter(104, 58, 55); body(0, 110, 46); }
        if (town) rows3(44, 26, 1); else items(18, 18, 7, 98);
        break;
    case 3:                      // 4. an arrival picture first, the plain list after it
        room(0, 62, town);
        draw_rect(0, 62, 160, 38, C_WHITE); draw_rect(0, 62, 160, 1, C_BLACK);
        if (!town) portrait32(sk_part(0), 2, 66);
        greeting(town ? 6 : 38, town ? 70 : 66, town, "Welcome.");
        black(town ? 6 : 38, 88, town ? "ENTER opens the menu" : "ENTER opens the shop");
        break;
    default:                     // 5. portrait and menu, with a small picture beside them
        border(town ? loc_name(wm.at) : "Shop");
        if (!town) portrait32(sk_part(0), 3, 15);
        greeting(town ? 6 : 40, town ? 18 : 16, town, "What now?");
        rows3(44, 48, town);
        if (town) { town_view(100, 40, 58, 40); body(0, 120, 56); }
        else { shelves(100, 40, 58, 26); body(1, 136, 44); counter(100, 70, 58); body(0, 104, 56); }
        funds(90);
        break;
    }
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
    sk = rt_file("ffsk", &size); if (!sk || memcmp(sk, "FSK1", 4)) return 6;
    game_scenario(0);
    wm.at = 6; wm.buy = 1; shop_refresh();
    for (v = 0; v < 5; v++) {
        variant(v, 0);
        snprintf(path, sizeof path, "%s/shop-%u-store.png", argv[1], v + 1);
        if (sw_write_png(path, 1)) return 7;
        variant(v, 1);
        snprintf(path, sizeof path, "%s/shop-%u-town.png", argv[1], v + 1);
        if (sw_write_png(path, 1)) return 7;
    }
    return 0;
}
