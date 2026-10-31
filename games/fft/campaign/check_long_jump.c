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
#include "../long_jump_geometry.h"
#ifdef __m68k__
#include "../../../tools/m68kbench/bench.h"
#endif
static u16 checks,failures;static u32 digest=17;
static void equal(u16 a,u16 b){checks++;digest=(digest<<1|digest>>31)^a;if(a!=b)failures++;}
static void trials(void)
{
    static const u8 jumps[]={0,1,2,3,4,5,6,7,8,255},limits[]={0,0,1,1,2,2,3,3,3,3};
    u16 bytes,n,i;const u8 *row=rt_file("flj00",&bytes);
    for(i=0;i<sizeof jumps;i++)equal(long_jump_limit(jumps[i]),limits[i]);
    equal(row!=0,1);if(!row)return;
    n=(u16)((u16)row[4]<<8|row[5]);equal(n,22);equal(bytes==6+48*n||bytes==12+48*n,1);row+=6;
    for(i=0;i<n;i++,row+=48) {
        u8 masks[3],j,direction,reversed[40],k,flags=surface_profile(row+2,row[45]);
        for(k=0;k<row[45];k++)memcpy(reversed+8*(row[45]-1-k),row+2+8*k,8);
        for(direction=0;direction<4;direction++) {
            u8 width=direction<2?row[45]:1,height=direction<2?1:row[45];
            const u8 *columns=direction&1?reversed:row+2;
            long_jump_masks(columns,width,height,direction==1?width-1:0,direction==3?height-1:0,
                            row[1],direction,row[0],flags,masks);
            for(j=0;j<3;j++)equal(masks[j],row[42+j]);
        }
    }
    {u8 columns[8]={0},masks[3]={255,255,255};long_jump_masks(columns,1,1,0,0,0,4,7,0,masks);
     equal(masks[0]|masks[1]|masks[2],0);}
}
void game_init(void){}
void game_scenario(u16 n){(void)n;}
u8 game_update(void){return 0;}
void game_render(void){}
#ifndef __m68k__
int main(void){trials();printf("longjump %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
