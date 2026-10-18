// PC-only source-sprite preview: export the real renderer's depth, poses and backgrounds.
// Including the implementation gives this diagnostic access without a production API.
#include <stdio.h>
#include <stdlib.h>
#include "../fft.c"
#include "../../../runtime/platform-sw/rt_sw.h"

int main(int argc, char **argv)
{
    unsigned view, n, i;
    char file[1024];
    if (argc != 3) return 2;
    view = (unsigned)atoi(argv[1]);
    if (view > 3) return 2;
    sw_init(62);
    if (view >= 2) { st.pdist = 80; st.ppitch = 6; }
    if (view & 1) st.pyaw ^= 32;
    puts("[");
    for (n = 0; n < 32; n++) {
        u8 side, rows[NUNIT];
        u16 before;
        if (!sw_step(0)) return 3;
        side = st.pyaw != st.pbase;
        if (!n) {
            FILE *out;
            snprintf(file, sizeof file, "%s/depth.bin", argv[2]);
            out = fopen(file, "wb");
            if (!out) return 4;
            if (fwrite(PDEPTH, 1, 16000, out) != 16000) return 4;
            fclose(out);
        }
        snprintf(file, sizeof file, "%s/current-%03u.png", argv[2], n);
        if (sw_write_png(file, 1)) return 4;
        before = sw_checksum();
        for (i = 0; i < NUNIT; i++) {
            rows[i] = FW->view[side].rect[i].rows;
            FW->view[side].rect[i].rows = 0;
        }
        game_render();
        snprintf(file, sizeof file, "%s/background-%03u.png", argv[2], n);
        if (sw_write_png(file, 1)) return 4;
        for (i = 0; i < NUNIT; i++) FW->view[side].rect[i].rows = rows[i];
        game_render();
        if (sw_checksum() != before) return 5; // exporting backgrounds must preserve the frame
        printf("%s{\"frame\":%u,\"checksum\":%u,\"actors\":[", n ? "," : "", n, before);
        for (i = 0; i < NUNIT; i++) {
            u8 who = FW->view[side].actors[i];
            const FRect *r = &FW->view[side].rect[who];
            const u8 *g = unit_frame((u8)(4-((st.pyaw+8)>>4)) & 3, who);
            unsigned source = (unsigned)((g-ugfx)/UNIT_FB);
            printf("%s{\"who\":%u,\"gfx\":%u,\"pose\":%u,\"x\":%d,\"y\":%d,"
                   "\"h\":%u,\"q\":%u,\"visible\":%u}", i ? "," : "", who,
                   source/(2*UNIT_FRAMES), source%(2*UNIT_FRAMES),
                   r->x+(r->w >> 1), r->y+(r->h*UNIT_FOOT)/UNIT_SH,
                   r->h, r->q, rows[who] != 0);
        }
        puts("]}");
    }
    puts("]");
    return 0;
}
