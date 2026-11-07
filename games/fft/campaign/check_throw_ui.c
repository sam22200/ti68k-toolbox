// Throw's existing menu, first-frame input transitions and range rendering.
#define FFT_CHECK_NO_ZONES // keep native transport bounded; explicit frame zones below
#define FFT_CHECK_NATIVE_ORDINAL // retain assertion IDs without native source-line arguments
#define REFLECT_CHECK
#define REACTION_FRAME_CHECK
#include "check_offensive_magic.c"
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
static void tick_ui(u32 keys)
{
    rt_prev=rt_keys;rt_keys=keys;
    BENCH_BEGIN(61);CHECK(game_update(),1);BENCH_END(61);
#ifdef THROW_UI_STONE_CONTROL
    if(st.mode==M_AIM&&st.abil>=512)aim(0x94);
#endif
    BENCH_BEGIN(62);game_render();BENCH_END(62);rt_frame++;picture();
}
static void ui_trials(void)
{
    u16 item;u8 k;u32 before;
    prepare_spell(19);st.unit[U_RAMZA].set=20;st.unit[U_RAMZA].learned=0xfff;
    st.acted=st.moved=0;st.mode=M_SKILLS;st.skill=0;st.banner=0;rt_frame=200;st.tick=0;
    for(item=0;item<128;item++)battle_save.inv[item]=3;
    before=rng32;CHECK(known(U_RAMZA,255),80);CHECK(known(U_RAMZA,79),639);
    for(k=0;k<12;k++)tick_ui(0);
    tick_ui(K_DOWN);CHECK(st.skill,1);tick_ui(0);
    tick_ui(K_ENTER);CHECK(st.mode,M_AIM);CHECK(st.abil,
#ifdef THROW_UI_STONE_CONTROL
        0x94
#else
        514
#endif
    );CHECK(st.acted,0);tick_ui(0);
    tick_ui(K_B);CHECK(st.mode,M_SKILLS);CHECK(battle_save.inv[2],3);CHECK(rng32==before,1);
    tick_ui(0);st.skill=79;W->win_k[0]=0;tick_ui(0);
    tick_ui(K_A);CHECK(st.mode,M_AIM);CHECK(st.abil,
#ifdef THROW_UI_STONE_CONTROL
        0x94
#else
        639
#endif
    );tick_ui(0);
    tick_ui(K_B);CHECK(st.mode,M_SKILLS);CHECK(battle_save.inv[127],3);CHECK(st.acted,0);CHECK(rng32==before,1);
    // The ordinary Status adapter must read weapon stock and dynamic Move range.
    st.mode=M_STATUS;st.back=M_MENU;st.page=1;st.status_col=1;st.status_row=79;st.status_info=1;W->status_i=0;
    for(k=0;k<11;k++)tick_ui(0);CHECK(W->status_n,9);CHECK(st.status_row,79);
    tick_ui(K_B);CHECK(st.status_info,0);tick_ui(0);tick_ui(K_B);CHECK(st.mode,M_MENU);
    CHECK(battle_save.inv[127],3);CHECK(rng32==before,1);
}
#ifndef __m68k__
int main(void){sw_init(61);ui_trials();printf("throw_ui %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();ui_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
