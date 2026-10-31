// Actual candidate world-map renderer, no campaign save writes.
#ifndef WM_SOURCE
#define WM_SOURCE "worldmap.c"
#endif
#include WM_SOURCE
#include "../../../runtime/platform-sw/rt_sw.h"
#include "fixture_save.h"
static void shot(const char *folder,const char *name)
{
    char path[512];snprintf(path,sizeof path,"%s/%s.png",folder,name);game_render();sw_write_png(path,1);
}
int main(int argc,char **argv)
{
    FILE *f;
    if(argc!=3)return 2;
    sw_init(0);if(!bank||!mgb)return 3;
    f=fopen(argv[2],"rb");if(!f)return 4;
    if(!read_save_fixture(f,&save))return 5;fclose(f);
    game_scenario(0);wm.tick=4;shot(argv[1],"map-actual-after");
    wm.handy+=25;wm.sel=255;shot(argv[1],"map-roads-clear");
    wm.handy=wm.y;wm.sel=wm.at;
    wm.menu=1;shot(argv[1],"map-actual-menu");
    wm.menu=0;wm.handx-=30;wm.handy+=8;wm.sel=magnet();shot(argv[1],"map-actual-hand");
    return 0;
}
