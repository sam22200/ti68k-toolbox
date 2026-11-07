// Math uses the actual three-stage skill menu and shared global confirmation.
#define FFT_PRODUCTION_CORE
#define FFT_CHECK_NO_ZONES
#define MATH_UI_CHECK
#define REACTION_FRAME_CHECK
#include "check_math.c"
#ifndef __m68k__
#define BENCH_BEGIN(n) ((void)0)
#define BENCH_END(n) ((void)0)
#endif
static void picture(void)
{
    const u8 *l=rt_light,*d=rt_dark;u16 y,x;u32 h=17;
    for(y=0;y<100;y++,l+=RT_PBYTES,d+=RT_PBYTES)for(x=0;x<20;x++){
        h=(h<<1|h>>31)^l[x];h=(h<<1|h>>31)^d[x];
    }
    digest=(digest<<1|digest>>31)^h;
}
static void ui_tick(u32 keys)
{
    rt_prev=rt_keys;rt_keys=keys;BENCH_BEGIN(61);CHECK(game_update(),1);BENCH_END(61);
    BENCH_BEGIN(62);game_render();BENCH_END(62);
    CHECK(st.mode!=M_SKILLS||W->win_n==W->win_m,1);rt_frame++;picture();
}
static void enter_math(void)
{
    st.mode=M_ACTS;st.sub=1;W->win_k[0]=0;ui_tick(0);ui_tick(K_ENTER);
    CHECK(st.mode,M_SKILLS);CHECK(st.sub,128);CHECK(known(U_RAMZA,255),4);ui_tick(0);
    ui_tick(K_DOWN);ui_tick(0);ui_tick(K_ENTER);CHECK(st.sub,164);CHECK(known(U_RAMZA,0),418);ui_tick(0);
    ui_tick(K_ENTER);CHECK(st.sub,196);CHECK(known(U_RAMZA,255),45);ui_tick(0);
}
#if !defined(MATH_UI_CONFIRM_ONLY) && !defined(MATH_UI_STATUS_ONLY) && !defined(MATH_UI_PREVIEW_ONLY)
static void math_menu_trials(void)
{
    u8 i;u32 seed;chapter_now=0;prepare_math();st.banner=0;st.tick=0;rt_frame=200;seed=rng32;
    for(i=0;i<12;i++)ui_tick(0);
    enter_math();ui_tick(K_B);CHECK(st.sub,164);ui_tick(0);ui_tick(K_B);CHECK(st.sub,132);
    ui_tick(0);ui_tick(K_B);CHECK(st.mode,M_ACTS);CHECK(st.acted,0);CHECK(rng32==seed,1);

}
#endif
#if defined(MATH_UI_PREVIEW_ONLY) || (!defined(MATH_UI_MENU_ONLY) && !defined(MATH_UI_CONFIRM_ONLY) && !defined(MATH_UI_STATUS_ONLY))
static void math_preview_trials(void)
{
    u8 i;u32 seed;chapter_now=0;prepare_math();st.banner=0;st.tick=0;rt_frame=200;seed=rng32;
    for(i=0;i<12;i++)ui_tick(0);
    ui_tick(0);enter_math();
    for(i=0;i<45&&known(U_RAMZA,i)!=20;i++){}CHECK(i<45,1);st.skill=i;W->win_k[0]=0;ui_tick(0);
    ui_tick(K_ENTER);CHECK(st.mode,M_AREA);CHECK(W->area_ui.ready,1);CHECK(W->area_ui.count,2);CHECK(st.acted,0);
    CHECK(W->area_ui.forecast.follow,MATH_BIND);CHECK(W->area_ui.forecast.ref,4);CHECK(W->area_ui.follow_id,255);ui_tick(0);
    ui_tick(K_B);CHECK(st.mode,M_SKILLS);CHECK(st.sub,196);CHECK(st.unit[U_RAMZA].mp,0);CHECK(rng32==seed,1);
}
#endif
#if !defined(MATH_UI_MENU_ONLY) && !defined(MATH_UI_STATUS_ONLY) && !defined(MATH_UI_PREVIEW_ONLY)
static void math_confirm_trials(void)
{
    u8 i;chapter_now=0;prepare_math();st.banner=0;st.tick=0;rt_frame=200;st.mode=M_SKILLS;st.sub=196;
    for(i=0;i<45&&known(U_RAMZA,i)!=20;i++){}st.skill=i;W->win_k[0]=0;
    for(i=0;i<12;i++)ui_tick(0);
    ui_tick(K_ENTER);CHECK(st.mode,M_AREA);CHECK(W->area_ui.count,2);ui_tick(0);ui_tick(K_ENTER);
    CHECK(W->area_ui.confirm,1);CHECK(st.acted,0);
    ui_tick(0);ui_tick(K_B);CHECK(W->area_ui.confirm,0);ui_tick(0);ui_tick(K_ENTER);ui_tick(0);ui_tick(K_ENTER);
    CHECK(st.mode,M_ACT);CHECK(st.acted,1);CHECK(st.unit[10].hp,999);CHECK(st.unit[U_RAMZA].mp,0);impact();finish_math();
}
#endif
#if !defined(MATH_UI_MENU_ONLY) && !defined(MATH_UI_CONFIRM_ONLY) && !defined(MATH_UI_PREVIEW_ONLY)
static void math_status_trials(void)
{
    u8 i;rt_frame=200;
    chapter_now=0;prepare_math();st.mode=M_ACTS;st.sub=0;ui_tick(0);ui_tick(K_ENTER);CHECK(st.mode,M_AIM);CHECK(st.abil,NO_ABIL);
    ui_tick(0);ui_tick(K_B);CHECK(st.mode,M_ACTS);
    st.mode=M_STATUS;st.sel=U_RAMZA;st.back=M_MENU;st.page=1;st.status_col=1;st.status_row=0;W->status_i=0;
    for(i=0;i<11;i++)ui_tick(0);CHECK(known(U_RAMZA,255),8);CHECK(st.unit[U_RAMZA].mp,0);
}
#endif
static void math_ui_trials(void)
{
#if !defined(MATH_UI_CONFIRM_ONLY) && !defined(MATH_UI_STATUS_ONLY) && !defined(MATH_UI_PREVIEW_ONLY)
    math_menu_trials();
#endif
#if defined(MATH_UI_PREVIEW_ONLY) || (!defined(MATH_UI_MENU_ONLY) && !defined(MATH_UI_CONFIRM_ONLY) && !defined(MATH_UI_STATUS_ONLY))
    math_preview_trials();
#endif
#if !defined(MATH_UI_MENU_ONLY) && !defined(MATH_UI_STATUS_ONLY) && !defined(MATH_UI_PREVIEW_ONLY)
    math_confirm_trials();
#endif
#if !defined(MATH_UI_MENU_ONLY) && !defined(MATH_UI_CONFIRM_ONLY) && !defined(MATH_UI_PREVIEW_ONLY)
    math_status_trials();
#endif
}
#if defined(MATH_UI_MENU_ONLY)
#define MATH_UI_LABEL "math_menu"
#elif defined(MATH_UI_PREVIEW_ONLY)
#define MATH_UI_LABEL "math_preview"
#elif defined(MATH_UI_CONFIRM_ONLY)
#define MATH_UI_LABEL "math_confirm"
#elif defined(MATH_UI_STATUS_ONLY)
#define MATH_UI_LABEL "math_status"
#else
#define MATH_UI_LABEL "math_ui"
#endif
#ifndef __m68k__
int main(void){sw_init(61);math_ui_trials();printf(MATH_UI_LABEL " %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();math_ui_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
