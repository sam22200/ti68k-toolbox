#include <stdio.h>
static int fail_replace;
static int service_rename(const char *a,const char *b){return fail_replace?-1:rename(a,b);}
#define SC_FILE_RENAME service_rename
#include "worldmap.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <assert.h>
static void tap(u32 key){assert(sw_step(key));assert(sw_step(0));}
int main(int argc,char **argv)
{
    Save initial,back;u8 n;assert(argc==3);sw_init(0);assert(bank&&shop&&service_gates);initial=save;
    if(!strcmp(argv[1],"travel")) {
        for(n=0;n<10;n++) {
            u16 frames=0;wm.sel=wm.at==6?24:6;tap(K_ENTER);assert(wm.moving);
            while(wm.moving&&frames++<4000)assert(sw_step(0));assert(!wm.moving&&frames<4000);
        }
        assert(wm.at==6&&save.world.day==initial.world.day+10);
        assert(save.missions.dispatches.job[0].elapsed==save.missions.dispatches.job[0].days);
        back=save;for(n=0;n<50;n++)assert(sw_step(0));assert(!memcmp(&back,&save,sizeof save));
        assert(!leave(0));assert(save_load(&back)&&back.missions.dispatches.job[0].flags&PROP_RETURNED);
    } else if(!strcmp(argv[1],"recall-day")) {
        assert(save.missions.dispatches.job[0].flags&PROP_RECALLED);
        wm.sel=24;tap(K_ENTER);while(wm.moving)assert(sw_step(0));
        assert(save.missions.dispatches.job[0].elapsed==1&&save.missions.dispatches.job[0].flags&PROP_RETURNED);
        wm.sel=6;tap(K_ENTER);while(wm.moving)assert(sw_step(0));
        assert(!leave(0));
    } else if(!strcmp(argv[1],"fur")) {
        u8 item=1;u32 funds;u8 stock,owned;assert(wm.at==9&&fur_open());
        tap(K_ENTER);assert(wm.screen==SC_TOWN);tap(K_DOWN);tap(K_DOWN);
        tap(K_ENTER);assert(wm.note==NOTE_HUNT);tap(K_ENTER);
        prop_put_word(save.unit[3].support,471);save.poach[item]=2;save.inv[item]=1;wm.changed=1;
        tap(K_ENTER);assert(wm.screen==SC_FUR);tap(K_ENTER);assert(wm.screen==SC_SHOP&&wm.fur&&wm.buy);
        while(shop_nth(wm.trow)!=item)tap(K_DOWN);
        game_render();{char p[512];snprintf(p,sizeof p,"%s/fur-buy.png",argv[2]);assert(!sw_write_png(p,1));}
        funds=gil();stock=save.poach[item];owned=save.inv[item];
        fail_replace=1;tap(K_ENTER);assert(wm.note==NOTE_UNAVAILABLE&&gil()==funds&&save.poach[item]==stock&&save.inv[item]==owned);
        fail_replace=0;tap(K_ENTER);tap(K_ENTER);
        assert(gil()==funds-(item_price(item)>>1)&&save.poach[item]==stock-1&&save.inv[item]==owned+1);
        assert(save_load(&back)&&back.poach[item]==1&&back.inv[item]==2);
        tap(K_LEFT);assert(!wm.buy);while(shop_nth(wm.trow)!=item)tap(K_DOWN);
        game_render();{char p[512];snprintf(p,sizeof p,"%s/fur-sell.png",argv[2]);assert(!sw_write_png(p,1));}
        tap(K_ENTER);assert(gil()==funds&&save.poach[item]==stock&&save.inv[item]==owned);
        save.poach[item]=255;tap(K_ENTER);assert(save.poach[item]==255&&save.inv[item]==0);
        assert(save_load(&back)&&back.poach[item]==255&&back.inv[item]==0);
    } else assert(0);
    puts("Playable world services: travel/day/Fur transactions passed");return 0;
}
