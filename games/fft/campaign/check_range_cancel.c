// Cancelling Move/Attack removes every old range pixel in the very next render.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main cancel_runtime_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#endif
#include "battle.c"
#ifndef __m68k__
#include "../../../runtime/platform-sw/rt_sw.h"
#include <stdio.h>
#endif
static u16 failures,checks;
static u32 digest=17;
#define CHECK(c) do{checks++;if(!(c))failures++;}while(0)
static u32 scene_hash(void)
{
    u32 hash=17;u16 i;
    for(i=0;i<2*SC_PLANE;i++)hash=(hash<<1|hash>>31)^scene_l[i];
    return hash;
}
static void trials(u16 scenario)
{
    u8 rot,aim,overflow;
    for(rot=0;rot<4;rot++)for(aim=0;aim<2;aim++)for(overflow=0;overflow<2;overflow++) {
        u8 frame;u32 plain;
        game_scenario(scenario);st.rot=rot;st.ai=st.banner=st.turn=0;st.want=255;
        st.act=st.sel=U_RAMZA;st.mode=M_MENU;st.menu=MN_MOVE;
        cursor_place(UNIT_CELL(&st.unit[st.act]));cam_target(&st.camx,&st.camy);
        for(frame=0;frame<16;frame++){W->actor_budget=255;game_render();}
        plain=scene_hash();
        rt_prev=0;rt_keys=K_ENTER;CHECK(game_update());CHECK(st.mode==M_TARGET);
        if(aim){st.mode=M_AIM;attack_range(st.act);}
        game_render();CHECK(W->ring_y0<W->ring_y1&&scene_hash()!=plain);
        if(overflow)W->bk_over=1; // Also exercise the original-colour reload path.
        // Exhaust optional work and keep a usable screen cache: neither may delay cleanup.
        W->left=-30000;W->drop_wait=0;W->hold=W->frz=0;
        rt_prev=rt_keys;rt_keys=K_B;CHECK(game_update());CHECK(!RINGS(st.mode));
        ensure_scene(rot); // exercise cleanup before game_render resets its normal budget
        CHECK(W->drop_y0==W->drop_y1&&W->ring_i==MAP_N);CHECK(scene_hash()==plain);
        game_render();CHECK(W->drop_y0==W->drop_y1&&W->ring_i==MAP_N);
        CHECK(scene_hash()==plain);
        digest=(digest<<1|digest>>31)^scene_hash();
    }
}
#ifndef __m68k__
int main(int argc,char **argv)
{
    u16 scenario=argc>1?(u16)atoi(argv[1]):201;
    sw_init(scenario);trials(scenario);
    printf("cancel %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;
}
#else
void _main(void){cancel_runtime_main();trials(BENCH_ARG);BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
