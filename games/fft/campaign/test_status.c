// The memory-saving Status/portrait union must restore the original panel pixels.
#include "gariland.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
int main(void)
{
    u8 i;u16 before[3][PANEL_ROWS*RT_PBYTES/2];
    if(access("ffgi.bin",R_OK))assert(!chdir("data"));
    sw_init(61);for(i=0;i<40;i++)assert(sw_step(0));
    assert(hud_ready());memcpy(before,W->pan[0][W->pan_f[0]],sizeof before);
    st.mode=M_STATUS;st.back=M_MENU;st.sel=U_RAMZA;W->status_i=0;
    for(i=0;i<20;i++)assert(sw_step(0));
    assert(sw_step(K_B));assert(st.mode==M_MENU);
    for(i=0;i<40;i++)assert(sw_step(0));
    assert(hud_ready());assert(!memcmp(before,W->pan[0][W->pan_f[0]],sizeof before));
    puts("Gariland: Status/portrait alias restores all three panel planes");
    return 0;
}
