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
    // ffs1, the chapter card (campaign/data): it fades in, ENTER goes on
    assert(!chdir("../../campaign/data"));
    sw_init(1);assert(st.open_stage==OPEN_DONE&&st.open_fade==9);
    for(i=0;i<10;i++)assert(sw_step(0));
    assert(!st.open_fade&&!sw_step(K_ENTER)&&chapter_completed());
    printf("Scene player: title, chapel beats, palette, skip, card and cancellation passed\n");
    return 0;
}
