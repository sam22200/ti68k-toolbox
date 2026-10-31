// Typed elemental scalar, signed HP and live cast/AI/packet boundaries.
#define ELEMENTS_CHECK
#include "check_magic.c"
#include "x/core-elements/oracle/source.h"
static AbId profile(const u16 *p)
{
    Unit *a,*t;u8 e=(u8)p[2];
    holy_prepare();a=&st.unit[U_RAMZA];t=&st.unit[10];a->mp=999;
    a->ma=p[0];a->faith=p[3];t->faith=p[4];t->hp=100;
    st.support[U_RAMZA]=p[5]?SUPPORT_MAGIC_ATTACK_UP:0;
    st.support[10]=p[6]?SUPPORT_MAGIC_DEFENSE_UP:0;
    if(p[7])t->statuses.innate[3]|=0x10;
    if(p[8])t->statuses.innate[2]|=2;
    memset(st.affinity[U_RAMZA],0,5);memset(st.affinity[10],0,5);
    st.affinity[U_RAMZA][4]=p[9]?e:0;
    st.affinity[10][0]=p[10]?e:0;st.affinity[10][1]=p[11]?e:0;
    st.affinity[10][2]=p[12]?e:0;st.affinity[10][3]=p[13]?e:0;
    return p[1]==255?0x104:p[1]==96?(e?0x100:0x101):e==128?0x102:e==0?0x103:0xf;
}
static void source_results(void)
{
    u16 n,amount;u8 kind,hit;u32 seed,hash;AbId ab;char text[40];
    for(n=0;n<sizeof source_elements/sizeof source_elements[0];n++){
        const u16 *p=source_elements[n];ab=profile(p);seed=rng32;hash=state_hash();
        kind=skill_kind(U_RAMZA,10,ab);CHECK(kind,p[16]==128?AB_DAMAGE:p[16]==64?AB_HEAL:AB_NULL);
        CHECK(state_hash()==hash,1);CHECK(rng32==seed,1);
        ZB(46);amount=skill_amount(U_RAMZA,10,ab,0);ZE(46);
        CHECK(amount,p[16]==64?p[18]:p[17]);hit=skill_hit(U_RAMZA,10,ab);CHECK(hit,100*p[15]);
        CHECK(rng32==seed,1);CHECK(magic_hp(kind,100,999,amount),p[19]);
        ZB(49);forecast_text(U_RAMZA,10,ab,text);ZE(49);
        CHECK(text[0],!p[15]?'M':((kind==AB_HEAL)^((s16)amount<0))?'+':(amount>=100?'K':'-'));
        act_start(U_RAMZA,10);st.abil=ab;st.hit=p[15];st.dmg=amount;st.effect_kind=kind;
        ZB(47);impact();ZE(47);CHECK(st.unit[10].hp,p[19]);CHECK(act_kind(),kind);
        impact();CHECK(st.unit[10].hp,p[19]);
    }
}
static void live_cast(u8 mode)
{
    u32 seed,after;u16 hp;u8 k;char text[40];
    holy_prepare();memset(st.affinity[U_RAMZA],0,5);memset(st.affinity[10],0,5);
    st.affinity[10][0]=2;if(mode==2)st.affinity[10][1]=2;
    st.unit[10].hp=500;st.reaction[10]=REACTION_POTION;
    if(mode==1)change(10,SID_SLEEP);
    seed=rng32;rand15();after=rng32;rng32=seed;
    CHECK(cast_order(U_RAMZA,10,0xf,1),2);CHECK(st.unit[U_RAMZA].mp,56);CHECK(rng32==seed,1);
    st.pending[U_RAMZA].ticks=0;ZB(48);CHECK(charge_resolve(),1);ZE(48);
    CHECK(st.unit[U_RAMZA].mp,0);CHECK(st.effect_kind,mode==2?AB_NULL:AB_HEAL);
    CHECK(st.hit,mode!=2);CHECK(st.dmg,mode==2?0:122);CHECK(rng32==(mode==2?seed:after),1);
    // A later query/live trait change cannot rewrite the prepared outcome/amount.
    st.affinity[10][1]=2;forecast_text(U_RAMZA,10,0xf,text);
    CHECK(st.effect_kind,mode==2?AB_NULL:AB_HEAL);
    hp=st.unit[10].hp;impact();CHECK(st.unit[10].hp,mode==2?hp:hp+122);
    CHECK(st.action.pending,0);CHECK(st.unit[10].kos,0);CHECK(rng32==(mode==2?seed:after),1);
    CHECK((core_status_current(&st.unit[10].statuses,4)&0x10)!=0,mode==1);
    if(mode==2)CHECK(st.gain,0);
    for(k=0;k<4&&st.action.phase!=CTX_IDLE;k++)end_anim();CHECK(st.unit[U_RAMZA].mp,0);
}
static void response_packet(void)
{
    holy_prepare();memset(st.affinity[U_RAMZA],0,5);memset(st.affinity[10],0,5);
    st.affinity[10][0]=2;st.unit[10].hp=100;st.reaction[10]=REACTION_POTION;
    cast_order(U_RAMZA,10,0xf,1);st.pending[U_RAMZA].ticks=0;charge_resolve();impact();
    CHECK(st.effect_kind,AB_HEAL);CHECK(st.action.pending,0);
    // Transport fixture only: healing does not naturally offer Auto-Potion.
    CHECK(core_action_queue(&st.action,10,REACTION_POTION,NUNIT),CTX_OK);end_anim();
    CHECK(st.primary.running,1);CHECK(st.primary.effect_kind,AB_HEAL);CHECK(st.effect_kind,AB_DAMAGE);
    impact();end_anim();CHECK(st.effect_kind,AB_HEAL);CHECK(act_kind(),AB_HEAL);
    CHECK(st.unit[U_RAMZA].mp,0);
}
static void gear_and_ai(void)
{
    u8 mode,k,found;u16 steps,body;Unit *t;
    holy_prepare();t=&st.unit[10];t->job=0x4a;memset(t->eq,255,5);
    for(body=0;body<255;body++){
        t->eq[EQ_BODY]=(u8)body;cj_affinity(common,t->job,t->eq,st.affinity[10]);
        if(st.affinity[10][0]&2)break;
    }
    CHECK(body<255,1);unit_stats(t);CHECK(st.affinity[10][0]&2,2);CHECK(skill_kind(U_RAMZA,10,0xf),AB_HEAL);
    for(mode=0;mode<3;mode++){
        holy_prepare();memset(st.affinity[U_RAMZA],0,5);memset(st.affinity[10],0,5);
        st.affinity[10][0]=mode?2:0;if(mode==2)st.affinity[10][1]=2;
        st.unit[10].team=mode==1?TEAM_PLAYER:TEAM_ENEMY;st.unit[10].hp=mode==1?100:999;
        st.unit[U_RAMZA].learned=0xffff;st.unit[U_RAMZA].defend=0;st.acted=0;st.moved=1;W->ai_k=0;
        steps=0;while(!ai_think()&&++steps<1024){}CHECK(steps<1024,1);found=0;
        for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==0xf&&W->ai_rank[k].pri>0)found=1;
        CHECK(found,mode!=2);CHECK(st.unit[U_RAMZA].mp,56);CHECK(st.unit[10].hp,mode==1?100:999);
    }
}
static void cold_forecast(void)
{
    char text[40];u32 seed;
    holy_prepare();memset(st.affinity[U_RAMZA],0,5);memset(st.affinity[10],0,5);seed=rng32;
#ifdef FFT_MODULES
    mod_call(MOD_PARTY,255,0);
#endif
    ZB(50);forecast_text(U_RAMZA,10,0xf,text);ZE(50);
    CHECK(memcmp(text,"-122HP 100%",12),0);CHECK(rng32==seed,1);CHECK(st.unit[U_RAMZA].mp,56);
}
static void elements_trials(void){source_results();live_cast(0);live_cast(1);live_cast(2);response_packet();gear_and_ai();cold_forecast();}
#ifndef __m68k__
int main(void){sw_init(61);elements_trials();printf("elements %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();elements_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
