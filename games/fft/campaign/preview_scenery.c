// Same tactical state, four source-handed views, for the selected scenery review.
#include "gariland.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <stdio.h>
int main(int argc,char **argv)
{
    u8 rot,i;
    if(argc!=3)return 2;
    sw_init(61);
    for(rot=0;rot<4;rot++) {
        char path[512];game_scenario(61);st.rot=rot;st.ai=0;st.banner=0;st.mode=M_MENU;st.tick=0;
        cam_target(&st.camx,&st.camy);
        for(i=0;i<30;i++){W->actor_budget=255;W->hold=W->frz=0;game_render();}
        snprintf(path,sizeof path,"%s/scenery-%s-%u.png",argv[1],argv[2],rot);
        if(sw_write_png(path,1))return 1;
    }
    return 0;
}
