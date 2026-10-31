// Existing renderer at secondary preparation/impact and restored primary states.
#define REACTION_FRAME_CHECK
#include "check_reactions.c"
static void frame(u8 id)
{
    u8 i;u16 n;u32 hash=17;
    cam_target(&st.camx,&st.camy);
    for(i=0;i<16;i++){W->actor_budget=255;W->ac_now++;W->hold=W->frz=0;game_render();}
    for(n=0;n<RT_PSIZE;n++){
        hash=(hash<<1|hash>>31)^((const u8 *)rt_light)[n];
        hash=(hash<<1|hash>>31)^((const u8 *)rt_dark)[n];
    }
#ifdef __m68k__
    BENCH_VALUE(hash);
#else
    printf("reactionframe %u %08lX\n",id,(unsigned long)hash);
    if(id==0&&getenv("FFT_REACTION_PNG"))sw_write_png(getenv("FFT_REACTION_PNG"),1);
#endif
}
static void pictures(void)
{
    arena();st.reaction[10]=REACTION_COUNTER;root(4);change(U_RAMZA,SID_DONT_ACT);end_anim();
    st.act_t=ACT_HIT-1;frame(0);impact();frame(1);end_anim();frame(2);
    arena();st.reaction[10]=REACTION_POTION;st.unit[10].hp=100;root(7);end_anim();
    st.act_t=ACT_HIT-1;frame(3);impact();frame(4);end_anim();frame(5);
}
#ifndef __m68k__
int main(void){sw_init(61);pictures();return failures!=0;}
#else
void _main(void){reaction_runtime_main();pictures();}
#endif
