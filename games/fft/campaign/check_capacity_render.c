// Real renderer/Status with a private13-record descriptor and existing sprites.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main capacity_runtime_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#define game_scenario capacity_source_scenario
#ifdef RT_CYCLES
#define FFT_MEASURE
#endif
#include "gariland.c"
#undef game_scenario
void game_scenario(u16 scenario)
{
    capacity_source_scenario(61);
    if(!W)return;
    if(scenario==1||scenario==2) {
        st.sel=(u8)(NUNIT-1);st.mode=M_STATUS;st.page=(u8)(scenario-1);
        W->status_i=0;W->status_page=255;
    }
}
static u32 identity(void)
{
    return (u32)NUNIT<<24|(u32)st.sel<<16|(u16)st.unit[NUNIT-1].hp;
}
#ifndef __m68k__
int main(int argc,char **argv)
{
    SwScript script;unsigned i,limit;if(argc!=5)return 2;
    limit=(unsigned)atoi(argv[2]);if(sw_load_script(&script,argv[3]))return 3;
    sw_init((u16)atoi(argv[1]));
    for(i=0;i<limit;i++)if(!sw_step(sw_script_keys(&script,rt_frame)))break;
    printf("capacity checksum %04X identity %08lX\n",sw_checksum(),(unsigned long)identity());
    return sw_write_png(argv[4],1);
}
#else
void _main(void){capacity_runtime_main();BENCH_VALUE(identity());}
#endif
