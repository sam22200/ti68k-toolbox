// Returning from Status must preserve each displayed panel plane. Use the
// cache accessor: native/PC chapter panels pack three ten-byte slots per row.
#include "gariland.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
int main(void)
{
    u8 i,p,r;u8 before[3][PANEL_ROWS][10];
    if(access("ffgi.bin",R_OK))assert(!chdir("data"));
    sw_init(61);for(i=0;i<40;i++)assert(sw_step(0));
    assert(hud_ready());
    for(p=0;p<3;p++)for(r=0;r<PANEL_ROWS;r++)
        memcpy(before[p][r],(const u8 *)PAN_CACHE(0,W->pan_f[0],p)+(u16)r*RT_PBYTES,10);
    st.mode=M_STATUS;st.back=M_MENU;st.sel=U_RAMZA;W->status_i=0;
    for(i=0;i<20;i++)assert(sw_step(0));
    assert(sw_step(K_B));assert(st.mode==M_MENU);
    for(i=0;i<40;i++)assert(sw_step(0));
    assert(hud_ready());
    for(p=0;p<3;p++)for(r=0;r<PANEL_ROWS;r++)
        assert(!memcmp(before[p][r],(const u8 *)PAN_CACHE(0,W->pan_f[0],p)+(u16)r*RT_PBYTES,10));
    puts("Gariland: Status restores all three displayed panel planes");
    return 0;
}
