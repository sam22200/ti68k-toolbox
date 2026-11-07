// The management menus as a program: fftmng() edits the campaign's party, returns 1.
#define USE_TI89
#define MIN_AMS 200
#define RETURN_VALUE
#include <tigcclib.h>
#undef _main
#define _main manage_runtime_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#include "manage.c"
void _main(void) { manage_runtime_main(); push_shortint(1); }
