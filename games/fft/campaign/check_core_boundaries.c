// Synthetic checks against the actual descriptor-driven11/13-actor engine.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main boundaries_runtime_main
#define game_render boundaries_no_render // this checker exercises rules, not the full LCD renderer
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef game_render
void boundaries_no_render(void) {}
#undef _main
#define _main __main
#undef off
#endif
#include <stdlib.h>
static unsigned char fail_next_allocation;
static void *capacity_malloc(unsigned long bytes)
{
    if(fail_next_allocation){fail_next_allocation=0;return 0;}
    return malloc(bytes);
}
#define malloc capacity_malloc
#include "battle.c"
#undef malloc
#ifndef __m68k__
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
static u32 digest=17;
static u16 failures,checks;
static void check(u16 actual,u16 expected,u16 line)
{
    checks++;digest=(digest<<1|digest>>31)^actual;
    if(actual!=expected){failures++;
#ifndef __m68k__
        printf("line%u: %u expected%u\n",line,actual,expected);
#else
        BENCH_VALUE(line);
#endif
    }
}
#define CHECK(a,b) check((u16)(a),(u16)(b),__LINE__)
static void reset(u16 scenario)
{
    game_scenario(scenario);st.ai=st.banner=st.turn=0;st.want=255;
    memset(st.pending,0,sizeof st.pending);st.open_ct_phase=0;
}
static void trials(u16 scenario)
{
    u8 i,j,origin;
    reset(scenario);CHECK(NUNIT,CD(UNIT_COUNT));CHECK(UNIT_CAPACITY,13);CHECK(MAP_N,MAP_W*MAP_H);
    for(i=NUNIT;i<UNIT_CAPACITY;i++) {
        CHECK(st.unit[i].dc,DC_GONE);CHECK(st.unit[i].x,255);
        CHECK(st.unit[i].slot,255);CHECK(st.unit[i].hp,0);
    }
    // CT ties must use stable battle order, including the final slot.
    for(i=0;i<NUNIT;i++){st.unit[i].ct=100;st.unit[i].hp=10;st.unit[i].dc=0;st.unit[i].sp=1;}
    for(i=0;i<NUNIT;i++)CHECK(ct_next(),i);
    // All slots become due at the SAME tick; no second tick is allowed between
    // draining them. Resolving damage is covered separately by Charge fixtures.
    reset(scenario);
    for(i=0;i<NUNIT;i++){
        Unit *u=&st.unit[i];u->hp=10;u->dc=0;u->ct=0;u->sp=1;
        st.pending[i].state=DEFER_CHARGING;st.pending[i].ticks=1;
        st.pending[i].origin=(u8)(u->z*MAP_W+u->x);st.pending[i].weapon=hand_weapon(u,0);st.pending[i].off_weapon=hand_weapon(u,1);
    }
    CHECK(ct_next(),254);
    for(i=0;i<NUNIT;i++){
        CHECK(pending_next(),i);
        for(j=0;j<NUNIT;j++){CHECK(st.unit[j].ct,0);CHECK(st.pending[j].ticks,0);}
        st.pending[i].state=DEFER_NONE;
    }
    CHECK(pending_next(),255);
    // Interrupt each pending slot by KO, displacement and re-equipping.
    for(j=0;j<3;j++)for(i=0;i<NUNIT;i++){
        Unit *u;reset(scenario);u=&st.unit[i];
        st.pending[i].state=DEFER_CHARGING;st.pending[i].ticks=0;
        st.pending[i].origin=(u8)(u->z*MAP_W+u->x);st.pending[i].weapon=hand_weapon(u,0);st.pending[i].off_weapon=hand_weapon(u,1);
        if(j==0)u->hp=0;else if(j==1)unit_place(u,(u8)node_at(u->x^1,u->z,0));else u->eq[EQ_WEAPON]^=1;
        CHECK(pending_next(),255);CHECK(st.pending[i].state,DEFER_NONE);
    }
    // Ordinary KO still occupies the tile and can return; crystals/chests/gone
    // do not block it. Keep source positions unique; no synthetic actors added.
    for(i=0;i<NUNIT;i++){
        Unit *u;reset(scenario);u=&st.unit[i];u->hp=0;u->dc=3;
        CHECK(unit_at(u->x,u->z),i+1);u->hp=1;u->dc=0;CHECK(unit_at(u->x,u->z),i+1);
        for(j=DC_CRYSTAL;j!=0;j++){u->dc=j;CHECK(unit_at(u->x,u->z),0);}
    }
    // Conversion of the highest slot must retain its mask bit and identity.
    reset(scenario);
    for(i=0;i<NUNIT;i++){st.unit[i].ct=0;st.unit[i].sp=1;st.unit[i].hp=10;st.unit[i].dc=0;}
    st.unit[0].ct=100;st.unit[NUNIT-1].ct=100;st.unit[NUNIT-1].hp=0;
    st.unit[NUNIT-1].team=TEAM_ENEMY;
    CHECK(ct_next(),0);(void)ct_next();
    CHECK((st.morph_mask>>(NUNIT-1))&1,1);CHECK(morph_first(),NUNIT-1);
    CHECK(DC_OBJ(st.unit[NUNIT-1].dc),1);
    // A topology with no exits must terminate and must not manufacture a path.
    reset(scenario);i=U_RAMZA;compute_reach(i);origin=(u8)(st.unit[i].z*MAP_W+st.unit[i].x);
    W->en[origin]=0;compute_reach(i);
    for(j=0;j<MAP_N;j++)CHECK(st.reach[j],j==origin?0:255);
    CHECK(make_path(i,(u8)((origin+1)%MAP_W),st.unit[i].z),0);
    CHECK(make_path(i,255,0),0);CHECK(make_path(i,0,255),0);CHECK(make_path(NUNIT,0,0),0);
    reset(scenario);
    // Formula/AI module contexts must include the final compact slot. Compare
    // both actors at that index and targets at that index, including12.
    for(i=0;i<2;i++) {
        u8 actor=i?0:(u8)(NUNIT-1),target=i?(u8)(NUNIT-1):0;
        u16 hit=skill_hit(actor,target,0x94),amount=skill_amount(actor,target,0x94,12345);
        CHECK(hit,hit_chance(actor,target));CHECK(amount>0,1);
        st.formula.actor=actor;st.formula.target=target;st.formula.ability=0x94;st.formula.roll=12345;
#ifdef __m68k__
        CHECK(mod_call(MOD_AI,2,0),hit);CHECK(mod_call(MOD_AI,3,0),amount);
#else
        CHECK(skill_hit(actor,target,0x94),hit);CHECK(skill_amount(actor,target,0x94,12345),amount);
#endif
    }
    if(NUNIT==13) {
        const u8 *previous=flow_bank;
        CHECK(st.unit[11].war_trophy,2);CHECK(st.unit[11].money_modifier,17);
        CHECK(st.unit[12].war_trophy,3);CHECK(st.unit[12].money_modifier,25);
        flow_bank=rt_file("ffflow",RT_NULL);CHECK(flow_bank!=RT_NULL,1);
        prepare_rewards();CHECK(st.reward_n,2);CHECK(st.reward_bonus,4200);
        CHECK(st.reward_items[0],2);CHECK(st.reward_items[1],3);
        flow_bank=previous;
    }
}
static void reject_trial(void)
{
    CHECK(W==RT_NULL,1);CHECK(igfx==RT_NULL,1);CHECK(encounter_units,0);
}
static void allocation_trial(void)
{
    reject_trial();CHECK(battle_state==RT_NULL,1);CHECK(game_update(),1);
#ifndef __m68k__
    game_render(); // missing-State error rendering must not dereference it
#endif
}
#ifndef __m68k__
int main(int argc,char **argv)
{
    u16 scenario=argc>1?(u16)atoi(argv[1]):201;
    if(scenario==298)fail_next_allocation=1;
    sw_init(scenario);
    if(scenario==298)allocation_trial();else if(scenario==299)reject_trial();else trials(scenario);
    printf("boundaries %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);
    return failures!=0;
}
#else
void _main(void)
{
    if(BENCH_ARG==298)fail_next_allocation=1;
    boundaries_runtime_main();
    if(BENCH_ARG==298)allocation_trial();else if(BENCH_ARG==299)reject_trial();else trials(BENCH_ARG);
    BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);
}
#endif
