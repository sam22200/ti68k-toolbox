// Focused source-backed recoil selection and post-selection rejection.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main recoil_unused_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#include "../knockback_geometry.h"
#ifdef __m68k__
#include "../../../tools/m68kbench/bench.h"
#endif
static u16 checks,failures;static u32 digest=17;
static void equal(u16 a,u16 b){checks++;digest=(digest<<1|digest>>31)^a;if(a!=b)failures++;}
static void trials(void)
{
    u16 bytes,count,i;const u8 *row=rt_file("fkb00",&bytes);
    equal(row!=0,1);if(!row)return;
    count=(u16)((u16)row[4]<<8|row[5]);equal(count,68);
    equal(bytes==6+18*count||bytes==12+18*count,1);row+=6;
    for(i=0;i<count;i++,row+=18) {
        u16 fall=0;u8 selected=knock_layer(row,row+4,row[12],&fall);
        u8 columns[16],k;
        for(k=0;k<4;k++)columns[k]=row[k];
        for(k=4;k<8;k++)columns[k]=0;columns[7]=128;
        for(k=0;k<8;k++)columns[k+8]=row[k+4];
        if(!(surface_profile(columns,2)&SURFACE_ROOM)) {
            u16 fast_fall=0;u8 fast=knock_lower(row,row+4,row[12],&fast_fall);
            equal(fast,selected);if(fast)equal(fast_fall,fall);
        }
        // Match the source's final targetability/occupancy check: no fallback.
        if(selected&&(!(row[13]&(1U<<(selected-1)))||(row[14]&(1U<<(selected-1)))))selected=0;
        equal(selected,row[15]);if(!selected)fall=0;equal(fall,row[16]);
    }
}
void game_init(void){}
void game_scenario(u16 n){(void)n;}
u8 game_update(void){return 0;}
void game_render(void){}
#ifndef __m68k__
int main(void){trials();printf("recoil %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
