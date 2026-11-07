// Source formula08 arithmetic and private Holy through cast/action/AI boundaries.
#define FFT_CHECK_NATIVE_ORDINAL // keep native assertion IDs within the variable ceiling
#define FFT_CHECK_NO_ZONES // equality carrier; dedicated planner/frame carriers retain timings
#define INCANTATION_CHECK
#include "check_casting.c"
#include "x/core-magic/oracle/source.h"
static void math_profiles(void)
{
    u16 i;u32 seed,hash;
    for(i=0;i<sizeof source_magic/sizeof source_magic[0];i++){
        const u16 *p=source_magic[i];Unit *a,*t;u8 ast,tst;
        prepared();a=&st.unit[U_RAMZA];t=&st.unit[10];a->ma=p[0];a->faith=p[2];t->faith=p[3];
        a->zodiac=p[10];t->zodiac=p[11];a->female=p[12];t->female=p[13];
        st.support[U_RAMZA]=p[4]?SUPPORT_MAGIC_ATTACK_UP:0;st.support[10]=p[5]?SUPPORT_MAGIC_DEFENSE_UP:0;
        ast=(p[8]&1?0x80:0)|(p[8]&2?0x40:0);tst=(p[9]&1?0x80:0)|(p[9]&2?0x40:0);
        a->statuses.innate[4]=ast;t->statuses.innate[4]=tst;
        if(p[6])t->statuses.innate[3]|=0x10;
        if(p[7]==1)t->statuses.innate[2]|=2;
        if(p[7]==2)t->statuses.innate[2]|=4;
        if(p[7]==4)t->statuses.innate[0]|=8;
        if(p[7]==8)t->statuses.innate[4]|=0x10;
        if(p[15]){t->eq[EQ_WEAPON]=0x88;t->eq[EQ_SHIELD]=0x8f;t->statuses.innate[0]|=2;}
        unit_status_refresh(a);unit_status_refresh(t);seed=rng32;hash=state_hash();
        ZB(39);CHECK(magic_amount(U_RAMZA,10,p[1]),p[16]);ZE(39);
        CHECK(state_hash()==hash,1);CHECK(rng32==seed,1);
        if(p[1]==50){
            CHECK(skill_amount(U_RAMZA,10,0xf,0),p[16]>999?999:p[16]);CHECK(rng32==seed,1);
            a->mp=56;CHECK(skill_hit(U_RAMZA,10,0xf),100*p[17]);
        }
    }
}
static void holy_prepare(void)
{
    prepared();st.unit[U_RAMZA].set=10;st.unit[U_RAMZA].ma=5;
    st.unit[U_RAMZA].faith=st.unit[10].faith=70;st.unit[U_RAMZA].zodiac=st.unit[10].zodiac=0;
    st.unit[U_RAMZA].mp=56;st.moved=st.acted=0;st.open_resolving=st.open_ct_phase=0;
    CHECK(abilities[0xf].formula,8);CHECK(abilities[0xf].y,50);CHECK(abilities[0xf].flags,AF_CAST);
}
static void execution(void)
{
    u16 hp;u32 seed,after;u8 k;
    holy_prepare();hp=st.unit[10].hp;seed=rng32;rand15();after=rng32;rng32=seed;
    CHECK(cast_order(U_RAMZA,10,0xf,1),2);CHECK(st.pending[U_RAMZA].ticks,6);
    CHECK(st.unit[U_RAMZA].mp,56);CHECK(rng32==seed,1);
    st.pending[U_RAMZA].ticks=0;ZB(40);CHECK(charge_resolve(),1);ZE(40);
    CHECK(st.unit[U_RAMZA].mp,0);CHECK(st.hit,1);CHECK(st.dmg,122);
    CHECK(st.crit,0);CHECK(st.push,0);CHECK(rng32==after,1);
    impact();CHECK(st.unit[10].hp,hp-122);impact();CHECK(st.unit[10].hp,hp-122);
    CHECK(st.unit[U_RAMZA].mp,0);
    for(k=0;k<3&&st.action.phase!=CTX_IDLE;k++)end_anim();CHECK(st.open_resolving,0);
    // Formula inputs remain live while a unit/tile-bound command is waiting.
    holy_prepare();CHECK(skill_amount(U_RAMZA,10,0xf,0),122);CHECK(cast_order(U_RAMZA,10,0xf,1),2);
    st.unit[U_RAMZA].ma=7;st.unit[10].faith=20;st.unit[10].statuses.innate[3]|=0x10;
    st.pending[U_RAMZA].ticks=0;CHECK(charge_resolve(),1);CHECK(st.dmg,28);CHECK(st.unit[U_RAMZA].mp,0);
    // A magical HP impact wakes Sleep, without using physical Sleep/Charging XA.
    holy_prepare();change(10,SID_SLEEP);CHECK(cast_order(U_RAMZA,10,0xf,1),2);
    st.pending[U_RAMZA].ticks=0;CHECK(charge_resolve(),1);CHECK(st.dmg,122);
    impact();CHECK(core_status_current(&st.unit[10].statuses,4)&0x10,0);
    // Post-impact reactions settle before the root resumes the clock.
    holy_prepare();st.reaction[10]=REACTION_POTION;CHECK(cast_order(U_RAMZA,10,0xf,1),2);
    st.pending[U_RAMZA].ticks=0;charge_resolve();impact();end_anim();
    CHECK(st.primary.running,1);CHECK(st.unit[U_RAMZA].mp,0);impact();end_anim();
    for(k=0;k<3&&st.action.phase!=CTX_IDLE;k++)end_anim();CHECK(st.action.phase,CTX_IDLE);
    CHECK(st.unit[U_RAMZA].mp,0);
    // Terminal detection waits for impact and the existing reward presentation.
    holy_prepare();st.unit[10].hp=1;cast_order(U_RAMZA,10,0xf,1);st.pending[U_RAMZA].ticks=0;
    charge_resolve();CHECK(st.end,END_NONE);impact();CHECK(st.end,END_NONE);
    for(k=0;k<3&&st.end==END_NONE;k++)end_anim();CHECK(st.end,END_VICTORY);
}
static void ai_formula(void)
{
    u8 k,level,found;u16 steps;s16 rank[2];u32 seed,after;
    for(level=0;level<2;level++){
        holy_prepare();st.unit[U_RAMZA].learned=0xffff;st.unit[U_RAMZA].faith=level?80:40;
        st.unit[U_RAMZA].defend=0;st.acted=0;st.moved=1;W->ai_k=0;
        seed=rng32;rand15();after=rng32;rng32=seed;
        steps=0;while(!ai_think()&&++steps<1024){}CHECK(steps<1024,1);found=0;
        for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==0xf){found=1;rank[level]=W->ai_rank[k].pri;}
        CHECK(found,1);CHECK(st.unit[U_RAMZA].mp,56);CHECK(st.unit[10].hp,999);
        // Only the source ranking draw: one rand() per ranked action until one passes its hit
        // chance (ai_choose); the evaluation itself draws nothing.
        after=rng32;rng32=seed;for(k=0;k<W->ai_n&&rand15()%100>=W->ai_rank[k].hit;k++){}
        CHECK(rng32==after,1);rng32=after;
    }
    CHECK(rank[1]>rank[0],1);
}
static void cold_boundaries(void)
{
    static const u8 credit[4]={47,27,27,7};u8 i,k;
    prepared();
    ZB(41);grid_tables();ZE(41);
    for(i=0;i<NUNIT;i++)for(k=0;k<4;k++){
        memset(&st.unit[i].statuses,0,sizeof st.unit[i].statuses);st.unit[i].ct=7;
        ct_end(i,k&1?2:0,k&2?4:0);CHECK(st.unit[i].ct,credit[k]);
    }
    ZB(42);bn_start(BN_START);ZE(42);CHECK(st.banner,BN_START);CHECK(st.banner_t,0);
    CHECK(W->bn_t0[0],33);CHECK(W->bn_t0[1],44);CHECK(W->bn_end,129);
    bn_start(BN_VICTORY);CHECK(W->bn_t0[0],33);CHECK(W->bn_end,117);
    bn_start(BN_DEFEAT);CHECK(W->bn_t0[0],33);CHECK(W->bn_end,115);
}
static void magic_trials(void){source_rules();math_profiles();execution();ai_formula();cold_boundaries();}
#ifndef ELEMENTS_CHECK
#ifndef __m68k__
int main(void){sw_init(61);magic_trials();printf("magic %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();magic_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
#endif
