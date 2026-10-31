// Source timers and actual FORMULA/scheduler/KO integration, no new art.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main lifetime_runtime_main
#define game_render lifetime_no_render
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef game_render
void lifetime_no_render(void){}
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#include "gariland.c"
#ifdef FFT_MODULES
#include "../status_policy.inc"
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
static u16 word(const u8 *p){return (u16)((u16)p[0]<<8|p[1]);}
static void oracle(void)
{
    u16 size,n,k;u8 i;const u8 *p=rt_file("fsl00",&size),*rules=cj_status_rules(common);
    CoreStatus s;CoreStatusDelta d;u8 ct[16];
    CHECK(p!=RT_NULL,1);if(!p)return;
    CHECK(size>=10,1);CHECK(!memcmp(p,"FSL1",4),1);
    {u16 need=10+word(p+4)*57+word(p+6)*65+word(p+8)*23;CHECK(size>=need&&size<=need+6,1);}
    n=word(p+4);p+=10;ZB(18);
    for(k=0;k<n;k++,p+=57){
        memcpy(&s,p,15);memcpy(ct,p+15,16);core_status_tick(&s,ct,d.remove);
        for(i=0;i<16;i++)CHECK(ct[i],p[31+i]);
        for(i=0;i<5;i++)CHECK(d.remove[i],p[47+i]);
        core_status_remove(&s,d.remove,ct);
        for(i=0;i<5;i++)CHECK(s.inflicted[i],p[52+i]);
    }
    ZE(18);n=word(rt_file("fsl00",&size)+6);
    for(k=0;k<n;k++,p+=65){
        u16 total;memcpy(&s,p,15);memcpy(ct,p+15,16);memcpy(&d,p+31,10);
        total=core_status_apply(&s,&d,0,p[41],rules,ct);CHECK(total,word(p+42));
        for(i=0;i<5;i++)CHECK(s.inflicted[i],p[44+i]);
        for(i=0;i<16;i++)CHECK(ct[i],p[49+i]);
    }
    n=word(rt_file("fsl00",&size)+8);
    for(k=0;k<n;k++,p+=23){
        u8 kind;u16 amount;memcpy(&s,p,15);kind=core_status_upkeep_kind(&s,p[15]);
        CHECK(kind,p[18]);amount=word(p+16)>>3;
        CHECK(kind==STATUS_UPKEEP_DAMAGE?amount:0,word(p+19));
        CHECK(kind==STATUS_UPKEEP_HEAL?amount:0,word(p+21));
    }
}
static void reset(void){game_scenario(61);st.act=U_RAMZA;st.ai=AI_OFF;st.banner=0;}
static u16 apply(u8 who,u8 bits)
{
    CoreStatusDelta d;memset(&d,0,sizeof d);d.add[3]=bits;
    return battle_status_apply(who,&d,0);
}
static void tick(void)
{
#ifdef FFT_MODULES
    mod_call(MOD_FORMULA,4,0);
#else
    status_tick();
#endif
}
static u8 upkeep(u8 who)
{
#ifdef FFT_MODULES
    return (u8)mod_call(MOD_FORMULA,5,who);
#else
    return status_upkeep(who);
#endif
}
static void integration(void)
{
    Unit *u,*v;u8 i,k;u16 jp,exp,hp,bag;CoreStatusDelta d;
    reset();u=&st.unit[U_RAMZA];CHECK(apply(U_RAMZA,0x80)!=0,1);
    CHECK(u->status,ST_POISON);CHECK(u->status_ct[0],36);CHECK(st.status_clock,(u16)1<<U_RAMZA);
    u->status_ct[0]=1;CHECK(apply(U_RAMZA,0x80)!=0,1);CHECK(u->status_ct[0],36);
    // Opposing statuses clear the old timer and start the new one.
    CHECK(apply(U_RAMZA,0x40)!=0,1);CHECK(u->status,0);
    CHECK(u->status_ct[0],0);CHECK(u->status_ct[1],36);
    CHECK(core_status_current(&u->statuses,3),0x40);
    u->hp=u->max_hp-1;ZB(20);CHECK(upkeep(U_RAMZA),0);ZE(20);CHECK(u->hp,u->max_hp);
    // A cure clears the timer/cache; innate effects do not expire or become curable.
    CHECK(apply(U_RAMZA,0x80)!=0,1);skill(U_RAMZA,U_RAMZA,0x176);
    st.act_t=ACT_HIT;attack_step();CHECK(u->status,0);CHECK(u->status_ct[0],0);CHECK(st.status_clock,0);
    CHECK(apply(U_RAMZA,0x80)!=0,1);u->statuses.innate[3]=0x80;u->status_ct[0]=1;
    memset(&d,0,sizeof d);battle_status_apply(U_RAMZA,&d,0);CHECK(st.status_clock,0);
    tick();CHECK(u->status_ct[0],1);CHECK(u->status,ST_POISON);
    hp=u->hp;CHECK(upkeep(U_RAMZA),0);CHECK(u->hp,hp-(u->max_hp>>3));
    // No input/menu animation advances the battle clock or applies upkeep.
    reset();u=&st.unit[U_RAMZA];CHECK(apply(U_RAMZA,0x80)!=0,1);hp=u->hp;
    st.mode=M_MENU;for(i=0;i<24;i++)game_update();
    CHECK(u->status_ct[0],36);CHECK(u->hp,hp);
    // End-turn upkeep is exactly once before a ready next unit, without rewards.
    jp=u->jp;exp=u->exp;bag=st.bag[0];
    for(i=0;i<NUNIT;i++)st.unit[i].ct=0;st.unit[0].ct=100;
    turn_end(1,1);CHECK(u->hp,hp-(u->max_hp>>3));CHECK(u->status_ct[0],36);
    CHECK(st.act,0);CHECK(u->jp,jp);CHECK(u->exp,exp);CHECK(st.bag[0],bag);
    // Charge at zero resolves before this tick expires statuses, then all
    // matching actors expire together before the next CT increment.
    reset();u=&st.unit[U_RAMZA];v=&st.unit[10];
    CHECK(apply(U_RAMZA,0x80)!=0,1);CHECK(apply(10,0x80)!=0,1);
    u->status_ct[0]=v->status_ct[0]=1;skill(U_RAMZA,10,0x196);
    st.pending[U_RAMZA].ticks=1;for(i=0;i<NUNIT;i++){st.unit[i].ct=0;st.unit[i].sp=1;}
    st.unit[0].sp=100;CHECK(ct_next(),254);CHECK(u->status_ct[0],1);CHECK(v->status_ct[0],1);
    st.pending[U_RAMZA].state=DEFER_NONE;unit_status_action(u,0);
    ZB(19);CHECK(ct_next(),0);ZE(19);CHECK(u->status,0);CHECK(v->status,0);CHECK(st.status_clock,0);
    // Jump/off-field callers suspend timer ticking; no mutation while absent.
    reset();u=&st.unit[U_RAMZA];CHECK(apply(U_RAMZA,0x80)!=0,1);u->status_ct[0]=1;
    u->statuses.inflicted[0]|=4;tick();CHECK(u->status_ct[0],1);u->statuses.inflicted[0]&=(u8)~4;
    k=u->x;u->x=255;tick();CHECK(u->status_ct[0],1);u->x=k;tick();CHECK(u->status,0);
    // Ordinary damage KO cancels mutable timed/command effects, preserving
    // Darkness/Silence, and revival cannot restore the old poison timer.
    reset();u=&st.unit[10];CHECK(apply(10,0x80)!=0,1);
    u->statuses.inflicted[1]=0x28;unit_status_refresh(u);u->hp=1;
    act_start(U_RAMZA,10);st.abil=NO_ABIL;st.hit=1;st.push=0;st.dmg=1;
    st.act_t=ACT_HIT;attack_step();CHECK(u->hp,0);CHECK(u->status,ST_DARK|ST_SILENCE);
    CHECK(u->status_ct[0],0);CHECK(st.status_clock,0);CHECK(u->dc,3);
    core_action_close(&st.action); // previous impact settled; presentation is outside this fixture
    skill(U_RAMZA,10,0x17D);st.dmg=10;st.act_t=ACT_HIT;attack_step();
    CHECK(u->hp,10);CHECK(u->status,ST_DARK|ST_SILENCE);CHECK(u->status_ct[0],0);
    // Wish's self-cost takes the same KO cleanup path without duplicate KOs.
    reset();u=&st.unit[U_RAMZA];CHECK(apply(U_RAMZA,0x80)!=0,1);u->hp=1;st.unit[10].hp=1;
    skill(U_RAMZA,10,0x98);st.act_t=ACT_HIT;attack_step();
    CHECK(u->hp,0);CHECK(u->kos,1);CHECK(u->status_ct[0],0);CHECK(st.status_clock,0);
    attack_step();CHECK(u->kos,1);
    // Every actor bit, including slot12, expires in one bounded native batch.
    reset();CHECK(NUNIT,13);
    for(i=0;i<NUNIT;i++){CHECK(apply(i,0x80)!=0,1);st.unit[i].status_ct[0]=1;}
    CHECK(st.status_clock,0x1fff);
#ifdef FFT_MODULES
    mod_id=0; // explicit cold-load measurement, fixture only
#endif
    ZB(21);tick();ZE(21);CHECK(st.status_clock,0);
    for(i=0;i<NUNIT;i++){CHECK(st.unit[i].status,0);CHECK(st.unit[i].status_ct[0],0);}
    // Ordinary clock work must not load the status image when no timers exist.
    for(i=0;i<NUNIT;i++){st.unit[i].ct=0;st.unit[i].sp=1;}st.unit[0].sp=100;
#ifdef FFT_MODULES
    mod_id=0;
#endif
    ZB(22);CHECK(ct_next(),0);ZE(22);
#ifdef FFT_MODULES
    CHECK(mod_id,0);
#else
    CHECK(0,0);
#endif
    // Lethal upkeep cancels a pending action and ends a lost battle without
    // entering an endless ct_next search when no player remains alive.
    reset();u=&st.unit[U_RAMZA];CHECK(apply(U_RAMZA,0x80)!=0,1);u->hp=1;
    for(i=0;i<NUNIT;i++)if(i!=U_RAMZA)st.unit[i].team=TEAM_ENEMY;
    st.pending[U_RAMZA].state=DEFER_CHARGING;u->statuses.inflicted[0]|=8;
    jp=u->jp;turn_end(1,1);CHECK(u->hp,0);CHECK(u->kos,1);CHECK(u->dc,3);
    CHECK(u->status,0);CHECK(st.pending[U_RAMZA].state,DEFER_NONE);CHECK(u->jp,jp);CHECK(st.end,END_DEFEAT);
}
static void trials(void){oracle();integration();}
#ifndef __m68k__
int main(void){sw_init(61);trials();printf("lifetime %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){lifetime_runtime_main();trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
