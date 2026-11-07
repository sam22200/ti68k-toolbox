// Actual world-map renderer with offline-scaled review banks. Never writes a save.
#include "worldmap.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include "fixture_save.h"

int main(int argc,char **argv)
{
    static const u8 ticks[8]={0,4,8,13,17,20,24,29};
    FILE *f;u8 i;
    if(argc!=3)return 2;
    sw_init(0);if(!bank||!mgb)return 3;
    f=fopen(argv[2],"rb");if(!f)return 4;
    if(!read_save_fixture(f,&save))return 5;
    fclose(f);game_scenario(0);
    if(wm.at!=6)return 6;
    for(i=0;i<8;i++) {
        char path[512];
        wm.tick=ticks[i];game_render();
        snprintf(path,sizeof path,"%s/map-%u.png",argv[1],i);
        sw_write_png(path,1);
    }
    return 0;
}
