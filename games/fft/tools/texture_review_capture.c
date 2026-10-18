// PC-only diagnostic: exact flat coverage, transformed vertices and cached textured replay.
#include <stdio.h>
#include <stdlib.h>
#include "../fft.c"
#include "../../../runtime/platform-sw/rt_sw.h"

static int save(const char *dir, const char *name, const void *data, size_t bytes)
{
    char path[1024]; FILE *file; size_t written;
    snprintf(path, sizeof path, "%s/%s", dir, name);
    file = fopen(path, "wb"); if (!file) return 0;
    written = fwrite(data, 1, bytes, file); fclose(file);
    return written == bytes;
}

int main(int argc, char **argv)
{
    unsigned view, n, i; u8 side;
    char path[1024];
    if (argc < 3 || argc > 4) return 2;
    view = (unsigned)atoi(argv[1]); if (view > 3) return 2;
    sw_init(62);
    if (view >= 2) { st.pdist = 80; st.ppitch = 6; }
    if (view & 1) st.pyaw ^= 32;
    if (!sw_step(0)) return 3;
    side = st.pyaw != st.pbase;
    if (!save(argv[2], "edges.bin", review_edges, sizeof review_edges)
        || !save(argv[2], "depth.bin", PDEPTH, 16000)
        || !save(argv[2], "background.bin", scene_l+side*6000, 6000)) return 4;
    // Export portable big-endian labels, independent of host word layout.
    { u8 labels[32000];
      for (i = 0; i < 16000; i++) { labels[i*2] = review_faces[i] >> 8; labels[i*2+1] = review_faces[i]; }
      if (!save(argv[2], "faces.bin", labels, sizeof labels)) return 4; }
    printf("{\"vertices\":[");
    for (i = 0; i < RD16(pbank, 0); i++) {
        const u8 *src = pbank+2120+i*6;
        PVertex v = ptransform((s16)RD16(src, 0), (s16)RD16(src, 1), (s16)RD16(src, 2));
        printf("%s[%d,%d,%d]", i ? "," : "", v.x, v.y, v.z);
    }
    printf("],\"checksums\":[");
    if (argc == 4) {
        u8 background[6000]; FILE *file = fopen(argv[3], "rb");
        if (!file) return 4;
        if (fread(background, 1, sizeof background, file) != sizeof background) return 4;
        fclose(file); memcpy(scene_l+side*6000, background, sizeof background);
        game_render();
    }
    for (n = 0; n < 32; n++) {
        if (n && !sw_step(0)) return 3;
        snprintf(path, sizeof path, "%s/%03u.png", argv[2], n);
        if (sw_write_png(path, 1)) return 4;
        printf("%s%u", n ? "," : "", sw_checksum());
    }
    puts("]}");
    return 0;
}
