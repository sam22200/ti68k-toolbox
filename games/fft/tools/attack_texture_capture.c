// Offline diagnostic: native winning faces, exact camera and clean scenery for a shot.
#include <stdio.h>
#include <stdlib.h>
#include "../fft.c"
#include "../../../runtime/platform-sw/rt_sw.h"
static int save(const char *dir, const char *name, const void *data, unsigned bytes)
{
    char path[1024]; FILE *file; unsigned n;
    snprintf(path, sizeof path, "%s/%s", dir, name); file = fopen(path, "wb");
    if (!file) return 0;
    n = (unsigned)fwrite(data, 1, bytes, file); fclose(file); return n == bytes;
}
int main(int argc, char **argv)
{
    unsigned i; u8 labels[32000];
    if (argc != 3) return 2;
    sw_init((u16)atoi(argv[1])); cimages = RT_NULL;
    st.acted = 1;
    if (atoi(argv[1]) == 66) skill(U_RAMZA, 1, 1); else attack(U_RAMZA, 1);
    cinema_begin(); st.cphase = 2; st.cframe = 0; pvalid = 0;
    game_render();
    for (i = 0; i < 16000; i++) { labels[2*i] = review_faces[i] >> 8; labels[2*i+1] = review_faces[i]; }
    if (!save(argv[2], "faces.bin", labels, sizeof labels)
        || !save(argv[2], "edges.bin", review_edges, sizeof review_edges)
        || !save(argv[2], "background.bin", scene_l, 6000)) return 3;
    printf("{\"camera\":[%u,%u,%u,%u,%u,%u,%u,%u,%u,0,0,0],\"vertices\":[", st.cax,st.caz,st.cah,st.cyaw,
           st.cdist,st.ceye,st.cpitch,(u8)st.cside,st.psolid);
    for (i = 0; i < RD16(pbank, 0); i++) {
        const u8 *src = pbank+2120+i*6;
        PVertex v = ptransform((s16)RD16(src, 0), (s16)RD16(src, 1), (s16)RD16(src, 2));
        printf("%s[%d,%d,%d]", i ? "," : "", v.x,v.y,v.z);
    }
    puts("]}"); return 0;
}
