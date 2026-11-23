// Bounded real-runtime bridge: load PC slot1, save TI slot2, load legacy slot3.
// Uses the production save reader and platform file I/O; no gameplay rewrite.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#define RT_CYCLES
#define RT_SAVE_ATOMIC
#include <tigcclib.h>
#undef _main
#define _main exchange_unused_runtime_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#endif
#include "save.h"
static Save current,legacy;
static u32 digest(const Save *s)
{
    const u8 *p=(const u8 *)s;u16 i;u32 value=17;
    for(i=0;i<sizeof *s;i++)value=(value<<1|value>>31)^*p++;
    return value;
}
void game_init(void){}
void game_scenario(u16 n){(void)n;}
u8 game_update(void){return 0;}
void game_render(void){}
static u8 exchange(const char *name)
{
    if(!save_load_from(name,&current)||!save_world_valid(&current))return 1;
    if(!save_load_from("ffsave3",&legacy)||!save_world_valid(&legacy))return 2;
    if(memcmp(legacy.magic,SAVE_MAGIC,4))return 3;
    if(!rt_save("ffsave2",&current,sizeof current))return 4;
    return 0;
}
#ifdef __m68k__
void _main(void)
{
    u8 error=exchange("ffsave1");
    if(!error){save_write();if(!rt_save_committed())error=5;}
    BENCH_VALUE(error);BENCH_VALUE(sizeof current);
    BENCH_VALUE(digest(&current));BENCH_VALUE(digest(&legacy));
}
#else
int main(int argc,char **argv)
{
    u8 error=exchange(argc>1?argv[1]:"ffsave1");
    printf("save-transfer %u %u %08lX %08lX\n",error,(unsigned)sizeof current,
           (unsigned long)digest(&current),(unsigned long)digest(&legacy));
    return error;
}
#endif
