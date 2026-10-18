// PC visual reference only. This filtered sampler uses doubles and the local disc atlas;
// never compile it into a TI target or infer calculator speed from its output.
#if defined(__m68k__) || !defined(FFT_TEXTURE_REVIEW)
#error FFT_TEXTURE_PREVIEW requires the PC winning-face diagnostic
#endif
#include <stdio.h>
#include <stdlib.h>
static u8 *preview_bank;
static u8 preview_ready;
static u32 preview_pixels;
static u8 *preview_iso_bank, preview_iso_style;

static void preview_iso_set(u8 style)
{
    preview_iso_style = style;
    scene_rot = vbuf_rot = 0xFF;
    if (W) { W->bg_ok = 0; W->frz = W->hold = 0; }
}

static void preview_iso_assets(void)
{
    FILE *f; unsigned bytes = 8+16*SC_PLANE;
    if (preview_iso_bank) { free(preview_iso_bank); preview_iso_bank = RT_NULL; }
    f = fopen("fftiso.bin","rb");
    if (f) {
        preview_iso_bank = malloc(bytes);
        if (preview_iso_bank && (fread(preview_iso_bank,1,bytes,f) != bytes
            || RD16(preview_iso_bank,0) != 0x4953 || RD16(preview_iso_bank,1) != 1
            || RD16(preview_iso_bank,2) != SC_BYTES || RD16(preview_iso_bank,3) != SC_H)) {
            free(preview_iso_bank); preview_iso_bank = RT_NULL;
        }
        fclose(f);
    }
    preview_iso_set(2);
}

// Replace colour only at view load. Original occlusion layers and all sprite data survive.
static void preview_iso_apply(u8 rot)
{
    if (preview_iso_bank && preview_iso_style && preview_iso_style <= 2)
        memcpy(vbuf,preview_iso_bank+8+((preview_iso_style-1)*8+rot*2)*SC_PLANE,2*SC_PLANE);
}

static void preview_assets(void)
{
    FILE *f; unsigned size = 0, expected;
    u8 header[20];
    if (preview_bank) { free(preview_bank); preview_bank = RT_NULL; }
    preview_ready = 0;
    if (!pbank) return;
    f = fopen("ffttex.bin", "rb"); if (!f) return;
    expected = 20+RD16(pbank,1)*8+512+131072;
    if (fread(header,1,20,f) != 20 || RD16(header,0) != 0x5458 || RD16(header,1) != 1
        || RD16(header,2) != RD16(pbank,1)) { fclose(f); return; }
    preview_bank = malloc(expected);
    if (preview_bank) {
        memcpy(preview_bank,header,20);
        size = (unsigned)fread(preview_bank+20,1,expected-20,f)+20;
        if (size != expected) { free(preview_bank); preview_bank = RT_NULL; }
    }
    fclose(f); preview_ready = preview_bank != RT_NULL;
}

