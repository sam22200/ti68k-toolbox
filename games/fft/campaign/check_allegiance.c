// Steal Heart/Invitation: original formulas, ownership, expiry and private campaign saves.
#define CLEANSE_RUNTIME_CHECK
#define FFT_CHECK_NATIVE_ORDINAL
#include "check_cleanse.c"
#include "x/core-allegiance/oracle/source.h"
#ifdef FFT_MODULES
#include "../allegiance.inc"
#endif
static void prepare_allegiance(AbId ab)
{
    u8 i;prepare_cleanse(ab);
    for(i=0;i<NUNIT;i++)if(i!=U_RAMZA&&i!=10){st.unit[i].x=255;st.unit[i].dc=DC_GONE;}
    st.unit[0]=st.unit[10];st.unit[0].team=TEAM_ENEMY;unit_place(&st.unit[0],third);
    st.unit[10].team=TEAM_ENEMY;st.unit[10].female=1;st.unit[10].job=0x4a;
    st.unit[U_RAMZA].female=0;st.unit[U_RAMZA].ma=99;st.unit[U_RAMZA].mp=0;
    CHECK(can_use(U_RAMZA,ab),1);
}
static void seed_allegiance(Unit *t,u8 mode)
{
    if(mode==1)t->statuses.inflicted[4]|=16;
    if(mode==2)t->statuses.inflicted[4]|=32;
    if(mode==3)t->statuses.immune[4]|=32;
    if(mode==4)t->statuses.immune[1]|=64;
    if(mode==5)t->statuses.inflicted[1]|=64;
    if(mode==6)t->statuses.inflicted[1]|=128;
    if(mode==7)t->statuses.inflicted[0]|=32;
    if(mode==8)t->statuses.inflicted[3]|=1;
    unit_status_refresh(t);
}
static void allegiance_profiles(void)
{
    u16 i;u8 k;u32 seed,expected;CastBatch b;Unit *a,*t;
    for(i=0;i<sizeof source_allegiance/sizeof source_allegiance[0];i++){
        const u8 *p=source_allegiance[i];prepare_allegiance(p[0]);a=&st.unit[U_RAMZA];t=&st.unit[10];
        a->ma=p[1];a->zodiac=p[2];t->zodiac=p[3];t->female=p[4];
        st.support[U_RAMZA]=p[5]?SUPPORT_MAGIC_ATTACK_UP:0;seed_allegiance(t,p[6]);
        if(p[6]>=9)a->faith=t->faith=p[6]==9?0:100;
        memset(&b,0,sizeof b);b.actor=U_RAMZA;b.ref=10;b.follow=1;b.ability=p[0];
        seed=rng32;CHECK(cast_forecast(&b),1);CHECK(b.mask>>10&1,1);
        CHECK(b.victim[10].hit,p[8]?p[9]:0);CHECK(rng32==seed,1);
        seed=p[7]?p[7]:333;rng32=seed;for(k=0;k<p[11];k++)rand15();expected=rng32;rng32=seed;
        b.phase=CAST_BATCH_EMPTY;ZB(61);CHECK(cast_batch(CAST_BATCH_PREPARE,&b),1);ZE(61);
        CHECK(b.victim[10].hit,p[10]);CHECK(rng32==expected,1);
        if(p[10]){CHECK(b.victim[10].effect,AB_STATUS);CHECK(b.victim[10].amount,((u16)p[12]<<8)|p[13]);}
        CHECK(t->team,TEAM_ENEMY);CHECK(t->hp,999);CHECK(a->mp,0);
    }
}
static void finish_allegiance(void)
{
    u8 n;st.open_resolving=0;st.primary.reward=0;
    for(n=0;n<16&&st.action.phase!=CTX_IDLE;n++)end_anim();
    CHECK(st.action.phase,CTX_IDLE);
}
static void social_cast(AbId ab)
{
    u8 before=st.unit[10].team;u16 mp=st.unit[U_RAMZA].mp;
    rng32=1;ZB(62);CHECK(cast_order(U_RAMZA,10,ab,1),1);ZE(62);
    CHECK(st.unit[10].team,before);CHECK(st.unit[U_RAMZA].mp,mp);
    ZB(63);impact();ZE(63);
}
static u8 source_camp(u8 flags){return flags&0x30?TEAM_ENEMY:TEAM_PLAYER;}
static void original_teams(void)
{
    u8 i;Unit *u;
    for(i=0;i<sizeof source_allegiance_teams/sizeof source_allegiance_teams[0];i++){
        const u8 *p=source_allegiance_teams[i];prepare_allegiance(109);u=&st.unit[10];
        u->team=p[1]&0x30?TEAM_ENEMY:p[1]&8?TEAM_PLAYER:TEAM_GUEST;
        u->statuses.inflicted[p[0]>>3]|=128>>(p[0]&7);
        unit_allegiance(u,source_camp(p[2]),p[0]==9);unit_status_refresh(u);
        CHECK(u->team==TEAM_ENEMY,source_camp(p[3])==TEAM_ENEMY);
        CHECK(unit_home_team(u)==TEAM_ENEMY,source_camp(p[4])==TEAM_ENEMY);
        if(p[0]==34)CHECK(u->status_flags&STATUS_NO_CONTROL,STATUS_NO_CONTROL);
        memset(u->statuses.inflicted,0,5);unit_status_refresh(u);
        CHECK(u->team==TEAM_ENEMY,source_camp(p[5])==TEAM_ENEMY);
        if(p[0]==34)CHECK(u->team,p[1]&0x30?TEAM_ENEMY:p[1]&8?TEAM_PLAYER:TEAM_GUEST);
    }
}
static void remove_social(u8 target,u8 sid)
{
    CoreStatusDelta d;memset(&d,0,sizeof d);d.remove[sid>>3]=128>>(sid&7);
    CHECK(battle_status_apply(target,&d,1)!=STATUS_REJECTED,1);
}
static void status_clock_once(void)
{
#ifdef FFT_MODULES
    mod_call(MOD_ORDER,4,0);
#else
    status_tick();
#endif
}
static void allegiance_production(void)
{
    AbId ab;u8 i;u16 hp;u32 seed;Unit *t;
    for(ab=109;ab<=116;ab+=7){
        prepare_allegiance(ab);t=&st.unit[10];seed=rng32;CHECK(cast_targets(U_RAMZA,10,ab,1)>>10&1,1);CHECK(rng32==seed,1);
        social_cast(ab);CHECK(t->team,TEAM_PLAYER);
        CHECK(unit_home_team(t),ab==109?TEAM_ENEMY:TEAM_PLAYER);
        CHECK(t->hp,999);CHECK(t->mp,999);CHECK(unit_party_member(t),ab==116);
        seed=rng32;impact();CHECK(rng32==seed,1);CHECK(t->team,TEAM_PLAYER);
        finish_allegiance();remove_social(10,ab==109?SID_CHARM:SID_INVITE);
        CHECK(t->team,ab==109?TEAM_ENEMY:TEAM_PLAYER);CHECK(unit_home_team(t),ab==109?TEAM_ENEMY:TEAM_PLAYER);
    }
    // Charm lasts32 clock ticks, retains its original camp across reinfliction,
    // and cannot make the last original enemy count as defeated.
    prepare_allegiance(109);social_cast(109);finish_allegiance();t=&st.unit[10];
    st.unit[0].x=255;st.unit[0].dc=DC_GONE;CHECK(outcome(),END_NONE);
    CHECK(t->status_ct[SID_CHARM-SID_POISON],32);
    for(i=0;i<31;i++)status_clock_once();CHECK(t->team,TEAM_PLAYER);CHECK(t->status_ct[10],1);
    status_clock_once();CHECK(t->team,TEAM_ENEMY);CHECK(core_status_current(&t->statuses,4)&32,0);
    // HP-damage type wakes Charm, including a zero-damage hit; a miss does not.
    prepare_allegiance(109);social_cast(109);finish_allegiance();t=&st.unit[10];hp=t->hp;
    st.unit[U_RAMZA].pa=1;st.unit[U_RAMZA].zodiac=t->zodiac=0;t->eq[EQ_SHIELD]=t->eq[EQ_ACC]=255;
    rng32=1;attack(U_RAMZA,10);CHECK(st.hit,1);impact();CHECK(t->team,TEAM_ENEMY);CHECK(t->hp<=hp,1);
    // Temporary blue allegiance does not make an original enemy consume party stock.
    prepare_allegiance(109);social_cast(109);finish_allegiance();st.bag[0]=2;
    skill(10,U_RAMZA,368);CHECK(st.bag[0],2);impact();finish_allegiance();
    // Plain blue guests share the original blue camp's inventory.
    prepare_allegiance(109);st.unit[U_RAMZA].team=TEAM_GUEST;st.bag[0]=1;
    skill(U_RAMZA,10,368);CHECK(st.bag[0],0);impact();finish_allegiance();CHECK(can_use(U_RAMZA,368),0);
    // Native Invite is permanent blue AI control, without the original guest's KO immunity.
    prepare_allegiance(116);social_cast(116);finish_allegiance();t=&st.unit[10];
    CHECK(t->team==TEAM_GUEST,0);st.unit[0].x=255;st.unit[0].dc=DC_GONE;CHECK(outcome(),END_VICTORY);
    prepare_allegiance(109);change(U_RAMZA,SID_SILENCE);CHECK(can_use(U_RAMZA,109),1);
    change(U_RAMZA,SID_FROG);CHECK(can_use(U_RAMZA,109),0);
}
static void allegiance_immunity(void)
{
    u16 i;u8 eq[5],base;memset(eq,255,sizeof eq);base=source_allegiance_immunity[0x4a];
    for(i=0;i<416;i++){
        CHECK(cj_section(common,CJ_BATTLE)[CJ_ALLEGIANCE+i],source_allegiance_immunity[i]);
        if(i<160)CHECK(cj_allegiance_immune(common,(u8)i,eq),source_allegiance_immunity[i]);
        else if(i<415){eq[0]=(u8)(i-160);CHECK(cj_allegiance_immune(common,0x4a,eq),base|source_allegiance_immunity[i]);eq[0]=255;}
    }
    prepare_allegiance(116);st.unit[10].job=1;social_cast(116);CHECK(st.unit[10].team,TEAM_ENEMY);finish_allegiance();
    for(i=160;i<415;i++)if(source_allegiance_immunity[i]&2)break;CHECK(i<415,1);
    prepare_allegiance(109);st.unit[10].eq[EQ_ACC]=(u8)(i-160);social_cast(109);CHECK(st.unit[10].team,TEAM_ENEMY);finish_allegiance();
    CHECK((u8 *)&st.unit[0].allegiance-(u8 *)&st.unit[0],73);
}
static void allegiance_edges(void)
{
    Unit *t;u8 initial;CoreStatusDelta d;
    // A second charmer changes current camp, retains home camp, and resets its source timer.
    prepare_allegiance(109);social_cast(109);finish_allegiance();t=&st.unit[10];t->status_ct[10]=7;
    st.unit[U_RAMZA].team=TEAM_ENEMY;social_cast(109);CHECK(t->team,TEAM_ENEMY);CHECK(unit_home_team(t),TEAM_ENEMY);
    CHECK(t->status_ct[10],source_allegiance_repeat[2]);finish_allegiance();remove_social(10,SID_CHARM);CHECK(t->team,TEAM_ENEMY);
    // A charmed player still spends blue inventory, and remains in the saved party.
    prepare_allegiance(109);st.unit[U_RAMZA].team=TEAM_ENEMY;st.unit[10].team=TEAM_PLAYER;
    social_cast(109);finish_allegiance();t=&st.unit[10];CHECK(unit_home_team(t),TEAM_PLAYER);CHECK(unit_party_member(t),1);
    st.bag[0]=3;skill(10,U_RAMZA,368);CHECK(st.bag[0],2);impact();finish_allegiance();
    // Invite during Charm updates the permanent camp; expiry still leaves blue AI control.
    st.unit[U_RAMZA].team=TEAM_PLAYER;social_cast(116);finish_allegiance();
    CHECK(core_status_current(&t->statuses,4)&32,32);CHECK(unit_home_team(t),TEAM_PLAYER);
    remove_social(10,SID_CHARM);CHECK(t->team,TEAM_PLAYER);CHECK(t->status_flags&STATUS_NO_CONTROL,STATUS_NO_CONTROL);
    // Actorless generic infliction cannot invent an allegiance; removal is supported.
    memset(&d,0,sizeof d);d.add[4]=32;CHECK(battle_status_apply(10,&d,0),STATUS_REJECTED);
    d.add[4]=0;d.add[1]=64;CHECK(battle_status_apply(10,&d,0),STATUS_REJECTED);
    // Auto-battle cancellation cannot hand an involuntarily controlled unit to the player.
    st.act=10;st.ai=AI_THINK;st.mode=M_BROWSE;auto_control(0);CHECK(st.ai,AI_THINK);CHECK(st.mode,M_BROWSE);
    for(initial=0;initial<NUNIT;initial++){st.unit[initial].ct=0;st.unit[initial].sp=1;}
    st.unit[10].ct=100;turn_start();CHECK(st.act,10);CHECK(st.ai,AI_THINK);CHECK(st.mode,M_BROWSE);
    // An enemy's real Charm cancels the other actor's persistent performance.
    prepare_allegiance(109);CHECK(cast_order(U_RAMZA,first,86,0),2);
    CHECK(st.pending[U_RAMZA].state,DEFER_PERFORMING);st.act=10;st.acted=0;st.unit[10].ma=99;
    rng32=1;CHECK(cast_order(10,U_RAMZA,109,1),1);
    {State *before=malloc(sizeof(State));u32 seed=rng32;CHECK(before!=RT_NULL,1);
     if(before){memcpy(before,&st,sizeof(State));skill(U_RAMZA,10,147);
        CHECK(memcmp(before,&st,sizeof(State))==0,1);CHECK(rng32==seed,1);free(before);}}
    impact();CHECK(core_status_current(&st.unit[U_RAMZA].statuses,4)&32,32);
    CHECK(st.pending[U_RAMZA].state,DEFER_NONE);finish_allegiance();
    // The original blue guest inventory path is real, even while current allegiance is hostile.
    for(initial=0;initial<4;initial++){
        const u8 *p=source_allegiance_stock[initial];prepare_allegiance(109);t=&st.unit[10];
        t->team=p[0]&0x30?TEAM_ENEMY:p[0]&8?TEAM_PLAYER:TEAM_GUEST;
        t->statuses.inflicted[4]=32;unit_allegiance(t,source_camp(p[1]),0);unit_status_refresh(t);st.bag[0]=3;
        skill(10,U_RAMZA,368);CHECK(st.bag[0],p[3]);impact();finish_allegiance();
    }
}
static void ai_admission(void)
{
    AbId ab;u8 k,found;u16 steps;
    for(ab=109;ab<=116;ab+=7){
        prepare_allegiance(ab);st.act=U_RAMZA;st.moved=1;st.acted=0;W->ai_k=0;steps=0;
        while(!ai_think()&&++steps<2048){}CHECK(steps<2048,1);found=0;
        for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==ab)found=1;
        CHECK(found,1);
    }
}
static void flush_allegiance_save(void)
{
#ifdef __m68k__
    if(sv_data){save_write();sv_data=0;}
#endif
}
static void recruitment(void)
{
    Save back;u8 before,slot;u16 crc;
    reset();save_campaign();flush_allegiance_save();CHECK(rt_load("ffsave",&back,sizeof back),1);before=back.n;
    // Use the real party import path before recruiting an enemy.
    party_load();prepare_allegiance(116);party_load();st.unit[10].team=TEAM_ENEMY;st.unit[10].slot=255;st.unit[U_RAMZA].mp=0;
    social_cast(116);finish_allegiance();slot=st.unit[10].name;
    CHECK(rt_load("ffsave",&back,sizeof back),1);CHECK(back.n,before); // no persistence before victory
    save_campaign();flush_allegiance_save();CHECK(rt_load("ffsave",&back,sizeof back),1);
    CHECK(back.n,before+1);CHECK(back.unit[before].name,slot);CHECK(st.unit[10].slot,before);
    crc=back.n;save_campaign();flush_allegiance_save();CHECK(rt_load("ffsave",&back,sizeof back),1);CHECK(back.n,crc);
    // A later enemy Invitation removes that permanent roster member, after curing the Invite bit.
    remove_social(10,SID_INVITE);st.unit[U_RAMZA].team=TEAM_ENEMY;
    social_cast(116);finish_allegiance();CHECK(unit_home_team(&st.unit[10]),TEAM_ENEMY);
    save_campaign();flush_allegiance_save();CHECK(rt_load("ffsave",&back,sizeof back),1);CHECK(back.n,before);
    // A full formation retains every existing member and does not overwrite its final slot.
    while(back.n<PARTY_MAX){back.unit[back.n]=back.unit[1];back.unit[back.n].name=254;back.n++;}
    CHECK(rt_save("ffsave",&back,sizeof back),1);flush_allegiance_save();
    prepare_allegiance(116);party_load();st.unit[10].team=TEAM_ENEMY;st.unit[10].slot=255;st.unit[U_RAMZA].mp=0;
    social_cast(116);finish_allegiance();save_campaign();flush_allegiance_save();
    CHECK(rt_load("ffsave",&back,sizeof back),1);CHECK(back.n,PARTY_MAX);CHECK(back.unit[PARTY_MAX-1].name,254);
}
static void allegiance_trials(void){
#ifndef ALLEGIANCE_RUNTIME_ONLY
    allegiance_profiles();original_teams();
#endif
#ifndef ALLEGIANCE_MATH_ONLY
    allegiance_production();allegiance_immunity();allegiance_edges();ai_admission();recruitment();
#endif
}
#ifndef ALLEGIANCE_LABEL
#define ALLEGIANCE_LABEL "allegiance"
#endif
#ifndef __m68k__
int main(void){sw_init(61);allegiance_trials();printf(ALLEGIANCE_LABEL " %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();allegiance_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
