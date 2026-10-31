// Source Sleep XA/wake rules and actual clock, Charge, impact and AI adapters.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main sleep_runtime_main
#define game_render sleep_no_render
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef game_render
void sleep_no_render(void){}
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#include "gariland.c"
#include "x/core-sleep/oracle/source.h"
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
static void reset(void){game_scenario(61);st.act=U_RAMZA;st.ai=AI_OFF;st.banner=0;st.mode=M_MENU;}
static u16 change(u8 who,u8 id,u8 add)
{
    CoreStatusDelta d;memset(&d,0,sizeof d);
    if(add)d.add[id>>3]=(u8)(0x80>>(id&7));else d.remove[id>>3]=(u8)(0x80>>(id&7));
    return battle_status_apply(who,&d,0);
}
static void xa_rules(void)
{
    u16 k;AbId ab,random=NO_ABIL,sword=NO_ABIL;
    reset();
    for(ab=0;ab<512;ab++){
        if(abilities[ab].formula==0x37)random=ab;
        if(abilities[ab].formula==0x2d)sword=ab;
    }
    CHECK(random!=NO_ABIL,1);CHECK(sword!=NO_ABIL,1);
    for(k=0;k<sizeof source_xa/sizeof source_xa[0];k++){
        const u16 *p=source_xa[k];Unit *a,*t;u16 d;
        reset();a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->eq[EQ_WEAPON]=255;a->pa=p[1];a->brave=100;a->zodiac=0;a->female=0;
        t->zodiac=p[2];t->female=p[3];
        if(p[0]&1)change(10,SID_SLEEP,1);
        if(p[0]&2)unit_status_action(t,8);
        CHECK(zodiac_xa(a,t,unit_status_xa(t,p[1])),p[4]);
        d=p[4]*p[1];CHECK(attack_damage(U_RAMZA,10,0,0),d>999?999:d);
        d=(p[4]+(p[4]>>1))*p[1];CHECK(attack_damage(U_RAMZA,10,1,0x4000),d>999?999:d);
        CHECK(skill_amount(U_RAMZA,10,random,0),p[4]);
        CHECK(skill_amount(U_RAMZA,10,sword,0),p[4]*(1+abilities[sword].y));
        // Charge adds its source power to XA before vulnerability/zodiac.
        if(p[1]>abilities[0x196].y){
            a->pa=p[1]-abilities[0x196].y;
            CHECK(skill_amount(U_RAMZA,10,0x196,0),p[4]*a->pa);
        }
        // Forecasts and actual AI evaluation must leave Sleep/timers/HP intact.
        CHECK((core_status_current(&t->statuses,4)&0x10)!=0,(p[0]&1)!=0);
    }
}
static void wakes(void)
{
    u16 k;AbId ab,break_ab=NO_ABIL;
    reset();for(ab=0;ab<512;ab++)if(abilities[ab].kind==AB_BREAK){break_ab=ab;break;}
    CHECK(break_ab!=NO_ABIL,1);
    for(k=0;k<sizeof source_wakes/sizeof source_wakes[0];k++){
        const u16 *p=source_wakes[k];Unit *t;u16 hp;
        // MP-damage and untyped HP are source-only evidence: no native producer yet.
        if(p[4]||(!p[1]&&p[2]))continue;
        reset();t=&st.unit[10];t->hp=t->max_hp=100;t->kos=0;
        change(10,SID_SLEEP,1);t->status_ct[11]=17;
        if(p[5]){t->statuses.innate[4]=0x10;unit_status_refresh(t);}
        act_start(U_RAMZA,10);st.abil=p[1]==0x40?0x170:p[1]?NO_ABIL:break_ab;
        st.hit=p[0];st.dmg=p[2];st.push=0;st.self=0;
        // Items commit their prepared effect kind (CORE_RESTORATION), not st.dmg alone.
        if(p[1]==0x40){st.dmg=p[3];st.effect_kind=abilities[0x170].kind;}
        st.act_t=ACT_HIT-1;attack_step();CHECK(core_status_current(&t->statuses,4)&0x10,0x10);
        W->frz=1;ZB(23);attack_step();ZE(23);
        CHECK(t->hp,p[6]);CHECK((core_status_current(&t->statuses,4)&0x10)!=0,p[7]);CHECK(t->status_ct[11],p[8]);
        if(!p[7])CHECK(t->status_flags&STATUS_CT_FROZEN,0);
        if(p[1]==0x80&&p[0]&&!p[5]&&t->hp)CHECK(W->frz,0);
        hp=t->hp;attack_step();CHECK(t->hp,hp);CHECK(t->kos,t->hp?0:1);
    }
}
static void integration(void)
{
    Unit *u,*t;u8 i,j;u16 steps,hp;AbId ab;
    reset();u=&st.unit[U_RAMZA];skill(U_RAMZA,10,0x196);
    CHECK(change(U_RAMZA,SID_SLEEP,1)!=0,1);CHECK(st.pending[U_RAMZA].state,DEFER_NONE);
    CHECK(core_status_current(&u->statuses,0)&15,0);CHECK(u->status_ct[11],60);
    CHECK(unit_can_move(u),0);CHECK(unit_can_act(u),0);CHECK(hit_chance(10,U_RAMZA),100);
    for(i=0;i<8;i++)CHECK(roll_hit(10,U_RAMZA),1);
    // Only Sleep's duration advances. A ready actor cannot act until cured/expired.
    for(i=0;i<NUNIT;i++){st.unit[i].ct=0;st.unit[i].sp=1;}u->ct=110;st.unit[0].ct=100;
    CHECK(ct_next(),0);CHECK(u->ct,110);
    u->ct=80;u->sp=6;u->status_ct[11]=1;st.unit[0].ct=0;st.unit[0].sp=100;
    CHECK(ct_next(),0);CHECK(u->ct,86);CHECK(u->status_flags&STATUS_CT_FROZEN,0);
    CHECK(st.status_clock&((u16)1<<U_RAMZA),0);
    // Both timers contribute to AT wait via max, not their sum.
    for(j=0;j<sizeof source_orders/sizeof source_orders[0];j++){
        const u16 *p=source_orders[j];reset();for(i=2;i<NUNIT;i++)st.unit[i].hp=0;st.act=0;
        st.unit[0].ct=0;st.unit[0].sp=6;st.unit[1].ct=50;st.unit[1].sp=6;
        change(1,SID_SLEEP,1);st.unit[1].status_ct[11]=p[1];
        if(p[0]){change(1,SID_STOP,1);st.unit[1].status_ct[6]=p[0];}
        turn_order();for(i=0;i<ORDER_N;i++)CHECK(st.turns[i],p[i+2]);
    }
    // Explicit cure does not also cure Stop; no control cache may stay stale.
    reset();u=&st.unit[12];change(12,SID_STOP,1);change(12,SID_SLEEP,1);
    CHECK(change(12,SID_SLEEP,0)!=0,1);CHECK(u->status_flags&STATUS_CT_FROZEN,STATUS_CT_FROZEN);
    CHECK(change(12,SID_STOP,0)!=0,1);CHECK(u->status_flags&STATUS_CT_FROZEN,0);CHECK(st.status_clock&0x1000,0);
    // Poison upkeep is not an HP-damage action. It must not wake Sleep.
    reset();u=&st.unit[U_RAMZA];change(U_RAMZA,SID_SLEEP,1);change(U_RAMZA,SID_POISON,1);
    hp=u->hp;st.unit[0].ct=100;turn_end(1,1);CHECK(u->hp,hp-(u->max_hp>>3));CHECK(core_status_current(&u->statuses,4)&0x10,0x10);
    // AI may evaluate a sleeping target without waking it, spending stock or cancelling its state.
    reset();t=&st.unit[10];change(10,SID_SLEEP,1);hp=t->hp;W->ai_k=0;steps=0;
    while(!ai_think()&&++steps<1024){}
    CHECK(steps<1024,1);CHECK(t->hp,hp);CHECK(t->status_ct[11],60);CHECK(core_status_current(&t->statuses,4)&0x10,0x10);
    // The same wake hook handles every currently supported cross-splash victim.
    // The cross splash is a weapon skill's; cast area magic has its own batch pipeline.
    reset();for(ab=0;ab<512;ab++)if((abilities[ab].flags&(AF_AREA|AF_CAST))==AF_AREA)break;
    CHECK(ab<512,1);t=&st.unit[10];t->hp=t->max_hp=999;
    u=&st.unit[0];u->x=t->x+1;u->z=t->z;u->hp=u->max_hp=999;change(0,SID_SLEEP,1);
    skill(U_RAMZA,10,ab);st.hit=1;st.push=0;st.dmg=1;st.act_t=ACT_HIT;attack_step();
    CHECK(u->hp<999,1);CHECK(core_status_current(&u->statuses,4)&0x10,0);CHECK(u->status_ct[11],0);
}
static void trials(void){xa_rules();wakes();integration();}
#ifndef __m68k__
int main(void){sw_init(61);trials();printf("sleep %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){sleep_runtime_main();trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