// Trace the current projected face back to its original UVs. Perspective and blended
// transition transforms both remain linear in 3D; the orthographic turn uses affine UVs.
static void preview_texture(u8 ortho)
{
    const u8 *uv, *palettes, *tex;
    u16 nf, face, width = plow ? 80 : 160, height = plow ? 50 : 100;
    u16 lo[867], hi[867], n; // local PC scratch, one bound per MAP022 triangle
    s16 x, y;
    static const double sample[4] = {-.375,-.125,.125,.375};
    preview_pixels = 0;
    if (!preview_ready) return;
    nf = RD16(pbank,1); if (nf > 866) return;
    uv = preview_bank+20; palettes = uv+nf*8; tex = palettes+512;
    for (n = 0; n <= nf; n++) { lo[n] = 16000; hi[n] = 0; }
    for (y = 0; y < height; y++) for (x = 0; x < width; x++) {
        u16 k = y*160+x, id = review_faces[k];
        if (id && id <= nf && !review_edges[k]) { if (k < lo[id]) lo[id] = k; hi[id] = k; }
    }
    for (face = 1; face <= nf; face++, uv += 8) {
        const u8 *fp = pbank+2120+RD16(pbank,0)*6+(face-1)*8;
        u16 i0 = RD16(fp,0), i1 = RD16(fp,1), i2 = RD16(fp,2), k;
        double ax, ay, az, bx, by, bz, cx, cy, cz, e0x, e0y, e0z, e1x, e1y, e1z;
        double nx, ny, nz, plane, d00, d01, d11, determinant;
        if (lo[face] > hi[face] || uv[6] == 255) continue;
        if (ortho) {
            ax = PPROJECT[i0].x; ay = PPROJECT[i0].y; az = 0;
            bx = PPROJECT[i1].x; by = PPROJECT[i1].y; bz = 0;
            cx = PPROJECT[i2].x; cy = PPROJECT[i2].y; cz = 0;
        } else {
            ax = PVERT[i0].x; ay = PVERT[i0].y; az = PVERT[i0].z;
            bx = PVERT[i1].x; by = PVERT[i1].y; bz = PVERT[i1].z;
            cx = PVERT[i2].x; cy = PVERT[i2].y; cz = PVERT[i2].z;
        }
        e0x = bx-ax; e0y = by-ay; e0z = bz-az;
        e1x = cx-ax; e1y = cy-ay; e1z = cz-az;
        nx = e0y*e1z-e0z*e1y; ny = e0z*e1x-e0x*e1z; nz = e0x*e1y-e0y*e1x;
        plane = nx*ax+ny*ay+nz*az;
        d00 = e0x*e0x+e0y*e0y+e0z*e0z;
        d01 = e0x*e1x+e0y*e1y+e0z*e1z;
        d11 = e1x*e1x+e1y*e1y+e1z*e1z; determinant = d00*d11-d01*d01;
        if (determinant < 1e-9) continue;
        for (k = lo[face]; k <= hi[face]; k++) {
            u32 total = 0; u16 count = 0, sx, sy, colour, q0, q1, q2;
            u8 water = uv[7] & 128;
            if (review_faces[k] != face || review_edges[k]) continue;
            x = k%160; y = k/160; if (x >= width) continue;
            for (sy = 0; sy < 4; sy++) for (sx = 0; sx < 4; sx++) {
                double px = x+.5+sample[sx], py = y+.5+sample[sy], pz = 0;
                double d20, d21, v, w, u, t;
                unsigned ix, iy, index, entry;
                if (!ortho) {
                    double rayx = ((plow ? px*2 : px)-80)/96;
                    double rayy = (48-(plow ? py*2 : py))/96;
                    double denominator = rayx*nx+rayy*ny+nz, distance;
                    if (denominator > -1e-12 && denominator < 1e-12) continue;
                    distance = plane/denominator;
                    px = rayx*distance; py = rayy*distance; pz = distance;
                }
                px -= ax; py -= ay; pz -= az;
                d20 = px*e0x+py*e0y+pz*e0z; d21 = px*e1x+py*e1y+pz*e1z;
                v = (d11*d20-d01*d21)/determinant; w = (d00*d21-d01*d20)/determinant;
                u = uv[0]+v*((s16)uv[2]-uv[0])+w*((s16)uv[4]-uv[0]);
                t = uv[1]+v*((s16)uv[3]-uv[1])+w*((s16)uv[5]-uv[1]);
                ix = u < 0 ? 0 : u > 255 ? 255 : (unsigned)u;
                iy = t < 0 ? 0 : t > 255 ? 255 : (unsigned)t;
                index = tex[(iy+(uv[7]&3)*256)*128+(ix>>1)];
                index = ix & 1 ? index >> 4 : index & 15;
                entry = RD16(palettes+uv[6]*32,index);
                if (entry == 65535) continue;
                total += entry; count++;
            }
            if (!count) continue;
            total /= count;
            q0 = RD16(preview_bank,water ? 4 : 7);
            q1 = RD16(preview_bank,water ? 5 : 8);
            q2 = RD16(preview_bank,water ? 6 : 9);
            colour = total > q2 ? 0 : total > q1 ? 1 : total > q0 ? 2 : 3;
            if (water) { if (!colour) colour = 1; if (colour == 3) colour = 2; }
            ppixel(x,y,(u8)colour); preview_pixels++;
        }
    }
}

