/* Cache ownership, eviction and animation-budget boundaries, on both CPUs.
   The decoder stand-in identifies each frame; real artwork is cross-checked
   by the scene and thirteen-actor render replays. */
#ifdef __m68k__
#define USE_TI89
#include <tigcclib.h>
#include "../../../tools/m68kbench/bench.h"
#else
#include <stdio.h>
#include <string.h>
#endif
#include "../../../runtime/core/rt.h"
#define UNIT_FB 208
#define ACTOR_SLOTS 15
#define ACTOR_UNITS 13
#define ACTOR_KEY(logical) (logical)
#define RD16(p,i) ((u16)((u16)(p)[2*(i)]<<8|(p)[2*(i)+1]))
#define ZB(n)
#define ZE(n)
#include "../actors.h"
static struct { ACTOR_FIELDS } work, *W=&work;
static u16 storage[AC_N * (UNIT_FB + 4) / 2];
static u8 banks[2][194];
static u16 decodes,checks,failures;
static u32 digest=17;
static void zx0_unpack(const u8 *source,u8 *dest)
{decodes++;memset(dest,*source,UNIT_FB);}
#ifndef ACTOR_CACHE_SOURCE
#define ACTOR_CACHE_SOURCE "../actors.inc"
#endif
#include ACTOR_CACHE_SOURCE
static void check(u16 ok)
{checks++;digest=(digest<<3|digest>>29)^ok;if(!ok)failures++;}
static void trials(void)
{
    u16 i,j;const u8 *p;
    for(j=0;j<2;j++){
        banks[j][1]=64;
        for(i=0;i<64;i++){
            banks[j][3+2*i]=(u8)i;
            banks[j][130+i]=(u8)(64*j+i);
        }
        abank[j]=banks[j];
    }
    abank[2]=RT_NULL;acache=(u16 (*)[UNIT_FB/2])storage;
    ac_key=(u16 *)acache[AC_N];ac_age=ac_key+AC_N;
    actor_reset();
    // The cache's size is AC_N, not a number: the trial fills it, then walks past its end.
    for(i=0;i<AC_N;i++){W->ac_now++;p=actor_frame(i,0);check(*p==i);}
    check(decodes==AC_N);
    p=actor_frame(AC_N-1,1);check(p==(const u8 *)acache[AC_N-1]);
#ifdef __m68k__
    BENCH_BEGIN(61);
#endif
    for(i=0;i<1000;i++){W->ac_now++;p=actor_frame(AC_N-1,1);}
#ifdef __m68k__
    BENCH_END(61);
#endif
    check(*p==AC_N-1&&decodes==AC_N&&ac_age[AC_N-1]==W->ac_now);
    for(i=AC_N;i<2*AC_N;i++){W->ac_now++;actor_frame(i,0);}
    check(ac_key[W->actor_ent[1]]!=W->actor_key[1]);
    p=actor_frame(AC_N-1,1);check(*p==AC_N-1&&decodes==2*AC_N+1);
    p=actor_frame(2*AC_N-2,1);check(*p==2*AC_N-2&&decodes==2*AC_N+1);
    W->actor_budget=0;
    p=actor_frame(2*AC_N,1);check(*p==2*AC_N-2&&W->actor_key[1]==2*AC_N-2&&decodes==2*AC_N+1);
    W->actor_budget=1;
    p=actor_frame(2*AC_N,1);check(*p==2*AC_N&&W->actor_key[1]==2*AC_N&&decodes==2*AC_N+2);
    actor_reset();W->actor_budget=0;
    p=actor_frame(2*AC_N+1,1);check(*p==2*AC_N+1&&decodes==2*AC_N+3);
    check(W->actor_misses==1);
    // Three banks (ffca 64, ffcb 20, ffcc 30 frames): frames are numbered across them in order,
    // the lookup stops at the last bank present.
    {
        static u8 third[2+2*30+30];
        banks[1][1]=20;
        for(i=0;i<20;i++){banks[1][3+2*i]=(u8)i;banks[1][42+i]=(u8)(100+i);}
        third[1]=30;
        for(i=0;i<30;i++){third[3+2*i]=(u8)i;third[62+i]=(u8)(150+i);}
        abank[2]=third;abank[3]=RT_NULL;actor_reset();
        W->ac_now++;p=actor_frame(63,0);check(*p==63);
        W->ac_now++;p=actor_frame(64,0);check(*p==100);
        W->ac_now++;p=actor_frame(83,0);check(*p==119);
        W->ac_now++;p=actor_frame(84,0);check(*p==150);
        W->ac_now++;p=actor_frame(113,0);check(*p==179);
    }
}
#ifdef __m68k__
void _main(void){trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#else
int main(void){trials();printf("actor cache %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#endif
