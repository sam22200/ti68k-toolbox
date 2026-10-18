// PC-only actual key replay: direct close/wide entry, both zoom paths, remembered re-entry.
#include <stdio.h>
#include <stdlib.h>
#include "../fft.c"
#include "../../../runtime/platform-sw/rt_sw.h"

int main(int argc, char **argv)
{
    unsigned n, wide, i; char path[1024]; u8 backgrounds[2][6000];
    State before; u32 seed;
    if (argc != 5) return 2;
    wide = (unsigned)atoi(argv[4]); if (wide > 1) return 2;
    for (i = 0; i < 2; i++) {
        FILE *file = fopen(argv[2+i], "rb"); if (!file) return 3;
        if (fread(backgrounds[i], 1, 6000, file) != 6000) return 3;
        fclose(file);
    }
    sw_init(61); for (n = 0; n < 24; n++) sw_step(0);
    before = st; seed = rt_seed;
    puts("frame,perspective,transition,step,distance,remembered,checksum");
    for (n = 0; n < 272; n++) {
        u32 keys = n == 16 ? K_C | (wide ? K_MODE : 0) : n == 64 || n == 112 ? K_MODE
            : n == 160 || n == 240 ? K_B : n == 192 ? K_C : 0;
        if (!sw_step(keys)) return 4;
        if (st.perspective && !st.ptrans) {
            memcpy(scene_l, backgrounds[st.pdist == 80], 6000); game_render();
        }
        if (rt_seed != seed || memcmp(before.unit, st.unit, sizeof st.unit)
            || st.act != before.act || st.mode != before.mode || st.ai != before.ai) return 5;
        if ((n == 48 || n == 144 || n == 224) && st.pdist != (wide ? 80 : 40)) return 8;
        if (n == 96 && st.pdist != (wide ? 40 : 80)) return 9;
        snprintf(path, sizeof path, "%s/%03u.png", argv[1], n);
        if (sw_write_png(path, 1)) return 6;
        printf("%u,%u,%u,%u,%u,%u,%04X\n", n, st.perspective, st.ptrans, st.ptrans_t, st.pdist, st.pzoom, sw_checksum());
    }
    if (st.perspective || st.ptrans || st.pzoom != wide) return 7;
    return 0;
}
