// Real settled action/outcome, archive binding and campaign receipts on PC first.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef off
#undef _main
#define _main objective_runtime_main
#define game_render objective_no_render
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef game_render
void objective_no_render(void){}
#undef _main
#define _main __main
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#include "flow_battle.c"
#include "save.h"
#include "../objective_bank.h"
static u16 checks,failures;static u32 digest=17;
static void check(u32 a,u32 b,u16 line)
{
    checks++;digest=(digest<<1|digest>>31)^a;
    if(a!=b){failures++;
#ifdef __m68k__
        BENCH_VALUE(line);
#else
        printf("objective line%u: %lu expected%lu\n",line,(unsigned long)a,(unsigned long)b);
#endif
    }
}
#define CHECK(a,b) check((u32)(a),(u32)(b),__LINE__)
static void reset(void)
{
    game_scenario(261);CHECK(W!=RT_NULL,1);if(!W)return;
    CHECK(W->goal!=RT_NULL,1);st.banner=st.end=st.open_stage=st.open_fade=st.scene_hide=0;
    st.act=U_RAMZA;st.ai=AI_OFF;st.mode=M_MENU;rt_keys=rt_prev=0;
}
static void finish(void)
{
    u8 n;for(n=0;n<24&&st.action.phase!=CTX_IDLE;n++){
        st.act_t=(st.gain?ACT_GAIN_END:ACT_END)-1;attack_step();
    }
    CHECK(st.action.phase,CTX_IDLE);
}
static void flush(void)
{
#ifdef __m68k__
    if(sv_data){save_write();sv_data=RT_NULL;}
#endif
}
static void trials(void)
{
    u8 i;u16 size;const u8 *bank;u8 *bad;Save before,after;
    reset();if(!W||!W->goal)return;
    CHECK(RD16(W->goal,1),2);CHECK(RD16(W->goal,2),1);CHECK(outcome(),END_NONE);
    // Visibility and temporary removal are not permanent defeat predicates.
    st.scene_hide=3;st.unit[1].x=255;CHECK(outcome(),END_NONE);reset();
    st.unit[1].hp=0;CHECK(outcome(),END_VICTORY);
    st.unit[0].hp=0;CHECK(outcome(),END_DEFEAT);reset();
    st.unit[1].dc=DC_GONE;CHECK(outcome(),END_VICTORY);reset();
    st.unit[0].dc=DC_GONE;CHECK(outcome(),END_DEFEAT);reset();
    st.unit[1].hp=0;st.unit[U_RAMZA].dc=DC_CRYSTAL;CHECK(outcome(),END_DEFEAT);reset();
    // Allegiance changes cannot rebind a named target to a different enemy.
    st.unit[1].team=TEAM_PLAYER;st.unit[1].hp=0;CHECK(outcome(),END_VICTORY);reset();
    for(i=1;i<NUNIT;i++)if(st.unit[i].team==TEAM_ENEMY&&i!=1)st.unit[i].hp=0;
    st.unit[1].statuses.inflicted[1]|=0x80;unit_status_refresh(&st.unit[1]);
    CHECK(outcome(),END_NONE);reset(); // Petrify of the living named target is insufficient
    // An actual lethal root impact must finish rewards/responses before ending.
    memset(st.reaction,0,sizeof st.reaction);memset(&W->mime,0,sizeof W->mime);
    attack(U_RAMZA,1);st.hit=1;st.dmg=999;st.push=st.self=0;
    st.act_t=ACT_HIT;attack_step();CHECK(st.unit[1].hp,0);CHECK(st.end,END_NONE);
    // A revival before settlement cancels that temporary target KO.
    st.unit[1].hp=1;st.unit[1].statuses.inflicted[0]&=0xdf;unit_status_refresh(&st.unit[1]);
    finish();CHECK(st.end,END_NONE);reset();
    // Both victims of one action: protection has priority at final settlement.
    memset(st.reaction,0,sizeof st.reaction);memset(&W->mime,0,sizeof W->mime);
    attack(U_RAMZA,1);st.hit=1;st.dmg=999;st.push=st.self=0;
    st.act_t=ACT_HIT;attack_step();st.unit[0].hp=0;finish();
    CHECK(st.end,END_DEFEAT);CHECK(st.reward_ready,0);
    CHECK(save_load(&before),1);st.banner=0;rt_keys=K_ESC;rt_prev=0;CHECK(game_update(),0);flush();
    CHECK(save_load(&after),1);CHECK(memcmp(&before,&after,sizeof before),0);
    reset();memset(st.reaction,0,sizeof st.reaction);memset(&W->mime,0,sizeof W->mime);
    attack(U_RAMZA,1);st.hit=1;st.dmg=999;st.push=st.self=0;
    st.act_t=ACT_HIT;attack_step();finish();CHECK(st.end,END_VICTORY);CHECK(st.reward_ready,1);
    CHECK(st.unit[2].hp>0,1); // unrelated enemies survive the named-target win
    // Complete the normal result confirmation and repeat settlement: one receipt.
    st.story_done=1;st.open_stage=st.open_fade=0;st.banner=0;st.reward_page=2;
    rt_prev=0;rt_keys=K_ENTER;
#ifdef __m68k__
    // Retain Work for the repeated-receipt assertion; the full PC flow above
    // uses game_update's real exit. Native checks the actual SAVE image here.
    CHECK(update(0),0);save_campaign();
#else
    CHECK(game_update(),0);
#endif
    flush();CHECK(save_load(&after),1);
    CHECK((after.flags[0]&2)!=0,1);CHECK(qlong(after.money)>qlong(before.money),1);
    save_campaign();flush();CHECK(save_load(&before),1);
#ifndef __m68k__
    if(memcmp(&before,&after,sizeof before))for(size=0;size<sizeof before;size++)
        if(((u8*)&before)[size]!=((u8*)&after)[size])printf("repeat byte%u: %u -> %u\n",size,((u8*)&after)[size],((u8*)&before)[size]);
#endif
    CHECK(memcmp(&before,&after,sizeof before)==0,1);
    // Mismatched authored identities reject entry; no random-name substitution.
    reset();st.unit[1].name^=1;
#ifdef FFT_MODULES
    CHECK(mod_call(MOD_RESOURCES,15,1),0);
#else
    CHECK(objective_load(1),0);
#endif
    CHECK(W->goal==RT_NULL,1);reset();
    bank=resource_file("ffgo",&size);bad=malloc(size);CHECK(bad!=RT_NULL,1);if(!bad)return;
    memcpy(bad,bank,size);bad[14]=bad[15]=0;CHECK(objective_bank_valid(bad,size),0);
    memcpy(bad,bank,size);bad[10]=0x80;CHECK(objective_bank_valid(bad,size),0);
    free(bad);
}
#ifndef __m68k__
int main(void){sw_init(261);trials();printf("objective-runtime %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){objective_runtime_main();trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
