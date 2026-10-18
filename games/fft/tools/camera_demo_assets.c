// Prepare only the two fixed inspection backgrounds; actors remain runtime draws.
#include <stdio.h>
#include <stdlib.h>
#include "../fft.c"
#include "../../../runtime/platform-sw/rt_sw.h"
int main(int argc, char **argv)
{
    FILE *f; unsigned wide;
    u8 header[8] = {0x46,0x4c,0,1,0,0,0,0};
    if (argc != 2) return 2;
    sw_init(65);
    if (!preview_ready || !W) return 3;
    f = fopen(argv[1], "wb"); if (!f) return 4;
    header[4] = st.unit[U_RAMZA].x; header[5] = st.unit[U_RAMZA].z;
    perspective_enter(1); header[6] = st.pyaw; header[7] = st.psolid;
    if (fwrite(header, 1, sizeof header, f) != sizeof header) return 5;
    for (wide = 0; wide < 2; wide++) {
        st.pdist = wide ? 80 : 40; st.ppitch = wide ? 6 : 1;
        st.ptrans = 0; pvalid = 0; game_render();
        if (fwrite(scene_l, 1, 6000, f) != 6000) return 6;
    }
    fclose(f); return 0;
}
