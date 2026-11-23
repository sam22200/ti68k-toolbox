// The management menus as a program: fftmng() edits the campaign's party, returns 1.
#define USE_TI89
#define MIN_AMS 200
#define RETURN_VALUE
#include <tigcclib.h>
#undef _main
#define _main manage_runtime_main
#define RT_TILOG                       // saves logged (lib/tilog.h, ti-mem log)
#define RT_SAVE_ATOMIC
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#include "manage.c"
void _main(void) { u16 result;manage_runtime_main();result=manage_result();push_shortint((short)(result==2&&!rt_save_committed()?3:result)); }
