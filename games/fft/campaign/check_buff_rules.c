// Existing buff assertions, split only for the native test-variable size limit.
#define BUFF_CAPACITY_CHECK
#include "check_buffs.c"
static void rule_trials(void){original_profiles();filter_equivalence();producers();eligibility_and_ai();reflected_status();}
#ifndef __m68k__
int main(void){sw_init(61);rule_trials();printf("buff_rules %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();rule_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
