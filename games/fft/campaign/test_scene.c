#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
static unsigned char fail_picture; // only armed for game_scenario's picture allocation
static void *scene_malloc(size_t bytes){return fail_picture?NULL:malloc(bytes);}
#define malloc scene_malloc
#include "scene.c"
#undef malloc
#include "../../../runtime/platform-sw/rt_sw.h"
static void press(u32 k){assert(sw_step(k));assert(sw_step(0));}
int main(void)
{
    unsigned i;u8 prior;
    // ffs0, Orbonne's title and chapel (opening/data)
    assert(!chdir("../opening/data"));
    sw_init(0);assert(st.open_stage==OPEN_TITLE);
    assert(vbuf&&picture_bytes==6U*28*100); // widest picture in this set, not the engine maximum
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
    assert(vbuf&&picture_bytes==6U*20*100);
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
    fail_picture=1;game_scenario(0);assert(!set&&!vbuf&&!picture_bytes&&W&&acache);
    fail_picture=0;game_scenario(0);assert(vbuf&&picture_bytes==6U*28*100);
    game_scenario(1);assert(vbuf&&picture_bytes==6U*20*100); // shrink on same-process reuse
    sw_init(2);assert(step_at(0)[0]==STEP_OVERLAY);
    press(K_ENTER);assert(!chapter_completed());
    for(i=0;i<160&&sw_step(0);i++){}
    assert(i<160&&chapter_completed());
    {
        // Title/card-only sets reserve no picture; reject a narrower-than-LCD
        // picture before allocating or asking the decoder to write anything.
        char folder[]="/tmp/fft-scene-memory-XXXXXX",cwd[4096];FILE *f;
        u8 card[22]={'F','S','C','1',0,0,0,0,0,0,0,1,STEP_CARD,0,0,20,0,0,0,0,0,0};
        assert(getcwd(cwd,sizeof cwd)&&mkdtemp(folder)&&!chdir(folder));
        f=fopen("ffs3.bin","wb");assert(f&&fwrite(card,1,sizeof card,f)==sizeof card);fclose(f);
        game_scenario(3);assert(set&&!vbuf&&!picture_bytes&&st.open_stage==OPEN_DONE);
        card[12]=STEP_STORY;card[17]=19;
        f=fopen("ffs4.bin","wb");assert(f&&fwrite(card,1,sizeof card,f)==sizeof card);fclose(f);
        game_scenario(4);assert(!set&&!vbuf&&!picture_bytes);
        assert(!unlink("ffs3.bin")&&!unlink("ffs4.bin")&&!chdir(cwd)&&!rmdir(folder));
    }
    release();assert(!W&&!vbuf&&!acache&&!picture_bytes);
    printf("Scene player: chapel, narration, timed titles, Academy, Gariland and cancellation passed\n");
    return 0;
}
