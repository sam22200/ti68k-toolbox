// Existing action presentation, now reached by three equipped self reactions.
#define FFT_PRODUCTION_CORE
#define FFT_CHECK_NO_ZONES
#define FFT_CHECK_NATIVE_QUIET
#define STAT_SAVE_UI_CHECK
#define REACTION_FRAME_CHECK
#include "check_stat_save.c"
static void stat_picture(u8 id)
{
    const u8 *l=rt_light,*d=rt_dark;u16 y,x;u8 warm;u32 h=17;
    cam_target(&st.camx,&st.camy);
    for(warm=0;warm<16;warm++){
        W->actor_budget=255;W->ac_now++;W->hold=W->frz=0;
#ifdef __m68k__
    BENCH_BEGIN(63);
#endif
    game_render();
#ifdef __m68k__
    BENCH_END(63);
#endif
    }
    for(y=0;y<100;y++,l+=RT_PBYTES,d+=RT_PBYTES)for(x=0;x<20;x++){
        h=(h<<1|h>>31)^l[x];h=(h<<1|h>>31)^d[x];
    }
    digest=(digest<<1|digest>>31)^h;rt_frame++;
#ifndef __m68k__
    if(getenv("FFT_STAT_SAVE_PNG")){
        char name[256];sprintf(name,"%s/stat-%u.png",getenv("FFT_STAT_SAVE_PNG"),id);sw_write_png(name,4);
    }
#else
    (void)id;
#endif
}
static void ui_stats(void)
{
    u8 i;
    for(i=0;i<3;i++){
        chapter_now=0;arena();st.banner=0;st.tick=0;rt_frame=200;
        st.unit[10].pa=st.unit[10].ma=st.unit[10].sp=5;st.reaction[10]=REACTION_PA_SAVE+i;
        root(1);end_anim();CHECK(st.action.phase,CTX_SECONDARY);CHECK(st.atk,10);CHECK(st.tgt,10);
        stat_picture(i*3);impact();CHECK(st.action.phase,CTX_APPLIED);
        CHECK(st.unit[10].pa,i==0?6:5);CHECK(st.unit[10].ma,i==1?6:5);CHECK(st.unit[10].sp,i==2?6:5);
        stat_picture(i*3+1);end_anim();CHECK(st.atk,U_RAMZA);CHECK(st.tgt,10);stat_picture(i*3+2);
        finish_stats();
    }
}
#ifndef __m68k__
int main(void){sw_init(61);ui_stats();printf("stat_save_ui %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();ui_stats();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
