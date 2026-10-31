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
static void ready(u16 began)
{
    game_scenario(61);clock_now=began;rt_keys=rt_prev=0;
    st.act=U_RAMZA;st.ai=AI_THINK;st.ai_t=0;st.banner=0;st.mode=M_BROWSE;
    st.moved=st.acted=1;st.ai_first=0;st.ai_view=255;
    W->frz=1;W->ai_k=3;W->ai_nab=W->ai_n=W->ai_self=0;
    W->ai_c0=UNIT_CELL(&st.unit[st.act]);
}
static void deadlines(void)
{
    static const u16 thought[]={0,100,256,383,384,512};
    Unit before[UNIT_CAPACITY];u8 i,j;u32 seed;
    for(j=0;j<2;j++)for(i=0;i<sizeof thought/sizeof thought[0];i++){
        u16 began=j?65400:100;
        ready(began);seed=rng32;memcpy(before,st.unit,sizeof before);
        ai_step();CHECK(st.ai,AI_GO);CHECK(st.ai_t,1);CHECK(ai_hold_start,began);
        clock_now=(u16)(began+thought[i]);
        // Inject elapsed time across a blocking computation, including clock wrap.
        ai_step();
        if(thought[i]<AI_HOLD_TICKS){
            CHECK(st.ai,AI_GO);CHECK(W->frz,1);
            rt_keys=K_A|K_ENTER;
            // No frame-based credit for repeated fast updates at the same clock.
            {u8 k;for(k=0;k<64;k++)ai_step();}
            CHECK(st.ai,AI_GO);CHECK(st.ai_t,1);
            CHECK(memcmp(before,st.unit,sizeof before)==0,1);
            clock_now=(u16)(began+AI_HOLD_TICKS-1);ai_step();CHECK(st.ai,AI_GO);
            clock_now=(u16)(began+AI_HOLD_TICKS);ai_step();
        }
        CHECK(st.ai,AI_FACE);CHECK(W->hold,1);CHECK(st.mode,M_FACE);
        CHECK(st.unit[st.act].hp,before[st.act].hp);CHECK(st.unit[st.act].ct,before[st.act].ct);
        CHECK(rng32==seed,1);
    }
}
static void restarts(void)
{
    ready(100);ai_step();clock_now=300;ai_step();CHECK(st.ai,AI_GO);
    // Starting the next unit/auto command arms a fresh pause despite old statics.
    st.ai=AI_THINK;st.ai_t=0;ai_step();CHECK(ai_hold_start,300);
    clock_now=484;ai_step();CHECK(st.ai,AI_GO);
    clock_now=684;ai_step();CHECK(st.ai,AI_FACE);
    ready(100);ai_step();st.ai=AI_THINK;st.ai_t=AI_HOLD_SKIP;
    ai_step();CHECK(st.ai,AI_GO);ai_step();CHECK(st.ai,AI_FACE);
    // A late cast rejection/weather refresh replans inside the same pause.
    ready(100);ai_step();clock_now=300;st.ai=AI_THINK;
    ai_step();CHECK(ai_hold_start,100);clock_now=484;ai_step();CHECK(st.ai,AI_FACE);
    ready(100);st.auto_units=(u16)1<<st.act;
    rt_keys=K_ESC;rt_prev=0;CHECK(update(0),1);CHECK(st.ai,AI_OFF);
    clock_now=800;auto_control(1);W->frz=1;W->ai_k=3;W->ai_nab=W->ai_n=W->ai_self=0;
    ai_step();CHECK(ai_hold_start,800);CHECK(st.ai,AI_GO);
}
static void frozen_picture(void)
{
    u16 i;u32 before,after;
    ready(100);ai_step();
    for(i=0;i<RT_PSIZE;i++){
        ((u8 *)rt_light)[i]=(u8)i;((u8 *)rt_dark)[i]=(u8)~i;
    }
    before=planes_sum();clock_now=356;ai_step();game_render();after=planes_sum();
    CHECK(before==after,1);CHECK(st.ai,AI_GO);CHECK(W->frz,1);
}
static void trials(void){deadlines();restarts();frozen_picture();}
#ifndef __m68k__
int main(void){sw_init(61);trials();printf("ai_pause %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){pause_runtime_main();trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
