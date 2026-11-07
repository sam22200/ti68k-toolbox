// Pre-impact Brave/Faith/Transparent reactions and their existing status owners.
#define STATUS_REACTIONS_UI_CHECK
#include "check_status_reactions.c"
#include "x/core-brave-faith-reactions/oracle/source.h"
static void staged_reactions(void)
{
    u16 i;u8 n;u32 seed,expected;
    for(i=0;i<sizeof source_reaction_stages/sizeof source_reaction_stages[0];i++){
        const u16 *p=source_reaction_stages[i];Ability local[21];const Ability *saved;AbId ab;Unit *t;
        arena();t=&st.unit[10];t->hp=t->max_hp=100;t->brave=p[9];st.reaction[10]=p[0];
        traits(t,0,0,(u8)p[4]);st.unit[U_RAMZA].set=p[7];
        // Frozen packets test category/dispatch. Faith reads only zero/nonzero
        // effective cost: real Cure6 is its nonzero carrier, Attack0 its zero.
        saved=abilities;memcpy(local,saved,sizeof local);
        ab=p[7]==21?20:p[0]==REACTION_FAITH_UP?(p[10]?1:0):p[6]?0:1;
        local[ab].formula=p[8];local[ab].kind=AB_DAMAGE;local[ab].flags=0;abilities=local;
#ifdef FFT_MODULES
        mod_id=0;
#endif
        act_start(U_RAMZA,10);st.abil=ab;st.hit=p[1];st.dmg=p[3];st.push=st.self=0;
        st.effect_kind=p[2]==128?AB_DAMAGE:p[2]==64?AB_HEAL:AB_NULL;
        if(p[5])st.action.phase=CTX_SECONDARY;
        else{
            CastBatch *b=&W->cast;memset(b,0,sizeof *b);b->phase=CAST_BATCH_READY;
            b->actor=U_RAMZA;b->ability=ab;b->mask=(u16)1<<10;
            b->victim[10].effect=st.effect_kind;b->victim[10].hit=st.hit;b->victim[10].amount=st.dmg;
        }
        seed=rng32;for(n=0;n<p[13];n++)rand15();expected=rng32;rng32=seed;
        impact();CHECK(t->hp,p[12]);CHECK(st.action.pending&((u16)1<<10)?st.action.reaction[10]:0,p[11]);CHECK(rng32==expected,1);
        abilities=saved;
#ifdef FFT_MODULES
        mod_id=0;
#endif
    }
}
static void prepared_reactions(void)
{
    u16 i;u8 j;u32 seed;State before;
    for(i=0;i<sizeof source_reaction_effects/sizeof source_reaction_effects[0];i++){
        const u16 *p=source_reaction_effects[i];Unit *t;u16 jp;
        arena();t=&st.unit[10];t->hp=t->max_hp=100;t->mp=4;
        t->brave=p[0]==REACTION_FAITH_UP?100:p[1];t->faith=p[1];traits(t,19,(u8)p[3],p[0]==429&&(p[2]==21||p[2]==22)?255:(u8)p[2]);
        if(p[2]==4||p[2]==7){st.pending[10].state=p[2]==4?DEFER_CHARGING:DEFER_PERFORMING;st.pending[10].ticks=7;}
        st.reaction[10]=0;act_start(U_RAMZA,10);st.abil=NO_ABIL;st.hit=0;impact();
        CHECK(core_action_queue(&st.action,10,p[0],NUNIT),CTX_OK);st.reaction[10]=p[0];jp=t->jp;seed=rng32;
        ZB(61);end_anim();ZE(61);CHECK(st.action.phase,CTX_SECONDARY);CHECK(st.atk,10);CHECK(st.tgt,10);CHECK(st.abil,p[0]);
        CHECK(st.hit,p[6]);CHECK(st.dmg,p[0]==425?p[9]:(p[7]|p[8])?253:0);
        CHECK(act_kind(),p[0]==425?AB_STATUS:p[0]==429?AB_BRAVE:AB_FAITH);
        if(p[0]==429&&(p[2]==21||p[2]==22))t->statuses.inflicted[2]|=0x80>>(p[2]&7);
        if(p[5])t->statuses.immune[2]|=16;
        if(p[4]!=255)t->statuses.inflicted[p[4]>>3]|=0x80>>(p[4]&7);
        unit_status_refresh(t);ZB(62);impact();ZE(62);
        CHECK(t->brave,p[10]);CHECK(t->faith,p[11]);for(j=0;j<5;j++)CHECK(core_status_current(&t->statuses,j),p[12+j]);
        CHECK(t->hp,100);CHECK(t->mp,4);CHECK(t->jp,jp);CHECK(t->ct,37);CHECK(st.gain,0);CHECK(rng32==seed,1);
        if(p[2]==4||p[2]==7){CHECK(st.pending[10].state,p[2]==4?DEFER_CHARGING:DEFER_PERFORMING);CHECK(st.pending[10].ticks,7);}
        before=st;impact();CHECK(memcmp(&before,&st,sizeof st)==0,1);finish_status();CHECK(st.atk,U_RAMZA);CHECK(st.tgt,10);CHECK(t->jp,jp);
    }
}
static void sunken(void)
{
    u8 i;u32 seed;Unit *t;State before;
    for(i=0;i<sizeof source_removal/sizeof source_removal[0];i++){
        const u16 *p=source_removal[i];arena();t=&st.unit[10];traits(t,19,p[2]?2:0,p[2]?255:19);
        t->allegiance=p[1]?UNIT_TRANSPARENT_ARMED:0;ct_end(10,0,(u8)p[0]);CHECK(unit_transparent(t)!=0,!p[4]);
    }
    arena();t=&st.unit[10];st.reaction[10]=REACTION_SUNKEN_STATE;root(1);finish_status();CHECK(unit_transparent(t)!=0,1);
    // Walking/Wait preserves it. A newly gained status survives that same turn's
    // action; the next turn arms removal. It still occupies/blocks its terrain.
    CHECK(unit_at_cell(second),11);ct_end(10,1,0);CHECK(unit_transparent(t)!=0,1);
    t->allegiance&=(u8)~UNIT_TRANSPARENT_ARMED;ct_end(10,0,1);CHECK(unit_transparent(t)!=0,1);
    t->ct=100;st.unit[U_RAMZA].ct=0;turn_start();CHECK(st.act,10);CHECK(t->allegiance&UNIT_TRANSPARENT_ARMED,UNIT_TRANSPARENT_ARMED);
    ct_end(10,0,1);CHECK(unit_transparent(t),0);
    // Zero HP damage clears mutable Transparent, a miss/heal does not.
    for(i=0;i<3;i++){
        arena();t=&st.unit[10];traits(t,19,0,19);st.reaction[10]=0;
        // These are resolved packets: physical hit/miss retain their damage
        // category, while the third case explicitly supplies healing.
        act_start(U_RAMZA,10);st.abil=i==2?1:NO_ABIL;st.effect_kind=i==2?AB_HEAL:AB_DAMAGE;st.hit=i!=1;st.dmg=0;impact();
        CHECK(unit_transparent(t)!=0,i!=0);finish_status();
        if(i==2){traits(t,19,2,255);act_start(U_RAMZA,10);st.abil=NO_ABIL;st.hit=1;st.dmg=0;impact();CHECK(unit_transparent(t)!=0,1);finish_status();}
    }
    arena();t=&st.unit[10];traits(t,19,0,19);before=st;seed=rng32;
    CHECK(hit_chance(10,U_RAMZA),100);CHECK(roll_hit(10,U_RAMZA),1);{u32 actual=rng32;u8 n;rng32=seed;for(n=0;n<source_transparent[1][5];n++)rand15();CHECK(rng32==actual,1);rng32=actual;}
    CHECK(memcmp(&before,&st,sizeof st)==0,1);CHECK(unit_hidden_enemy(&st.unit[U_RAMZA],t)!=0,1);
    st.unit[U_RAMZA].learned=0;memset(st.bag,0,sizeof st.bag);
    st.act=U_RAMZA;st.moved=1;st.acted=0;st.ai=AI_THINK;st.mode=M_BROWSE;st.banner=st.turn=0;W->ai_k=0;
    {u16 n;u8 done=0;for(n=0;n<4096&&!done;n++)done=ai_think();CHECK(done,1);CHECK(st.ai_ab,AI_NONE);}
    arena();st.unit[U_RAMZA].set=18;st.unit[U_RAMZA].learned=0xffff;st.moved=st.acted=0;
    traits(&st.unit[U_RAMZA],19,0,19);skill(U_RAMZA,U_RAMZA,JUMP_FIRST);CHECK(unit_transparent(&st.unit[U_RAMZA])!=0,1);
    skill(U_RAMZA,10,JUMP_FIRST);CHECK(st.pending[U_RAMZA].state,DEFER_AIRBORNE);CHECK(unit_transparent(&st.unit[U_RAMZA]),0);
}
// Faith Up also sees HP-free status impacts, in both FORMULA entry paths.
static void status_packets(void)
{
    u8 mode;
    for(mode=0;mode<2;mode++){
        arena();st.unit[U_RAMZA].set=12;st.unit[10].faith=50;st.reaction[10]=REACTION_FAITH_UP;
        act_start(U_RAMZA,10);st.abil=34;st.effect_kind=AB_STATUS;st.hit=0;st.dmg=0;
        if(mode){CastBatch *b=&W->cast;memset(b,0,sizeof *b);b->phase=CAST_BATCH_READY;b->actor=U_RAMZA;b->ability=34;b->mask=(u16)1<<10;b->victim[10].effect=AB_STATUS;}
        impact();CHECK(st.action.pending,(u16)1<<10);finish_status();CHECK(st.unit[10].faith,53);
    }
}
static void self_faith(void)
{
    u8 mode;
    for(mode=0;mode<3;mode++){
        Unit *u;u16 jp;u32 seed;
        arena();u=&st.unit[U_RAMZA];u->set=11;u->faith=50;st.reaction[U_RAMZA]=REACTION_FAITH_UP;jp=u->jp;
        act_start(U_RAMZA,U_RAMZA);st.abil=mode==0?0:mode==1?1:34;st.hit=0;st.effect_kind=mode==2?AB_STATUS:AB_HEAL;
        impact();CHECK(st.action.pending,mode?(u16)1<<U_RAMZA:0);seed=rng32;finish_status();
        CHECK(u->faith,mode?53:50);CHECK(u->jp,jp);CHECK(rng32==seed,1);CHECK(st.atk,U_RAMZA);CHECK(st.tgt,U_RAMZA);
    }
}
static void guarded(void)
{
    static const u16 ids[3]={425,429,430};u8 i;State before;u32 seed;
    for(i=0;i<3;i++){
        arena();before=st;seed=rng32;CHECK(can_use(U_RAMZA,ids[i]),0);skill(U_RAMZA,10,ids[i]);CHECK(memcmp(&before,&st,sizeof st)==0,1);CHECK(rng32==seed,1);
        arena();st.reaction[10]=ids[i];act_start(U_RAMZA,10);st.abil=1;st.hit=0;impact();
        if(ids[i]==430){CHECK(st.action.pending,(u16)1<<10);change(10,SID_DONT_ACT);end_anim();CHECK(st.action.phase!=CTX_SECONDARY,1);}
    }
}
static void brave_faith_trials(void){staged_reactions();prepared_reactions();sunken();status_packets();self_faith();guarded();}
#ifndef BRAVE_FAITH_UI_CHECK
#ifndef __m68k__
int main(void){sw_init(61);brave_faith_trials();printf("brave_faith_reactions %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();brave_faith_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
#endif
