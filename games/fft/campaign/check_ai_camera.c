// Requested turn pause includes thinking; deterministic elapsed-clock injection.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main pause_runtime_main
#define game_render pause_no_render
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef game_render
void pause_no_render(void){}
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
static u16 clock_now;
static u16 pause_clock(void){return clock_now;}
#define rt_ticks pause_clock
#include "gariland.c"
#undef rt_ticks

static u16 checks,failures;
static u32 digest=17;
static void check(u16 got,u16 expected,u16 line)
{
    checks++;digest=(digest<<1|digest>>31)^got;
    if(got!=expected){failures++;
#ifndef __m68k__
        printf("line%u: %u expected%u\n",line,got,expected);
#else
        BENCH_VALUE(line);
#endif
    }
}
#define CHECK(a,b) check((u16)(a),(u16)(b),__LINE__)
static void trials(void)
{
    u8 r,i,t;Unit before[UNIT_CAPACITY];
    CHECK(sizeof(AreaUi),228);
    game_scenario(61);st.mode=M_BROWSE;st.ai=AI_OFF;st.acted=0;
    st.ai_ab=NO_ABIL;
    for(i=0;i<NUNIT;i++)for(t=0;t<NUNIT;t++)if(i!=t){
        st.act=i;st.ai_o=UNIT_CELL(&st.unit[i]);st.ai_tgt=UNIT_CELL(&st.unit[t]);
        memcpy(before,st.unit,sizeof before);
        for(r=0;r<4;r++){
            st.rot=r;CHECK(view_hides(r,i)<=88,1);
            digest=(digest<<1|digest>>31)^view_hides(r,i);
            ai_camera();CHECK(st.ai_view<4||st.ai_view==255,1);
            digest=(digest<<1|digest>>31)^st.ai_view;
            CHECK(memcmp(before,st.unit,sizeof before)==0,1);
        }
    }
}
#ifndef __m68k__
int main(void){sw_init(61);trials();printf("ai_camera %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){pause_runtime_main();trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
