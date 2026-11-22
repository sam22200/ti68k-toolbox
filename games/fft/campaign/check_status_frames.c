// The battle's Status screen draws as little as it can, and the picture never changes for
// it. The screen is full, so once both hidden planes carry a finished page the frame has
// nothing left to do (GrayDBufToggleSync alternates two of them); the skills page keeps
// moving only its glove, so it restores the band under the glove instead of the screen.
//
// This walks the three tabs for 120 frames each, moving the cursor over the rows that have
// one, and hashes every plane of every frame. The hash must match a build that redraws
// everything, and the full-screen copies must stay far below the number of frames.
unsigned long blits, bands;
#define STATUS_BLIT() (blits++)
#define STATUS_BAND() (bands++)
#include "gariland.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <stdio.h>

static unsigned long walk(void)
{
    unsigned long h = 5381;
    u16 i, f;
    for (i = 0; i < 3; i++) {                  // Eqp., Abil., Status
        u8 rows = i == 0 ? 5 : i == 1 ? status_ability_count() : 0;
        st.mode = M_STATUS; st.back = M_MENU; st.page = (u8)i; st.sel = U_RAMZA;
        st.status_info = 0; st.status_col = (u8)(i == 1); st.status_row = 0;
        if (W) W->status_i = 0;
        for (f = 0; f < 120; f++) {
            const unsigned char *p;
            unsigned n;
            if (rows && f && !(f % 17)) st.status_row = (u8)((st.status_row + 1) % rows);
            game_render(); rt_frame++; st.tick++;
            p = (const unsigned char *)rt_light;
            for (n = 0; n < RT_PSIZE; n++) h = h * 33 + p[n];
            p = (const unsigned char *)rt_dark;
            for (n = 0; n < RT_PSIZE; n++) h = h * 33 + p[n];
        }
    }
    return h & 0xFFFFFFFFUL;
}

int main(void)
{
    unsigned long h;
    u16 i;
    unsigned failures = 0;
    sw_init(0); game_init(); game_scenario(26);
    for (i = 0; i < 400 && (st.open_stage || st.open_fade); i++) sw_step(0);
    h = walk();
    printf("Status frames: 360 frames hash %08lX, %lu full-screen copies, %lu glove bands\n",
           h, blits, bands);
    if (blits > 60) { printf("  FAIL: %lu full-screen copies for 360 frames\n", blits); failures++; }
    if (!bands) { printf("  FAIL: the glove band never ran\n"); failures++; }
    if (h != STATUS_FRAMES_HASH) {
        printf("  FAIL: the pictures changed (expected %08lX)\n", (unsigned long)STATUS_FRAMES_HASH);
        failures++;
    }
    return failures != 0;
}
