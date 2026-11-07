// Canonical status policy/state through PC and actual core/FORMULA/AI images.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main status_state_runtime_main
#define game_render status_state_no_render
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef game_render
void status_state_no_render(void){}
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#include "gariland.c"
#ifdef FFT_MODULES
#include "../status_policy.inc" // same pure kernel; actual adapter below uses FORMULA
#endif
static u16 checks,failures;static u32 digest=17;
static void check(u16 a,u16 b,u16 line)
{
    checks++;digest=(digest<<1|digest>>31)^a;
    if(a!=b){failures++;
#ifndef __m68k__
        printf("line%u: %u expected%u\n",line,a,b);
#else
        BENCH_VALUE(line);
#endif
    }
}
#define CHECK(a,b) check((u16)(a),(u16)(b),__LINE__)
static void reset(void){game_scenario(61);st.act=U_RAMZA;st.ai=AI_OFF;}
static void oracle(void)
{
    u16 size,count,k;u8 i;const u8 *p=rt_file("fsp00",&size),*rules=cj_status_rules(common);
    CHECK(p!=RT_NULL,1);if(!p)return;
    CHECK(size>=6,1);CHECK(!memcmp(p,"FSP1",4),1);count=RD16(p,2);
    CHECK(size>=6+count*44U,1);CHECK(sizeof(CoreStatus),15);CHECK(sizeof(CoreStatusDelta),10);
    p+=6;ZB(16);
    for(k=0;k<count;k++,p+=44){
        CoreStatus s;CoreStatusDelta d;u16 total;
        memcpy(&s,p,15);memcpy(&d,p+15,10);
        total=core_status_filter(&s,&d,p[25],p[26],rules);
        CHECK(total,(u16)((u16)p[37]<<8|p[38]));
        for(i=0;i<10;i++)CHECK(((u8 *)&d)[i],p[27+i]);
        memcpy(&s,p,15);memcpy(&d,p+15,10);
        total=core_status_apply(&s,&d,p[25],p[26],rules,RT_NULL);
        CHECK(total,(u16)((u16)p[37]<<8|p[38]));
        for(i=0;i<5;i++)CHECK(s.inflicted[i],p[39+i]);
        for(i=0;i<10;i++)CHECK(((u8 *)&s)[5+i],p[5+i]);
    }
    ZE(16);
}
static void mutate(u8 id,u8 add,u8 remove,u8 expected)
{
    CoreStatusDelta d;u16 result;memset(&d,0,sizeof d);
    core_status_compact_mask(d.add,add);core_status_compact_mask(d.remove,remove);
    W->fc_key=77;W->status_i=11;
    ZB(17);result=battle_status_apply(id,&d,0);ZE(17);
    CHECK(result!=0,expected);
    CHECK(st.unit[id].status,core_status_compact(&st.unit[id].statuses));
    CHECK(W->fc_key,expected?0:77);CHECK(W->status_i,expected?0:11);
}
static void integration(void)
{
    Unit *u,*t;CoreStatusDelta d;u16 steps;u8 i,n;
    reset();u=&st.unit[U_RAMZA];t=&st.unit[10];
    // Immunity stops application; already-inflicted effects may still be cured.
    u->statuses.immune[1]=0x20;mutate(U_RAMZA,ST_DARK,0,0);CHECK(u->status,0);
    u->statuses.immune[1]=0;mutate(U_RAMZA,ST_DARK,0,1);CHECK(u->status,ST_DARK);
    u->statuses.immune[1]=0x20;mutate(U_RAMZA,0,ST_DARK,1);CHECK(u->status,0);
    // Innate Darkness remains active and cannot be cured or earn a fake reward.
    unit_status_seed(t,ST_DARK);t->statuses.innate[1]=0x20;unit_status_refresh(t);
    CHECK(status_curable(t,ST_DARK),0);CHECK(skill_hit(U_RAMZA,10,0x177),0);
    skill(U_RAMZA,10,0x177);st.act_t=ACT_HIT;attack_step();
    CHECK(t->status,ST_DARK);CHECK(st.gain,0);CHECK(st.bag[abilities[0x177].bag],1);
    core_action_close(&st.action); // skip remaining presentation before the next independent action
    // Mixed immutable Darkness/mutable Poison: every prediction and the commit
    // select Poison only, preserving the innate effect and its mutable overlap.
    unit_status_seed(t,ST_DARK|ST_POISON);t->statuses.innate[1]=0x20;unit_status_refresh(t);
    CHECK(status_curable(t,ST_DARK|ST_POISON),ST_POISON);
    skill(U_RAMZA,10,0x95);CHECK(st.dmg,ST_POISON);st.act_t=ACT_HIT;attack_step();
    CHECK(t->status,ST_DARK);CHECK(t->statuses.inflicted[1],0x20);CHECK(st.gain!=0,1);
    // The actual AI must not select a cure for an innate-only ailment.
    reset();u=&st.unit[U_RAMZA];t=&st.unit[10];u->set=0x19;u->learned=0;
    for(i=0;i<16;i++)if(cj_set_ability(common,u->set,i)==0x95)u->learned|=(u16)1<<i;
    CHECK(u->learned!=0,1);t->statuses.innate[1]=0x20;unit_status_refresh(t);
    st.moved=st.acted=0;W->ai_k=0;steps=0;
    while(!ai_think()&&++steps<1024){}
    CHECK(steps<1024,1);n=0;for(i=0;i<W->ai_n;i++)if(W->ai_rank[i].ab==0x95)n++;
    CHECK(n,0);
    // Command state and pending Charge agree; Defend interrupts the charge.
    reset();u=&st.unit[U_RAMZA];skill(U_RAMZA,10,0x196);
    CHECK(st.pending[U_RAMZA].state,DEFER_CHARGING);CHECK(core_status_current(&u->statuses,0)&8,8);
    defend(U_RAMZA);st.act_t=ACT_HIT;attack_step();CHECK(st.pending[U_RAMZA].state,DEFER_NONE);
    CHECK(core_status_current(&u->statuses,0)&15,2);CHECK(u->status,ST_DEFEND);
    core_action_close(&st.action);
    // Starting an ordinary attack clears the pending command without losing an
    // unrelated ailment, and unsupported effects fail before any mutation.
    skill(U_RAMZA,10,0x196);mutate(U_RAMZA,ST_DARK,0,1);attack(U_RAMZA,10);
    CHECK(st.pending[U_RAMZA].state,DEFER_NONE);CHECK(core_status_current(&u->statuses,0)&15,0);
    CHECK(u->status,ST_DARK);
    memset(&d,0,sizeof d);d.add[3]=1; // Reraise still needs its KO handler.
    CHECK(battle_status_apply(U_RAMZA,&d,0),0xffff);CHECK(u->status,ST_DARK);
    CHECK(battle_status_apply(NUNIT,&d,0),0xffff);CHECK(battle_status_apply(0,RT_NULL,0),0xffff);
    CHECK(battle_status_apply(0,&d,2),0xffff);
    memset(&d,0,sizeof d);core_status_compact_mask(d.add,ST_DARK);
    CHECK(battle_status_apply(U_RAMZA,&d,1),STATUS_REJECTED); // cannot bypass immunity
    memset(&d,0,sizeof d);core_status_compact_mask(d.remove,ST_DARK);
    CHECK(battle_status_apply(U_RAMZA,&d,1)!=0,1);CHECK(u->status,0);
    core_status_compact_mask(d.add,ST_DARK);memset(d.remove,0,5);
    u->hp=0;CHECK(battle_status_apply(U_RAMZA,&d,0),STATUS_REJECTED);u->hp=u->max_hp;
    {u8 x=u->x;u->x=255;CHECK(battle_status_apply(U_RAMZA,&d,0),STATUS_REJECTED);u->x=x;}
    reset();mutate(U_RAMZA,13,0,1);CHECK(st.unit[U_RAMZA].status,13);
    memset(&d,0,sizeof d);core_status_compact_mask(d.add,ST_POISON);
    CHECK(battle_status_apply(U_RAMZA,&d,0)!=0,1);CHECK(st.unit[U_RAMZA].status,15);
    CHECK(st.unit[U_RAMZA].status_ct[0],36);
}
static void trials(void){
#ifndef STATUS_STATE_INTEGRATION_ONLY
    oracle();
#endif
    integration();
}
#ifndef __m68k__
int main(void)
{
    sw_init(61);trials();printf("statusstate %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);
    return failures?1:0;
}
#else
void _main(void){status_state_runtime_main();trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
