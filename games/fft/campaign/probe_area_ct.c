// Actual compiler layouts, not host-ABI estimates. No game/hardware init.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#undef off
#undef _main
#define _main layout_unused_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#include "../../../tools/m68kbench/bench.h"
#else
#include <stdio.h>
#endif
#include "battle.c"
#include "save.h"

typedef char OwnedCastNear[(__builtin_offsetof(Work,cast)+sizeof(CastBatch)<=32768)?1:-1];
static unsigned long sizes[]={__builtin_offsetof(Work,cast),sizeof(CastBatch),
    __builtin_offsetof(Work,pan),sizeof(Work),sizeof(State),sizeof(Save)};
#ifdef __m68k__
void _main(void)
{
    unsigned short i;
    for(i=0;i<sizeof(sizes)/sizeof(sizes[0]);i++)BENCH_VALUE(sizes[i]);
}
#else
int main(void)
{
    unsigned short i;
    for(i=0;i<sizeof(sizes)/sizeof(sizes[0]);i++)printf("%lu\n",sizes[i]);
    return 0;
}
#endif
