// Deterministic presentation replay. Same C engine, no calculator interaction.
#include "opening.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <stdio.h>
static unsigned shot;
static void capture(const char *dir,unsigned frame)
{
    char path[512];
    if(frame%3)return;
    snprintf(path,sizeof path,"%s/%05u.png",dir,shot++);sw_write_png(path,1);
}
int main(int argc,char **argv)
{
    unsigned frame,done_hold=0;u32 keys;u8 normal,title;
    if(argc<3)return 2;
    normal=argc>3&&!strcmp(argv[3],"normal");
    title=argc>3&&!strcmp(argv[3],"title");
    sw_init((u16)atoi(argv[1]));
    for(frame=0;frame<(title?150:normal?1200:10000);frame++){
        keys=0;
        if(st.open_stage){
            if(st.open_stage==OPEN_TITLE){if(!title&&st.open_clock>=110&&frame%30==0)keys=K_ENTER;}
            else if(st.open_stage==OPEN_DONE){if(!st.open_fade&&++done_hold>=45)break;}
            else if(st.open_reveal>=W->dialog_total && st.open_clock>=50 && frame%6==0)keys=K_ENTER;
        }else keys=normal?0:K_A; // normal animation control or ordinary fast AI
        if(!sw_step(keys))return 3;
        capture(argv[2],frame);
        if(!atoi(argv[1]) && !st.open_stage && !st.open_fade && st.act==0 && !st.ai && frame>300)break;
        if(frame==180 && !st.open_stage && argc>3&&!normal) { st.ai_all=1; } // full encounter demonstration
    }
    printf("review frames %u, captured %u, stage %u\n",frame,shot,st.open_stage);
    return 0;
}
