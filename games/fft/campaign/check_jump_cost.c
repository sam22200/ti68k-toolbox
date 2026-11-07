// Terrain-only long-span prototype; never used by the shipping search yet.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main long_jump_unused_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include <string.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#include "../movement_cost.h"
#ifdef __m68k__
#include "../../../tools/m68kbench/bench.h"
#endif
static u16 checks,failures;static u32 digest=17;
static void equal(u16 a,u16 b){checks++;digest=(digest<<1|digest>>31)^a;if(a!=b)failures++;}
static void trials(void)
{
    u16 bytes,n,i;const u8 *row=rt_file("fjc00",&bytes);
    equal(row!=0,1);if(!row)return;
    n=(u16)((u16)row[4]<<8|row[5]);equal(n,57);equal(bytes==6+6*n||bytes==12+6*n,1);row+=6;
    for(i=0;i<n;i++,row+=6)equal(movement_gap_remaining(row[2],row[0],row[1]),row[5]);
    // Static producer policy, distinct from the executed propagation trials.
    equal(movement_terrain_cost(0,1,0),1);equal(movement_terrain_cost(0,3,0),3);
    equal(movement_terrain_cost(0,3,1),3);equal(movement_terrain_cost(2,3,1),1);
    equal(movement_terrain_cost(2,3,0),2);equal(movement_terrain_cost(255,3,1),255);
    equal(movement_gap_remaining(7,3,3),1);equal(movement_gap_remaining(6,3,3),0);
}
void game_init(void){}
void game_scenario(u16 n){(void)n;}
u8 game_update(void){return 0;}
void game_render(void){}
#ifndef __m68k__
int main(void){trials();printf("jumpcost %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
