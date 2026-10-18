// PC diagnostic: real F1/F5 inputs, original mesh, affine isometric projection and actors.
// Export winning face IDs for offline texture review; no angle-background bank is shipped.
#include <stdio.h>
#include <stdlib.h>
#define game_render fft_render
#include "../fft.c"
#undef game_render
#include "../../../runtime/platform-sw/rt_sw.h"
static PScreen projected[500];
static u8 actor_mask[2000];
static s16 pivot_x, pivot_y, yaw;

static PScreen project(s16 x, s16 y, s16 z)
{
    PScreen p;
    s16 dx = x-(st.cx*28+14), dz = z-(st.cz*28+14);
    s16 across = (s16)((muls16(dx, pc)+muls16(dz, ps)) >> 7);
    s16 forward = (s16)((muls16(dx, ps)-muls16(dz, pc)) >> 7);
    // Same 12x6 tile diamonds and six pixels per height as the settled view.
    p.x = pivot_x+(s16)(muls16(across, 155) >> 8);
    p.y = pivot_y-(y >> 1)-(s16)(muls16(forward, 77) >> 8);
    p.q = 128-(forward >> 2)+(y >> 2);
    if (p.q < 1) p.q = 1;
    if (p.q > 254) p.q = 254;
    return p;
}

void game_render(void)
{
    u16 i, nv, nf;
    s16 u, v;
    const u8 *src;
    if (!st.turn || !pbank) { fft_render(); return; }
    yaw = (56-16*st.rot-st.turn*(st.turn_t+1)*4) & 63;
    ps = psine((u8)yaw); pc = psine((u8)(yaw+16)); plow = 0;
    view_of(st.rot, st.cx, st.cz, &u, &v);
    pivot_x = scene_x(st.rot, u, v)-st.camx;
    pivot_y = scene_y(u, v)+HH-st.camy;
    draw_clear(); memset(PDEPTH, 0, 16000); memset(PCACHE, 0, 2000);
    memset(review_faces, 0, sizeof review_faces); memset(review_edges, 0, sizeof review_edges);
    memset(actor_mask, 0, sizeof actor_mask);
    nv = RD16(pbank, 0); nf = RD16(pbank, 1);
    src = pbank+2120;
    for (i = 0; i < nv; i++, src += 6)
        projected[i] = project((s16)RD16(src,0), (s16)RD16(src,1), (s16)RD16(src,2));
    for (i = 0; i < nf; i++, src += 8) {
        PScreen triangle[3]; u8 j;
        for (j = 0; j < 3; j++) triangle[j] = projected[RD16(src,j)];
        review_face = i+1; praster(triangle, 3, src[6]);
    }
    review_face = 0;
    // Real outlined TI actor frames; the scene's depth clips each opaque pixel.
    for (i = 0; i < NUNIT; i++) {
        const Unit *un = &st.unit[i]; const u8 *g;
        PScreen p; s16 x, y;
        if (un->x >= MAP_W || un->z >= MAP_H) continue;
        p = project(un->x*28+14, map_tiles[un->z*MAP_W+un->x].stand*6, un->z*28+14);
        g = unit_frame((u8)(4-((yaw+8)>>4)) & 3, (u8)i);
        for (y = 0; y < UNIT_SH; y++) {
            u16 l = RD16(g,y), d = RD16(g,UNIT_SH+y), m = RD16(g,2*UNIT_SH+y);
            s16 yy = p.y-UNIT_FOOT+y;
            if ((u16)yy >= 100) continue;
            for (x = 0; x < 16; x++) {
                s16 xx = p.x-8+x; u16 bit = 0x8000U >> x, off;
                if ((u16)xx >= 160 || (m & bit) || p.q+1 < PDEPTH[yy*160+xx]) continue;
                ppixel(xx, yy, ((l & bit) ? 1 : 0)+((d & bit) ? 2 : 0));
                PDEPTH[yy*160+xx] = (u8)p.q;
                off = yy*20+((u16)xx >> 3); actor_mask[off] |= 0x80 >> (xx & 7);
            }
        }
    }
    scene_rot = vbuf_rot = 0xFF; W->bg_ok = 0;
}

static void save(const char *folder, unsigned frame, const char *suffix, const void *data, unsigned bytes)
{
    char path[1024]; FILE *f;
    snprintf(path, sizeof path, "%s/%03u.%s", folder, frame, suffix);
    f = fopen(path,"wb"); if (!f || fwrite(data,1,bytes,f) != bytes) exit(4);
    fclose(f);
}

int main(int argc, char **argv)
{
    unsigned n, i; u8 labels[32000]; Unit original[NUNIT]; u32 rng; u8 rot;
    char path[1024];
    if (argc != 2) return 2;
    sw_init(61); memcpy(original,st.unit,sizeof original); rng = rng32; rot = st.rot;
    for (n = 0; n < 160; n++) {
        u32 keys = n >= 12 && n < 140 && (n-12)%16 == 0 ? (n < 76 ? K_F5 : K_F1) : 0;
        if (!sw_step(keys)) return 3;
        if (memcmp(original,st.unit,sizeof original) || rng != rng32 || st.mode != M_MENU) return 5;
        snprintf(path,sizeof path,"%s/%03u.png",argv[1],n); if (sw_write_png(path,1)) return 4;
        printf("{\"frame\":%u,\"rot\":%u,\"turn\":%d,\"sample\":%u",n,st.rot,st.turn,st.turn_t);
        if (st.turn) {
            for (i = 0; i < 16000; i++) { labels[i*2] = review_faces[i] >> 8; labels[i*2+1] = review_faces[i]; }
            save(argv[1],n,"faces",labels,sizeof labels); save(argv[1],n,"actors",actor_mask,sizeof actor_mask);
            printf(",\"yaw\":%d,\"vertices\":[",yaw);
            for (i = 0; i < RD16(pbank,0); i++)
                printf("%s[%d,%d,%d]",i ? "," : "",projected[i].x,projected[i].y,projected[i].q);
            printf("]");
        }
        puts("}");
    }
    return st.rot != rot || st.turn;
}
