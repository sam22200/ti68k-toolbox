// The battle program fftbat (battle.c): returns the continuation flag (the chapter handed over)
// after the grayscale runtime has restored AMS.
#define USE_TI89
#define MIN_AMS 200
#define RETURN_VALUE
#define RT_GRAY_COPY_PRESENT           // reviewed fixed plane addresses avoid TiEmu frame reversal
#include <tigcclib.h>
#undef _main
#define _main battle_runtime_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#define FFT_CAMPAIGN_FLOW
#define FFT_PRODUCTION_CORE   // test/tool-only engine entries are left out (resident budget)
#include "battle.c"
void _main(void) { battle_runtime_main(); push_shortint(battle_state?st.open_handoff:0); }
