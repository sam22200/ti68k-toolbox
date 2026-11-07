// These fixtures use --frames0 and time their own zones. Omit only the
// unused runtime loop/names; retain every scripted frame, marker and pixel hash.
#ifdef RT_CYCLES
#include "../../../tools/m68kbench/bench.h"
#undef BENCH_NAME
#define BENCH_NAME(id,s) ((void)0)
#undef BENCH_FRAMES
#define BENCH_FRAMES 0
#endif
// Bounded native carrier for global preview and cancel to spell selection.
#define MATH_UI_PREVIEW_ONLY
#include "check_math_ui.c"
