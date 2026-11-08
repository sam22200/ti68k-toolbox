/* Real tactical initialization through installed, replaceable pack routes. */
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main routing_runtime_main
#define game_render routing_no_render
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef game_render
void routing_no_render(void){}
#undef _main
#define _main __main
#undef off
#endif
#define FFT_PRODUCTION_CORE
#include "battle.c"
#ifndef __m68k__
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
static u16 failures,checks;
static u32 hash=17;
static void check(u16 ok,u16 line)
{
    checks++;hash=(hash<<3|hash>>29)^ok;
    if(!ok){failures++;
#ifndef __m68k__
        printf("routing failure %u\n",line);
#else
        BENCH_VALUE(line);
#endif
    }
}
#define CHECK(x) check((x)!=0,__LINE__)
static void trials(u16 scenario)
{
    CHECK(battle_state!=RT_NULL);
    if(scenario==401||scenario==299){CHECK(!W);CHECK(!igfx);CHECK(!encounter_units);return;}
    CHECK(W!=RT_NULL);CHECK(igfx!=RT_NULL);CHECK(encounter_units==11);
    if(!W||!igfx||!cdesc)return;
    CHECK(CD(LOCATION)==(scenario/100==1?19:7));
    CHECK(chapter_now==scenario/100);
    CHECK(MAP_N==MAP_W*MAP_H);CHECK(MAP_N<=256);
    CHECK(st.unit[U_RAMZA].hp>0);
    CHECK(st.unit[U_RAMZA].x<MAP_W);CHECK(st.unit[U_RAMZA].z<MAP_H);
    CHECK(W->surface!=RT_NULL);
    CHECK(st.unit[0].level==(scenario/100==1?9:1));
    /* Actual rules still use the loaded descriptor and catalogue. */
    CHECK(unit_on_field(&st.unit[U_RAMZA]));CHECK(WEAPON_REACH(&st.unit[U_RAMZA])>0);
}
#ifndef __m68k__
int main(int argc,char **argv)
{
    u16 scenario=argc>1?(u16)atoi(argv[1]):301;
    sw_init(scenario);trials(scenario);
    printf("routing %08lX failures %u checks %u\n",(unsigned long)hash,failures,checks);
    final_release();return failures!=0;
}
#else
void _main(void)
{routing_runtime_main();trials(BENCH_ARG);BENCH_VALUE(hash);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
