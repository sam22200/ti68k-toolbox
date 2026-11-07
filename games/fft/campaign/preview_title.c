#include "title.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <stdio.h>
static void shot(const char *folder,const char *name)
{
    char path[512];game_render();snprintf(path,sizeof path,"%s/%s.png",folder,name);sw_write_png(path,1);
}
int main(int argc,char **argv)
{
    u8 i;if(argc!=2)return 2;sw_init(0);if(!title_bank||!title_mg)return 3;
    shot(argv[1],"title-new");title.row=1;shot(argv[1],"title-continue");
    title.screen=TITLE_FILES;title.action=1;title.row=0;shot(argv[1],"files");
    title.screen=TITLE_OVERWRITE;title.action=0;title.answer=1;shot(argv[1],"overwrite");
    title.screen=TITLE_MAIN;title.row=1;
    for(i=0;i<8;i++){char name[32];u8 n;snprintf(name,sizeof name,"hand-%u",i);shot(argv[1],name);for(n=0;n<4;n++)sw_step(0);}
    title_cleanup();return 0;
}
