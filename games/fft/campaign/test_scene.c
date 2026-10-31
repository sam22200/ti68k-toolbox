#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include "scene.c"
#include "../../../runtime/platform-sw/rt_sw.h"
static void press(u32 k){assert(sw_step(k));assert(sw_step(0));}
int main(void)
{
    unsigned i;u8 prior;
    // ffs0, Orbonne's title and chapel (opening/data)
    assert(!chdir("../opening/data"));
    sw_init(0);assert(st.open_stage==OPEN_TITLE);
    press(K_ENTER);assert(st.open_stage==OPEN_TITLE);   // the launch key cannot hide the title
    for(i=0;i<110;i++)assert(sw_step(0));
    press(K_ENTER);assert(st.open_stage==OPEN_CHAPEL);
    for(i=0;i<30;i++)assert(sw_step(0));
    assert(st.open_reveal>30);press(K_CATALOG);assert(st.open_white);
    press(K_CATALOG);assert(!st.open_white);
    for(i=0;i<10;i++)assert(sw_step(0));
    prior=st.open_beat;press(K_ENTER);press(K_ENTER);assert(st.open_beat>prior);
    for(i=0;i<1000&&sw_step(i%2?0:K_ENTER);i++){}  // every chapel message, then the fade
    assert(i<1000&&chapter_completed());
    sw_init(0);for(i=0;i<110;i++)assert(sw_step(0));press(K_ENTER);press(K_B);
    for(i=0;i<20&&sw_step(0);i++){}
    assert(chapter_completed());   // SHIFT skips the chapel
    sw_init(0);assert(!sw_step(K_ESC)&&!chapter_completed());
    // ffs1: narration, dark chapter/location titles, then all Academy dialogue.
    assert(!chdir("../../campaign/data"));
    sw_init(1);assert(st.open_stage==OPEN_CHAPEL&&st.open_fade==9);
    assert(st.open_pan==0); // fade-in flag must never become a camera offset
    assert(step_at(0)[0]==STEP_NARRATE&&step_at(1)[0]==STEP_OVERLAY);
    assert(step_at(2)[0]==STEP_OVERLAY&&step_at(3)[0]==STEP_STORY);
    for(i=0;i<10;i++)assert(sw_step(0));
    assert(!st.open_fade);
    {
        // Both directions and a target not divisible by8: settle exactly.
        static const u8 script[]={0,1,0,4,0,0,0,3};
        const u8 *prior_story=opening_story;u8 prior_beat=st.open_beat,start;
        opening_story=script;st.open_beat=0;
        for(start=0;start<24;start++) {
            st.open_pan=start;st.tick=0;
            for(i=0;i<8;i++)opening_positions();
            assert(st.open_pan==3);
        }
        opening_story=prior_story;st.open_beat=prior_beat;st.open_pan=0;
    }
    for(i=0;i<1000&&st.step==0;i++)assert(sw_step(i%2?0:K_ENTER));
    assert(st.step==1&&st.open_stage==OPEN_DONE);
    for(i=0;i<30;i++)assert(sw_step(0));
    press(K_ENTER);assert(st.step==2);
    for(i=0;i<30;i++)assert(sw_step(0));
    press(K_ENTER);assert(st.step==3&&st.open_stage==OPEN_CHAPEL);
    for(i=0;i<2000&&sw_step(i%2?0:K_ENTER);i++){}
    assert(i<2000&&chapter_completed());
    sw_init(2);assert(step_at(0)[0]==STEP_OVERLAY);
    press(K_ENTER);assert(!chapter_completed());
    for(i=0;i<160&&sw_step(0);i++){}
    assert(i<160&&chapter_completed());
    printf("Scene player: chapel, narration, timed titles, Academy, Gariland and cancellation passed\n");
    return 0;
}
