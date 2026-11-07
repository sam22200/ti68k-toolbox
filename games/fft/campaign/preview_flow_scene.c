// Actual scene renderer: source narration/title/dialogue phases, private review save only.
#include "scene.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <stdio.h>
static const char *out;
static void shot(const char *name)
{
    char path[512];game_render();snprintf(path,sizeof path,"%s/%s.png",out,name);sw_write_png(path,1);
}
int main(int argc,char **argv)
{
    u8 i;u16 page=0;
    if(argc!=2)return 2;out=argv[1];sw_init(1);
    if(!set||!scene_flow)return 3;
    st.open_fade=0;
    do {
        char name[40];
        opening_layout();st.open_reveal=W->dialog_total;
        snprintf(name,sizeof name,"academy-narration-%u",page++);shot(name);
        if(!opening_text()[W->dialog_next])break;
        st.open_offset+=W->dialog_next;W->dialog_key=65535;
    }while(page<10);
    next_step();st.open_clock=80;shot("academy-chapter");
    next_step();st.open_clock=80;shot("academy-location");
    next_step();st.open_beat=2;opening_dialog_reset();
    for(i=0;i<80;i++)sw_step(0);st.open_reveal=W->dialog_total;shot("academy-dialogue");
    game_scenario(2);st.open_clock=80;shot("gariland-location");return 0;
}
