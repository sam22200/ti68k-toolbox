/* Pure cold-reference boundaries, including the actual big-endian 68000. */
#ifdef __m68k__
#define USE_TI89
#include <tigcclib.h>
#include "../../../tools/m68kbench/bench.h"
#undef off
#else
#include <stdio.h>
#include <string.h>
#endif
#include "../../../runtime/core/rt.h"
static u8 ref[22],other[16],payload[38];
static u16 ref_n,other_n,payload_n,failures,checks;
static u32 digest=17;
static u8 same(const char *a,const char *b)
{
    while(*a&&*a==*b){a++;b++;}
    return *a==*b;
}
const void *rt_file(const char *name,u16 *size)
{
    const u8 *p=0;u16 n=0;
    if(same(name,"alias")){p=ref;n=ref_n;}
    if(same(name,"bank")){p=payload;n=payload_n;}
    if(same(name,"other")){p=other;n=other_n;}
    if(size)*size=n;
    return p;
}
#include "../resource_file.h"
static void check(u16 ok)
{checks++;digest=(digest<<3|digest>>29)^ok;if(!ok)failures++;}
static void reset(void)
{
    u16 i;memset(ref,0,sizeof ref);memcpy(ref,"FRF1bank",8);
    ref[13]=2;ref[15]=12;ref_n=16;payload_n=32;other_n=16;
    for(i=0;i<32;i++)payload[i]=(u8)(i+1);
}
static void trials(void)
{
    const u8 *p;u16 n,i;
    reset();p=resource_file("alias",&n);check(p==payload+2);check(n==12);
    check(resource_file("alias",0)==payload+2);
    check(resource_file("bank",&n)==payload&&n==32);
    n=99;check(!resource_file("missing",&n)&&!n);
    memcpy(ref+16,"\0dat\0\370",6);ref_n=22;
    memcpy(payload+32,"\0dat\0\370",6);payload_n=38;
    check(resource_file("alias",&n)==payload+2&&n==12);
    check(resource_file("bank",&n)==payload&&n==32);
    for(i=4;i<16;i++){reset();ref_n=i;check(!resource_file("alias",&n)&&!n);}
    reset();ref_n=17;check(!resource_file("alias",&n)&&!n);
    reset();ref[13]=1;check(!resource_file("alias",&n)&&!n);
    reset();ref[13]=34;check(!resource_file("alias",&n)&&!n);
    reset();ref[15]=31;check(!resource_file("alias",&n)&&!n);
    reset();ref[15]=0;check(!resource_file("alias",&n)&&!n);
    reset();ref[13]=32;ref[15]=1;check(!resource_file("alias",&n)&&!n);
    reset();ref[13]=0;ref[15]=32;check(resource_file("alias",&n)==payload&&n==32);
    reset();memcpy(ref+4,"other\0\0\0",8);memcpy(other,ref,16);
    check(!resource_file("alias",&n)&&!n); /* cycle/nesting */
    reset();memcpy(ref+4,"missing\0",8);check(!resource_file("alias",&n)&&!n);
    reset();ref[4]=0;check(!resource_file("alias",&n)&&!n);
    reset();ref[4]='.';check(!resource_file("alias",&n)&&!n);
    reset();ref[10]='x';check(!resource_file("alias",&n)&&!n);
    reset();ref[6]='/';check(!resource_file("alias",&n)&&!n);
    reset();memcpy(payload,"FRF1",4);check(!resource_file("alias",&n)&&!n);
}
#ifdef __m68k__
void _main(void){trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#else
int main(void){trials();printf("resources %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#endif
