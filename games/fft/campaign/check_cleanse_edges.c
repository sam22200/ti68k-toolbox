// Scoped integration: real AI candidates, Reflect, timing and packed boundaries.
#define CLEANSE_RUNTIME_CHECK
#include "check_cleanse.c"
static void ai_cases(void)
{
    u8 j,k,found;u16 steps;const u8 *r;
    for(j=0;j<sizeof cleanses/sizeof cleanses[0];j++){
        AbId ab=cleanses[j];prepare_cleanse(ab);r=cj_section(common,CJ_STATUS)+6*abilities[ab].y;
        seed_profile(&st.unit[10],r,1);st.unit[U_RAMZA].ma=99;
        // Dispel's complete source set includes Transparent. Positive hostile
        // utility needs a visible target; hidden-enemy rejection is checked below.
        if(ab==56){st.unit[10].statuses.inflicted[2]&=(u8)~16;unit_status_refresh(&st.unit[10]);}
        st.moved=1;W->ai_k=0;steps=0;
        while(!ai_think()&&++steps<2048){}CHECK(steps<2048,1);found=0;
        for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==ab)found=1;
        CHECK(found,1);CHECK(st.unit[U_RAMZA].mp,999);
        if(ab>=368)CHECK(st.bag[abilities[ab].bag],3);
        prepare_cleanse(ab);st.moved=1;W->ai_k=0;steps=0;
        while(!ai_think()&&++steps<2048){}CHECK(steps<2048,1);found=0;
        for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==ab)found=1;
        CHECK(found,0);
    }
    prepare_cleanse(56);r=cj_section(common,CJ_STATUS)+6*abilities[56].y;seed_profile(&st.unit[10],r,1);
    st.moved=1;W->ai_k=0;steps=0;while(!ai_think()&&++steps<2048){}CHECK(steps<2048,1);
    found=0;for(k=0;k<W->ai_n;k++)if(W->ai_rank[k].ab==56)found=1;CHECK(found,0);
}
static void reflection(void)
{
    u8 k;u16 cost;
    prepare_cleanse(14);st.unit[U_RAMZA].ma=99;st.support[U_RAMZA]=SUPPORT_NON_CHARGE;
    st.unit[8]=st.unit[10];unit_place(&st.unit[8],third);
    st.unit[10].faith=0;change(10,SID_REFLECT);cost=skill_cost(U_RAMZA,14);
    CHECK(cast_order(U_RAMZA,10,14,1),1);impact();CHECK(st.action.pending&((u16)1<<10),(u16)1<<10);
    // The primary AoE has settled. The bounce sees the current endpoint status.
    change(8,SID_POISON);change(8,SID_REFLECT);end_anim();CHECK(st.action.phase,CTX_SECONDARY);
    CHECK(st.tgt,8);CHECK(st.hit,1);impact();CHECK(st.unit[8].status&ST_POISON,0);
    CHECK(core_status_current(&st.unit[8].statuses,4)&2,2);CHECK(st.action.pending&((u16)1<<8),0);
    for(k=0;k<20&&st.action.phase!=CTX_IDLE;k++){if(st.action.phase==CTX_SECONDARY)impact();end_anim();}
    CHECK(st.action.phase,CTX_IDLE);CHECK(st.unit[U_RAMZA].mp,999-cost);
    prepare_cleanse(56);st.unit[U_RAMZA].ma=99;st.support[U_RAMZA]=SUPPORT_NON_CHARGE;
    change(10,SID_REFLECT);change(10,SID_PROTECT);CHECK(cast_order(U_RAMZA,10,56,1),1);
    CHECK(W->cast.victim[10].effect,AB_CLEANSE);impact();CHECK(core_status_current(&st.unit[10].statuses,4)&2,0);
    CHECK(core_status_current(&st.unit[10].statuses,3)&32,0);CHECK(st.action.pending,0);
}
static void timing_and_bounds(void)
{
    CoreStatus s;u8 request[5]={255,255,0,0,0},remove[5];u8 j;char text[28];u16 cost;
    memset(&s,0,sizeof s);memset(s.inflicted,255,5);
    CHECK(core_cleanse_pack(&s,request),65535);core_cleanse_unpack(request,65535,remove);
    for(j=0;j<5;j++)CHECK(remove[j],request[j]);
    for(j=0;j<2;j++){
        AbId ab=j?56:14;prepare_cleanse(ab);change(10,j?SID_PROTECT:SID_POISON);
        cost=skill_cost(U_RAMZA,ab);st.support[U_RAMZA]=SUPPORT_HALF_MP;st.unit[U_RAMZA].mp=cost>>1;
        CHECK(cast_order(U_RAMZA,10,ab,1),2);CHECK(st.unit[U_RAMZA].mp,cost>>1);
        st.pending[U_RAMZA].ticks=0;CHECK(charge_resolve(),1);CHECK(st.unit[U_RAMZA].mp,0);
        prepare_cleanse(ab);change(U_RAMZA,SID_SILENCE);CHECK(cast_order(U_RAMZA,10,ab,1),0);
    }
    prepare_cleanse(378);change(U_RAMZA,SID_SILENCE);st.unit[10].statuses.inflicted[1]=128;unit_status_refresh(&st.unit[10]);
    CHECK(can_use(U_RAMZA,378),1);forecast_text(U_RAMZA,10,378,text);CHECK(memcmp(text,"Status 100%",11),0);
    st.abil=378;CHECK(aim_ok(10),1);skill(U_RAMZA,10,378);impact();CHECK(unit_petrified(&st.unit[10]),0);
    prepare_cleanse(380);change(10,SID_POISON);st.reaction[10]=REACTION_POTION;
    skill(U_RAMZA,10,380);impact();CHECK(st.action.pending,0);CHECK(st.unit[10].hp,999);
    prepare_cleanse(378);skill(U_RAMZA,10,378);CHECK(st.hit,0);CHECK(st.bag[abilities[378].bag],2);
    impact();CHECK(st.gain,0);CHECK(st.unit[10].hp,999);impact();CHECK(st.bag[abilities[378].bag],2);
    prepare_cleanse(378);st.unit[10].dc=DC_CRYSTAL;CHECK(skill_hit(U_RAMZA,10,378),0);
    st.unit[10].dc=3;st.unit[10].statuses.inflicted[0]=4;CHECK(skill_hit(U_RAMZA,10,378),0);
}
static void edge_trials(void){ai_cases();reflection();timing_and_bounds();}
#ifndef __m68k__
int main(void){sw_init(61);edge_trials();printf("cleanse_edges %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();edge_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
