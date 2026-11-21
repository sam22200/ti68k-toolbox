// The town's rooms for the user's visual decision (SHOP_SCENE.md). PC only: no shipping
// behaviour, no save write. It drives worldmap.c's own screens through the WM_ROOM_PICK door,
// so what it shows is exactly what the game draws.
#define WM_ROOM_PICK
#include "worldmap.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include "fixture_save.h"

int main(int argc, char **argv)
{
    char path[512];
    u8 v;
    FILE *f;
    if (argc != 3) return 2;
    sw_init(0); if (!bank || !mgb || !shop || !scene) return 3;
    f = fopen(argv[2], "rb"); if (!f) return 4;
    if (!read_save_fixture(f, &save)) return 5;
    fclose(f);
    game_scenario(0);
    wm.at = 6; wm.x = loc_x(6); wm.y = loc_y(6); wm.tick = 4;
    for (v = 1; v <= scene[4]; v++) {
        wm_room_pick = v;
        wm.screen = SC_OFFICE; wm.trow = 0; p_screen = 255;
        draw_clear(); town_render();
        snprintf(path, sizeof path, "%s/room-%u.png", argv[1], v);
        if (sw_write_png(path, 1)) return 8;
    }
    return 0;
}
