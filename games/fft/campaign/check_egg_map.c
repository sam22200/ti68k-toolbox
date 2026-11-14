#include <assert.h>
#include <stdio.h>
#include "worldmap.c"
#include "../../../runtime/platform-sw/rt_sw.h"
static void journey(u8 to)
{
    u16 frames=0;u8 day=save.world.day;u32 rng;
    assert(find_path(to));wm.moving=1;wm.leg=wm.step=0;
    rng=(u32)b16(save.world_rng)<<16|b16(save.world_rng+2);
    while(wm.moving){assert(++frames<1000);assert(sw_step(K_ENTER|K_ESC|K_D));}
    assert(wm.at==to&&save.loc==to+1&&save.world.day==day+1);
    assert(((u32)b16(save.world_rng)<<16|b16(save.world_rng+2))!=rng);
    assert(sw_step(0));assert(save_world_valid(&save));
}
int main(int argc,char **argv)
{
    Save before;u8 timer;
    if(argc!=2)return 2;sw_init(0);assert(bank&&mgb&&breed_rules);
    if(!strcmp(argv[1],"review")){
        save.world.month=3;save.world.day=21;game_render();assert(sw_checksum()==0xeaaa);
        assert(!sw_write_png("map.png",1));release_map();puts("map approved calendar pixels pass");return 0;
    }
    before=save;assert(sw_step(0));assert(sw_step(K_D));assert(sw_step(0));assert(sw_step(K_B));assert(sw_step(0));
    assert(memcmp(&save,&before,sizeof save)==0); // entry/menu/idle never advance time/RNG
    if(!strcmp(argv[1],"full")){
        u8 i;for(i=8;i<16;i++)save.unit[i]=save.unit[1];save.n=16;before=save;
        journey(24);assert(save.n==16&&memcmp(save.unit,before.unit,sizeof save.unit)==0);
        for(i=0;i<16;i++)assert(!save.world.egg[i]);assert(!leave(1));
        puts("map full: a completed day/RNG advance cannot overwrite the16-member roster");
    }else if(!strcmp(argv[1],"multi")){
        u16 frames=0;
        assert(find_path(24)&&wm.legs==1);wm.path[1]=wm.path[0]^0x80;wm.legs=2;
        wm.moving=1;wm.leg=wm.step=0;
        while(wm.moving){assert(++frames<1500);assert(sw_step(0));}
        assert(wm.at==6&&save.loc==7&&save.world.day==3);assert(save_world_valid(&save));assert(!leave(1));
        puts("map two-leg cycle fixture: exactly two days over the original route and its reverse");
    }else if(!strcmp(argv[1],"first")){
        journey(24);assert(save.n==9&&save.world.egg[8]);
        assert(!leave(1));puts("map first: one birth, durable route/RNG; idle and held controls harmless");
    }else if(!strcmp(argv[1],"hatch")){
        SaveUnit child=save.unit[8];u16 trips=0;
        while(save.world.egg[8]){journey(wm.at==24?6:24);assert(++trips<10);}
        child.zodiac=save.unit[8].zodiac;assert(memcmp(&child,&save.unit[8],sizeof child)==0);
        assert(wb_word(save.world.birthday[8])==save.world.day);assert(!leave(1));
        puts("map hatch: original child seeds/name/level/Brave/Faith survive saved journeys without reroll");
    }else{
        assert(save.n>=9);timer=save.world.egg[8]&15;assert(timer);
        journey(wm.at==24?6:24);
        assert((save.world.egg[8]&15)==timer-1);assert(!leave(1));
        puts("map next: cold resume consumes one day, existing egg advances once");
    }
    return 0;
}
