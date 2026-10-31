#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main deploy_runtime_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#endif
#include "deploy.c"
#ifndef __m68k__
#include "../../../runtime/platform-sw/rt_sw.h"
#include <stdio.h>
#endif
static u32 state_hash(void)
{
    const u8 *p=(const u8 *)&save;u16 i;u32 h=17;
    for(i=0;i<sizeof save;i++)h=(h<<1|h>>31)^p[i];
    p=(const u8 *)&dp;for(i=0;i<sizeof dp;i++)h=(h<<1|h>>31)^p[i];return h;
}
#ifndef __m68k__
int main(int argc,char **argv)
{
    SwScript script;u16 f,limit;
    if(argc!=5)return 2;limit=(u16)atoi(argv[2]);if(sw_load_script(&script,argv[3]))return 3;
    sw_init((u16)atoi(argv[1]));for(f=0;f<limit;f++)if(!sw_step(sw_script_keys(&script,rt_frame)))break;
    printf("executed %u checksum %04X state %08lX\n",rt_frame,sw_checksum(),(unsigned long)state_hash());
    return sw_write_png(argv[4],1);
}
#else
void _main(void){deploy_runtime_main();BENCH_VALUE(0x44504c31UL);BENCH_VALUE(state_hash());}
#endif
