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
#define game_update capacity_source_update
#ifdef RT_CYCLES
#define FFT_MEASURE
#endif
#include "gariland.c"
#undef game_scenario
#undef game_update
static u8 ko_board;
static u8 ko_original_job;
u8 game_update(void){if(ko_board){
    FRAME_BEGIN();st.tick++;W->actor_budget=1;W->actor_misses=0;W->ac_now++;
    if(ko_board==2)st.morph_t=(u8)((rt_frame%5)*8);
    if(ko_board==3&&rt_frame==15){st.unit[0].job=133;st.unit[0].hp=0;st.unit[0].dc=3;}
    if(ko_board==3&&rt_frame==35){st.unit[0].job=ko_original_job;st.unit[0].hp=st.unit[0].max_hp;}
    return 1;}return capacity_source_update();}
void game_scenario(u16 scenario)
{
    ko_board=0;
    capacity_source_scenario(61);
    if(!W)return;
    if(scenario==1||scenario==2) {
        st.sel=(u8)(NUNIT-1);st.mode=M_STATUS;st.page=(u8)(scenario-1);
        W->status_i=0;W->status_page=255;
    }
    if(scenario>=3){
        static const u8 jobs_ko[6]={133,100,97,109,106,94};u8 i;
        ko_board=1;st.mode=M_MENU;st.ai=AI_OFF;st.open_stage=st.open_fade=st.banner=0;
        st.morph_mask=0;st.end=0;st.act=st.sel=U_RAMZA;
        for(i=0;i<NUNIT;i++){
            if(scenario==5){if(!i)ko_original_job=st.unit[i].job;continue;}
            st.unit[i].job=jobs_ko[i%6];st.unit[i].hp=0;st.unit[i].dc=3;
            st.unit[i].face=i&3;st.unit[i].team=TEAM_ENEMY;
            memset(&st.unit[i].statuses,0,sizeof st.unit[i].statuses);st.pending[i].state=0;
        }
        if(scenario==4){ko_board=2;st.morph_mask=(u16)((1U<<NUNIT)-1);
            for(i=0;i<NUNIT;i++)st.unit[i].dc=DC_CRYSTAL;}
        if(scenario==5)ko_board=3;
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
