#include "deploy.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <stdio.h>
static const char *out;
static void shot(const char *name)
{
    char path[512];game_render();snprintf(path,sizeof path,"%s/%s.png",out,name);sw_write_png(path,1);
}
int main(int argc,char **argv)
{
    static const u8 selected[5]={0,2,3,4,6};u8 i,n=0;
    if(argc!=2)return 2;out=argv[1];sw_init(0);
    if(!flowbank||!bank)return 3;
    save.deploy_n=0;memset(save.deploy_battle,0,sizeof save.deploy_battle);deploy_init();shot("deploy-initial");
    for(i=0;i<save.n;i++) {
        char name[32];snprintf(name,sizeof name,"deploy-unit-%u",i);shot(name);
        if(i+1<save.n){sw_step(K_F5);sw_step(0);}
    }
    while(dp.unit){
        char name[32];sw_step(K_F1);sw_step(0);
        snprintf(name,sizeof name,"deploy-prev-%u",dp.unit);shot(name);
    }
    for(i=0;i<25&&n<5;i++)if(deploy_cell(i)[0]) {
        dp.unit=selected[n];dp.row=i/5;dp.col=i%5;deploy_place();n++;
        if(n==1)shot("deploy-ramza");
    }
    shot("deploy-full");dp.confirm=1;shot("deploy-confirm");dp.confirm=2;shot("deploy-return");
    dp.confirm=1;
    for(i=0;i<8;i++) {
        char name[32];u8 step;
        snprintf(name,sizeof name,"deploy-glove-%u",i);shot(name);
        for(step=0;step<4;step++)sw_step(0);
    }
    return 0;
}
