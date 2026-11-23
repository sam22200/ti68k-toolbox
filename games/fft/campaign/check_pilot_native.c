// One full native fight replay. Uses the shipping engine and matching archived
// modules, actual menu/held2nd inputs and a real FSV9 session. The controller
// waits for UI states: PIC preparation takes a different number of PC frames.
#define USE_TI89
#define MIN_AMS 200
#define RT_SAVE_ATOMIC
#define RT_DRAW_ROWS 100
#include <tigcclib.h>
#undef _main
#define _main pilot_runtime_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#define FFT_CAMPAIGN_FLOW
#define FFT_PRODUCTION_CORE
#define game_update pilot_source_update
#include "battle.c"
#undef game_update
static u32 pilot_native_keys(void)
{
    if(st.open_fade)return 0;
    if(st.open_stage){
        if(!(rt_frame&1))return 0;
        return !st.quit?K_ESC:st.quit==1?K_DOWN:K_ENTER;
    }
    if(st.mode==M_END)return rt_frame&1?K_ENTER:0;
    if(st.ai)return K_A;               // ordinary held2nd acceleration, no debug flag
    if(st.banner||st.turn||st.want!=255)return 0;
    if(!(rt_frame&1))return 0;
    if(st.mode==M_MENU)return st.menu==MN_AUTO?K_ENTER:K_DOWN;
    if(st.mode==M_AUTO)return st.sub==2?K_ENTER:K_DOWN;
    return 0;
}
u8 game_update(void)
{
    if(battle_state&&W)rt_keys=pilot_native_keys();
    if(!(rt_frame&1023)&&battle_state)BENCH_VALUE(0x80000000UL|((u32)st.open_stage<<24)|((u32)st.act<<16)|((u16)st.ai<<8)|st.mode);
    return pilot_source_update();
}
void _main(void)
{
    pilot_runtime_main();
    BENCH_VALUE(battle_state?st.open_handoff:0);
    BENCH_VALUE(battle_state?st.dbg:255);
    BENCH_VALUE(rt_save_committed());
    final_release();
    BENCH_VALUE(!battle_state&&!rt_state&&!sbuf&&!vbuf&&!navbuf&&!acache&&!comp_block&&!mod_zone);
}
