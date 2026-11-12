// Selected native boundary check: fixed-unit PARTY initialization and SAVE receipt.
// Full combat remains in the PC/sanitizer replay, not repeated on the calculator.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main monster_unused_runtime_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#include "flow_battle.c"
#include "save.h"

static u16 checks,failures;
static u32 digest=17;
static Save initial,settled;
static void check(u16 got,u16 expected,u16 line)
{
    checks++;digest=(digest<<1|digest>>31)^got;
    if(got!=expected){
        failures++;
#ifndef __m68k__
        printf("monster_boot line%u: %u expected%u\n",line,got,expected);
#else
        BENCH_VALUE(line);
#endif
    }
}
#define CHECK(v,e) check((u16)(v),(u16)(e),__LINE__)
static void flush_save(void)
{
#ifdef __m68k__
    if(sv_data){save_write();sv_data=0;}
#endif
}
static void trials(void)
{
    static const u8 hp[6]={53,53,40,38,38,54};
    u8 i;
    CHECK(W!=RT_NULL,1);if(!W)return;
    CHECK(NUNIT,13);CHECK(MAP_W,11);CHECK(MAP_H,12);CHECK(CD(LOCATION),26);
    CHECK(st.party_in,1);CHECK(rt_load(SAVE_NAME,&initial,sizeof initial),1);
    for(i=7;i<13;i++){
        CHECK(st.unit[i].hp,hp[i-7]);CHECK(st.unit[i].level,3);
        CHECK(st.unit[i].zodiac,12);CHECK(st.unit[i].learned,7);
    }
    CHECK(st.unit[10].statuses.innate[2]&64,64);
    CHECK(st.unit[11].statuses.innate[2]&64,64);
    CHECK(st.unit[7].statuses.innate[2]&64,0);
    prepare_rewards();CHECK(st.reward_ready,1);CHECK(st.reward_base,1800);CHECK(st.reward_bonus,0);
    save_campaign();flush_save();
    CHECK(rt_load(SAVE_NAME,&settled,sizeof settled),1);
    CHECK(qlong(settled.money)==qlong(initial.money)+1800,1);
    CHECK(settled.n,initial.n);CHECK(settled.flags[0],initial.flags[0]|128);
    CHECK(settled.loc,26);CHECK(settled.deploy_n,0);
    save_campaign();flush_save();
    CHECK(rt_load(SAVE_NAME,&settled,sizeof settled),1);
    CHECK(qlong(settled.money)==qlong(initial.money)+1800,1);
}
#ifndef __m68k__
int main(void)
{
    sw_init(216);trials();
    printf("monster_boot %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);
    final_release();return failures!=0;
}
#else
void _main(void)
{
    game_init();game_scenario(216);trials();
    BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);
    final_release();
}
#endif
