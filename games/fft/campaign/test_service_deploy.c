#include "deploy.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    Save before;sw_init(200);assert(bank&&flowbank&&save.missions.dispatches.n);before=save;
    dp.unit=save.missions.dispatches.job[0].unit[0];assert(!deploy_place()&&dp.note==7);
    assert(!memcmp(&before,&save,sizeof save));
    {u8 i;for(i=0;i<25;i++)assert(dp.placed[i]!=dp.unit);}
    puts("Away workers cannot deploy or enter a restored squad");return 0;
}
