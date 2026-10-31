#define USE_TI89
#define MIN_AMS 200
#define RETURN_VALUE
#include <tigcclib.h>
#undef _main
#define _main title_runtime_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#include "title.c"
void _main(void)
{
    u8 result;title_runtime_main();result=title_finish();
    if(sv_data){save_write();sv_data=0;}
    if(result)result=title_verified();title_cleanup();push_shortint(result);
}
