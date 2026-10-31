// Unchanged cached-menu assertions in their own bounded native carrier.
#define BUFF_CAPACITY_CHECK
#include "check_buffs.c"
#ifndef __m68k__
int main(void){sw_init(61);same_windows();printf("buff_windows %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();same_windows();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
