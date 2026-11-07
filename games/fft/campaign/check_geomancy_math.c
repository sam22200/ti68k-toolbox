// Transport split only: same assertions as check_geomancy.c, within the TI variable ceiling.
#define FFT_CHECK_NATIVE_QUIET // final failure count/digest retained; PC keeps source-line diagnostics
#define FFT_CHECK_NO_ZONES // math carrier checks values; runtime carrier retains timings
#define GEOMANCY_MATH_ONLY
#ifndef GEOMANCY_LABEL
#define GEOMANCY_LABEL "geomancy_math"
#endif
#include "check_geomancy.c"
