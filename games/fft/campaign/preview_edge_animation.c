// Actual renderer, matching logical ticks/poses for both widths; no UI/emulator.
#include "gariland.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <stdio.h>

int main(int argc,char **argv)
{
    u8 rot,wide,t,i;
    if(argc!=2)return 2;
    sw_init(61);
    for(rot=0;rot<4;rot++)for(wide=0;wide<2;wide++) {
        game_scenario(61);st.rot=rot;st.scenery=wide;
        st.ai=st.banner=st.turn=0;st.mode=M_MENU;st.want=255;st.tick=0;rt_frame=0;
        scene_rot=vbuf_rot=255;cam_target(&st.camx,&st.camy);
        for(i=0;i<24;i++){W->actor_budget=255;W->ac_now++;W->hold=W->frz=0;game_render();}
        for(t=0;t<48;t++) {
            char path[512];st.tick=2*t;rt_frame=2*t;
            W->actor_budget=255;W->ac_now++;W->hold=W->frz=0;game_render();
            snprintf(path,sizeof path,"%s/view-%u-width-%u-frame-%02u.png",argv[1],rot,wide,t);
            if(sw_write_png(path,1))return 1;
        }
    }
    return 0;
}
