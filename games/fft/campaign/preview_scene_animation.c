#include "scene.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <stdio.h>
int main(int argc,char **argv)
{
    u16 f;u8 title;
    if(argc!=2)return 2;sw_init(1);if(!set||!scene_flow)return 3;
    for(title=0;title<3;title++) {
        if(title==2)game_scenario(2);else{st.step=title+1;step_start();}
        st.open_fade=0;
        for(f=0;f<=100;f+=2) {
            char path[512];st.open_clock=(u8)f;game_render();
            snprintf(path,sizeof path,"%s/scene-%u-%03u.png",argv[1],title,f);sw_write_png(path,1);
        }
    }
    game_scenario(0);
    if(st.open_stage!=OPEN_TITLE)return 4;
    for(f=0;f<150;f+=2) {
        char path[512];st.open_clock=(u8)f;game_render();
        snprintf(path,sizeof path,"%s/orbonne-%03u.png",argv[1],f);sw_write_png(path,1);
    }
    return 0;
}
