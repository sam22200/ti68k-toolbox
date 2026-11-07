// Actual result renderer and confirmation path, with a private battle fixture.
#include "flow_battle.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <stdio.h>
static const char *out;
static void shot(const char *name)
{
    char path[512];game_render();snprintf(path,sizeof path,"%s/%s.png",out,name);sw_write_png(path,1);
}
int main(int argc,char **argv)
{
    u16 f;u8 i,page;
    if(argc!=2)return 2;out=argv[1];sw_init(261);
    if(!W||!flow_bank)return 3;
    for(i=0;i<NUNIT;i++)if(st.unit[i].team==TEAM_ENEMY){st.unit[i].hp=0;st.unit[i].dc=3;}
    end_battle(END_VICTORY);shot("kill-first-frame");
    for(i=0;i<20;i++)sw_step(0);
    st.open_reveal=999;shot("after-dialogue");
    sw_step(K_B);for(i=0;i<24;i++)sw_step(0);
    if(!st.story_done||st.mode!=M_END)return 4;
    for(page=0;page<3;page++) {
        st.open_clock=0;
        for(f=0;f<166;f++) {
            char name[40];
            if(!(f&3)){snprintf(name,sizeof name,"victory-%u-%03u",page,f);shot(name);}
            if(!sw_step(0))return 5;
        }
        if(sw_step(page&1?K_A:K_ENTER)!=(page<2))return 6;
        if(page<2)sw_step(0);
    }
    return !st.open_handoff;
}
