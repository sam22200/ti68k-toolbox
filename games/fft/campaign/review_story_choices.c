// Isolated PC presentation review using the production scene/menu renderer.
// The recruitment question is authored QA, not a retail Delita recruitment.
#include <assert.h>
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#include "scene.c"
#include "story_choice.h"
#include "x/story-choices/fixture.h"
static const u8 question[]={0,1,0,4,7,0,0,0,
    'R','a','m','z','a',0,
    'S','h','o','u','l','d',' ','D','e','l','i','t','a',' ','j','o','i','n',' ',
    't','h','e',' ','p','a','r','t','y','?',0};
int main(int argc,char **argv)
{
    u8 i;char path[4096];Save s;
    if(argc!=2)return 2;
    sw_init(1);assert(W&&set&&vbuf&&pgfx&&contour_gfx);
    st.step=3;step_start();opening_story=question;st.open_beat=0;
    st.open_white=st.open_offset=st.open_fade=0;st.open_reveal=65535;st.tick=0;
    W->dialog_key=65535;opening_layout();assert(W->dialog_face!=255);
    for(i=0;i<16;i++)game_render(); // finish the production dialogue cache/reveal
    snprintf(path,sizeof path,"%s/before.png",argv[1]);assert(!sw_write_png(path,1));
    scene_choice_draw(recruit_choice,0,0,0);snprintf(path,sizeof path,"%s/choice.png",argv[1]);assert(!sw_write_png(path,1));
    printf("choice review portrait%u checksum%04X\n",W->dialog_face,sw_checksum());
    memset(&s,0,sizeof s);memcpy(s.magic,SAVE_MAGIC,4);s.n=16;s.guest_id[0][1]=4;sc_flag_set(&s,3,1);
    assert(story_choice_apply(&s,recruit_choice,sizeof recruit_choice,1)==SC_PARTY_FULL);
    assert(!sc_flag(&s,40)&&save_guest_find(&s,4)==0);
    game_render();scene_choice_draw(recruit_choice,choice_move(0,2,K_DOWN),SC_PARTY_FULL,0);
    snprintf(path,sizeof path,"%s/full.png",argv[1]);assert(!sw_write_png(path,1));
    printf("full review checksum%04X; production menu layout\n",sw_checksum());
    release();return 0;
}
