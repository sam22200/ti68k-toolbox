// One selected68000 owner/reentry check. No hardware ports or live save.
#define USE_TI89
#define MIN_AMS 200
#define RT_CYCLES
#include <tigcclib.h>
#undef _main
#define _main shared_runtime_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#define game_scenario shared_source_scenario
#define game_update shared_source_update
#define game_render shared_source_render
#include "flow_battle.c"
#undef game_scenario
#undef game_update
#undef game_render
static u16 checks,failures;static u32 digest=17;
static void check(u8 ok){checks++;if(!ok)failures++;}
void game_scenario(u16 n)
{
    Save *a,*b;u8 k,owner;shared_source_scenario(n);
    check(W&&battle_state);if(!W)return;
    a=malloc(sizeof *a);b=malloc(sizeof *b);check(a&&b);if(!a||!b)return;
    check(rt_load("ffsave",a,sizeof *a));check(!memcmp(a->magic,SAVE_MAGIC,4));
    mod_call(MOD_RESULTS,255,0);owner=mod_id;
    for(k=0;k<3;k++){
        check(module_save_load(b));check(!memcmp(a,b,sizeof *a));
        check(comp_id==MOD_SAVE_READER+1&&!comp_ctx&&mod_id==owner);
        check(special(3,2,71,COMPANION_PROBE)==(u16)((3U<<12)^(2U<<8)^71));
        check(comp_id==MOD_COMPANION+1&&mod_id==owner);
        // Populate the fast body entry/context, then prove the next read
        // invalidates it before the graphics image can be called again.
        st.unit[U_RAMZA].job=94;st.unit[U_RAMZA].gfx=255;
        companion_body(80,80,st.rot,U_RAMZA,RT_NULL);check(comp_ctx!=RT_NULL);
    }
    for(k=0;k<4;k++)digest=(digest<<1|digest>>31)^((u8 *)b)[4024+k];
    free(a);free(b);
}
u8 game_update(void){return 0;}
void game_render(void){}
void _main(void){shared_runtime_main();BENCH_VALUE(checks);BENCH_VALUE(failures);BENCH_VALUE(digest);}
