// PC-only: capture real DIAMOND/SHIFT entry/exit, injecting the reviewed textured cache.
#include <stdio.h>
#include <stdlib.h>
#include "../fft.c"
#include "../../../runtime/platform-sw/rt_sw.h"

int main(int argc, char **argv)
{
    unsigned n; char path[1024]; u8 background[6000]; FILE *file;
    State before;
    u32 seed;
    if (argc != 3) return 2;
    file = fopen(argv[2], "rb"); if (!file) return 3;
    if (fread(background, 1, 6000, file) != 6000) return 3;
    fclose(file);
    sw_init(61);
    for (n = 0; n < 24; n++) sw_step(0); // let the ordinary tactical HUD/camera settle
    before = st; seed = rt_seed;
    puts("frame,perspective,transition,step,checksum");
    for (n = 0; n < 136; n++) {
        u32 keys = n == 16 ? K_C : n == 88 ? K_B : 0;
        if (!sw_step(keys)) return 4;
        if (st.perspective && !st.ptrans) {
            memcpy(scene_l, background, 6000);
            game_render();
        }
        if (rt_seed != seed || memcmp(before.unit, st.unit, sizeof st.unit)
            || st.act != before.act || st.mode != before.mode || st.ai != before.ai) return 5;
        snprintf(path, sizeof path, "%s/%03u.png", argv[1], n);
        if (sw_write_png(path, 1)) return 6;
        printf("%u,%u,%u,%u,%04X\n", n, st.perspective, st.ptrans, st.ptrans_t, sw_checksum());
    }
    if (st.perspective || st.ptrans) return 7;
    return 0;
}
