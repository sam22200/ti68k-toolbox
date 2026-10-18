// PC-only review of the actual game with identical keys/actors in three scenery styles.
#include <stdio.h>
#include <stdlib.h>
#include "../fft.c"
#include "../../../runtime/platform-sw/rt_sw.h"
int main(int argc,char **argv)
{
    Unit units[NUNIT]; u32 rng; u16 seed; unsigned frame;
    u8 *original; char path[1024];
    if (argc != 3) return 2;
    sw_init(61); if (!preview_iso_bank) return 3;
    preview_iso_set((u8)atoi(argv[1])); st.mode = M_BROWSE;
    st.cx = 5; st.cz = 6; cam_target(&st.camx,&st.camy);
    memcpy(units,st.unit,sizeof units); rng = rng32; seed = rt_seed;
    original = malloc(VIEW_BYTES); if (!original) return 4;
    puts("frame,rot,cursor_x,cursor_z,camera_x,camera_y,rng");
    for (frame = 0; frame < 144; frame++) {
        u32 keys = 0;
        if (frame == 12) keys = K_RIGHT;
        if (frame == 24) keys = K_DOWN;
        if (frame == 40 || frame == 64 || frame == 88 || frame == 112) keys = K_F5;
        if (!sw_step(keys)) return 5;
        if (memcmp(units,st.unit,sizeof units) || rng32 != rng || rt_seed != seed || st.mode != M_BROWSE) return 6;
        if (!st.turn) {
            zx0_unpack(views[st.rot],original);
            if (memcmp(original+2*SC_PLANE,vbuf+2*SC_PLANE,4*SC_PLANE)) return 7;
        }
        snprintf(path,sizeof path,"%s/%03u.png",argv[2],frame);
        if (sw_write_png(path,1)) return 8;
        printf("%u,%u,%u,%u,%d,%d,%lu\n",frame,st.rot,st.cx,st.cz,st.camx,st.camy,(unsigned long)rng32);
    }
    free(original);
    // The preview toggle must change scenery alone, and force a clean view rebuild.
    preview_iso_set(2); sw_step(0); sw_step(K_DIGIT(8));
    if (preview_iso_style || memcmp(units,st.unit,sizeof units) || rng32 != rng) return 9;
    return st.rot || st.turn;
}
