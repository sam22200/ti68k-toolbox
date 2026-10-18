// Renderer/logic benchmark only. The full PC game keeps the complete isometric UI.
// Omitting that unused UI here keeps the diagnostic under one TI variable's 64 KB limit.
#define game_render original_full_render
#include "../fft.c"
#undef game_render

void game_render(void)
{
    if (st.cphase && st.perspective) cinema_render();
    else if (st.perspective) perspective_render();
    else {
        draw_clear();
        draw_text(2, 30, "Attack camera benchmark", F_SMALL, C_BLACK);
        draw_text(2, 44, "ISO UI omitted here", F_SMALL, C_BLACK);
    }
    W->hud_ok = 1; W->left = 80; W->frz = W->hold = 0;
    FRAME_END();
}

#ifndef __m68k__
#include <stdio.h>
#include <stdlib.h>
#include "../../../runtime/platform-sw/rt_sw.h"
static SwScript script;
int main(int argc, char **argv)
{
    unsigned n, frames;
    if (argc != 4 || sw_load_script(&script, argv[2])) return 2;
    frames = (unsigned)atoi(argv[3]); sw_init((u16)atoi(argv[1]));
    for (n = 0; n < frames; n++) {
        if (!sw_step(sw_script_keys(&script, (u16)n))) return 3;
        printf("%u %04X\n", n, sw_checksum());
    }
    return 0;
}
#endif
