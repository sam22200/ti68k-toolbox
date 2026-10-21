// The scene player as a packed program: fftscn(k) plays ffs<k>, returns 1 once it is over.
#define USE_TI89
#define MIN_AMS 200
#define RETURN_VALUE
#include <tigcclib.h>
#undef _main
#define _main scene_runtime_main
#include "../../../runtime/platform-ti68k/rt_ti.c"
#undef _main
#define _main __main
#undef off
#include "scene.c"
void _main(void) { scene_runtime_main(); push_shortint(chapter_completed()); }
