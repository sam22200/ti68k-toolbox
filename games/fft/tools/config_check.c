// Native/PC input replay of the real configuration menu, renderer choices and inspection.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main rt_unused_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#endif
#include "camera_tiemu_demo.c"
#ifndef __m68k__
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
static u16 frames;
static u8 ok=1;
static void step(u32 keys)
{
    rt_prev=rt_keys; rt_keys=keys;
    if(!game_update()) ok=0;
#ifdef __m68k__
    if(st.config) BENCH_BEGIN(3);
#endif
    game_render();
#ifdef __m68k__
    if(st.config) BENCH_END(3);
    BENCH_SHOT(rt_dark);
#else
    printf("%u,%04X\n",frames,sw_checksum());
#endif
    rt_frame++; frames++;
}
static void press(u32 keys) { step(keys); step(0); }
#define REQUIRE(v) do { if(!(v)) ok=0; } while(0)
// Exclude HUD labels: the preference must change scenery pixels at the same camera/pose.
static u16 scenery_hash(void)
{
    const u8 *planes[2]; u16 sum=0; u8 plane,y,x;
    planes[0]=rt_light;planes[1]=rt_dark;
    for(plane=0;plane<2;plane++) for(y=8;y<93;y++) for(x=0;x<20;x++)
        sum=(u16)((sum<<1)|(sum>>15))^planes[plane][y*RT_PBYTES+x];
    return sum;
}
static void replay(void)
{
    Unit original[NUNIT]; u32 seed; u8 i; u16 idle_count;
    game_scenario(65); memcpy(original,st.unit,sizeof original); seed=rng32;
    REQUIRE(st.pzoom && !st.pwire);
    press(K_CATALOG|K_APPS); REQUIRE(st.config && st.mode==M_MENU);
    for(idle_count=0;idle_count<420;idle_count++) step(0);
    REQUIRE(pidle && pidle->stage==PIDLE_READY && st.tick==0);
    press(K_ESC); press(K_C); REQUIRE(st.perspective && pidle->stage==PIDLE_READY);
    press(K_B); press(K_CATALOG|K_APPS);
    press(K_ENTER); REQUIRE(!st.pzoom);
    press(K_ENTER); REQUIRE(st.pzoom);
    press(K_ESC); REQUIRE(!st.config);
    press(K_F5); for(i=0;i<4;i++) step(0); REQUIRE(st.rot==1 && !st.turn);
    press(K_CATALOG|K_APPS); press(K_ENTER); REQUIRE(!st.pzoom);
    press(K_DOWN); press(K_ENTER); REQUIRE(st.pwire);
    press(K_CATALOG|K_APPS); REQUIRE(!st.config);
    press(K_F1); for(i=0;i<4;i++) step(0); REQUIRE(!st.rot && !st.turn);
    press(K_C); REQUIRE(st.perspective && st.pdist==40 && st.pwire && !st.ptrans);
    press(K_F5);
    press(K_CATALOG|K_APPS); press(K_ENTER); REQUIRE(st.pdist==80);
    press(K_DOWN); press(K_ENTER); REQUIRE(!st.pwire);
    press(K_ESC); REQUIRE(st.perspective && !st.config);
    press(K_F5); // same two archived, textured fixture cameras now facing forward
    press(K_MODE); REQUIRE(st.pdist==40);
    press(K_MODE); REQUIRE(st.pdist==80);
    {
        u16 textured,flat; u8 tick=st.tick;
        st.tick=0;game_render();textured=scenery_hash();
        st.pwire=1;pvalid=0;game_render();flat=scenery_hash();REQUIRE(flat!=textured);
        st.pwire=0;pvalid=0;game_render();REQUIRE(scenery_hash()==textured);
        st.tick=tick;
    }
    press(K_B); REQUIRE(!st.perspective && !st.ptrans);
    press(K_APPS); REQUIRE(st.mode==M_ORDER && !st.config);
    press(K_APPS); REQUIRE(st.mode==M_MENU);
    REQUIRE(!memcmp(original,st.unit,sizeof original) && rng32==seed);
}
#ifndef __m68k__
int main(void)
{
    sw_init(65); replay(); return !ok;
}
#else
void _main(void)
{
    void *planes=malloc(2*RT_PSIZE);
    rt_keys=rt_prev=0;rt_frame=0;rt_seed=1;rt_state=0;rt_state_size=0;
    if(!planes || !get_fonts()) return;
    rt_dark=planes;rt_light=(u8 *)planes+RT_PSIZE;BENCH_DARK_FIRST(1);
    game_init();replay();BENCH_VALUE(0x76000000UL|((u32)ok<<16)|frames);
    free(planes);
}
#endif
