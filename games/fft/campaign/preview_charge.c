// Local visual review door: the same eight learned actions, first/middle/last.
#include "gariland.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <stdio.h>

int main(int argc,char **argv)
{
    u8 page,i;
    if(argc!=2)return 2;
    for(page=0;page<3;page++) {
        char path[512];
        sw_init(61);
        st.unit[U_RAMZA].job=0x4D;st.unit[U_RAMZA].set=8;st.unit[U_RAMZA].learned=255;
        st.mode=M_SKILLS;st.skill=page==2?7:page==1?5:0;
        for(i=0;i<30;i++)sw_step(0);
        snprintf(path,sizeof path,"%s/%s.png",argv[1],page==2?"last":page==1?"middle":"first");
        if(sw_write_png(path,1))return 1;
    }
    return 0;
}
