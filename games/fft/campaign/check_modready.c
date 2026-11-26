// Which step of mod_ready() fails, reported one value at a time. The TI legs of
// three checks die because the module images are never installed, and
// mod_ready() is where that happens: it wants the four banks in the VAT, the
// execution zone from the heap, and module_four_valid() to accept the set.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef _main
#define _main modready_runtime_main
#define game_render modready_no_render
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef game_render
void modready_no_render(void){}
#undef _main
#define _main __main
#undef off
#else
#include <stdio.h>
#endif
#include "../../../runtime/platform-sw/rt_sw.h"
#include "gariland.c"
#ifdef __m68k__
#include "../../../tools/m68kbench/bench.h"
static void steps(void)
{
    u16 a=0,b=0,c=0,d=0;
    const u8 *pa=rt_file("ffbm",&a),*pb=rt_file("ffbn",&b),
             *pc=rt_file("ffbo",&c),*pd=rt_file("ffbp",&d);
    BENCH_VALUE(W!=RT_NULL);
    BENCH_VALUE(pa!=RT_NULL);BENCH_VALUE(a);
    BENCH_VALUE(pb!=RT_NULL);BENCH_VALUE(pc!=RT_NULL);BENCH_VALUE(pd!=RT_NULL);
    BENCH_VALUE(module_four_valid(pa,a,pb,b,pc,c,pd,d,MOD_BANK0_COUNT,MOD_BANK1_END,
                                  MOD_BANK2_END,MOD_COUNT,sizeof(ModCtx),MOD_SIG,MOD_ZONE,
                                  MOD_COMPANION));
    BENCH_VALUE(mod_ready());
    BENCH_VALUE(mod_images[0]!=RT_NULL);
    BENCH_VALUE(sizeof(ModCtx));
}
#endif
#ifndef __m68k__
int main(void){printf("mod_ready probe: TI only\n");return 0;}
#else
void _main(void){modready_runtime_main();steps();}
#endif
