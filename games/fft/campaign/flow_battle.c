// Active campaign-flow PC/check entry. Historical direct Gariland controls use
// gariland.c without this opt-in; the native supervisor build enables it too.
#define FFT_CAMPAIGN_FLOW
#include "gariland.c"
u8 chapter_completed(void){return st.open_handoff;}
