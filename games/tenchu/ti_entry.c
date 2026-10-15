/* Release the one render workspace after both interactive and cycle runs.
 * Runtime's entry point is renamed by the game's build, runtime is unchanged. */
#ifdef __m68k__
#define USE_TI89
#include <tigcclib.h>
#undef __main
#include "engine.h"
void te_runtime_main(void);
void _main(void) {te_runtime_main();te_finish();}
#endif
