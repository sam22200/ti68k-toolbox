// Math criteria, learned spell ownership and real shared execution.
#define REFLECT_CHECK
#define FFT_CHECK_NATIVE_ORDINAL
#include "check_offensive_magic.c"
#include "x/core-math/oracle/source.h"
#include "x/core-math/oracle/spells.h"
static void math_fixture(u8 fixture)
{
    u8 i;reset();
    for(i=0;i<NUNIT;i++){
        Unit *u=&st.unit[i];u8 sid=255,x=i%5,z=i/5;
        memset(&u->statuses,0,sizeof u->statuses);u->status_flags=0;u->hp=u->max_hp=999;u->dc=3;u->job=0x4a;
        if(fixture==1){static const u8 ids[]={SID_CRYSTAL,15,SID_JUMP,SID_DEAD,SID_PETRIFY};if(i<5)sid=ids[i];}
        if(fixture==2&&(i&1))sid=17;
        if(fixture==3&&(i==4||i==5)){x=3;z=0;if(i==4)sid=17;}
        if(fixture==5)z+=7;
        unit_place(u,(u8)node_at(x,z,0));
        u->ct=math_values[i];u->level=math_values[(i+fixture)%13];u->exp=math_values[(i+2*fixture)%13];
        if(sid!=255)u->statuses.inflicted[sid>>3]|=128>>(sid&7);
        if(sid==2)u->hp=0;
        unit_status_refresh(u);
        if(fixture==4&&(i==2||i==8)){u->x=255;u->dc=DC_GONE;}
    }
    st.unit[0].set=21;st.unit[0].learned=255;

}
static void geometry(void)
{
    u16 i;State before;u32 seed;u8 fixture=255;
    for(i=0;i<sizeof source_math/sizeof source_math[0];i++){
        if(fixture!=source_math[i].fixture){fixture=source_math[i].fixture;math_fixture(fixture);}
        before=st;seed=rng32;ZB(61);CHECK(cast_targets(0,source_math[i].code,20,MATH_BIND),source_math[i].mask);ZE(61);
        CHECK(memcmp(&before,&st,sizeof st)==0,1);CHECK(rng32==seed,1);
    }
    CHECK(cast_targets(0,16,20,MATH_BIND),TARGETS_INVALID);
    CHECK(cast_targets(0,0,414,MATH_BIND),TARGETS_INVALID);
    CHECK(cast_targets(NUNIT,0,20,MATH_BIND),TARGETS_INVALID);
}
static void prepare_math(void)
{
    prepare_spell(19);st.unit[U_RAMZA].set=21;st.unit[U_RAMZA].learned=255;
    st.unit[U_RAMZA].slot=255;st.unit[U_RAMZA].level=99;st.unit[U_RAMZA].mp=0;
    st.unit[10].level=2;st.unit[8].level=3;st.unit[8].hp=st.unit[10].hp=999;
    st.mode=M_MENU;st.sub=192|4;
}
static void finish_math(void)
{
    u8 n;for(n=0;n<20&&st.action.phase!=CTX_IDLE;n++){if(st.action.phase==CTX_SECONDARY)impact();end_anim();}CHECK(st.action.phase,CTX_IDLE);
}
static void evasion(void)
{
    u8 i,k;u32 seed,after;Unit *t;
    for(i=0;i<sizeof source_math_evade/sizeof source_math_evade[0];i++){
        const typeof(source_math_evade[0])*p=&source_math_evade[i];prepare_math();t=&st.unit[10];
        t->eq[EQ_ACC]=p->gear&1?232:255;t->eq[EQ_SHIELD]=p->gear&2?128:255;
        rng32=p->seed;seed=rng32;CHECK(magic_hit(U_RAMZA,10,20,0),p->chance);CHECK(rng32==seed,1);
        for(k=0;k<p->draws;k++)rand15();after=rng32;rng32=seed;
        CHECK(magic_hit(U_RAMZA,10,20,1),p->hit);CHECK(rng32==after,1);
    }
    prepare_math();CHECK(can_use(U_RAMZA,19),0); // retail Fire4 lacks the Math eligibility flag
}
static void ownership(void)
{
    AbId ab;u8 i,n;State before;u32 seed;CastBatch b;
    prepare_math();CHECK(known(U_RAMZA,255),45);

    for(i=0;i<8;i++){CHECK(abilities[414+i].kind,AB_MATH_SELECTOR);CHECK(can_use(U_RAMZA,414+i),0);}
    st.mode=M_STATUS;CHECK(known(U_RAMZA,255),8);st.mode=M_MENU;
    st.unit[U_RAMZA].slot=0;battle_save.n=1;memset(battle_save.unit[0].learned,0,sizeof battle_save.unit[0].learned);
    CHECK(known(U_RAMZA,255),0);CHECK(can_use(U_RAMZA,20),0);
    for(i=0;i<20;i++)memset(battle_save.unit[0].learned[i],255,sizeof battle_save.unit[0].learned[i]);
    CHECK(known(U_RAMZA,255),45);CHECK(can_use(U_RAMZA,20),1);
    st.unit[U_RAMZA].learned=15;CHECK(known(U_RAMZA,255),0);st.unit[U_RAMZA].learned=240;CHECK(known(U_RAMZA,255),0);
    prepare_math();change(U_RAMZA,SID_SILENCE);CHECK(can_use(U_RAMZA,20),1);CHECK(skill_cost(U_RAMZA,20),0);
    before=st;seed=rng32;CHECK(cast_order(U_RAMZA,second,20,0),0);CHECK(memcmp(&before,&st,sizeof st)==0,1);CHECK(rng32==seed,1);
    st.unit[U_RAMZA].learned=254;before=st;CHECK(cast_order(U_RAMZA,0,20,MATH_BIND),0);CHECK(memcmp(&before,&st,sizeof st)==0,1);
    prepare_math();change(U_RAMZA,SID_FROG);before=st;CHECK(cast_order(U_RAMZA,4,20,MATH_BIND),0);CHECK(memcmp(&before,&st,sizeof st)==0,1);
    prepare_math();change(U_RAMZA,SID_DONT_ACT);before=st;CHECK(cast_order(U_RAMZA,4,20,MATH_BIND),0);CHECK(memcmp(&before,&st,sizeof st)==0,1);
    // All45 learned spells use the same owned global packet, including radius-zero Holy/Flare.
    for(i=0;i<45;i++){
        prepare_math();ab=known(U_RAMZA,i);CHECK(ab<60,1);
        memset(&b,0,sizeof b);b.actor=U_RAMZA;b.ability=ab;b.ref=4;b.follow=MATH_BIND;before=st;seed=rng32;
        CHECK(cast_forecast(&b),1);CHECK(b.mask,((u16)1<<10)|((u16)1<<8));CHECK(memcmp(&before,&st,sizeof st)==0,1);CHECK(rng32==seed,1);
        ZB(62);CHECK(cast_order(U_RAMZA,4,ab,MATH_BIND),1);ZE(62);
        CHECK(st.mode,M_ACT);CHECK(st.pending[U_RAMZA].state,DEFER_NONE);CHECK(st.unit[U_RAMZA].mp,0);
        CHECK(W->cast.phase,CAST_BATCH_READY);CHECK(W->cast.mask,b.mask);CHECK(st.acted,1);
        ZB(63);impact();ZE(63);n=W->cast.phase;before=st;impact();CHECK(memcmp(&before,&st,sizeof st)==0,1);CHECK(W->cast.phase,n);
        finish_math();
    }
    prepare_math();change(10,38);st.reaction[10]=REACTION_COUNTER;st.unit[10].brave=100;
    CHECK(cast_order(U_RAMZA,4,20,MATH_BIND),1);CHECK(W->cast.victim[10].hit,1);
    impact();CHECK(st.unit[10].hp,999-5*cj_range(common,20)[10]);CHECK(st.action.pending,0);finish_math();
    // A Math user's ordinary Attack still permits Counter.
    prepare_math();st.reaction[10]=REACTION_COUNTER;st.unit[10].brave=100;
    attack(U_RAMZA,10);st.hit=1;st.dmg=1;impact();CHECK(st.action.pending&((u16)1<<10),(u16)1<<10);finish_math();
    prepare_math();st.moved=1;W->ai_k=0;seed=rng32;
    ZB(60);for(n=0,i=0;i<255&&!ai_think();i++){}ZE(60);CHECK(i<255,1);CHECK(W->ai_nab<=2,1);
    for(i=0;i<W->ai_n;i++)if(W->ai_rank[i].ab<60){CHECK(W->ai_rank[i].f&128,128);n++;}
    CHECK(n>0,1);CHECK(st.unit[U_RAMZA].mp,0);CHECK(st.unit[10].hp,999);
}
static void ai_execution(void)
{
    static const u8 spells[]={15,31,20};u8 i;
    for(i=0;i<3;i++){
        prepare_math();st.ai=AI_AIM;st.ai_ab=spells[i];st.ai_tgt=UNIT_CELL(&st.unit[U_RAMZA]);
        cursor_place(st.ai_tgt);W->hud_ok=1;st.ai_t=AI_PREVIEW+AI_NAME;st.mode=M_CAST_RANGE;
        ai_step();CHECK(st.mode,M_ACT);CHECK(W->cast.phase,CAST_BATCH_READY);
        CHECK(W->cast.follow,MATH_BIND);CHECK(W->cast.ref,4);CHECK(st.unit[U_RAMZA].mp,0);
        st.ai=AI_OFF;impact();finish_math();
    }
}
static void math_trials(void){geometry();evasion();ownership();ai_execution();}
#ifndef MATH_UI_CHECK
#ifndef __m68k__
int main(void){sw_init(61);math_trials();printf("math_skill %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();math_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif

#endif
