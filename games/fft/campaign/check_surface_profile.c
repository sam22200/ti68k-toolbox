// Reduced-layer kernels must match full checks for every retained map profile.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main profile_unused_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#include "../surface_geometry.h"
#ifdef __m68k__
#include "../../../tools/m68kbench/bench.h"
#endif
static u32 checks,digest=17;static u16 failures;
static void equal(u16 a,u16 b){checks++;digest=(digest<<1|digest>>31)^a;if(a!=b)failures++;}
static u16 word(const u8 *p){return (u16)((u16)p[0]<<8|p[1]);}
static void deep_trials(void)
{
    u8 bank,col[16]={0,0,0,0,0,0,0,128,0,0,0,0,0,0,0,128};
    equal(surface_profile(col,2),0);col[12]=4;equal(surface_profile(col,2),SURFACE_HIT_UPPER);
    col[15]=0;equal(surface_profile(col,2),SURFACE_ROOM|SURFACE_MOVE_UPPER|SURFACE_HIT_UPPER);
    col[12]=0;equal(surface_profile(col,2),SURFACE_ROOM|SURFACE_MOVE_UPPER);
    col[15]=128;col[8]=63;equal(surface_profile(col,2),SURFACE_ROOM);
    for(bank=0;bank<16;bank++) {
        char name[6]="fsp00";const u8 *row;u16 bytes,n,i;
        name[3]=(char)('0'+bank/10);name[4]=(char)('0'+bank%10);
        row=rt_file(name,&bytes);if(!row)break;
        equal(bytes>=6,1);n=word(row+4);equal(bytes==6+10*n||bytes==12+10*n,1);
        equal(row[0]=='F'&&row[1]=='S'&&row[2]=='P'&&row[3]=='1',1);row+=6;
#ifdef __m68k__
        BENCH_BEGIN(4);
#endif
        for(i=0;i<n;i++,row+=10) {
            u8 flags=row[0],layers=flags&SURFACE_MOVE_UPPER?2:1,side;u16 h;
            const u8 *column=row+1;
            equal(surface_profile(column,1)&~flags,0);
            for(h=0;h<256;h++) {
                // The runtime ray sampler only queries strictly positive heights.
                if(h)equal(surface_hit_layers(column,(s16)h,flags&SURFACE_HIT_UPPER?2:1),surface_hit(column,(s16)h));
                for(side=0;side<8;side+=2) {
                    equal(surface_ceiling_layers(column,side,(u8)h,layers),surface_ceiling(column,side,(u8)h));
                    equal(surface_jump_layers(column,side,(u8)h,4,layers),surface_jump_clear(column,side,(u8)h,4));
                    equal(surface_jump_layers(column,side,(u8)h,6,layers),surface_jump_clear(column,side,(u8)h,6));
                }
            }
        }
#ifdef __m68k__
        BENCH_END(4);
#endif
    }
    equal(bank>0,1);
}
#ifndef PROFILE_DEEP
static void trials(void)
{
    // Curated semantic boundaries; the full retail sweep is opt-in.
    static const u8 rows[][9]={
        {0,0,0,0,0,0,0,0,128},
        {SURFACE_HIT_UPPER,0,0,0,0,4,0,0,128},
        {SURFACE_ROOM|SURFACE_MOVE_UPPER|SURFACE_HIT_UPPER,0,0,0,0,4,0,0,1},
        {SURFACE_ROOM|SURFACE_MOVE_UPPER,0,0,0,0,0,0,0,0},
        {SURFACE_ROOM,63,0,0,0,0,0,0,128},
        {0,2,35,0x44,1,0,0,0,128},
        {SURFACE_ROOM|SURFACE_MOVE_UPPER|SURFACE_HIT_UPPER,2,3,0xe4,5,8,0,0,1},
        {SURFACE_HIT_UPPER,2,3,0x1b,5,8,0,0,129},
        {SURFACE_ROOM,62,1,0xff,0,0,0,0,128}
    };
    static const u8 heights[]={0,1,2,3,4,5,6,7,8,11,12,13,15,16,17,63,126,127,128,250,254,255};
    u8 i,j,side;
    for(i=0;i<sizeof(rows)/sizeof(rows[0]);i++) {
        const u8 *column=rows[i]+1;u8 flags=rows[i][0],layers=flags&SURFACE_MOVE_UPPER?2:1;
        equal(surface_profile(column,1),flags);
        for(j=0;j<sizeof heights;j++) {
            u8 h=heights[j];
            if(h)equal(surface_hit_layers(column,h,flags&SURFACE_HIT_UPPER?2:1),surface_hit(column,h));
            for(side=0;side<8;side+=2) {
                equal(surface_ceiling_layers(column,side,h,layers),surface_ceiling(column,side,h));
                equal(surface_jump_layers(column,side,h,4,layers),surface_jump_clear(column,side,h,4));
                equal(surface_jump_layers(column,side,h,6,layers),surface_jump_clear(column,side,h,6));
            }
        }
    }
}
#else
#define trials deep_trials
#endif
void game_init(void){}
void game_scenario(u16 n){(void)n;}
u8 game_update(void){return 0;}
void game_render(void){}
#ifndef __m68k__
int main(void){trials();printf("profile %08lX failures %u checks %lu\n",(unsigned long)digest,failures,(unsigned long)checks);return failures!=0;}
#else
void _main(void){BENCH_NAME(4,"Surface profile equality");trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
