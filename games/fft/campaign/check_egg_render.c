// Selected actual render/archive-pointer carrier. No broad TI sweep.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main egg_runtime_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#define game_scenario egg_source_scenario
#ifdef EGG_MAP
#include "worldmap.c"
#else
#include "manage.c"
#endif
#undef game_scenario
static u32 fixture_hash;
void game_scenario(u16 n)
{
    egg_source_scenario(0);
#ifdef EGG_MAP
    if(n==1){
        u16 k;fixture_hash=17;
        if(world_party_day(&save.world,&save,breed_rules,world_draw,&save)!=WB_LAID)return;
        for(k=0;k<sizeof save;k++)fixture_hash=(fixture_hash<<1|fixture_hash>>31)^((const u8 *)&save)[k];
    }else{save.world.month=3;save.world.day=21;}
#else
    {u8 i;save.n=10;for(i=7;i<10;i++){
        memset(&save.unit[i],0,sizeof save.unit[i]);save.unit[i].type=3;save.unit[i].job=94;
        save.unit[i].level=1;save.unit[i].gfx=255;
        save.world.egg[i]=i==7?0x32:i==8?0x31:0x55;wb_color_set(&save.world,i,(u8)(i-7));
    }}
    mg.unit=7;mg.slide=0;mg.screen=n?S_MENU:S_FORM;mg.confirm=n==2?1:0;dirty=1;
#endif
}
#ifndef __m68k__
int main(int argc,char **argv)
{if(argc<2||argc>3)return 2;sw_init((u16)atoi(argv[1]));sw_step(0);printf("egg-render %04X state%08lX\n",sw_checksum(),(unsigned long)fixture_hash);return argc==3?sw_write_png(argv[2],1):0;}
#else
void _main(void){egg_runtime_main();BENCH_VALUE(fixture_hash);BENCH_VALUE(sizeof(Save));}
#endif
