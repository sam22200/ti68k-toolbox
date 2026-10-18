// Actual portable game replay: isometric validation, target shot, original combat, return.
#include <stdio.h>
#include <stdlib.h>
#include "../fft.c"
#include "../../../runtime/platform-sw/rt_sw.h"
static SwScript script;
int main(int argc, char **argv)
{
    unsigned n, frames; char path[1024];
    if (argc != 5 || sw_load_script(&script, argv[2])) return 2;
    frames = (unsigned)atoi(argv[3]); sw_init((u16)atoi(argv[1]));
    puts("frame,checksum,mode,phase,motion,action,target_hp,target_x,target_z,attacker_x,attacker_z,anchor_x,anchor_z,yaw,rng,distance,textured,sampled,perspective,transition,zoom");
    for (n = 0; n < frames; n++) {
        if (!sw_step(sw_script_keys(&script, (u16)n))) return 3;
        if (argv[4][0] != '-') {
            snprintf(path, sizeof path, "%s/%03u.png", argv[4], n);
            if (sw_write_png(path, 1)) return 4;
        }
        { unsigned sampled = 0;
#ifdef FFT_TEXTURE_PREVIEW
          sampled = preview_pixels;
#endif
        printf("%u,%04X,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%lu,%u,%u,%u,%u,%u,%u\n", n, sw_checksum(), st.mode,
               st.cphase, st.cframe, st.act_t, st.unit[1].hp, st.unit[1].x, st.unit[1].z,
               st.unit[U_RAMZA].x, st.unit[U_RAMZA].z, st.cax, st.caz, st.cyaw, (unsigned long)rng32,
               st.cdist, st.cphase == 2 ? FW->textured : 0, sampled, st.perspective, st.ptrans, st.pdist); }
    }
    return 0;
}
