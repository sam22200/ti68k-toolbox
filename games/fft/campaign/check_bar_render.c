// Selected native pointer/render/input path, including the new profile nibble stores.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main bar_runtime_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#define game_scenario bar_source_scenario
#include "worldmap.c"
#undef game_scenario
static u32 fixture_hash;
void game_scenario(u16 n)
{
    u16 i;bar_source_scenario(0);
    if(!bar_art||!bar_text)return;
    wm.screen=SC_BAR;
    if(n==4) {
        // Complete-source fixture bindings:02D -> flag33,03A -> flag34.
        save.flags[4]|=3;wm.screen=SC_RUMORS;wm.trow=3;wm.ttop=1;rumor_open();
    } else if(n){wm.screen=SC_RUMORS;wm.trow=n==2?1:2;rumor_open();if(n==3)wm.rpage=wm.rlast;}
    fixture_hash=17;
    for(i=0;i<sizeof save;i++)fixture_hash=(fixture_hash<<1|fixture_hash>>31)^((const u8 *)&save)[i];
    fixture_hash^=(u32)wm.screen<<24|(u32)wm.rpage<<16|(u16)wm.rlast<<8|wm.changed;
}
#ifndef __m68k__
int main(int argc,char **argv)
{u16 frames=argc==4?(u16)atoi(argv[3]):1,i;if(argc!=3&&argc!=4)return 2;sw_init((u16)atoi(argv[1]));for(i=0;i<frames;i++)sw_step(0);printf("bar-render %04X state%08lX\n",sw_checksum(),(unsigned long)fixture_hash);release_map();return sw_write_png(argv[2],1);}
#else
void _main(void){bar_runtime_main();BENCH_VALUE(fixture_hash);BENCH_VALUE(sizeof(Save));}
#endif
