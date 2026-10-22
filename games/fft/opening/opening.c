// The battle program (campaign/ENGINE.md M4), Orbonne by default (its doors: 0..5).
#define FFT_BATTLE
#define FFT_DEFAULT_CHAPTER 0
#define FFT_SOURCE_CAMERA
#include "../fft.c"
#ifndef __m68k__
u8 chapter_completed(void) { return st.open_handoff; }
#endif
