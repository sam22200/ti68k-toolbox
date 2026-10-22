// The battle program fftbat (battle.c): returns the continuation flag (the chapter handed over)
// after the grayscale runtime has restored AMS.
#define USE_TI89
#define MIN_AMS 200
#define RETURN_VALUE
#include <tigcclib.h>
#undef _main
#define _main battle_runtime_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#include "battle.c"
void _main(void) { battle_runtime_main(); push_shortint(st.open_handoff); }
