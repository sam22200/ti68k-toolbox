// Focused ordinary step geometry versus original forward propagation.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main step_unused_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#include "../step_geometry.h"
#ifdef __m68k__
#include "../../../tools/m68kbench/bench.h"
#endif
static u16 checks,failures;static u32 digest=17;
static void equal(u16 a,u16 b){checks++;digest=(digest<<1|digest>>31)^a;if(a!=b){failures++;
#ifndef __m68k__
    if(failures<8)printf("case%u: %u expected%u\n",checks,a,b);
#endif
}}
static void trials(void)
{
    u16 bytes,n,i;const u8 *row=rt_file("fst00",&bytes);
    equal(row!=0,1);if(!row)return;
    n=(u16)((u16)row[4]<<8|row[5]);equal(n,112);equal(bytes==6+26*n||bytes==12+26*n,1);row+=6;
    for(i=0;i<n;i++,row+=26) {
        u8 flags=surface_profile(row,2),value;
        equal(step_steep(row+4*row[16],step_jump_cap(row[19])),row[23]);
        equal(step_steep(row+8+4*row[17],step_jump_cap(row[19])),row[24]);
        value=step_clear(row,row+8,row[16],row[17],row[18],row[19],row[20],row[21],2,1,row[25]);
        equal(value,row[22]);
        equal(step_clear(row,row+8,row[16],row[17],row[18],row[19],row[20],row[21],flags&SURFACE_MOVE_UPPER?2:1,flags&SURFACE_ROOM,row[25]),value);
    }
}
void game_init(void){}
void game_scenario(u16 n){(void)n;}
u8 game_update(void){return 0;}
void game_render(void){}
#ifndef __m68k__
int main(void){trials();printf("step %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
