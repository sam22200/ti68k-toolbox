// The actual pinned compositor against a scalar pixel oracle, PC and68000.
#ifdef __m68k__
#define USE_TI89
#include <tigcclib.h>
#include "../../../tools/m68kbench/bench.h"
#else
#include <stdio.h>
#endif
#include "../../../runtime/core/rt.h"
#include "../base.inc"
static u16 cache[2][RT_PSIZE/2],planes[2][RT_PSIZE/2],expected[2][RT_PSIZE/2];
void *rt_light=planes[0],*rt_dark=planes[1];
static struct {u16 *win[2];u8 win_h,win_w;} work,*W=&work;
#include "../window_blit.inc"
static u16 failures,checks;static u32 digest=17;
static void trials(void)
{
    static const u8 widths[]={64,52,34,17,1};
    u8 k,p,r,x;u16 at;W->win[0]=cache[0];W->win[1]=cache[1];W->win_h=41;
    for(k=0;k<sizeof widths;k++) {
        W->win_w=widths[k];memset(cache,0,sizeof cache);
        for(p=0;p<2;p++)for(at=0;at<RT_PSIZE;at++)((u8 *)planes[p])[at]=(u8)(at*13+p*71);
        memcpy(expected,planes,sizeof planes);
        for(r=0;r<W->win_h;r++)for(x=(u8)(64-W->win_w);x<64;x++) {
            u8 bit=(u8)(128U>>(x&7));
            // A shorter title is transparent outside its first-row silhouette.
            if(r<3&&x<54)continue;
            for(p=0;p<2;p++) {
                u8 color=(u8)(r==0||r==W->win_h-1||((r+x+p)&3)==0);
                at=(u16)r*RT_PBYTES+(x>>3);
                if(color)((u8 *)cache[p])[at]|=bit;
                at=(u16)(r+51)*RT_PBYTES+12+(x>>3);
                ((u8 *)expected[p])[at]&=(u8)~bit;
                if(color)((u8 *)expected[p])[at]|=bit;
            }
        }
#ifdef __m68k__
        BENCH_BEGIN(3);
#endif
        window_blit(51);
#ifdef __m68k__
        BENCH_END(3);
#endif
        for(p=0;p<2;p++)for(at=0;at<RT_PSIZE;at++) {
            u8 value=((u8 *)planes[p])[at];checks++;
            if(value!=((u8 *)expected[p])[at])failures++;
            digest=(digest<<1|digest>>31)^value;
        }
    }
}
#ifdef __m68k__
void _main(void){trials();BENCH_VALUE(checks);BENCH_VALUE(failures);BENCH_VALUE(digest);}
#else
int main(void){trials();printf("window-blit %u checks failures%u digest%08lX\n",checks,failures,(unsigned long)digest);return failures!=0;}
#endif
