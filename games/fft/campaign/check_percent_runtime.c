// Separate native carrier for impacts/AI/presentation/bootstrap; same production data.
#define PERCENT_RUNTIME_CHECK
#include "check_percent.c"
static void percent_runtime_trials(void){production();eligibility_ai();reversal_and_sleep();presentation_and_boot();}
#ifndef __m68k__
int main(void){sw_init(61);percent_runtime_trials();printf("percent_runtime %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);return failures!=0;}
#else
void _main(void){reaction_runtime_main();percent_runtime_trials();BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);}
#endif
