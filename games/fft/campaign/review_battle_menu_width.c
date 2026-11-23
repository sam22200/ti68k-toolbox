// Same-state PC art review, legacy width is an explicitly historical control.
#include "flow_battle.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <assert.h>
#include <stdio.h>
int main(int argc,char **argv)
{
    u8 mode,k,phase;char path[512];assert(argc==2);sw_init(200);assert(W&&battle_state);
    st.open_stage=st.open_fade=st.turn=st.ai=0;st.act=st.sel=U_RAMZA;st.moved=st.acted=0;
    for(mode=0;mode<2;mode++) {
        st.mode=mode?M_ACTS:M_MENU;st.menu=st.sub=0;W->win_n=W->win_m=0;W->win_k[0]=0;
        rt_frame=0;st.tick=12; // Maximum rightward glove phase.
        for(k=0;k<20;k++){W->actor_budget=255;W->ac_now++;W->hold=W->frz=0;game_render();}
        W->left=30000;W->hud_wait=0;draw_hud();
        snprintf(path,sizeof path,"%s/%s.png",argv[1],mode?"act":"menu");assert(!sw_write_png(path,1));
        printf("%s width%u\n",mode?"Act":"Menu",W->win_w);
        // Exercise all eight poses through the actual battle renderer. At
        // maximum sway the ink at +16 leaves exactly one pixel before text.
        assert(CHOICE_HAND_X(6)+3+16==6-2);
        for(phase=0;phase<=8;phase++) {
            st.tick=(u16)(phase*4);W->actor_budget=255;W->ac_now++;W->hold=W->frz=0;
            game_render();W->left=30000;W->hud_wait=0;draw_hud();
            snprintf(path,sizeof path,"%s/%s-%u.png",argv[1],mode?"act":"menu",phase);assert(!sw_write_png(path,1));
        }
    }
    return 0;
}
