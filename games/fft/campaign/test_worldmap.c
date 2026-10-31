// The world map (worldmap.c) on the save a Gariland victory wrote (x/check/ffsave_won.sav,
// campaign check.py), headless: shown locations, the cheapest route, travel, the save.
#include "worldmap.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <assert.h>
#include "fixture_save.h"
#include <unistd.h>
static void use_save(const Save *s) { remove("ffsave.sav"); assert(rt_save(SAVE_NAME, s, sizeof *s)); }
static void point_at(u8 to)
{
    u16 n=0;
    while(abs(wm.handx-loc_x(to))>1||abs(wm.handy-loc_y(to))>1) {
        u32 keys=0;
        if(abs(wm.handx-loc_x(to))>1)keys|=wm.handx<loc_x(to)?K_RIGHT:K_LEFT;
        if(abs(wm.handy-loc_y(to))>1)keys|=wm.handy<loc_y(to)?K_DOWN:K_UP;
        assert(++n<500&&sw_step(keys));
    }
    assert(sw_step(0));assert(wm.sel==to);
}
int main(void)
{
    Save won, fresh;
    u16 f;
    if (access("ffwm.bin", R_OK)) assert(!chdir("data"));
    {
        FILE *fp = fopen("../x/check/ffsave_won.sav", "rb"); assert(fp);
        assert(read_save_fixture(fp,&won)); fclose(fp);
    }
    assert(won.loc == 7 && (won.flags[0] & 2));             // Gariland's battle wrote its place
    use_save(&won);
    sw_init(0); assert(sw_step(0));
    assert(wm.at == 6 && shown(6) && shown(24) && !shown(2) && !shown(18));
    wm.handx=loc_x(6)+18;wm.handy=loc_y(6);assert(magnet()==6);
    wm.handx=loc_x(6);wm.handy=loc_y(6);
    wm.handx=wm.handy=0;wm.sel=255;
    assert(sw_step(K_ENTER)&&!wm.moving&&wm.at==6&&wm.handx==wm.x&&wm.handy==wm.y);
    assert(sw_step(0));wm.handx=wm.handy=0;wm.sel=255;
    assert(sw_step(K_A)&&!wm.moving&&wm.at==6&&wm.sel==6);assert(sw_step(0));
    assert(find_path(24) && wm.legs == 1);                   // one route: Gariland - Mandalia
    point_at(24);assert(!wm.moving&&wm.at==6); // hover must never travel
    point_at(6);point_at(24);
    assert(sw_step(K_ENTER)); assert(wm.moving);
    for (f = 0; f < 400 && wm.moving; f++) assert(sw_step(0));
    assert(!wm.moving && wm.at == 24 && save.loc == 25 && wm.x == loc_x(24) && wm.y == loc_y(24));
    assert(sw_step(0)); assert(sw_step(K_ENTER)); assert(wm.note == NOTE_LATER);   // not ripped yet
    assert(sw_step(K_B)); assert(!wm.note);
    assert(sw_step(0)); assert(sw_step(K_D));assert(wm.menu);
    assert(sw_step(0));assert(sw_step(K_DOWN));assert(sw_step(0));
    assert(!sw_step(K_ENTER));assert(wm.result==1);                            // Formation
    { Save back; assert(rt_load(SAVE_NAME, &back, sizeof back) && back.loc == 25); }
    sw_init(0);assert(sw_step(K_D)&&sw_step(0));
    wm.row=4;assert(!sw_step(K_A)&&wm.result==2); // supervisor opens common ffttitle(3)
    // before Gariland's victory its battle is entered: fftmap returns its door
    fresh = won; fresh.flags[0] = 0;
    use_save(&fresh);
    sw_init(0); assert(sw_step(0)); assert(!shown(24));
    assert(sw_step(K_ENTER) == 0 && wm.result == 200);
    remove("ffsave.sav");
    puts("World map: shown locations, routes, travel to Mandalia, its notice, the save, menus and a battle door");
    return 0;
}
