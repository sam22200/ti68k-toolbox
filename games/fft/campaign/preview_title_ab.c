#include "title.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <stdio.h>
static const char *out;
static void shot(const char *name)
{
    char path[512];game_render();snprintf(path,sizeof path,"%s/%s.png",out,name);sw_write_png(path,1);
}
int main(int argc,char **argv)
{
    u16 f;
    if(argc!=2)return 2;out=argv[1];sw_init(0);
    if(!title_bank||!title_mg||!title_back)return 3;
    for(f=0;f<226;f++) {
        char name[30];
        if(!(f&1)){snprintf(name,sizeof name,"frame-%03u",f);shot(name);}
        if(!sw_step(0))return 4;
    }
    shot("new-game");title.row=1;shot("continue");
    title.screen=TITLE_FILES;title.action=1;title.row=0;shot("files");
    title_cleanup();return 0;
}
