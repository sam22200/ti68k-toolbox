// Real menu inputs, per-unit ownership and safe cancellation of committed actions.
#include <assert.h>
#include <stdio.h>
#include "fft.c"
#include "../../runtime/platform-sw/rt_sw.h"

static void step(u32 keys) { assert(sw_step(keys)); }
static void press(u32 keys) { step(keys); step(0); }
static void choose(u8 choice)
{
    assert(!st.ai && st.mode==M_MENU);
    st.menu=MN_AUTO; press(K_ENTER); assert(st.mode==M_AUTO);
    if (!choice) press(K_UP);
    else if (choice==2) press(K_DOWN);
    press(K_ENTER);
}

static void selectors(void)
{
    Unit units[NUNIT]; u32 seed; u8 i, n, act;
    sw_init(10); memcpy(units,st.unit,sizeof units); seed=rng32;
    st.menu=MN_AUTO; press(K_ENTER); assert(st.mode==M_AUTO && st.sub==1);
    for(i=0;i<12;i++) { press(K_DOWN); assert(st.sub<3); }
    press(K_B); assert(st.mode==M_MENU && !st.auto_units && !st.ai);
    assert(!memcmp(units,st.unit,sizeof units) && rng32==seed);
    choose(1); assert(st.auto_units==(1U<<U_RAMZA) && st.ai);
    assert(!memcmp(units,st.unit,sizeof units) && rng32==seed && !st.moved && !st.acted);
    // The per-unit policy persists; other player units remain manual.
    n=0;
    do {
        turn_end(0,0); act=st.act;
        assert(!!st.ai==(st.unit[act].team!=TEAM_PLAYER || act==U_RAMZA));
        assert(++n<40);
    } while(act!=U_RAMZA);
    step(K_ESC); assert(!st.ai && !st.auto_units && st.mode==M_MENU);
    step(0); choose(2);
    for(i=0;i<NUNIT;i++) assert(!!(st.auto_units&(1U<<i))==(st.unit[i].team==TEAM_PLAYER));
    // Cancellation during an enemy turn affects future player control, not the enemy.
    while(st.unit[st.act].team==TEAM_PLAYER) turn_end(0,0);
    memcpy(units,st.unit,sizeof units); seed=rng32; act=st.act; i=st.ai;
    step(K_B); assert(!st.auto_units && st.act==act && st.ai==i);
    assert(!memcmp(units,st.unit,sizeof units) && rng32==seed);
    sw_init(10); assert(!st.auto_units);
    st.auto_units=1U<<(U_RAMZA+1);
    st.menu=MN_AUTO; press(K_ENTER); press(K_B);
    assert(st.auto_units==(1U<<(U_RAMZA+1)) && st.mode==M_MENU);
    choose(0); assert(!st.auto_units && !st.ai && st.mode==M_MENU);
    puts("auto: selector, per-unit turn ownership, party mask, reset and enemy cancellation passed");
}

static void stop_walk(void)
{
    u8 x,z,act; unsigned frame;
    sw_init(36); assert(st.mode==M_WALK && !st.moved);
    x=st.path[st.path_n-1][0]; z=st.path[st.path_n-1][1];
    st.auto_units=1U<<st.act; st.ai=AI_MOVE;
    step(K_ESC); assert(st.mode==M_WALK && !st.ai && !st.auto_units);
    for(frame=0;frame<200 && st.mode==M_WALK;frame++) step(0);
    assert(st.mode==M_MENU && st.moved && st.unit[st.act].x==x && st.unit[st.act].z==z);
    act=st.act; choose(1); assert(st.moved);
    for(frame=0;frame<3000 && st.act==act && !st.end;frame++) {
        step(0); assert(st.unit[act].x==x && st.unit[act].z==z);
    }
    assert(frame<3000);
    puts("auto: committed walk completes; delegation cannot grant another move");
}

static void stop_attack(void)
{
    static const u8 phases[]={0,ACT_HIT,ACT_HIT+1,ACT_HIT+JP_T};
    Unit reference[NUNIT]; u8 bag[NBAG],p; u32 seed; unsigned frame;
    sw_init(19); assert(st.mode==M_ACT); st.acted=1;
    while(st.mode==M_ACT) step(0);
    memcpy(reference,st.unit,sizeof reference); memcpy(bag,st.bag,sizeof bag); seed=rng32;
    for(p=0;p<sizeof phases;p++) {
        sw_init(19); st.acted=1;
        while(st.act_t<phases[p]) step(0);
        st.auto_units=1U<<st.act; st.ai=AI_AIM;
        step(K_B); assert(!st.ai && !st.auto_units && st.mode==M_ACT && st.act_t==phases[p]);
        while(st.mode==M_ACT) step(0);
        assert(st.mode==M_MENU && st.acted);
        assert(!memcmp(reference,st.unit,sizeof reference) && !memcmp(bag,st.bag,sizeof bag) && seed==rng32);
    }
    // Handing over after an action may move/wait, never attack twice in the same turn.
    choose(1);
    for(frame=0;frame<3000 && st.act==U_RAMZA && !st.end;frame++) {
        step(0); assert(st.act!=U_RAMZA || st.mode!=M_ACT);
    }
    assert(frame<3000);
    puts("auto: cancelling an attack preserves damage, rewards and RNG; no second action");
}

static void stop_pickup(void)
{
    u8 act; unsigned frame; u32 seed;
    sw_init(10); act=st.act; st.unit[act].hp=1;
    st.unit[1].hp=0; st.unit[1].dc=DC_CRYSTAL;
    pick_up(act,1); seed=rng32; st.ai=AI_MOVE; st.auto_units=1U<<act;
    step(K_ESC); assert(st.mode==M_PICK && !st.ai && !st.auto_units);
    for(frame=0;frame<PICK_FRAMES && st.mode==M_PICK;frame++) step(0);
    assert(st.mode==M_MENU && st.act==act && st.unit[1].dc==DC_GONE);
    assert(st.unit[act].hp==st.unit[act].max_hp && rng32==seed);
    puts("auto: committed crystal pickup completes once after cancellation");
}

static void whole_battle(void)
{
    unsigned frame, actions=0, turns=0; u8 previous, i;
    sw_init(10); choose(2); previous=st.act;
    for(frame=0;frame<24000 && !st.end;frame++) {
        if(st.mode==M_ACT && st.act_t==ACT_HIT) actions++;
        step(0);
        if(st.act!=previous) { turns++; previous=st.act; }
        assert(st.mode!=M_MENU || st.ai || st.banner || st.morph_mask);
        for(i=0;i<NUNIT;i++) assert(st.unit[i].hp<=st.unit[i].max_hp);
    }
    assert(frame<24000 && st.end && actions && turns>10);
    printf("auto: full battle ends after %u frames, %u turn changes, %u attacks/abilities (outcome %u)\n",
           frame,turns,actions,st.end);
}

int main(int argc,char **argv)
{
    selectors(); stop_walk(); stop_attack(); stop_pickup(); whole_battle();
    if(argc>1) {
        sw_init(10); st.menu=MN_AUTO; press(K_ENTER); for(unsigned i=0;i<12;i++) step(0);
        assert(!sw_write_png(argv[1],1));
    }
    puts("Auto-battle tests passed"); return 0;
}
