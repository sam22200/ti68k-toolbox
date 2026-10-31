// Pixel-exact strip/cache checks against the frozen uncompressed source map.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main storage_runtime_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#endif
#define WM_REFERENCE
#include "worldmap.c"
#ifndef __m68k__
#include "../../../runtime/platform-sw/rt_sw.h"
#include <stdio.h>
#endif
static u16 failures,checks;
static u32 digest=17;
static void decor_trial(void)
{
    static const u16 ys[]={0,1,7,8,63,127,155,156};
    static const u16 xs[]={0,1,7,8,15,16,175,176};
    const u8 *source=bank,*all=rt_file("ffwall",RT_NULL);
    u8 *expected=malloc(4000);
    u8 fixture,yi,xi,pass,p,r,k,i,warm;
    u16 row;
    if(!all||!expected){failures++;if(expected)free(expected);return;}
    save.flags[0]|=2;wm.at=6;
    for(fixture=0;fixture<2;fixture++) {
        bank=fixture?all:source;decor_ready=0;memset(strip_tag,255,sizeof strip_tag);
        for(yi=0;yi<8;yi++)for(xi=0;xi<8;xi++) {
            s16 cx=(s16)xs[xi],cy=(s16)ys[yi];
            map_background(cx,cy,0);
            for(pass=0;pass<2;pass++)for(r=0;r<ROUTE_N;r++) {
                const u8 *rt=route(r),*pt=bank+b16(rt+4);
                if(!shown(rt[0])||!shown(rt[1]))continue;
                for(k=0;k+1<rt[3];k++,pt+=4)
                    line(s16v(pt)-cx,s16v(pt+2)-cy,s16v(pt+4)-cx,s16v(pt+6)-cy,!pass);
            }
            for(i=0;i<LOC_N;i++)if(shown(i))marker(loc_x(i)-cx,loc_y(i)-cy,loc(i)[4]==1);
            for(p=0;p<2;p++)for(row=0;row<100;row++)
                memcpy(expected+(u16)p*2000+row*20,(p?rt_dark:rt_light)+row*RT_PBYTES,20);
            for(warm=0;warm<2;warm++) {
                map_background(cx,cy,1);
                for(p=0;p<2;p++)for(row=0;row<100;row++) {
                    const u8 *a=expected+(u16)p*2000+row*20,*b=(p?rt_dark:rt_light)+row*RT_PBYTES;
                    for(k=0;k<20;k++) {
                        if(a[k]!=b[k])failures=1;
                        digest=(digest<<1|digest>>31)^b[k];
                    }
                }
                checks++;
            }
        }
    }
    bank=source;decor_ready=0;free(expected);
}
static void trial(void)
{
    static const u16 ys[]={0,1,7,8,15,16,63,127};
    const u8 *ref=rt_file("ffwref",RT_NULL);
    u16 at,plane,pass,row;
    u8 y,x,p,k;
    if(!bank||!ref||memcmp(ref,"FWM2",4)){failures++;return;}
    at=b16(ref+4);plane=stride*map.h*16;
    for(pass=0;pass<2;pass++)for(y=0;y<12;y++)for(x=0;x<18;x++) {
        u16 last=map.h*16-100,index=pass?11-y:y;
        s16 cx=x>=16?(s16)(map.w*16-160-(x-16)):(s16)(x+pass*80);
        s16 cy=index<8?ys[index]:index==8?last>>1:index==9?last-8:index==10?last-1:last;
        if(cy>last)cy=(s16)last;
#ifdef __m68k__
        BENCH_BEGIN(7);
#endif
        map_background(cx,cy,0);
#ifdef __m68k__
        BENCH_END(7);BENCH_BEGIN(8);
#endif
        map_background(cx,cy,0); // identical warmed viewport, no new strip decode
#ifdef __m68k__
        BENCH_END(8);
#endif
        for(p=0;p<2;p++) {
            const u8 *src=ref+at+p*plane+(u16)cy*stride+(cx>>3);
            const u8 *dst=p?rt_dark:rt_light;
            u8 sh=(u8)(cx&7);
            for(row=0;row<100;row++,src+=stride,dst+=RT_PBYTES)for(k=0;k<20;k++) {
                u8 expected=sh?(u8)(src[k]<<sh|src[k+1]>>(8-sh)):src[k];
                if(dst[k]!=expected)failures=1;
                digest=(digest<<1|digest>>31)^dst[k];
            }
        }
        checks++;
    }
    // Animation reuse: the eight visible poses must be the former exact bytes.
    for(x=0;x<8;x++) {
        if(memcmp(idle_sprites+(u16)part(1)[x]*156,ref+b16(ref+6)+(u16)x*156,156))failures++;
        checks++;
    }
    decor_trial();
}
#ifndef __m68k__
int main(void){sw_init(0);trial();printf("storage %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);release_map();return failures!=0;}
#else
void _main(void){storage_runtime_main();trial();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
