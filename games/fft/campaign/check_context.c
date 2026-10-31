// Root/impact ownership, bounded response collection and source reaction guards.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main context_runtime_main
#define game_render context_no_render
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef game_render
void context_no_render(void){}
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#include "gariland.c"
#include "x/core-context/oracle/source.h"
static u16 checks,failures;static u32 digest=17;
static void check(u16 a,u16 b,u16 line)
{
    checks++;digest=(digest<<1|digest>>31)^a;
    if(a!=b){failures++;
#ifndef __m68k__
        printf("line%u: %u expected%u\n",line,a,b);
#else
        BENCH_VALUE(line);
#endif
    }
}
#define CHECK(a,b) check((u16)(a),(u16)(b),__LINE__)
static void reset(void){game_scenario(61);st.act=U_RAMZA;st.ai=AI_OFF;st.banner=0;st.mode=M_MENU;}
static u32 state_hash(void)
{
    const u8 *p=(const u8 *)&st;u16 n;u32 h=17;
    for(n=0;n<sizeof st;n++)h=(h<<1|h>>31)^*p++;
    return h;
}
static u8 commit(void)
{
#ifdef FFT_MODULES
    return (u8)mod_call(MOD_ACTION,0,0);
#else
    return action_commit();
#endif
}
static void policy(void)
{
    CoreStatus s;u16 k;u8 i;memset(&s,0,sizeof s);
    for(k=0;k<sizeof source_reaction/sizeof source_reaction[0];k++){
        const u16 *p=source_reaction[k];for(i=0;i<5;i++)s.inflicted[i]=p[i];
        CHECK(core_reaction_blocked(&s,p[5],p[6],p[7],p[8]),p[9]);
        memcpy(s.innate,s.inflicted,5);memset(s.inflicted,0,5);
        CHECK(core_reaction_blocked(&s,p[5],p[6],p[7],p[8]),p[9]);memset(s.innate,0,5);
    }
    // Check the compact predicate against the initializer's complete five-byte set.
    for(i=0;i<40;i++){
        memset(&s,0,sizeof s);s.inflicted[i>>3]=(u8)(0x80>>(i&7));
        CHECK(core_reaction_blocked(&s,0,0,0,0)!=0,
            (source_prevention[i>>3]&(0x80>>(i&7)))!=0);
    }
}
static void queue_rules(void)
{
    CoreAction c;u8 i,j,id;u16 before;
    for(j=0;j<sizeof source_priority/sizeof source_priority[0];j++){
        const u16 *p=source_priority[j];u16 allowed=0x1fff;
        memset(&c,0,sizeof c);CHECK(core_action_begin(&c,6,13),CTX_OK);
        CHECK(core_action_queue(&c,12,0x1ba,13),CTX_OK);
        CHECK(core_action_queue(&c,1,0x1ba,13),CTX_OK);CHECK(core_action_queue(&c,0,0x1ba,13),CTX_OK);
        CHECK(core_action_queue(&c,6,0x1ba,13),CTX_INVALID);
        before=c.pending;CHECK(core_action_close(&c),0);CHECK(core_action_take(&c,allowed),255);CHECK(c.pending,before);
        CHECK(core_action_impact(&c,0),0);CHECK(core_action_impact(&c,6),1);CHECK(core_action_impact(&c,6),0);
        if(p[0]!=255)allowed&=(u16)~((u16)1<<p[0]);
        for(i=0;i<4;i++){
            id=core_action_take(&c,allowed);CHECK(id,p[i+1]);
            if(id==255)continue;
            CHECK(c.reaction[id],0x1ba);CHECK(core_action_close(&c),0);
            CHECK(core_action_queue(&c,2,0x1ba,13),CTX_NESTED);
            CHECK(core_action_impact(&c,id),1);CHECK(core_action_impact(&c,id),0);
            CHECK(core_action_return(&c),1);CHECK(c.actor,6);
        }
        CHECK(core_action_close(&c),1);CHECK(c.phase,CTX_IDLE);
    }
    // Fill all twelve eligible actor slots, reversed. Duplicate/invalid requests
    // return a reason and retain the existing request instead of overwriting it.
    memset(&c,0,sizeof c);CHECK(core_action_begin(&c,6,13),CTX_OK);
    for(i=13;i--;){if(i!=6)CHECK(core_action_queue(&c,i,0x1a6+i,13),CTX_OK);}
    before=c.pending;CHECK(before,0x1fbf);CHECK(core_action_begin(&c,0,13),CTX_BUSY);
    CHECK(core_action_queue(&c,12,0x1c5,13),CTX_DUPLICATE);CHECK(c.reaction[12],0x1b2);
    CHECK(core_action_queue(&c,13,0x1ba,13),CTX_INVALID);CHECK(core_action_queue(&c,0,0x1c6,13),CTX_INVALID);
    CHECK(core_action_queue(&c,0,0x1a5,13),CTX_INVALID);CHECK(c.pending,before);
    CHECK(core_action_impact(&c,6),1);CHECK(core_action_close(&c),0);
    for(i=0;i<13;i++)if(i!=6){CHECK(core_action_take(&c,0x1fff),i);CHECK(core_action_return(&c),1);}
    CHECK(core_action_take(&c,0x1fff),255);CHECK(core_action_close(&c),1);
    CHECK(core_action_impact(&c,6),0);CHECK(core_action_queue(&c,0,0x1ba,13),CTX_BUSY);
    CHECK(core_action_begin(&c,13,13),CTX_INVALID);CHECK(core_action_begin(&c,0,14),CTX_INVALID);
    CHECK(core_action_begin(&c,0,0),CTX_INVALID);
}
static void integration(void)
{
    Unit *a,*t;u16 hp,jp,seed,bag,steps;u32 snapshot;u8 i,target;CoreStatusDelta d;
    reset();a=&st.unit[U_RAMZA];t=&st.unit[10];CHECK(st.action.phase,CTX_IDLE);
    CHECK(battle_reaction_blocked(10),1);hp=t->hp;st.atk=U_RAMZA;st.tgt=10;st.hit=1;st.dmg=4;
    CHECK(commit(),0);CHECK(t->hp,hp); // no valid root, no impact
    attack(U_RAMZA,10);CHECK(st.action.phase,CTX_ROOT);CHECK(st.action.root,U_RAMZA);
    st.hit=1;st.dmg=4;st.push=0;CHECK(t->hp,hp);CHECK(battle_reaction_blocked(10),0);
    snapshot=state_hash();seed=rt_seed;
    skill(U_RAMZA,U_RAMZA,0x170);CHECK(state_hash()==snapshot,1);CHECK(rt_seed,seed);
    skill(U_RAMZA,10,0x196);CHECK(state_hash()==snapshot,1);
    defend(U_RAMZA);CHECK(state_hash()==snapshot,1);attack(0,10);CHECK(state_hash()==snapshot,1);
    st.act_t=ACT_HIT;ZB(26);attack_step();ZE(26);CHECK(st.action.phase,CTX_IMPACT);
    CHECK(t->hp,hp-4);hp=t->hp;jp=a->jp;CHECK(commit(),0);CHECK(t->hp,hp);CHECK(a->jp,jp);
    st.act_t=ACT_HIT;attack_step();CHECK(t->hp,hp);CHECK(a->jp,jp);
    // Actual response draining is covered by check_reactions. An ordinary
    // root still settles directly after its unchanged reward presentation.
    st.act_t=ACT_GAIN_END-1;attack_step();CHECK(st.action.phase,CTX_IDLE);CHECK(st.mode,M_MENU);
    // Rejections precede any RNG, item use, pending mutation or pointer access.
    snapshot=state_hash();seed=rt_seed;attack(NUNIT,10);skill(U_RAMZA,NUNIT,0x170);defend(NUNIT);
    CHECK(state_hash()==snapshot,1);CHECK(rt_seed,seed);
    // A miss still consumes its one impact slot; it cannot later become a hit.
    reset();t=&st.unit[10];hp=t->hp;attack(U_RAMZA,10);st.hit=0;st.act_t=ACT_HIT;attack_step();
    CHECK(st.action.phase,CTX_IMPACT);st.hit=1;st.dmg=99;CHECK(commit(),0);CHECK(t->hp,hp);
    st.act_t=ACT_END-1;attack_step();CHECK(st.action.phase,CTX_IDLE);
    // Item cost is paid once at start, even on a miss, and never refunded by close.
    reset();a=&st.unit[U_RAMZA];a->hp=1;bag=st.bag[abilities[0x170].bag];skill(U_RAMZA,U_RAMZA,0x170);
    CHECK(st.bag[abilities[0x170].bag],bag-1);st.hit=0;st.act_t=ACT_HIT;attack_step();
    CHECK(a->hp,1);st.act_t=ACT_END-1;attack_step();CHECK(st.bag[abilities[0x170].bag],bag-1);CHECK(st.action.phase,CTX_IDLE);
    // Deferred Charge arms without a live impact context, then begins its own root.
    reset();skill(U_RAMZA,10,0x196);CHECK(st.action.phase,CTX_IDLE);
    st.pending[U_RAMZA].ticks=0;charge_resolve();CHECK(st.action.phase,CTX_ROOT);CHECK(st.action.root,U_RAMZA);
    // Live source eligibility is read after a status change, not at enqueue time.
    reset();attack(U_RAMZA,10);memset(&d,0,sizeof d);d.add[3]=2;
    CHECK(battle_status_apply(10,&d,0)!=0,1);CHECK(battle_reaction_blocked(10),1);
    memset(&d,0,sizeof d);d.remove[3]=2;CHECK(battle_status_apply(10,&d,0)!=0,1);CHECK(battle_reaction_blocked(10),0);
    t=&st.unit[10];t->hp=0;CHECK(battle_reaction_blocked(10),1);CHECK(battle_reaction_blocked(NUNIT),1);
    // Cold and prepared real command starts include the changed image boundary.
    reset();
#ifdef FFT_MODULES
    mod_id=0;
#endif
    ZB(24);attack(U_RAMZA,10);ZE(24);st.hit=0;commit();core_action_close(&st.action);
    ZB(25);attack(U_RAMZA,10);ZE(25);
    // The source guard does not alter canonical state or use RNG.
    snapshot=state_hash();seed=rt_seed;target=st.formula.target;for(i=0;i<NUNIT;i++)battle_reaction_blocked(i);
    // formula.target is dispatch scratch, not battle/action state ownership.
    st.formula.target=target;CHECK(rt_seed,seed);CHECK(st.action.phase,CTX_ROOT);CHECK(state_hash()==snapshot,1);
    // Real AI_AIM dispatch after its HUD wait: share the same root and settlement.
    reset();st.ai=AI_AIM;st.ai_ab=NO_ABIL;st.ai_tgt=UNIT_CELL(&st.unit[10]);
    cursor_place(st.ai_tgt);st.ai_t=AI_PREVIEW-1;W->hud_ok=1;st.want=255;st.turn=0;
    ai_step();CHECK(st.action.phase,CTX_ROOT);CHECK(st.action.root,U_RAMZA);
    steps=0;while(st.action.phase!=CTX_IDLE&&steps++<ACT_GAIN_END+1)game_update();
    CHECK(steps<ACT_GAIN_END+1,1);CHECK(st.action.phase,CTX_IDLE);
}
static void trials(void){policy();queue_rules();integration();}
#ifndef __m68k__
int main(void){sw_init(61);trials();printf("context %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){context_runtime_main();trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
