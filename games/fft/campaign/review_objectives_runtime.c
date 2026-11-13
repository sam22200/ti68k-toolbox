// Production goals/rendering, compared with the user's approved preview pixels.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#define RT_DRAW_ROWS 100
#include <tigcclib.h>
#undef off
#undef _main
#define _main objective_visual_main
#define game_render objective_no_render
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef game_render
void objective_no_render(void){}
#undef _main
#define _main __main
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#include "gariland.c"
static void picture(void)
{
    u8 i;
    st.open_stage=st.open_fade=st.banner=st.end=0;st.mode=M_MENU;st.ai=AI_OFF;
    for(i=0;i<16;i++){rt_frame++;W->hold=W->frz=0;W->actor_budget=255;game_render();}
    bn_start(BN_START);st.banner_t=70;rt_frame++;
#ifdef __m68k__
    {u32 start=BENCH_CYCLES;game_render();BENCH_VALUE(BENCH_CYCLES-start);BENCH_SHOT(rt_dark);}
#else
    game_render();
#endif
}
#ifndef __m68k__
int main(int argc,char **argv)
{
    if(argc!=2)return 2;sw_init(61);if(!W)return 3;picture();
    printf("objective-screen %04X end%u\n",sw_checksum(),W->bn_end);
    return sw_write_png(argv[1],1);
}
#else
void _main(void){objective_visual_main();if(W)picture();}
#endif
