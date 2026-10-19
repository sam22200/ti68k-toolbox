// A content variant, deliberately sharing every tactical rule and renderer.
#define FFT_OPENING
#define FFT_SOURCE_CAMERA
#include "../fft.c"
#ifndef __m68k__
u8 chapter_completed(void) { return st.open_handoff; }
#endif
