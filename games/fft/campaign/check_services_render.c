// Selected current-layout cold bank pointers and accepted worker/report views.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#define RT_CYCLES
#include <tigcclib.h>
#undef _main
#define _main services_runtime_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#define game_scenario services_source_scenario
#include "manage.c"
#undef game_scenario
static u32 fixture_hash;static u8 fixture_ok;
void game_scenario(u16 n)
{
    u16 i;PropositionContext c;u32 gil;u8 team[2]={1,2};
    services_source_scenario(100);fixture_ok=ps.valid;
    if(!fixture_ok)return;
    ps.id=30;ps.n=2;ps.team[0]=1;ps.team[1]=2;ps.team[2]=255;
    ps_candidates();ps.screen=PS_WORKERS;ps.row=1;ps.top=0;
    if(n) {
        c=ps_context();gil=ps_money();
        fixture_ok=prop_dispatch(ps.rules,&save.missions.dispatches,30,&c,team,2,9,save.n,service_workers(&save),&gil)==PROP_OK;
        prop_put_long(save.money,gil);
        for(i=0;i<9;i++)prop_day(&save.missions.dispatches);
        fixture_ok&=prop_prepare(ps.rules,ps.rewards,&save.missions,&save,30,c.location,service_world_draw,&save)==PROP_OK;
        ps.screen=PS_REPORT;
    }
    fixture_hash=17;for(i=0;i<sizeof save;i++)fixture_hash=(fixture_hash<<1|fixture_hash>>31)^((const u8 *)&save)[i];
}
#ifdef __m68k__
void _main(void){services_runtime_main();BENCH_VALUE(fixture_ok);BENCH_VALUE(fixture_hash);BENCH_VALUE(sizeof(Save));}
#else
int main(int argc,char **argv)
{if(argc!=3)return 2;sw_init((u16)atoi(argv[1]));sw_step(0);sw_step(0);printf("services %04X valid%u state%08lX Save%u\n",sw_checksum(),fixture_ok,(unsigned long)fixture_hash,(unsigned)sizeof(Save));return !fixture_ok||sw_write_png(argv[2],1);}
#endif
