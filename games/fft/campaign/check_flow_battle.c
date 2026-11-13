// Bounded source-backed campaign flow checks; screenshots are review candidates.
#if defined(FLOW_GARILAND_RESULTS_ONLY)||defined(FLOW_GARILAND_AFTERMATH_ONLY)
#define FLOW_GARILAND_LATE_ONLY
#endif
#if defined(FLOW_GARILAND_EARLY_ONLY)||defined(FLOW_GARILAND_LATE_ONLY)
#define FLOW_GARILAND_ONLY
#endif
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main flow_runtime_main
#define game_render flow_no_frame_render // fixture draws real screens explicitly after runtime setup
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef game_render
void flow_no_frame_render(void){}
#undef _main
#define _main __main
#undef off
#endif
#include "flow_battle.c"
#include "save.h"
#ifndef __m68k__
#include "../../../runtime/platform-sw/rt_sw.h"
#include <stdio.h>
static const char *out;
#endif
static u16 checks,failures;
static u32 hash=17;
static u32 upper_screen(void)
{
    u16 n=48*RT_PBYTES;u32 h=17;
    const u8 *p=rt_light,*q=rt_dark;
    while(n--){h=(h<<1|h>>31)^*p++;h=(h<<1|h>>31)^*q++;}
    return h;
}
static void check(u8 ok
#ifndef __m68k__
                  ,u16 line
#endif
                  )
{
    checks++;hash=(hash<<3)+(hash>>29)+ok;
    if(!ok){failures++;
#ifndef __m68k__
        printf("flow failure line %u\n",line);
#else
        BENCH_VALUE(checks); // native ordinal; PC failures retain their source line
#endif
    }
}
#ifdef __m68k__
#define CHECK(v) check((v))
#else
#define CHECK(v) check((v),__LINE__)
#endif
// Split the instrumented native late fixture at the scene/results boundary.
// Results still execute the full setup; the companion checks every prefix
// assertion. The complete PC fixture retains all assertions in one run.
#ifdef FLOW_GARILAND_RESULTS_ONLY
#define PRECHECK(v) ((void)(v))
#else
#define PRECHECK(v) CHECK(v)
#endif
static void flush_save(void)
{
#ifdef __m68k__
    if(sv_data){
        if(sv_data!=(const u8 *)battle_state+sizeof(State)) {
            failures++;BENCH_VALUE(0xBADC0DE); // pending bytes must survive Work/module teardown
        }
        save_write();sv_data=0;
    }
#endif
}
static void shot(
#ifndef __m68k__
                 const char *name
#else
                 void
#endif
                 )
{
    u8 i,p,x;u16 y;
    if(st.open_stage||st.open_fade)for(i=0;i<100;i++) {
        rt_prev=rt_keys;rt_keys=0;game_update();game_render();
    }
    for(i=0;i<16;i++){W->actor_budget=255;W->ac_now++;W->hold=W->frz=0;game_render();}
    for(p=0;p<2;p++)for(y=0;y<100;y++)for(x=0;x<20;x++)
        hash=(hash<<3)+(hash>>29)+(p?((u8 *)rt_dark):((u8 *)rt_light))[y*RT_PBYTES+x];
#ifndef __m68k__
    if(out){char p[512];snprintf(p,sizeof p,"%s/%s.png",out,name);sw_write_png(p,1);}
#else
    BENCH_SHOT(rt_dark);
#endif
}
#ifdef __m68k__
#define SHOT(name) shot()
#else
#define SHOT(name) shot(name)
#endif
static void trials(void)
{
    u8 i;u16 selected=0;Save input;
    CHECK(flow_bank&&flow_ui&&W);if(!flow_bank||!W)return;
    // The fixture is generated FSV5. Runtime save migration is exercised in the
    // save suite/module, not duplicated in this size-bounded flow harness.
    CHECK(rt_load(SAVE_NAME,&input,sizeof input)&&!memcmp(input.magic,SAVE_MAGIC,4)&&input.n==7);
#ifdef FLOW_FIRST_FRAME_ONLY
    game_scenario(261);
    for(i=0;i<NUNIT;i++)if(st.unit[i].team==TEAM_ENEMY){st.unit[i].hp=0;st.unit[i].dc=3;}
    {
        u32 before;
        for(i=0;i<16;i++){W->actor_budget=255;W->ac_now++;game_render();}
        before=upper_screen();end_battle(END_VICTORY);W->actor_budget=255;game_render();
        CHECK(upper_screen()==before);CHECK(!st.story_done&&!st.open_stage&&st.open_clock==0);
        SHOT("kill-first-frame");
    }
    return;
#endif
#ifndef FLOW_GARILAND_ONLY
    if(CD(LOCATION)==19) {
        for(i=0;i<NUNIT;i++)if(st.unit[i].team==TEAM_ENEMY){st.unit[i].hp=0;st.unit[i].dc=DC_CHEST;}
        st.unit[2].hp=0;st.unit[2].dc=1;map_start(1);
        for(i=0;i<NUNIT;i++)CHECK((st.unit[i].team==TEAM_ENEMY)==!!(st.scene_hide&(1U<<i)));
        CHECK(st.unit[2].hp==0&&st.unit[2].dc==1);SHOT("orbonne-after-ko");return;
    }
#endif
#ifndef FLOW_ORBONNE_ONLY
    game_scenario(261);
    for(i=0;i<NUNIT;i++)if(st.unit[i].team==TEAM_PLAYER&&st.unit[i].x<MAP_W)selected|=(u16)(1U<<st.unit[i].slot);
    if(input.flags[0]&2) {
        prepare_rewards();save_campaign();flush_save();return;
    }
#ifndef FLOW_GARILAND_LATE_ONLY
    CHECK(selected==0x5d&&st.party_in); // selected0,2,3,4,6; reserve1,5 stay in the save
    CHECK(st.unit[U_RAMZA].max_hp==52);
    for(i=0;i<NUNIT;i++)if(st.unit[i].team==TEAM_PLAYER&&st.unit[i].x<MAP_W) {
        u8 j;for(j=0;j<input.deploy_n;j++)if(input.deploy[j][0]==st.unit[i].slot)break;
        CHECK(j<input.deploy_n&&st.unit[i].x==input.deploy[j][1]&&st.unit[i].z==input.deploy[j][2]);
    }
    SHOT("gariland-deployed");
    game_scenario(200);CHECK(st.open_stage==OPEN_GATE);
    for(i=0;i<NUNIT;i++)CHECK(!st.unit[i].ct);
    SHOT("gariland-before-dialogue");
    for(i=0;i<NUNIT;i++)CHECK(!st.unit[i].ct);
#ifdef FLOW_GARILAND_EARLY_ONLY
    return;
#endif
#endif
    // Deliberately remove the Potion trophy holder: live presence, not the
    // initial ENTD list, decides the result. KO allies stay ordinary KO.
    game_scenario(261);
    for(i=0;i<NUNIT;i++)if(st.unit[i].team==TEAM_ENEMY){st.unit[i].hp=0;st.unit[i].dc=3;}
    prepare_rewards();PRECHECK(st.reward_n==3&&st.reward_bonus==2000);
    st.unit[2].dc=DC_CRYSTAL;st.unit[3].dc=DC_GONE;st.unit[3].x=255;
    st.unit[7].hp=0;st.unit[7].dc=2;
    st.reward_ready=0;st.unit[1].money_modifier=0;prepare_rewards();PRECHECK(st.reward_bonus==0);
    st.unit[1].money_modifier=20;st.reward_ready=0;
#ifndef FLOW_GARILAND_ONLY
    {
        u32 before;u8 j;
        for(j=0;j<16;j++){W->actor_budget=255;W->ac_now++;game_render();}
        before=upper_screen();
        end_battle(END_VICTORY); // the kill's actual first render, before any update
        W->actor_budget=255;game_render();
        CHECK(upper_screen()==before); // no darkening/congratulations before dialogue
        CHECK(!st.story_done&&!st.open_stage&&st.open_clock==0);
    }
#else
    end_battle(END_VICTORY);
#endif
    PRECHECK(st.reward_base==500&&st.reward_bonus==2000&&st.reward_n==2);
    PRECHECK(st.reward_items[0]==2&&st.reward_items[1]==253);
    rt_prev=rt_keys=0;PRECHECK(game_update());
    SHOT("gariland-after-dialogue");
    PRECHECK(st.scene_hide==0x3e&&st.unit[7].hp==0&&st.unit[7].dc==2);
    PRECHECK(W->dialog_face!=255);
    // Complete the actual post-scene fade/handoff rather than bypassing it.
    rt_prev=0;rt_keys=K_B;PRECHECK(game_update());
    for(i=0;i<24;i++){rt_prev=rt_keys;rt_keys=0;PRECHECK(game_update());game_render();}
    PRECHECK(st.story_done&&!st.open_stage&&st.mode==M_END&&st.end==END_VICTORY);
#ifndef FLOW_GARILAND_AFTERMATH_ONLY
    for(i=0;i<3;i++) {
        st.open_clock=160; // settled review; production starts each page at zero
        CHECK(st.reward_page==i);SHOT(i==0?"victory-1":i==1?"victory-2":"victory-3");
        rt_prev=0;rt_keys=i&1?K_A:K_ENTER;CHECK(game_update()==(i<2));
        CHECK(!st.open_clock);
        if(i<2){rt_prev=rt_keys;rt_keys=0;CHECK(game_update());CHECK(st.open_clock==1);}
    }
    CHECK(st.open_handoff); // normal result confirmation saves and returns to the supervisor
    flush_save();
#endif
#endif
}
#ifndef __m68k__
int main(int argc,char **argv)
{
    out=argc>1?argv[1]:RT_NULL;sw_init(argc>2?101:261);trials();
    printf("flow %08lX failures %u checks %u\n",(unsigned long)hash,failures,checks);return failures!=0;
}
#else
void _main(void)
{
    void *planes;
    flow_runtime_main();planes=malloc(2*RT_PSIZE);rt_dark=planes;rt_light=(u8 *)planes+RT_PSIZE;
    trials();BENCH_VALUE(hash);BENCH_VALUE(failures);BENCH_VALUE(checks);
    free(planes);
}
#endif
