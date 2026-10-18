// Export actual runtime frames, with the same scripted input as the interactive review.
#include <stdio.h>
#include <stdlib.h>
#include "menu.h"
#include "../../../runtime/platform-sw/rt_sw.h"
void game_cleanup(void);
int main(int argc,char **argv)
{
    SwScript script;
    char path[512];unsigned f,n;
    if(argc!=5)return 2;
    n=(unsigned)atoi(argv[3]);sw_init((u16)atoi(argv[1]));
    if(sw_load_script(&script,argv[2]))return 3;
    for(f=0;f<n;f++) {
        if(!sw_step(sw_script_keys(&script,(u16)f)))return 4;
        if(!(f&3)) {
            snprintf(path,sizeof(path),"%s/%04u.png",argv[4],f);
            if(sw_write_png(path,1))return 5;
            printf("%u,%u,%u,%u,%04x\n",f,menu.screen,menu.unit,menu.row,sw_checksum());
        }
    }
    game_cleanup();return 0;
}
