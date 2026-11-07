// Equipped stat-save reactions share root/secondary ownership, never commands.
#define REFLECT_CHECK
#define FFT_CHECK_NATIVE_ORDINAL
#include "check_offensive_magic.c"
#include "x/core-stat-save/oracle/source.h"
static void finish_stats(void)
{
    u8 n;
    for(n=0;n<20&&st.action.phase!=CTX_IDLE;n++){
        if(st.action.phase==CTX_SECONDARY)impact();
        end_anim();
    }
    CHECK(st.action.phase,CTX_IDLE);
}
static void source_stats(void)
{
    u16 i;
    for(i=0;i<sizeof source_stat_stages/sizeof source_stat_stages[0];i++){
        const u16 *p=source_stat_stages[i];Unit *t;
        arena();t=&st.unit[10];t->hp=t->max_hp=100;st.reaction[10]=p[0];
        if(p[4]!=255)change(10,(u8)p[4]);
        act_start(U_RAMZA,10);st.abil=choose(p[2]==128?AB_DAMAGE:AB_HEAL,0);
        CHECK(st.abil!=0xfffd,1);st.hit=p[1];st.dmg=p[3];st.push=st.self=0;
        st.effect_kind=p[2]==128?AB_DAMAGE:AB_HEAL;
        if(p[5])st.action.phase=CTX_SECONDARY;
        impact();CHECK(t->hp,p[7]);
        CHECK(st.action.pending&((u16)1<<10)?st.action.reaction[10]:0,p[6]);
    }
    for(i=0;i<sizeof source_stat_effects/sizeof source_stat_effects[0];i++){
        const u16 *p=source_stat_effects[i];Unit *t;State before;u16 root_jp,target_jp;u32 seed;
        arena();t=&st.unit[10];t->pa=t->ma=p[1];t->sp=p[1]>50?50:p[1];
        st.reaction[10]=p[0];t->mp=4;t->max_mp=10;
        st.pending[10].state=DEFER_CHARGING;st.pending[10].ticks=7;unit_status_action(t,8);
        root_jp=st.unit[U_RAMZA].jp;target_jp=t->jp;
        root(1);CHECK(st.action.pending,(u16)1<<10);CHECK(st.unit[U_RAMZA].jp,root_jp);
        seed=rng32;ZB(61);end_anim();ZE(61);
        CHECK(st.action.phase,CTX_SECONDARY);CHECK(st.atk,10);CHECK(st.tgt,10);CHECK(st.abil,p[0]);
        CHECK(st.hit,p[2]);CHECK(st.dmg,1);CHECK(rng32==seed,1);
        ZB(62);impact();ZE(62);CHECK(t->pa,p[7]);CHECK(t->ma,p[8]);CHECK(t->sp,p[9]);
        CHECK(t->hp,998);CHECK(t->mp,4);CHECK(t->ct,37);CHECK(t->jp,target_jp);
        CHECK(st.pending[10].state,DEFER_CHARGING);CHECK(st.pending[10].ticks,7);
        before=st;impact();CHECK(memcmp(&before,&st,sizeof st)==0,1);
        finish_stats();CHECK(st.unit[U_RAMZA].jp>root_jp,1);CHECK(t->jp,target_jp);
        CHECK(st.atk,U_RAMZA);CHECK(st.tgt,10);CHECK(st.abil,NO_ABIL);CHECK(t->ct,37);
    }
}
static void boundaries_stats(void)
{
    u8 i;State before;u32 seed;Unit *t;
    for(i=0;i<3;i++){
        arena();t=&st.unit[10];st.reaction[10]=REACTION_PA_SAVE+i;t->brave=0;
        root(1);CHECK(st.action.pending,0);
        arena();t=&st.unit[10];st.reaction[10]=REACTION_PA_SAVE+i;root(1);
        change(10,SID_DONT_ACT);end_anim();CHECK(st.action.phase!=CTX_SECONDARY,1);CHECK(t->pa,5);
        arena();t=&st.unit[10];st.reaction[10]=REACTION_PA_SAVE+i;root(1);
        t->hp=0;end_anim();CHECK(st.action.phase!=CTX_SECONDARY,1);CHECK(st.action.pending,0);
        arena();t=&st.unit[10];st.reaction[10]=0;t->learned=0xffff;root(1);CHECK(st.action.pending,0);
        arena();before=st;seed=rng32;
        CHECK(can_use(U_RAMZA,REACTION_PA_SAVE+i),0);
        skill(U_RAMZA,10,REACTION_PA_SAVE+i);CHECK(memcmp(&before,&st,sizeof st)==0,1);CHECK(rng32==seed,1);
    }
    // target_query reserves254/255 for other pure queries. Invalid Math
    // criteria must never enter either callback through the public API.
    prepare_spell(20);st.unit[U_RAMZA].set=21;st.unit[U_RAMZA].learned=255;
    before=st;seed=rng32;
    CHECK(cast_targets(U_RAMZA,16,20,MATH_BIND),TARGETS_INVALID);
    CHECK(cast_targets(U_RAMZA,253,20,MATH_BIND),TARGETS_INVALID);
    CHECK(cast_targets(U_RAMZA,254,20,MATH_BIND),TARGETS_INVALID);
    CHECK(cast_targets(U_RAMZA,255,20,MATH_BIND),TARGETS_INVALID);
    CHECK(memcmp(&before,&st,sizeof st)==0,1);CHECK(rng32==seed,1);
    // Secondary damage cannot enqueue another secondary, even with a live equipped save.
    arena();st.reaction[10]=REACTION_PA_SAVE;attack(U_RAMZA,10);
    st.action.phase=CTX_SECONDARY;st.hit=1;st.dmg=1;impact();CHECK(st.action.pending,0);
}
static void area_stats(void)
{
    u16 mask;u8 pa,ma;u32 seed;
    prepare_spell(19);st.reaction[10]=REACTION_PA_SAVE;st.reaction[8]=REACTION_MA_SAVE;
    st.unit[10].brave=st.unit[8].brave=100;pa=st.unit[10].pa;ma=st.unit[8].ma;
    CHECK(cast_order(U_RAMZA,second,19,0),2);st.pending[U_RAMZA].ticks=0;CHECK(charge_resolve(),1);
    mask=((u16)1<<10)|((u16)1<<8);impact();CHECK(st.action.pending,mask);
    seed=rng32;finish_stats();CHECK(st.unit[10].pa,pa+1);CHECK(st.unit[8].ma,ma+1);CHECK(rng32==seed,1);
    CHECK(st.unit[10].pa,pa+1);CHECK(W->cast.phase,CAST_BATCH_EMPTY);
}
static void stat_trials(void){source_stats();boundaries_stats();area_stats();}
#ifndef STAT_SAVE_UI_CHECK
#ifndef __m68k__
int main(void){sw_init(61);stat_trials();printf("stat_save %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();stat_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
#endif
