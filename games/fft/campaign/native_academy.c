#define USE_TI89
#define MIN_AMS 200
#define RETURN_VALUE
#include <tigcclib.h>
#undef _main
#define _main academy_runtime_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#include "academy.c"
void _main(void){academy_runtime_main();if(cache){free(cache);cache=0;}push_shortint(academy.done);}
