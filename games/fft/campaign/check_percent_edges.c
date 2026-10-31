// Reflected percentages and empty/missed/capped paired resource events.
#define PERCENT_RUNTIME_CHECK
#include "check_percent.c"
static void percent_reflection(void)
{
    u8 j,k;u16 amount,cost;
    for(j=0;j<2;j++){
        AbId ab=42+j;prepare_percent(ab);st.unit[U_RAMZA].hp=999;
        st.unit[U_RAMZA].ma=99;st.unit[8]=st.unit[10];unit_place(&st.unit[8],third);
        st.unit[10].faith=0;change(10,SID_REFLECT);st.support[U_RAMZA]=SUPPORT_NON_CHARGE;
        amount=j?500:250;cost=skill_cost(U_RAMZA,ab);
        CHECK(cast_order(U_RAMZA,10,ab,1),1);impact();CHECK(st.unit[10].hp,999);
        CHECK(st.action.pending&((u16)1<<10),(u16)1<<10);change(8,SID_REFLECT);
        end_anim();CHECK(st.action.phase,CTX_SECONDARY);CHECK(st.tgt,8);
        CHECK(st.abil,ab);CHECK(st.hit,1);CHECK(st.dmg,amount);impact();
        CHECK(st.unit[8].hp,999>2*amount?999-2*amount:0);CHECK(st.action.pending&((u16)1<<8),0);
        for(k=0;k<20&&st.action.phase!=CTX_IDLE;k++){if(st.action.phase==CTX_SECONDARY)impact();end_anim();}
        CHECK(st.action.phase,CTX_IDLE);CHECK(st.unit[U_RAMZA].mp,999-cost);
    }
}
static void drain_boundaries(void)
{
    u8 j,mode;u16 cost,amount,mp,hp;
    for(j=0;j<2;j++)for(mode=0;mode<5;mode++){
        AbId ab=47+j;prepare_percent(ab);st.unit[U_RAMZA].ma=99;
        st.support[U_RAMZA]=SUPPORT_NON_CHARGE;cost=skill_cost(U_RAMZA,ab);amount=j?250:330;
        if(mode==0)change(10,SID_REFLECT); // Neither source drain is reflectable.
        if(mode==2){st.unit[U_RAMZA].ma=5;st.unit[U_RAMZA].faith=st.unit[10].faith=50;rng32=0x12345678UL;}
        if(mode==3){st.unit[U_RAMZA].hp=998;st.unit[U_RAMZA].mp=999;}
        if(mode==4){st.support[U_RAMZA]=SUPPORT_HALF_MP;cost>>=1;st.unit[U_RAMZA].mp=cost;}
        hp=st.unit[U_RAMZA].hp;mp=st.unit[U_RAMZA].mp;
        CHECK(cast_order(U_RAMZA,mode==1?third:10,ab,mode==1?0:1),mode==4?2:1);
        if(mode==4){st.pending[U_RAMZA].ticks=0;CHECK(charge_resolve(),1);}
        if(mode==1||mode==2){
            CHECK(W->cast.mask&((u16)1<<U_RAMZA),0);CHECK(W->cast.victim[U_RAMZA].hit,0);
        }else{
            CHECK(W->cast.victim[U_RAMZA].amount,amount);
            CHECK(W->cast.victim[U_RAMZA].effect,EFFECT_RETURN|(j?AB_HEAL:AB_MP_HEAL));
        }
        impact();CHECK(st.action.pending,0);
        CHECK(st.unit[U_RAMZA].hp,j&&mode!=1&&mode!=2?(hp+amount>999?999:hp+amount):hp);
        CHECK(st.unit[U_RAMZA].mp,!j&&mode!=1&&mode!=2?(mp-cost+amount>999?999:mp-cost+amount):mp-cost);
    }
}
static void mp_ai_boundaries(void)
{
    u8 mode,k,found;u16 steps;
    for(mode=0;mode<4;mode++){
        prepare_percent(47);st.unit[U_RAMZA].mp=mode==1?100:999;
        st.unit[10].mp=mode==1||mode==3?0:500;
        st.unit[10].set=11;st.unit[10].learned=mode>=2?1:0;
        st.moved=1;W->ai_k=0;steps=0;
        while(!ai_think()&&++steps<2048){}CHECK(steps<2048,1);found=0;
        for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==47)found=1;
        CHECK(found,mode==1||mode==2);CHECK(st.unit[U_RAMZA].mp,mode==1?100:999);
    }
}
static void edge_trials(void){percent_reflection();drain_boundaries();mp_ai_boundaries();}
#ifndef __m68k__
int main(void){sw_init(61);edge_trials();printf("percent_edges %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();edge_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
