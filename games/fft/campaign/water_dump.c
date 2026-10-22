// Gariland's water glints exactly as the engine computes them (fft.c make_glints, per view),
// for its descriptor (tools/chapter_desc.py): run in campaign/data, writes water.bin in the
// layout the chapters read (u16 offset per view, then per phase u16 count, x/2 and y bytes).
#define FFT_WATER_DUMP
#include "gariland.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <stdio.h>
int main(void)
{
    static u8 out[8 + 4 * 4 * (2 + 2 * GLINTS)];
    u16 at = 8; u8 rot, ph, i; FILE *f;
    sw_init(61);
    if (!W) return 1;
    for (rot = 0; rot < 4; rot++) {
        out[2 * rot] = (u8)(at >> 8); out[2 * rot + 1] = (u8)at;
        unpack_view(rot); make_glints(rot);
        for (ph = 0; ph < 4; ph++) {
            out[at++] = 0; out[at++] = W->glints[ph];
            for (i = 0; i < W->glints[ph]; i++) {
                out[at++] = (u8)(W->glint[ph][i].x >> 1); out[at++] = (u8)W->glint[ph][i].y;
            }
        }
    }
    f = fopen("water.bin", "wb");
    if (!f || fwrite(out, 1, at, f) != at) return 2;
    fclose(f);
    printf("water.bin: %u bytes\n", at);
    return 0;
}
