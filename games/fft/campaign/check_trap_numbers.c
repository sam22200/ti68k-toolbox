// Independent source arithmetic/serialized bitmap check; no renderer or overlays.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#include "../../../tools/m68kbench/bench.h"
#else
#include <stdio.h>
#endif
#include "../../../runtime/core/rt.h"
#include <string.h>
#include "../raw_stats.h"
#include "save_types.h"
#include "x/tile-traps/oracle/fixture.h"
static u16 checks,failures;static u32 digest=17;
static void equal(u32 a,u32 b)
{checks++;digest=(digest<<1|digest>>31)^a;failures+=a!=b;}
static void trials(void)
{
    Save s;u8 i,j;u32 expected;
    memset(&s,0,sizeof s);
    equal(sizeof s,3204);equal(sizeof(SaveUnit),176);
    for(i=0;i<sizeof trap_growth_oracle/sizeof *trap_growth_oracle;i++){
        expected=trap_growth_oracle[i].after;
        equal(core_raw_level(trap_growth_oracle[i].before,trap_growth_oracle[i].growth,
                             trap_growth_oracle[i].level,trap_growth_oracle[i].down),expected);
    }
    // Roster identity spans both serialized bytes; changing one slot preserves all others.
    for(i=0;i<PARTY_MAX;i++){
        save_raw_set(&s,i,1);
        for(j=0;j<PARTY_MAX;j++)equal(save_raw_current(&s,j),j<=i);
    }
    for(i=0;i<PARTY_MAX;i++){
        save_raw_set(&s,i,0);
        for(j=0;j<PARTY_MAX;j++)equal(save_raw_current(&s,j),j>i);
    }
    save_raw_set(&s,255,1);equal(s.raw_current[0]|s.raw_current[1],0);equal(save_raw_current(&s,255),0);
}
#ifndef __m68k__
int main(void){trials();printf("trap-numbers %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
