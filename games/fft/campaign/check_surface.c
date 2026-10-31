// Compare portable geometry with expectations executed by original disc MIPS.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main surface_unused_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#endif
#include "../surface_geometry.h"
#include <string.h>
#ifdef __m68k__
#include "../../../tools/m68kbench/bench.h"
#endif
static u32 digest=17,checks;static u16 failures;
static u16 word(const u8 *p){return (u16)((u16)p[0]<<8|p[1]);}
static void check(u16 value,u16 expected)
{
    checks++;digest=(digest<<1|digest>>31)^value;if(value!=expected){failures++;
#ifndef __m68k__
        if(failures<8)printf("case%lu value%u expected%u\n",(unsigned long)checks,value,expected);
#endif
    }
}
static void trials(void)
{
    u16 bank;u8 descriptor[134];u8 data[24]={ 'F','G','S','1',1,1,0,1, 0,0,0,0, 8,0,0,0 };
    check(surface_simple_lower(data+8,1),0);data[15]=128;
    check(surface_simple_lower(data+8,1),1);data[8]=63;
    check(surface_simple_lower(data+8,1),0);data[8]=0;data[15]=0;
    check(surface_valid(data,16,1,1),1);check(surface_valid(data,22,1,1),1);
    for(bank=0;bank<16;bank++)check(surface_valid(data,bank,1,1),0);
    data[15]=32;check(surface_valid(data,16,1,1),0);data[15]=0;
    check(surface_valid(data,16,2,1),0);check(surface_valid(data,16,0,1),0);
    data[3]='0';check(surface_valid(data,16,1,1),0);data[3]='1';
    memset(descriptor,0,sizeof descriptor);memcpy(descriptor,"FCD3",4);
    memcpy(descriptor+104,data,16);memcpy(descriptor+120,"SGE1",4);
    descriptor[125]=104;descriptor[127]=16;
    check(surface_descriptor(descriptor,128,1,1,104)==descriptor+104,1);
    check(surface_descriptor(descriptor,134,1,1,104)==descriptor+104,1);
    for(bank=0;bank<128;bank++)check(surface_descriptor(descriptor,bank,1,1,104)==0,1);
    descriptor[125]=103;check(surface_descriptor(descriptor,128,1,1,104)==0,1);descriptor[125]=104;
    descriptor[127]=15;check(surface_descriptor(descriptor,128,1,1,104)==0,1);descriptor[127]=16;
    descriptor[119]=32;check(surface_descriptor(descriptor,128,1,1,104)==0,1);

    for(bank=0;;bank++) {
        char name[6]="fgq00";const u8 *cases;u16 bytes,count,i;
        name[3]=(char)('0'+bank/10);name[4]=(char)('0'+bank%10);
        cases=rt_file(name,&bytes);if(!cases)break;
        check(bytes>=6,1);count=word(cases+4);
        check(cases[0]=='F'&&cases[1]=='G'&&cases[2]=='O'&&cases[3]=='1',1);
        check(bytes==6+14*count||bytes==12+14*count,1);
        cases+=6;
#ifdef __m68k__
        BENCH_BEGIN(4);
#endif
        for(i=0;i<count;i++,cases+=14) {
            u8 value=0;
            if(cases[8]==0)value=surface_hit(cases,(s16)word(cases+10));
            else if(cases[8]==1)value=surface_ceiling(cases,cases[9],(u8)word(cases+10));
            else if(cases[8]==2)value=surface_room(cases[0],cases[1],cases[2],cases[3],cases[4]);
            else failures++;
            check(value,cases[12]);
        }
#ifdef __m68k__
        BENCH_END(4);
#endif
    }
    check(bank,3);
}
void game_init(void){}
void game_scenario(u16 n){(void)n;}
u8 game_update(void){return 0;}
void game_render(void){}
#ifndef __m68k__
int main(void){trials();printf("surface %08lX failures %u checks %lu\n",(unsigned long)digest,failures,(unsigned long)checks);return failures!=0;}
#else
void _main(void){BENCH_NAME(4,"Source surface queries");trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
