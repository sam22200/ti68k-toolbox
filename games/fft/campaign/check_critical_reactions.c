// Critical self reactions: post-damage eligibility, frozen effects and settlement.
#define STAT_SAVE_UI_CHECK
#include "check_stat_save.c"
#include "x/core-critical-reactions/oracle/source.h"
static void critical_traits(Unit *u,u8 sid,u8 traits)
{
    if(sid!=255)u->statuses.inflicted[sid>>3]|=(u8)(128>>(sid&7));
    if(traits&1)u->statuses.immune[2]|=1;
    if(traits&2)u->statuses.innate[2]|=1;
    unit_status_refresh(u);
}
static void critical_stages(void)
{
    u8 r;u16 i;
    for(r=0;r<3;r++)for(i=0;i<sizeof source_critical_stages/sizeof source_critical_stages[0];i++){
        const u16 *p=source_critical_stages[i];Unit *t;u32 seed,expected;
        arena();t=&st.unit[10];t->hp=p[0];t->max_hp=p[1];t->brave=p[8];
        st.reaction[10]=REACTION_HP_RESTORE+r;critical_traits(t,(u8)p[5],(u8)p[7]);
        act_start(U_RAMZA,10);st.abil=choose(p[3]==128?AB_DAMAGE:AB_HEAL,0);
        st.hit=p[2];st.dmg=p[4];st.push=st.self=0;st.effect_kind=p[3]==128?AB_DAMAGE:AB_HEAL;
        if(p[6])st.action.phase=CTX_SECONDARY;
        seed=rng32;if(p[12])rand15();expected=rng32;rng32=seed;
        impact();CHECK(t->hp,p[10]);CHECK(st.action.pending&((u16)1<<10)?st.action.reaction[10]:0,p[9]?REACTION_HP_RESTORE+r:0);
        CHECK(rng32==expected,1);
    }
}
static void critical_effects(void)
{
    u16 i;
    for(i=0;i<sizeof source_critical_effects/sizeof source_critical_effects[0];i++){
        const u16 *p=source_critical_effects[i];Unit *t;State once;u16 jp;u32 seed;
        arena();t=&st.unit[10];st.reaction[10]=p[0];t->hp=p[1];t->max_hp=p[2];
        t->mp=p[3];t->max_mp=p[4];t->ct=p[5];critical_traits(t,(u8)p[6],(u8)p[7]);
        st.pending[10].state=DEFER_CHARGING;st.pending[10].ticks=7;unit_status_action(t,8);
        // A Wall present at primary impact correctly prevents the hit. Exercise
        // the source response packet with Wall acquired after it was queued.
        if(p[6]==31){t->statuses.inflicted[3]&=0xfe;unit_status_refresh(t);}
        jp=t->jp;root(0);CHECK(st.action.pending,(u16)1<<10);CHECK(st.gain,0);
        if(p[6]==31)critical_traits(t,31,0);
        seed=rng32;ZB(61);end_anim();ZE(61);
        CHECK(st.action.phase,CTX_SECONDARY);CHECK(st.atk,10);CHECK(st.tgt,10);CHECK(st.abil,p[0]);
        CHECK(st.hit,p[11]);CHECK(st.dmg,p[0]==REACTION_HP_RESTORE?p[13]:p[0]==REACTION_MP_RESTORE?p[14]:p[15]);
        CHECK(act_kind(),p[11]?(p[0]==REACTION_HP_RESTORE?AB_HEAL:p[0]==REACTION_MP_RESTORE?AB_MP_HEAL:AB_QUICK):AB_NULL);
        CHECK(rng32==seed,1);
        if(p[8]!=65535)t->hp=p[8];if(p[9]!=65535)t->mp=p[9];
        ZB(62);impact();ZE(62);CHECK(t->hp,p[16]);CHECK(t->mp,p[17]);CHECK(t->ct,p[18]);CHECK(rng32==seed,1);
        CHECK(st.pending[10].state,DEFER_CHARGING);CHECK(st.pending[10].ticks,7);CHECK(t->jp,jp);
        once=st;impact();CHECK(memcmp(&once,&st,sizeof st)==0,1);
        finish_stats();CHECK(st.atk,U_RAMZA);CHECK(st.tgt,10);CHECK(st.abil,NO_ABIL);
        CHECK(t->ct,p[18]);CHECK(t->jp,jp);CHECK(st.unit[U_RAMZA].ct,21);
    }
}
static void critical_boundaries(void)
{
    u8 r;State before;u32 seed;Unit *t;
    for(r=0;r<3;r++){
        arena();t=&st.unit[10];t->hp=20;t->max_hp=100;st.reaction[10]=REACTION_HP_RESTORE+r;
        root(0);CHECK(st.action.pending,(u16)1<<10);change(10,SID_DONT_ACT);end_anim();CHECK(st.action.phase!=CTX_SECONDARY,1);
        arena();t=&st.unit[10];t->hp=20;t->max_hp=100;st.reaction[10]=REACTION_HP_RESTORE+r;
        root(0);CHECK(st.action.pending,(u16)1<<10);t->hp=0;end_anim();CHECK(st.action.phase!=CTX_SECONDARY,1);
        arena();before=st;seed=rng32;CHECK(can_use(U_RAMZA,REACTION_HP_RESTORE+r),0);
        skill(U_RAMZA,10,REACTION_HP_RESTORE+r);CHECK(memcmp(&before,&st,sizeof st)==0,1);CHECK(rng32==seed,1);
    }
    // A prepared child never recurses, including when its own reactor has a
    // different supported reaction equipped after preparation.
    arena();t=&st.unit[10];t->hp=20;t->max_hp=100;st.reaction[10]=REACTION_HP_RESTORE;root(0);end_anim();
    st.reaction[10]=REACTION_POTION;impact();CHECK(st.action.pending,0);CHECK(t->hp,100);finish_stats();
}
static void quick_settlement(void)
{
    u16 mask;prepare_spell(19);st.reaction[8]=st.reaction[10]=REACTION_CRITICAL_QUICK;
    st.unit[8].hp=st.unit[10].hp=200;st.unit[8].ct=st.unit[10].ct=37;
    CHECK(cast_order(U_RAMZA,second,19,0),2);st.pending[U_RAMZA].ticks=0;CHECK(charge_resolve(),1);
    mask=((u16)1<<8)|((u16)1<<10);impact();CHECK(st.action.pending,mask);
    end_anim();CHECK(st.action.phase,CTX_SECONDARY);CHECK(st.atk,8);CHECK(st.act,U_RAMZA);
    impact();CHECK(st.unit[8].ct,255);CHECK(st.action.pending,(u16)1<<10);CHECK(st.act,U_RAMZA);
    end_anim();CHECK(st.action.phase,CTX_SECONDARY);CHECK(st.atk,10);CHECK(st.act,U_RAMZA);
    impact();CHECK(st.unit[10].ct,255);CHECK(st.act,U_RAMZA);
    finish_stats();CHECK(st.action.pending,0);CHECK(st.primary.reward,0);
    // Due-root continuation may select Quick only after the root fully closes.
    CHECK(st.act,8);CHECK(st.unit[8].ct,0);CHECK(st.unit[10].ct,255);CHECK(W->cast.phase,CAST_BATCH_EMPTY);
}
static void critical_trials(void){critical_stages();critical_effects();critical_boundaries();quick_settlement();}
#ifndef CRITICAL_UI_CHECK
#ifndef __m68k__
int main(void){sw_init(61);critical_trials();printf("critical_reactions %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();critical_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
#endif
