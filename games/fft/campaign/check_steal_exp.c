// Steal Exp: source hit percent/amount oracle, production theft with the thief's level check,
// gates and AI policy (CORE_STEAL_EXP.md).
#define CLEANSE_RUNTIME_CHECK
#include "check_cleanse.c"
#include "x/core-steal-exp/oracle/source.h"
#include "../reward_value.h"
static const u8 steal_status[6][5]={{0},{0,0,0,0x20,0},{0,0,0,0,0x10},{8,0,0,0,0},{0,0,4,0,0},{0,0,2,0,0}};
static void prepare_steal(void)
{
    prepare_cleanse(115);st.unit[10].team=TEAM_ENEMY;st.unit[10].ct=10;
    st.unit[10].hp=st.unit[10].max_hp=999;
}
static void profiles(void)
{
    u16 i;u8 j;
    for(i=0;i<sizeof source_steal/sizeof source_steal[0];i++){
        const u8 *p=source_steal[i];Unit *a,*t;u16 c;
        prepare_steal();a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->sp=p[0];a->zodiac=p[1];t->zodiac=p[2];t->female=0;
        a->eq[EQ_WEAPON]=a->eq[EQ_SHIELD]=255;   // the source profiles are bare-handed (Martial Arts)
        st.support[U_RAMZA]=p[3]?SUPPORT_ATTACK_UP:p[4]?SUPPORT_MARTIAL_ARTS:0;
        memset(&t->statuses,0,sizeof t->statuses);
        for(j=0;j<5;j++)t->statuses.inflicted[j]=steal_status[p[5]][j];
        unit_status_refresh(t);t->exp=p[6];a->exp=p[7];a->level=p[8];
        c=charge_query(U_RAMZA,10,115,0,0);
        CHECK(skill_amount(U_RAMZA,10,115,0),p[12]&0x7f);          // 0: a forced miss in the source
        if(p[10])CHECK(c<100?c:100,p[11]);
        CHECK(p[13],p[12]!=0);                                      // the source roll passes these seeds
    }
}
static void steal_production(void)
{
    u8 s,found=0,lv,earn;Unit *a,*t;
    // A thief at 99 EXP levels up on the theft, then earns the ordinary success reward.
    for(s=1;s<200&&!found;s++){
        prepare_steal();a=&st.unit[U_RAMZA];t=&st.unit[10];a->sp=12;t->exp=50;a->exp=99;lv=a->level;
        rng32=s;skill(U_RAMZA,10,115);CHECK(st.mode,M_ACT);
        if(!st.hit)continue;
        found=1;CHECK(st.dmg,17);impact();
        CHECK(t->exp,33);CHECK(a->level,lv+1);
        earn=earned_exp(U_RAMZA,10,0);CHECK(a->exp,earn);
    }
    CHECK(found,1);
    // Nothing to steal: no hit is forecast.
    prepare_steal();st.unit[10].exp=0;CHECK(skill_hit(U_RAMZA,10,115),0);
    // Record, range and gates: physical (not blocked by Silence), unusable by a Frog.
    CHECK(abilities[115].kind,AB_STEAL_EXP);CHECK(abilities[115].range,1);CHECK(abilities[115].flags,AF_EVADE);
    prepare_steal();st.unit[U_RAMZA].statuses.inflicted[1]|=8;unit_status_refresh(&st.unit[U_RAMZA]);
    CHECK(can_use(U_RAMZA,115),1);
    st.unit[U_RAMZA].statuses.inflicted[2]|=2;unit_status_refresh(&st.unit[U_RAMZA]);CHECK(can_use(U_RAMZA,115),0);
}
static u8 plan(u8 who)
{
    u16 steps=0;st.act=who;st.moved=0;st.acted=0;W->ai_k=0;
    while(!ai_think()&&++steps<2048){}
    return steps<2048;
}
static void steal_ai(void)
{
    u8 k,found=0;
    prepare_steal();CHECK(plan(U_RAMZA),1);
    for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==115)found=1;
    CHECK(found,0);   // conservative AI policy, as for the stat breaks
}
static void steal_trials(void){profiles();steal_production();steal_ai();}
#ifndef __m68k__
int main(void){sw_init(61);steal_trials();printf("steal_exp %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();steal_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
