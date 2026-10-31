// Original-instruction expectations for the portable intermediate jump kernel.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main jump_unused_main
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
static u16 word(const u8 *p){return (u16)((u16)p[0]<<8|p[1]);}
static void check(u16 value,u16 expected)
{
    checks++;digest=(digest<<1|digest>>31)^value;
    if(value!=expected){failures++;
#ifndef __m68k__
        if(failures<8)printf("case%lu value%u expected%u\n",(unsigned long)checks,value,expected);
#endif
    }
}
static void trials(void)
{
    u8 bank;
    for(bank=0;bank<16;bank++) {
        char name[6]="fjq00";const u8 *cases;u16 bytes,count,i;
        name[3]=(char)('0'+bank/10);name[4]=(char)('0'+bank%10);
        cases=rt_file(name,&bytes);if(!cases)break;
        check(bytes>=6,1);count=word(cases+4);
        check(cases[0]=='F'&&cases[1]=='J'&&cases[2]=='O'&&cases[3]=='1',1);
        check(bytes==6+14*count||bytes==12+14*count,1);cases+=6;
#ifdef __m68k__
        BENCH_BEGIN(4);
#endif
        for(i=0;i<count;i++,cases+=14)
            check(surface_jump_clear(cases,cases[9],(u8)word(cases+10),cases[8]),cases[12]);
#ifdef __m68k__
        BENCH_END(4);
#endif
    }
    check(bank>0,1);
}
void game_init(void){}
void game_scenario(u16 n){(void)n;}
u8 game_update(void){return 0;}
void game_render(void){}
#ifndef __m68k__
int main(void){trials();printf("jump %08lX failures %u checks %lu\n",(unsigned long)digest,failures,(unsigned long)checks);return failures!=0;}
#else
void _main(void){BENCH_NAME(4,"Source jump queries");trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