static s16 preview_pivot_x, preview_pivot_y;
static PScreen preview_ortho(s16 x, s16 y, s16 z)
{
    PScreen p;
    s16 dx = x-(st.cx*28+14), dz = z-(st.cz*28+14);
    s16 across = (s16)((muls16(dx,pc)+muls16(dz,ps)) >> 7);
    s16 forward = (s16)((muls16(dx,ps)-muls16(dz,pc)) >> 7);
    p.x = preview_pivot_x+(s16)(muls16(across,155) >> 8);
    p.y = preview_pivot_y-(y >> 1)-(s16)(muls16(forward,77) >> 8);
    p.q = 128-(forward >> 2)+(y >> 2);
    if (p.q < 1) p.q = 1;
    if (p.q > 254) p.q = 254;
    return p;
}

// F1/F5 use the same atlas and winner-based sampler as perspective motion.
static void preview_turn(void)
{
    static const u8 angle[6] = {2,5,7,9,11,14};
    u16 i, nv = RD16(pbank,0), nf = RD16(pbank,1);
    s16 u, v, yaw = (56-16*st.rot-st.turn*angle[st.turn_t]) & 63;
    const u8 *src = pbank+2120;
    ps = psine((u8)yaw); pc = psine((u8)(yaw+16)); plow = 0;
    view_of(st.rot,st.cx,st.cz,&u,&v);
    preview_pivot_x = scene_x(st.rot,u,v)-st.camx;
    preview_pivot_y = scene_y(u,v)+HH-st.camy;
    draw_clear(); memset(PDEPTH,0,16000); memset(PCACHE,0,2000);
    memset(review_faces,0,sizeof review_faces); memset(review_edges,0,sizeof review_edges);
    for (i = 0; i < nv; i++, src += 6)
        PPROJECT[i] = preview_ortho((s16)RD16(src,0),(s16)RD16(src,1),(s16)RD16(src,2));
    for (i = 0; i < nf; i++, src += 8) {
        PScreen triangle[3]; u8 j;
        for (j = 0; j < 3; j++) triangle[j] = PPROJECT[RD16(src,j)];
        review_face = i+1; praster(triangle,3,src[6]);
    }
    review_face = 0; preview_texture(1);
    for (i = 0; i < NUNIT; i++) {
        const Unit *un = &st.unit[i]; const u8 *g;
        PScreen p; s16 x, y;
        if (un->x >= MAP_W || un->z >= MAP_H) continue;
        p = preview_ortho(un->x*28+14,map_tiles[un->z*MAP_W+un->x].stand*6,un->z*28+14);
        g = unit_frame((u8)(4-((yaw+8)>>4)) & 3,(u8)i);
        for (y = 0; y < UNIT_SH; y++) {
            u16 l = RD16(g,y), d = RD16(g,UNIT_SH+y), m = RD16(g,2*UNIT_SH+y);
            s16 yy = p.y-UNIT_FOOT+y;
            if ((u16)yy >= 100) continue;
            for (x = 0; x < 16; x++) {
                s16 xx = p.x-8+x; u16 bit = 0x8000U >> x;
                if ((u16)xx >= 160 || (m & bit) || p.q+1 < PDEPTH[yy*160+xx]) continue;
                ppixel(xx,yy,((l & bit) ? 1 : 0)+((d & bit) ? 2 : 0));
                PDEPTH[yy*160+xx] = (u8)p.q;
            }
        }
    }
    scene_rot = vbuf_rot = 0xFF; W->bg_ok = 0;
}
