// Next-turn estimates against the real scheduler, and bounded weapon planning.
#define COMBAT_UI_CHECK
#include "check_combat.c"
#include "../ai_clock.h"
static u16 clock_actual(const Unit *profile)
{
    Unit *u,*w;u8 i,pick;u16 elapsed=0;
    combat_arena();
    for(i=0;i<NUNIT;i++){st.unit[i].hp=0;st.unit[i].dc=DC_CRYSTAL;}
    u=&st.unit[U_RAMZA];*u=*profile;unit_place(u,first);u->dc=3;unit_status_refresh(u);
    w=&st.unit[8];w->hp=w->max_hp=999;w->dc=3;unit_place(w,third);
    memset(&w->statuses,0,sizeof w->statuses);w->statuses.innate[1]=128;
    w->statuses.inflicted[3]=64;unit_status_refresh(w);
    st.status_clock=((u16)1<<8)|((u16)1<<U_RAMZA);st.open_ct_phase=0;st.mode=M_CLOCK;
    memset(st.pending,0,sizeof st.pending);
    do{
        w->status_ct[SID_REGEN-SID_POISON]=255;
        pick=ct_next();elapsed+=255-w->status_ct[SID_REGEN-SID_POISON];
        CHECK(pick==U_RAMZA||pick==CT_WAIT,1);
    }while(pick!=U_RAMZA&&elapsed<768);
    return pick==U_RAMZA?elapsed:65535U;
}
static void clock_profiles(void)
{
    static const u8 speeds[]={0,1,9,50},cts[]={0,91,100},timers[]={0,1,3,17};
    Unit profile;u8 speed,ct,mode,timer;u16 forecast,actual;u32 seed;
    for(speed=0;speed<4;speed++)for(ct=0;ct<3;ct++)for(mode=0;mode<11;mode++)for(timer=0;timer<4;timer++){
        combat_arena();profile=st.unit[U_RAMZA];profile.sp=speeds[speed];profile.ct=cts[ct];
        if(mode==1||mode==3||mode==7)profile.statuses.inflicted[3]|=8;
        if(mode==2||mode==3||mode==8)profile.statuses.inflicted[3]|=4;
        if(mode==4||mode==7)profile.statuses.inflicted[3]|=2;
        if(mode==5||mode==8)profile.statuses.inflicted[4]|=16;
        if(mode==6)profile.statuses.innate[3]=4;
        if(mode==9)profile.statuses.innate[3]=2;
        if(mode==10)profile.statuses.innate[4]=16;
        profile.status_ct[SID_HASTE-SID_POISON]=profile.status_ct[SID_SLOW-SID_POISON]=timers[timer];
        profile.status_ct[SID_STOP-SID_POISON]=profile.status_ct[SID_SLEEP-SID_POISON]=timers[timer];
        unit_status_refresh(&profile);seed=rng32;forecast=ai_ticks(&profile);CHECK(rng32==seed,1);actual=clock_actual(&profile);
#ifndef __m68k__
        if(forecast!=actual)printf("clock speed%u CT%u mode%u timer%u: %u expected%u\n",profile.sp,profile.ct,mode,timers[timer],forecast,actual);
#endif
        CHECK(forecast,actual);
    }
    combat_arena();profile=st.unit[U_RAMZA];profile.hp=0;CHECK(ai_ticks(&profile),65535U);
    profile=st.unit[U_RAMZA];profile.statuses.innate[1]=128;CHECK(ai_ticks(&profile),65535U);
    profile=st.unit[U_RAMZA];profile.statuses.inflicted[0]=4;unit_status_refresh(&profile);CHECK(ai_ticks(&profile),65535U);
}
static void prepare_plan(AbId ab)
{
    u8 i=0;State before=st;u32 seed=rng32;st.act=U_RAMZA;
    W->ai_weapon_left=(u16)1<<(ab==NO_ABIL?0:ab-0x195);W->ai_weapon_at=0;
    while(W->ai_weapon_left&&i++<8){
        ZB(61);
#ifdef FFT_MODULES
        mod_call(MOD_WEAPON,6,0);
#else
        weapon_ai_prepare();
#endif
        ZE(61);
    }
    CHECK(i<=8,1);CHECK(W->ai_weapon_left,0);CHECK(rng32==seed,1);
    before.act=U_RAMZA;CHECK(memcmp(&before,&st,sizeof st)==0,1);
}
static s16 bonus(u8 weapon,u8 status_id,u8 innate,u8 immunity,u8 ally,u16 hp,AbId ab)
{
    Unit *u,*a;combat_arena();a=&st.unit[U_RAMZA];u=&st.unit[10];
    a->eq[EQ_WEAPON]=weapon;a->hp=300;u->hp=hp;u->team=ally?TEAM_PLAYER:TEAM_ENEMY;
    if(status_id!=255){
        if(innate)u->statuses.innate[status_id>>3]|=128>>(status_id&7);
        else if(immunity)u->statuses.immune[status_id>>3]|=128>>(status_id&7);
        else status(10,status_id);
    }
    prepare_plan(ab);return W->ai_weapon_bonus[ab==NO_ABIL?0:ab-0x195][10];
}
static void weapon_plans(void)
{
    s16 value;Unit *a,*t;u16 hp;
    // Plain attacks add no hidden utility; drain values its actor, including Undead reversal.
    CHECK(bonus(19,255,0,0,0,999,NO_ABIL),0);
    CHECK(bonus(61,255,0,0,0,999,NO_ABIL)>0,1);
    CHECK(bonus(61,SID_UNDEAD,0,0,1,500,NO_ABIL)<0,1);
    // Conditional poison is useful against an enemy, harmful on a friend, and
    // absent when immunity, an innate copy, a lethal hit or Golem prevents it.
    value=bonus(55,255,0,0,0,999,NO_ABIL);CHECK(value>0,1);
    CHECK(bonus(55,255,0,0,1,999,NO_ABIL)<0,1);
    CHECK(bonus(55,SID_POISON,0,1,0,999,NO_ABIL),0);
    CHECK(bonus(55,SID_POISON,1,0,0,999,NO_ABIL),0);
    CHECK(bonus(55,255,0,0,0,1,NO_ABIL),0);
    combat_arena();st.unit[U_RAMZA].eq[EQ_WEAPON]=55;st.golem[1]=100;prepare_plan(NO_ABIL);CHECK(W->ai_weapon_bonus[0][10],0);
    // Source cancellation cannot cure an innate ailment; Frog toggles are valued as removal.
    CHECK(bonus(31,SID_FROG,0,0,1,999,NO_ABIL)>0,1);
    CHECK(bonus(31,SID_FROG,1,0,1,999,NO_ABIL),0);
    // A free spell contributes its post-physical HP gain and respects elemental null/absorb.
    CHECK(bonus(53,255,0,0,0,999,NO_ABIL)>0,1);
    combat_arena();a=&st.unit[U_RAMZA];t=&st.unit[10];a->eq[EQ_WEAPON]=53;t->hp=500;st.affinity[10][1]=128;
    prepare_plan(NO_ABIL);CHECK(W->ai_weapon_bonus[0][10],0);
    st.affinity[10][1]=0;st.affinity[10][0]=128;prepare_plan(NO_ABIL);CHECK(W->ai_weapon_bonus[0][10]<0,1);
    // Cache rows include Charge's own power, not just the ordinary amount.
    combat_arena();a=&st.unit[U_RAMZA];a->eq[EQ_WEAPON]=61;a->hp=100;prepare_plan(NO_ABIL);hp=W->ai_weapon_bonus[0][10];
    prepare_plan(406);CHECK(W->ai_weapon_bonus[1][10]>=hp,1);
    // Dual weapons include the second hand only while target and actor survive.
    combat_arena();a=&st.unit[U_RAMZA];a->eq[EQ_WEAPON]=19;a->eq[EQ_SHIELD]=61;st.support[U_RAMZA]=SUPPORT_TWO_SWORDS;a->hp=100;
    prepare_plan(NO_ABIL);CHECK(W->ai_weapon_bonus[0][10]>0,1);
}
static void planner_choice(void)
{
    u16 i;Unit *a,*t;u8 ready;
    // The actual planner still has to choose a legal hit. Preview/cache setup is pure RNG.
    combat_arena();a=&st.unit[10];t=&st.unit[U_RAMZA];a->eq[EQ_WEAPON]=61;a->move=0;a->hp=100;a->learned=0;
    st.act=10;st.ai=AI_THINK;st.acted=st.moved=0;W->ai_k=0;
    for(i=0,ready=0;i<1000&&!ready;i++)ready=ai_think();
    CHECK(ready,1);CHECK(st.ai_ab,NO_ABIL);CHECK(st.ai_tgt,UNIT_CELL(t));
}
static void proc_turn_list(void)
{
    u8 i,found=0;combat_arena();st.unit[10].hp=1;status(10,SID_SLEEP);
    turn_order();for(i=0;i<ORDER_N;i++)if(st.turns[i]==10)found=1;CHECK(found,1);
    act_start(U_RAMZA,10);st.abil=NO_ABIL;st.hit=1;st.dmg=0;st.effect_kind=AB_DAMAGE;st.self=16;
    impact();CHECK(st.unit[10].hp,0);
    found=0;for(i=0;i<ORDER_N;i++)if(st.turns[i]==10)found=1;CHECK(found,0);
}
static void trials_ai_interactions(void){clock_profiles();weapon_plans();planner_choice();proc_turn_list();}
#ifndef __m68k__
int main(void){setbuf(stdout,NULL);sw_init(61);trials_ai_interactions();printf("ai_interactions %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();trials_ai_interactions();BENCH_VALUE(sizeof(Unit));BENCH_VALUE(sizeof(State));BENCH_VALUE(sizeof(Work));BENCH_VALUE(sizeof(Save));BENCH_VALUE(sizeof(ModCtx));BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
