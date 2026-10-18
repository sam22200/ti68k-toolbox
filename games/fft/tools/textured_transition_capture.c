// Full PC game: inspect both entry presets, zoom both ways, leave, then F5/F1 turns.
#include <stdio.h>
#include <stdlib.h>
#include "../fft.c"
#include "../../../runtime/platform-sw/rt_sw.h"
int main(int argc, char **argv)
{
    unsigned frame; Unit units[NUNIT]; u32 seed; u16 runtime_seed;
    char path[1024]; u8 wide;
    if (argc != 3) return 2;
    wide = (u8)atoi(argv[1]); sw_init(61);
    if (!preview_ready) return 3;
    memcpy(units,st.unit,sizeof units); seed = rng32; runtime_seed = rt_seed;
    puts("frame,perspective,transition,zoom,turn,rot,sampled");
    for (frame = 0; frame < 204; frame++) {
        u32 keys = 0;
        if (frame == 8) keys = K_C | (wide ? K_MODE : 0);
        if (frame == 40 || frame == 72 || frame == 104) keys = K_MODE;
        if (frame == 136) keys = K_B;
        if (frame == 168) keys = K_F5;
        if (frame == 184) keys = K_F1;
        if (!sw_step(keys)) return 4;
        if (memcmp(units,st.unit,sizeof units) || rng32 != seed || rt_seed != runtime_seed || st.mode != M_MENU) return 5;
        if ((st.perspective || st.turn) && preview_pixels < 1000) return 6;
        snprintf(path,sizeof path,"%s/%03u.png",argv[2],frame);
        if (sw_write_png(path,1)) return 7;
        printf("%u,%u,%u,%u,%d,%u,%lu\n",frame,st.perspective,st.ptrans,st.pdist,st.turn,st.rot,(unsigned long)preview_pixels);
    }
    return st.perspective || st.ptrans || st.turn || st.rot;
}
