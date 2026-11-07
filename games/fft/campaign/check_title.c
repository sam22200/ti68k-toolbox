#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main title_runtime_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#endif
#include "title.c"
#ifndef __m68k__
#include "../../../runtime/platform-sw/rt_sw.h"
#include <stdio.h>
#endif
static u32 title_hash(void)
{
    u16 i;u32 h=17;const u8 *p=(const u8 *)&title;
    for(i=0;i<sizeof title;i++)h=(h<<1|h>>31)^p[i];
    p=(const u8 *)title_slots;for(i=0;i<sizeof title_slots;i++)h=(h<<1|h>>31)^p[i];return h;
}
#ifdef __m68k__
void _main(void)
{
    u8 result;title_runtime_main();result=title_finish();
    if(sv_data){save_write();sv_data=0;}
    if(result)result=title_verified();
    BENCH_VALUE(0x54495431UL);BENCH_VALUE(title_hash());BENCH_VALUE(result);title_cleanup();
}
#else
int main(int argc,char **argv)
{
    SwScript script;u16 f,limit;u8 result;
    if(argc!=5)return 2;limit=(u16)atoi(argv[2]);if(sw_load_script(&script,argv[3]))return 3;
    sw_init((u16)atoi(argv[1]));for(f=0;f<limit;f++)if(!sw_step(sw_script_keys(&script,rt_frame)))break;
    result=title_finish();if(result)result=title_verified();
    printf("executed %u checksum %04X state %08lX result %u\n",rt_frame,sw_checksum(),(unsigned long)title_hash(),result);
    f=sw_write_png(argv[4],1);title_cleanup();return f;
}
#endif
