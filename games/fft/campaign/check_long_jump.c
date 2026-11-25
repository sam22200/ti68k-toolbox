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
    // movement_edges.inc does not call long_jump_masks(): it walks the same span
    // itself, one surface_jump_layers() per crossed cell. The risk of that change
    // is the two disagreeing, so cross them on every fixture and direction -- if
    // the geometry reports a landing at distance g, the builder's loop must
    // accept all g cells, or a legal jump is silently dropped.
    {
        static const u8 sides[4]={0,2,6,4};
        u16 bytes2,n2,i2,deep2=0,deep3=0;const u8 *row2=rt_file("flj00",&bytes2);
        if(row2) {
            n2=(u16)((u16)row2[4]<<8|row2[5]);row2+=6;
            for(i2=0;i2<n2;i2++,row2+=48) {
                u8 masks[3],direction,reversed2[40],k2,g,m;
                u8 flags=surface_profile(row2+2,row2[45]);
                u8 layers=(flags&SURFACE_MOVE_UPPER)?2:1;
                for(k2=0;k2<row2[45];k2++)memcpy(reversed2+8*(row2[45]-1-k2),row2+2+8*k2,8);
                for(direction=0;direction<4;direction++) {
                    u8 width=direction<2?row2[45]:1,height=direction<2?1:row2[45];
                    const u8 *columns=direction&1?reversed2:row2+2;
                    u8 sx=direction==1?width-1:0,sz=direction==3?height-1:0;
                    const u8 *src=columns+((u16)((u16)sz*width+sx)<<3);
                    u8 exit=surface_edge(src+4*row2[1],sides[direction]);
                    s8 dx=direction==0?1:direction==1?-1:0,dz=direction==2?1:direction==3?-1:0;
                    long_jump_masks(columns,width,height,sx,sz,row2[1],direction,row2[0],flags,masks);
                    if(masks[1])deep2++;
                    if(masks[2])deep3++;
                    for(g=1;g<=3;g++) {
                        u8 span=1;
                        if(!masks[g-1])continue;          // nothing lands there anyway
                        for(m=1;m<=g;m++) {
                            s16 mx=(s16)sx+dx*m,mz=(s16)sz+dz*m;
                            if((u16)mx>=width||(u16)mz>=height){span=0;break;}
                            if(!surface_jump_layers(columns+((u16)((u16)mz*width+mx)<<3),
                                                    sides[direction],exit,6,layers)){span=0;break;}
                        }
                        equal(span,1);                    // the builder accepts what lands
                    }
                }
            }
            // and the fixture really reaches that far, or the cross-check above
            // would be comparing nothing at the distances this change added
            equal(deep2>0,1);equal(deep3>0,1);
        }
    }
    // The link byte movement_edges.inc packs for a jump of j intervening cells.
    // j rides in bit 0 as "this link crosses something" and the cells beyond the
    // first in bits 3-4, which tactical_search.inc already adds to the cost. The
    // risk is a collision: bit 1 is water, bit 2 "presence is explicit".
    {
        u8 j,water;
        for(water=0;water<=2;water+=2)for(j=0;j<4;j++) {
            u8 kind=(u8)((j?1:0)|water|((j>1?j-1:0)<<3));
            equal(kind&1,j?1:0);              // crossing, for any distance
            equal(kind&2,water);              // water survives every j
            equal((kind>>3)&3,j>1?j-1:0);     // the extra cost the search charges
            equal(kind&4,0);                  // set later, only for j>1
            equal(kind&0xA0,0);               // nothing strays into 5 or 7
        }
    }
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
