// Validate both executable archive ranges before any image can run.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#include "../../../tools/m68kbench/bench.h"
#else
#include <stdio.h>
#include <string.h>
#endif
#include "../module_bank.h"
static u8 a[96],b[96],c[96];static u16 checks,failures;static u32 digest=17;
static void put(u8 *p,u16 at,u16 value){p[at]=value>>8;p[at+1]=value;}
static void fresh(void)
{
    u16 i;memset(a,0,sizeof a);memset(b,0,sizeof b);
    memcpy(a,"FMB3",4);memcpy(b,"FMB3",4);
    put(a,6,1);put(b,4,1);put(b,6,2);put(a,8,3);put(b,8,3);
    put(a,10,0x1234);put(b,10,0x1234);put(a,12,0x5678);put(b,12,0x5678);
    put(a,14,16);put(b,14,18);put(b,16,36);
    memcpy(a+16,"FMD2",4);put(a,22,10);put(a,24,2);put(a,28,0x1234);
    for(i=18;i<=36;i+=18){memcpy(b+i,"FMD2",4);put(b,i+6,10);put(b,i+8,2);put(b,i+12,0x1234);}
}
static void check(u8 got,u8 expected)
{
    checks++;digest=(digest<<1|digest>>31)^got;if(got!=expected)failures++;
}
static u8 valid(u16 asize,u16 bsize){return module_banks_valid(a,asize,b,bsize,1,3,4,0x1234,10,255);}
static u8 valid_three(u16 csize)
{return module_three_valid(a,34,b,54,c,csize,1,3,4,4,0x1234,10,255);}
static void three_trials(void)
{
    fresh();put(a,8,4);put(b,8,4);memcpy(c,a,34);put(c,4,3);
    check(valid_three(34),1);check(valid_three(40),1);
    check(valid_three(33),0);check(valid_three(41),0);
    check(module_three_valid(a,34,b,54,0,34,1,3,4,4,0x1234,10,255),0);
    c[12]^=1;check(valid_three(34),0);c[12]^=1;
    put(c,4,2);check(valid_three(34),0);put(c,4,3);
    put(c,8,3);check(valid_three(34),0);put(c,8,4);
    put(c,28,0);check(valid_three(34),0);put(c,28,0x1234);
    check(module_three_valid(a,34,b,54,c,34,1,1,4,4,0x1234,10,255),0);
}
static void trials(void)
{
    u16 i;three_trials();fresh();check(valid(34,54),1);check(valid(40,60),1);
    for(i=0;i<34;i++)check(valid(i,54),0);
    for(i=0;i<54;i++)check(valid(34,i),0);
    for(i=35;i<40;i++)check(valid(i,54),0);
    for(i=55;i<60;i++)check(valid(34,i),0);
    check(valid(41,54),0);check(valid(34,61),0);
    check(module_banks_valid(0,34,b,54,1,3,4,0x1234,10,255),0);
    check(module_banks_valid(a,34,0,54,1,3,4,0x1234,10,255),0);
    check(module_banks_valid(b,54,a,34,1,3,4,0x1234,10,255),0);
    check(module_banks_valid(a,34,a,34,1,3,4,0x1234,10,255),0);
    check(module_banks_valid(a,34,b,54,0,3,4,0x1234,8,255),0);
    check(module_banks_valid(a,34,b,54,3,3,4,0x1234,8,255),0);
    check(module_banks_valid(a,34,b,54,1,256,4,0x1234,10,255),0);
    check(module_banks_valid(a,34,b,54,1,3,1,0x1234,10,255),0);
    for(i=0;i<20;i++){
        fresh();
        switch(i){
        case 0:a[3]='2';break;
        case 1:put(a,4,1);break;
        case 2:put(b,4,0);break;
        case 3:put(a,6,2);break;
        case 4:put(b,6,1);break;
        case 5:put(b,8,4);break;
        case 6:put(b,10,1);break;
        case 7:put(b,12,1);break;
        case 8:put(b,16,18);break;
        case 9:put(a,14,17);break;
        case 10:b[39]='0';break;
        case 11:put(b,40,1);break;
        case 12:put(b,40,10);break;
        case 13:put(b,42,11);break;
        case 14:put(b,42,12);break;
        case 15:put(b,44,3);break;
        case 16:put(b,44,8);break;
        case 17:put(b,48,0);break;
        case 18:put(b,14,20);break;
        default:put(a,22,65534);break;
        }
        check(valid(34,54),0);
    }
}
#ifndef __m68k__
int main(void){trials();printf("modules %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
