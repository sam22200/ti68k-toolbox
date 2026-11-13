// Actual map-scene controls, including the transition before/after a battle.
#include "flow_battle.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <assert.h>
#include <stdio.h>
static void frame(u32 buttons){assert(sw_step(buttons));}
static void press(u32 buttons){frame(buttons);frame(0);}
static void begin(void)
{
    u16 i;game_scenario(200);
    for(i=0;i<50;i++)frame(0);
    assert(st.open_stage==OPEN_GATE&&!st.open_fade);
}
static void no_and_cancel(void)
{
    u8 stage,beat,clock,fade;u16 reveal,tick;
    begin();stage=st.open_stage;beat=st.open_beat;
    frame(K_ESC);assert(st.quit==1&&!chapter_completed());
    clock=st.open_clock;fade=st.open_fade;reveal=st.open_reveal;tick=st.tick;
    frame(K_ESC);frame(0);frame(0); // a held opening key must not dismiss its own prompt
    assert(st.quit==1&&st.open_clock==clock&&st.open_fade==fade&&st.open_reveal==reveal&&st.tick==tick);
    assert(st.open_stage==stage&&st.open_beat==beat);
    press(K_ENTER);assert(!st.quit&&st.open_stage==stage&&!chapter_completed());
    press(K_ESC);assert(st.quit==1);press(K_DOWN);assert(st.quit==2);
    press(K_B);assert(!st.quit&&st.open_stage==stage&&!chapter_completed());
    press(K_ESC);press(K_ESC);assert(!st.quit&&!chapter_completed());
    press(K_ESC);press(K_UP);assert(st.quit==2);press(K_ENTER);
    assert(!st.quit&&st.open_fade&&!chapter_completed());
}
static void skip_to_battle(void)
{
    u16 i;begin();press(K_ESC);press(K_DOWN);press(K_A);
    for(i=0;i<30&&(st.open_stage||st.open_fade);i++)frame(0);
    assert(!st.open_stage&&!st.open_fade&&!chapter_completed());
    assert(st.mode!=M_QUIT&&st.end==END_NONE); // still in the running fight, never title/HOME
}
static void skip_aftermath(void)
{
    u16 i;begin();
    st.open_stage=st.open_fade=0;st.end=END_VICTORY;st.mode=M_END;
    st.story_done=0;st.reward_ready=1;st.quit=1; // an old cancelled battle-quit selector
    frame(K_ESC);assert(st.quit==1&&st.open_fade&&!chapter_completed());
    frame(0);press(K_DOWN);press(K_A);
    for(i=0;i<30&&!st.story_done;i++)frame(0);
    assert(st.story_done&&st.end==END_VICTORY&&!chapter_completed());
    assert(!st.open_stage&&!st.open_fade&&st.mode==M_END); // proceed to ordinary reward pages
}
int main(void)
{
    sw_init(200);no_and_cancel();skip_to_battle();skip_aftermath();
    puts("Map scene controls: default No, held ESC, cancel, 2nd/ENTER, frozen timers, battle and reward handoffs pass");
    return 0;
}
