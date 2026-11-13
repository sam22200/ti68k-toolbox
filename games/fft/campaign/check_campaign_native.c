// Selected real overlay/archived-art/FSV4 path, shared with the PC oracle.
#define RT_DRAW_ROWS 100
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef off
#undef _main
#define _main prerequisite_runtime_main
#define game_render prerequisite_no_frame_render
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef game_render
void prerequisite_no_frame_render(void){}
#undef _main
#define _main __main
#endif
#include "flow_battle.c"
#include "save.h"
#ifndef __m68k__
#include "../../../runtime/platform-sw/rt_sw.h"
#include <stdio.h>
#endif
static u16 checks,failures;
static u32 digest=17;
static void checked(u8 ok,u16 line)
{
    checks++;digest=(digest<<1|digest>>31)^ok;
    if(!ok){failures++;
#ifdef __m68k__
        BENCH_VALUE(line);
#else
        printf("prerequisite failure line %u\n",line);
#endif
    }
}
#define CHECK(v) checked((v)!=0,__LINE__)
static void frame(u32 buttons)
{
    rt_prev=rt_keys;rt_keys=buttons;rt_frame++;
    CHECK(game_update());
}
static void press(u32 buttons){frame(buttons);frame(0);}
static void screen(void)
{
    u16 row,col;u8 plane;u32 hash=17;
    W->actor_budget=255;W->ac_now++;game_render();
    for(plane=0;plane<2;plane++)for(row=0;row<100;row++)for(col=0;col<20;col++)
        hash=(hash<<1|hash>>31)^((const u8 *)(plane?rt_dark:rt_light))[row*RT_PBYTES+col];
    digest=(digest<<1|digest>>31)^hash;
#ifdef __m68k__
    BENCH_VALUE(hash);
#else
    printf("scene-screen %08lX\n",(unsigned long)hash);
#endif
}
static void scene_trials(void)
{
    u8 i,clock,beat,fade;u16 tick,reveal;
    game_scenario(200);for(i=0;i<50;i++)frame(0);
    CHECK(st.open_stage==OPEN_GATE&&!st.open_fade);
    press(K_ESC);CHECK(st.quit==1&&!chapter_completed());screen();
    clock=st.open_clock;beat=st.open_beat;fade=st.open_fade;tick=st.tick;reveal=st.open_reveal;
    for(i=0;i<3;i++)frame(0);
    CHECK(st.open_clock==clock&&st.open_beat==beat&&st.open_fade==fade&&st.tick==tick&&st.open_reveal==reveal);
    press(K_ENTER);CHECK(!st.quit&&!chapter_completed());
    press(K_ESC);press(K_DOWN);CHECK(st.quit==2);screen();
    press(K_ESC);CHECK(!st.quit&&!chapter_completed());
    press(K_ESC);press(K_DOWN);press(K_A);
    for(i=0;i<30&&(st.open_stage||st.open_fade);i++)frame(0);
    CHECK(!st.open_stage&&!st.open_fade&&!chapter_completed()&&st.end==END_NONE);
    st.open_stage=st.open_fade=0;st.banner=0;st.ai=AI_OFF;W->hold=W->frz=0;
    end_battle(END_VICTORY);
    st.story_done=0;st.reward_ready=1;st.quit=1;
    press(K_ESC);CHECK(st.quit==1&&st.open_fade&&!chapter_completed());
    press(K_DOWN);press(K_ENTER);
    for(i=0;i<30&&!st.story_done;i++)frame(0);
    CHECK(st.story_done&&st.end==END_VICTORY&&st.mode==M_END&&!chapter_completed());
}
static void flush_save(void)
{
#ifdef __m68k__
    if(sv_data){save_write();sv_data=RT_NULL;}
#endif
}
static void save_trials(void)
{
    static const s8 changes[]={-70,-8,-7,-4,-3,0,3,4,7,8,30};
    static const u8 expected[]={53,68,69,69,70,70,70,71,71,72,77};
    u8 i,slot;Unit *u;Save loaded;
    game_scenario(201);save_campaign();flush_save();party_load();
    CHECK(st.party_in);u=&st.unit[U_RAMZA];slot=u->slot;CHECK(slot<battle_save.n);
    for(i=0;i<sizeof changes;i++){
        W->permanent_base[U_RAMZA][0]=70;W->permanent_base[U_RAMZA][1]=30;
        u->brave=(u8)(70+changes[i]);u->faith=(u8)(30-changes[i]/2);u->raw[0]-=3;
        save_campaign();flush_save();CHECK(save_load(&loaded));
        CHECK(loaded.unit[slot].brave==expected[i]);
        CHECK(loaded.unit[slot].faith==30+(-changes[i]/2)/4);
        CHECK(loaded.unit[slot].raw[0][2]==(u8)u->raw[0]);
        CHECK(u->brave==expected[i]&&W->permanent_base[U_RAMZA][0]==expected[i]);
        save_campaign();flush_save();CHECK(save_load(&loaded));
        CHECK(loaded.unit[slot].brave==expected[i]);
        party_load();CHECK(u->brave==expected[i]&&u->faith==loaded.unit[slot].faith);
    }
}
static void trials(void)
{
    CHECK(W&&flow_bank&&contour_gfx);if(!W||!flow_bank||!contour_gfx)return;
    scene_trials();save_trials();
}
#ifndef __m68k__
int main(void)
{
    sw_init(200);trials();
    printf("prerequisites %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);
    return failures!=0;
}
#else
void _main(void)
{
    prerequisite_runtime_main();trials();
    BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);
}
#endif
