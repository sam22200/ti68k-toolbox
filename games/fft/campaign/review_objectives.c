// PC-only art proposal. tools/objective_review.py supplies a private engine copy
// with banner highlighting restricted to review_focus; production is untouched.
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
static u16 review_focus;
#include "gariland.c"
int main(int argc,char **argv)
{
    u8 kind,i,target=255,protect=255;const u8 *packet;u16 size;u8 *copy=RT_NULL;
    if(argc!=3)return 2;kind=(u8)atoi(argv[1]);if(kind>4)return 2;
    sw_init(61);if(!W)return 3;
    for(i=0;i<NUNIT;i++){
        if(st.unit[i].team==TEAM_ENEMY){review_focus|=(u16)1<<i;if(target==255)target=i;}
        if(st.unit[i].team==TEAM_GUEST&&protect==255)protect=i;
    }
    if(target==255||protect==255)return 4;
    if(kind==4){printf("target:%s\nprotect:%s\n",unit_name(target),unit_name(protect));return 0;}
    if(kind){
        packet=rt_file("review",&size);if(!packet)return 5;
        copy=malloc(UIC_BANNERS+size);if(!copy)return 6;
        memcpy(copy,cgfx,UIC_BANNERS);memcpy(copy+UIC_BANNERS,packet,size);cgfx=copy;
        review_focus=kind==1?(u16)1<<target:kind==2?(u16)1<<protect:
                     (u16)((1U<<target)|(1U<<protect));
    }
    st.open_stage=st.open_fade=st.banner=st.end=0;st.mode=M_MENU;st.ai=AI_OFF;
    for(i=0;i<16;i++){rt_frame++;W->hold=W->frz=0;W->actor_budget=255;game_render();}
    bn_start(BN_START);st.banner_t=70;rt_frame++;game_render();
    printf("objective-review %u target%u protect%u checksum%04X\n",kind,target,protect,sw_checksum());
    i=(u8)sw_write_png(argv[2],1);if(copy)free(copy);return i;
}
