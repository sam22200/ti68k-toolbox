// The world map (worldmap.c) on the save a Gariland victory wrote (x/check/ffsave_won.sav,
// campaign check.py), headless: shown locations, the cheapest route, travel, the save, and the
// town's services -- the shop's stock and prices, selling back, the Soldier Office's recruit.
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
    // ------------------------------------------------ the town's services (ffsh)
    use_save(&won);
    sw_init(0); assert(sw_step(0));
    assert(shop && shop_type() == 1 && sh_place[24] == 2);   // Gariland a town, Mandalia a field
    assert(sw_step(K_ENTER) && wm.screen == SC_TOWN);        // its services, not its won battle
    assert(sw_step(0) && sw_step(K_ENTER) && wm.screen == SC_STORE);     // Shop
    assert(sw_step(0) && sw_step(K_ENTER) && wm.screen == SC_SHOP && wm.buy);   // Buy
    {
        u8 dagger = shop_nth(0), i, n = shop_n, total = 0;
        assert(!wm.cat && n == 4 && item_price(dagger) == 100);   // the weapon tab: four of them
        for (i = 0; i < SHOP_TABS; i++) { wm.cat = i; shop_refresh(); total = (u8)(total + shop_n); }
        assert(total == 10);                                 // the guide's whole Gariland stock
        wm.cat = 0; shop_refresh();
        assert(sw_step(0) && sw_step(K_F5) && wm.cat == 1);  // the source's L1/R1
        assert(sw_step(0) && sw_step(K_F1) && wm.cat == 0);
        assert(sw_step(0) && sw_step(K_F1) && wm.cat == SHOP_TABS - 1);   // wrapping
        assert(sw_step(0) && sw_step(K_F5) && !wm.cat && shop_n == 4);
        assert(sw_step(0) && sw_step(K_MODE) && wm.detail);  // the shared item detail
        assert(sw_step(0) && sw_step(K_MODE) && !wm.detail);
        assert(item_wearers(dagger) <= save.n);
        n = shop_n;
        for (i = 0; i < n; i++) assert(on_sale(shop_nth(i)) && item_price(shop_nth(i)));
        memset(save.inv, 0, sizeof save.inv); gil_set(100);
        assert(sw_step(0) && sw_step(K_ENTER));
        assert(gil() == 0 && save.inv[dagger] == 1 && !wm.note);
        assert(sw_step(0) && sw_step(K_ENTER) && wm.note == NOTE_NOGIL);
        assert(sw_step(K_B) && !wm.note);
        assert(sw_step(0) && sw_step(K_LEFT) && !wm.buy && shop_n == 1);
        assert(sw_step(0) && sw_step(K_ENTER));
        assert(gil() == 50 && !save.inv[dagger] && !shop_n && !wm.trow);
    }
    assert(sw_step(K_ESC) && wm.screen == SC_STORE);
    assert(sw_step(0) && sw_step(K_ESC) && wm.screen == SC_TOWN);
    assert(sw_step(0) && sw_step(K_DOWN) && sw_step(0));
    assert(sw_step(K_ENTER) && wm.screen == SC_OFFICE && wm.trow == 0);
    {
        u8 before = save.n;
        const SaveUnit *u = &save.unit[before];
        assert(office_job(0) == 0x4A && office_job(1) == 0x4A);   // the source hires a Squire
        assert(office_price(0) == 1500 && office_price(1) == 1400);
        gil_set(1499);
        assert(sw_step(0) && sw_step(K_ENTER) && wm.note == NOTE_NOGIL && save.n == before);
        assert(sw_step(K_B) && !wm.note);
        gil_set(2000);
        assert(sw_step(0) && sw_step(K_ENTER) && !wm.note);       // Male Soldier
        assert(save.n == before + 1 && gil() == 500);
        assert(u->job == 0x4A && u->type == 0 && u->level == 1 && u->gfx == 255);
        assert(u->brave >= 40 && u->brave <= 71 && u->faith >= 40 && u->faith <= 71);
        assert(u->zodiac < 12 && !save_egg(&save, before) && !save_raw_current(&save, before));
        assert(u->eq[0] && u->eq[1] && !u->eq[2] && u->eq[3] && !u->eq[4]);  // hat, clothes, sword
        assert(u->eq[3] == shop_nth(1 - 1) || item_price(u->eq[3]) == 200);  // a Broad Sword
    }
    assert(sw_step(K_ESC) && wm.screen == SC_TOWN);
    assert(sw_step(0) && sw_step(K_ESC) && wm.screen == SC_MAP && !wm.moving);
    // The Fur Shop: its town row waits on the source's Chapter III, and its own door shows it.
    assert(!fur_open() && scene && scene[4] == ROOM_N);
    game_scenario(2); assert(wm.screen == SC_FUR && wm.trow == 0);
    assert(sw_step(0) && sw_step(K_DOWN) && sw_step(0) && sw_step(K_DOWN) && sw_step(0));
    assert(sw_step(K_ENTER) && wm.screen == SC_TOWN);         // Buy / Sell / Leave store
    assert(sw_step(0) && sw_step(K_ESC) && wm.screen == SC_MAP);
    game_scenario(0);
    // A field answers with one of its formations; Chapter I has none ripped, so it still waits.
    wm.at = 24; assert(!encounter() && sh_enc[2 * 3 * 24] == 0);
    remove("ffsave.sav");
    puts("World map: shown locations, routes, travel to Mandalia, its notice, the save, menus, a battle door,\n           Gariland's shop (10 items, buying, selling back) and its Soldier Office");
    return 0;
}
