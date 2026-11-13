// Actual compiler layouts, not host-ABI estimates. No game/hardware init.
#define RT_DRAW_ROWS 100               // production native_battle.c's fixed-address planes
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
#include "data/modules.h"

static unsigned long sizes[] = {
    UNIT_CAPACITY, sizeof(Unit), sizeof(State), sizeof(Work), sizeof(Cover), sizeof(Feet),
    sizeof(SaveUnit), sizeof(Save), MAP_NMAX, SC_HMAX,
    (unsigned long)FFT_CORE_DEPTH_MAX,
    2UL * SC_BYTES * SC_HMAX + sizeof(Work),
    AC_N * (UNIT_FB + 4UL),
    // Additional storage per unit with CURRENT fields/algorithms only.
    sizeof(Unit) + sizeof(((State *)0)->pending[0]) + 2 + sizeof(((State *)0)->affinity[0]),
    sizeof(Cover) + sizeof(Feet) + 2 + 2 + 3,
    2UL * RT_PSIZE,
    BATTLE_STORAGE_BYTES,
    NAV_BYTES(1), NAV_BYTES(MAP_NMAX), NAV_SINGLE_BYTES(1),
    NAV_ENVELOPE, MOD_ZONE, FFT_CORE_COMPANION_MAX, 2UL*RT_DRAW_BYTES
};
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
