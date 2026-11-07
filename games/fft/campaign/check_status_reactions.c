// Self-status reactions: distinct pre/post filters and the common status owner.
#define REACTION_SEQUENCE_CHECK
#define FFT_CHECK_NATIVE_ORDINAL
#include "check_reactions.c"
#include "x/core-status-reactions/oracle/source.h"
static void traits(Unit *u,u8 sid,u8 immutable,u8 existing)
{
    memset(&u->statuses,0,sizeof u->statuses);
    if(existing!=255)u->statuses.inflicted[existing>>3]|=0x80>>(existing&7);
    if(immutable&1)u->statuses.immune[sid>>3]|=0x80>>(sid&7);
    if(immutable&2)u->statuses.innate[sid>>3]|=0x80>>(sid&7);
    unit_status_refresh(u);
}
static void finish_status(void)
{
    u8 n;
    for(n=0;n<20&&st.action.phase!=CTX_IDLE;n++){
        if(st.action.phase==CTX_SECONDARY)impact();
        end_anim();
    }
    CHECK(st.action.phase,CTX_IDLE);
}
static void source_staging(void)
{
    u16 i;u32 seed,expected;u8 n;
    for(i=0;i<sizeof source_status_stages/sizeof source_status_stages[0];i++){
        const u16 *p=source_status_stages[i];Unit *t;Ability local[21];const Ability *saved;AbId ab;
        arena();t=&st.unit[10];t->hp=t->max_hp=100;t->brave=p[9];st.reaction[10]=p[0];
        traits(t,0,0,(u8)p[4]);st.unit[U_RAMZA].set=p[7];
        // Inject a frozen single-victim result into the real owned batch commit.
        // These profiles test packet categories/dispatch, not invented spells.
        // Row0 supplies the ordinary eligibility bit; row1 omits it; Math20
        // exercises the existing calculator exclusion. Only formula7 needs a
        // local evaluator record because healing weapons remain unexposed.
        saved=abilities;memcpy(local,saved,sizeof local);ab=p[7]==21?20:p[6]?0:1;
        local[ab].formula=p[8];local[ab].kind=AB_DAMAGE;local[ab].flags=0;abilities=local;
#ifdef FFT_MODULES
        mod_id=0;
#endif
        act_start(U_RAMZA,10);st.abil=ab;st.hit=p[1];st.dmg=p[3];st.push=st.self=0;
        st.effect_kind=p[2]==128?AB_DAMAGE:p[2]==64?AB_HEAL:AB_NULL;
        if(p[5])st.action.phase=CTX_SECONDARY;
        else{
            CastBatch *b=&W->cast;memset(b,0,sizeof *b);
            b->phase=CAST_BATCH_READY;b->actor=U_RAMZA;b->ability=ab;b->mask=(u16)1<<10;
            b->victim[10].effect=st.effect_kind;b->victim[10].hit=st.hit;b->victim[10].amount=st.dmg;
        }
        seed=rng32;for(n=0;n<p[12];n++)rand15();expected=rng32;rng32=seed;
        impact();CHECK(t->hp,p[11]);
        CHECK(st.action.pending&((u16)1<<10)?st.action.reaction[10]:0,p[10]);CHECK(rng32==expected,1);
        abilities=saved;
#ifdef FFT_MODULES
        mod_id=0;
#endif
    }
}
static void source_effects(void)
{
    u16 i;u8 sid,j;u32 seed;State before;
    for(i=0;i<sizeof source_status_effects/sizeof source_status_effects[0];i++){
        const u16 *p=source_status_effects[i];Unit *t;u16 jp;
        arena();t=&st.unit[10];t->hp=t->max_hp=100;t->mp=4;sid=p[0]==426?6:p[0]==427?18:25;
        traits(t,sid,(u8)p[2],(u8)p[1]);memset(t->status_ct,(u8)p[5],sizeof t->status_ct);
        if(p[1]==4||p[1]==7){st.pending[10].state=p[1]==4?DEFER_CHARGING:DEFER_PERFORMING;st.pending[10].ticks=7;}
        st.reaction[10]=0;act_start(U_RAMZA,10);st.abil=NO_ABIL;st.hit=0;impact();
        CHECK(core_action_queue(&st.action,10,p[0],NUNIT),CTX_OK);st.reaction[10]=p[0];jp=t->jp;seed=rng32;
        ZB(61);end_anim();ZE(61);CHECK(st.action.phase,CTX_SECONDARY);CHECK(st.atk,10);CHECK(st.tgt,10);
        CHECK(st.abil,p[0]);CHECK(st.hit,p[7]);CHECK(st.dmg,p[9]);CHECK(act_kind(),AB_STATUS);
        if(p[4])t->statuses.immune[sid>>3]|=0x80>>(sid&7);
        if(p[3]!=255)t->statuses.inflicted[p[3]>>3]|=0x80>>(p[3]&7);
        unit_status_refresh(t);
        ZB(62);impact();ZE(62);
        for(j=0;j<5;j++)CHECK(core_status_current(&t->statuses,j),p[10+j]);
        CHECK(t->status_ct[1],p[15]);CHECK(t->hp,100);CHECK(t->mp,4);CHECK(t->jp,jp);
        CHECK(st.gain,0);CHECK(rng32==seed,1);
        if(p[1]==24&&p[0]==428)CHECK(t->status_ct[0],0);
        if(p[1]==4||p[1]==7){CHECK(st.pending[10].state,p[1]==4?DEFER_CHARGING:DEFER_PERFORMING);CHECK(st.pending[10].ticks,7);}
        before=st;impact();CHECK(memcmp(&before,&st,sizeof st)==0,1);
        finish_status();CHECK(st.atk,U_RAMZA);CHECK(st.tgt,10);CHECK(t->jp,jp);
    }
}
static void real_reactions(void)
{
    u8 i;u16 root_jp,jp;u32 seed;State before;
    for(i=0;i<3;i++){
        arena();st.reaction[10]=REACTION_CAUTION+i;jp=st.unit[10].jp;root_jp=st.unit[U_RAMZA].jp;
        root(1);CHECK(st.action.pending,(u16)1<<10);CHECK(st.unit[U_RAMZA].jp,root_jp);
        seed=rng32;finish_status();CHECK(st.unit[10].jp,jp);CHECK(st.unit[U_RAMZA].jp>root_jp,1);
        CHECK(rng32==seed,1);CHECK(st.unit[10].ct,37);
        arena();before=st;seed=rng32;CHECK(can_use(U_RAMZA,REACTION_CAUTION+i),0);
        skill(U_RAMZA,10,REACTION_CAUTION+i);CHECK(memcmp(&before,&st,sizeof st)==0,1);CHECK(rng32==seed,1);
        arena();st.reaction[10]=REACTION_CAUTION+i;root(1);change(10,SID_DONT_ACT);end_anim();
        CHECK(st.action.phase!=CTX_SECONDARY,1);CHECK(st.action.pending,0);
        arena();st.reaction[10]=REACTION_CAUTION+i;root(1);st.unit[10].hp=0;end_anim();CHECK(st.action.phase!=CTX_SECONDARY,1);
    }
    // Caution/Dragon Spirit react to an eligible miss; Regenerator does not.
    for(i=0;i<3;i++){
        arena();st.reaction[10]=REACTION_CAUTION+i;act_start(U_RAMZA,10);st.abil=NO_ABIL;st.hit=0;impact();
        CHECK(st.action.pending,i==2?0:(u16)1<<10);finish_status();
    }
    // Regenerator removes mutable Poison, refreshes36 ticks and heals at upkeep.
    arena();st.reaction[10]=REACTION_REGENERATOR;change(10,SID_POISON);root(1);finish_status();
    CHECK(core_status_current(&st.unit[10].statuses,3)&0xc0,0x40);CHECK(st.unit[10].status_ct[0],0);
    CHECK(st.unit[10].status_ct[1],36);st.unit[10].hp=500;ct_end(10,1,1);CHECK(st.unit[10].hp,624);
    st.unit[10].status_ct[1]=1;st.unit[10].ct=0;st.unit[10].sp=0;st.unit[U_RAMZA].ct=0;st.unit[U_RAMZA].sp=100;
    CHECK(ct_next(),U_RAMZA);CHECK(core_status_current(&st.unit[10].statuses,3)&64,0);
    // Dragon Spirit's ordinary Reraise survives KO and is consumed on revival.
    arena();st.reaction[10]=REACTION_DRAGON_SPIRIT;root(1);finish_status();
    CHECK(core_status_current(&st.unit[10].statuses,2)&32,32);st.reaction[10]=0;
    act_start(U_RAMZA,10);st.abil=NO_ABIL;st.hit=1;st.dmg=999;st.push=0;impact();CHECK(st.unit[10].hp,0);
    finish_status();st.unit[10].ct=100;CHECK(ct_next(),10);CHECK(st.unit[10].hp,99);
    CHECK(core_status_current(&st.unit[10].statuses,2)&32,0);
    // Defending is an ordinary command-state bit, cleared at next turn entry.
    arena();st.reaction[10]=REACTION_CAUTION;root(1);finish_status();CHECK(st.unit[10].status&ST_DEFEND,ST_DEFEND);
    st.unit[10].ct=100;st.unit[U_RAMZA].ct=0;turn_start();CHECK(st.act,10);CHECK(st.unit[10].status&ST_DEFEND,0);
}
static void status_trials(void){source_staging();source_effects();real_reactions();}
#ifndef STATUS_REACTIONS_UI_CHECK
#ifndef __m68k__
int main(void){sw_init(61);status_trials();printf("status_reactions %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();status_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
#endif
