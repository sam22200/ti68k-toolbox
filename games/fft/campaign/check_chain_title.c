// Real Save/Continue child: selected slot, overwrite confirmation and verification.
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#include "title.c"
static u8 press(u32 key){u8 run=sw_step(key);if(run)sw_step(0);return run;}
int main(int argc,char **argv)
{
    u8 result;const char *action;
    if(argc!=2)return 111;action=argv[1];sw_init(!strcmp(action,"continue")?0:3);
    if(!title_bank||!title_mg||!title_back||!title_draft)return 111;
    if(!strcmp(action,"decline")){
        if(sw_step(K_B)||title.result)return 111;
        title_cleanup();puts("chain-title decline");return 0;
    }
    if(!strcmp(action,"continue")){
        if(!press(K_DOWN)||!press(K_ENTER)||title.screen!=TITLE_FILES)return 111;
    }
    if(press(K_ENTER)){
        if(title.screen!=TITLE_OVERWRITE||title.answer!=1)return 111;
        if(!press(K_UP)||press(K_ENTER))return 111;
    }
    result=title_finish();if(result)result=title_verified();
    printf("chain-title %s result%u\n",action,result);
    title_cleanup();return result==(!strcmp(action,"continue")?2:3)?result:111;
}
