// These fixtures use --frames0 and time their own zones. Omit only the
// unused runtime loop/names; retain every scripted frame, marker and pixel hash.
#ifdef RT_CYCLES
#include "../../../tools/m68kbench/bench.h"
#undef BENCH_NAME
#define BENCH_NAME(id,s) ((void)0)
#undef BENCH_FRAMES
#define BENCH_FRAMES 0
#endif
// Actual renderer and unchanged root/secondary/reward flow after visual approval.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main impact_runtime_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
// Its native binary sits at the program-size limit: engine zones off, 57/58 kept,
// and the test/tool-only engine entries it never calls left out.
#define FFT_CHECK_NO_ZONES
#define FFT_PRODUCTION_CORE
#include "gariland.c"
#ifndef AREA_IMPACT_FIRST
#define AREA_IMPACT_FIRST 0
#define AREA_IMPACT_LAST 8
#endif
// The first/last native carriers need their marker bytes for gameplay.
// Every assertion and pixel sample remains; part4 retains cycle zones.
#if defined(RT_CYCLES) && AREA_IMPACT_FIRST == 4
#define TZB(n) BENCH_BEGIN(n)
#define TZE(n) BENCH_END(n)
#else
#define TZB(n)
#define TZE(n)
#endif
#include "area_impact_pixels.inc"
static u16 checks,failures;static u32 digest=17,pictures=17;
static void check(u16 got,u16 expected
#ifndef __m68k__
                  ,u16 line
#endif
                  )
{
    checks++;digest=(digest<<1|digest>>31)^got;
    if(got!=expected){failures++;
#ifndef __m68k__
        printf("line%u: %u expected%u\n",line,got,expected);
#else
        // Native carrier budget: final failure count is retained; PC reports source lines.
#endif
    }
}
#ifdef __m68k__
#define CHECK(a,b) check((u16)(a),(u16)(b))
#else
#define CHECK(a,b) check((u16)(a),(u16)(b),__LINE__)
#endif
#ifdef __m68k__
#include "area_impact_native_fixture.inc"
#else
#include "area_ui_fixture.inc"
#include "area_impact_fixture.inc"
#endif
static u32 state_bytes(void)
{
    const u8 *p=(const u8 *)&st;u16 n;u32 h=17;
    for(n=0;n<sizeof st;n++)h=(h<<1|h>>31)^*p++;
    return h;
}
static void trials(void)
{
    u8 kind;u16 n; // Exact cache capacity is checked by check_impact_bounds.
    for(kind=AREA_IMPACT_FIRST;kind<AREA_IMPACT_LAST;kind++){
        u8 committed=0,secondary=0;u16 paid;impact_fixture(kind);paid=st.unit[st.act].mp;
#if AREA_IMPACT_FIRST == 6
        // The full sequence reaches case6 with its unchanged-camera caches warm.
        // Prime only presentation; do not advance game state, RNG or the clock.
        if(kind==6){
#ifndef __m68k__
            u32 before=state_bytes(),seed=rng32;
#endif
            for(n=0;n<8;n++)game_render();
#ifndef __m68k__
            if(state_bytes()!=before||rng32!=seed)failures++;
#endif
        }
#endif
        for(n=0;n<160;n++){
            u8 sample=n==0||n==12||n==16||n==20||n==30||n==50||n==90||n==130;
            TZB(57);if(st.mode==M_ACT)CHECK(game_update(),1);else st.tick++;TZE(57);
            // Identical reviewed camera; projection and tactical positions are unchanged.
            st.camx=impact_fx.camx;st.camy=impact_fx.camy;
            if(st.act_t==ACT_HIT&&area_impact_root()){
                CHECK(W->cast.phase,CAST_BATCH_READY);CHECK(W->area_impact.ready,W->cast.mask);
            }
            if(sample){
                CastBatch packet=W->cast;u32 before=state_bytes(),seed=rng32;
                TZB(58);game_render();TZE(58);
                CHECK(state_bytes()==before,1);CHECK(rng32==seed,1);
                CHECK(memcmp(&packet,&W->cast,sizeof packet)==0,1);
                {u32 pixels=impact_picture_hash();pictures=(pictures<<1|pictures>>31)^pixels;
#ifdef __m68k__
                 BENCH_VALUE(pixels);
#else
                 printf("frame %u %u %08lX\n",kind,n,(unsigned long)pixels);
#endif
                }
            }else{TZB(58);game_render();TZE(58);}
            if(W->cast.phase==CAST_BATCH_APPLIED&&!committed){
                committed=1;CHECK(st.unit[st.act].mp,paid);
                CHECK(W->area_impact.ready,W->cast.mask);
#if AREA_IMPACT_FIRST < 6 && AREA_IMPACT_LAST > 4
                if(kind==4)CHECK(W->cast.mask,0);
                if(kind==5)CHECK(core_status_current(&st.unit[fixture_ui.enemy[1]].statuses,2)&128,0);
#endif
            }
            if(st.action.phase==CTX_SECONDARY||st.action.phase==CTX_APPLIED){secondary=1;CHECK(area_impact_root(),0);}
#if AREA_IMPACT_LAST > 6
            if(kind==6&&st.mode==M_ACT)CHECK(st.end,END_NONE);
#endif
            rt_frame++;
        }
        CHECK(committed,1);CHECK(st.action.phase,CTX_IDLE);CHECK(W->cast.phase,CAST_BATCH_EMPTY);
        CHECK(st.unit[st.act].mp,paid);
#if AREA_IMPACT_LAST > 4
#if AREA_IMPACT_LAST > 6
        if(kind==6)CHECK(st.end,END_VICTORY);
        if(kind==7){CHECK(secondary,1);CHECK(st.bag[0],0);}
#endif
#endif
    }
}
#ifndef __m68k__
int main(void){sw_init(61);trials();printf("area_impact %08lX pictures %08lX failures %u checks %u\n",(unsigned long)digest,(unsigned long)pictures,failures,checks);return failures!=0;}
#else
void _main(void){impact_runtime_main();trials();BENCH_VALUE(digest);BENCH_VALUE(pictures);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
