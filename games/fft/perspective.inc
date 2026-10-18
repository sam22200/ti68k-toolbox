// Perspective inspection of the original MAP022 mesh. Included after unit_frame so the
// existing animations, unit records and archived sprite bank remain the single authority.
// Scratch aliases: vbuf = transformed vertices; status = 80x50 reciprocal depth;
// bg = cached half-resolution colour planes. No additional heap allocation.
typedef struct { s16 x, y, z; } PVertex;
typedef struct { s16 x, y, q; } PScreen;
typedef struct { const u8 *frame; u8 h, w; u32 row[3][52]; } PScale;
#ifdef FFT_CONFIG
// Independent idle job: ISO keeps its decoded view, scene, HUD and cover data intact.
typedef struct {
    u8 *depth,*water,(*planes)[1700];
    u8 codes[500],visible[128],hidden[128];
    s16 spans[4][100], row, end;
    u16 index, faces;
    u8 stage, shade, delay, descriptor[5]; // Ramza x,z,facing, zoom, filled/textured
} PIdle;
static PIdle *pidle;
static u8 pidle_active;
static u8 pidle_take(void);
#endif
static const u8 *pbank;
static u8 pvalid;
#ifndef FFT_FIXED
static u8 pold[6];
#else
static void fixed_edges(void);
static void fixed_cutaway(const u8 *src);
static u8 fixed_obstruction(void);
static const u8 *pgroup, *pegroup;
static u8 powner_group;
#ifdef FFT_CONFIG
#define PHIDDEN (pidle_active ? pidle->hidden : vbuf + 6640)
#else
#define PHIDDEN (vbuf + 6640)
#endif
#define PCOUNTS (vbuf + 6784)
#define POWNER (vbuf + 8000)          // cold-only pixel ownership; pose pool reuses it later
#endif
static u16 pfaces;
static s16 ps, pc, psp, pcp, pcamx, pcamy, pcamz;
#ifdef FFT_FIXED
static u8 plow;
static s16 ptalpha, ptdepth, ptfx, ptfy, ptcx, ptcy, ptpx, ptpy, ptpz;
// Smoothstep samples generated offline; reused for inspection entry, exit and zoom.
#ifdef FFT_CINEMA
static const u8 ctween[10] = {0,2,4,5,7,9,11,12,14,16};
#endif
static const u16 pease[17] = {0,3,11,24,40,59,81,104,128,152,175,197,216,232,245,253,256};
#endif
#define P_NEAR 16
#ifdef FFT_FIXED
#define PW 160
#define PH 100
#define PDEPTH (vbuf + 24000)
#else
#define PW 80
#define PH 50
#define PDEPTH ((u8 *)W->status)
#endif
#ifdef FFT_CONFIG
#define PCACHE (pidle_active ? pidle->water : (u8 *)W->bg)
#else
#define PCACHE ((u8 *)W->bg)
#endif
#ifdef FFT_FIXED
#define PTHALF ((u8 (*)[500])(PCACHE+2000)) // transition planes reuse cached HUD scratch
#endif
#ifdef FFT_CONFIG
#define PVERT ((PVertex *)(pidle_active ? (u8 *)W->status : vbuf))
#else
#define PVERT ((PVertex *)vbuf)
#endif
#define PPROJECT ((PScreen *)(vbuf + 3000))
#ifdef FFT_CONFIG
#define PCODES (pidle_active ? pidle->codes : vbuf + 6000)
#else
#define PCODES (vbuf + 6000)
#endif
#define PSCALES ((PScale *)(vbuf + 6504))    // 8-byte alignment also on an LP64 PC
#define PFRAME (vbuf + 15000)
#define PFRAMEKEY ((const u8 **)(vbuf + 22000))
#ifdef FFT_TEXTURE_REVIEW
#if defined(__m68k__) || !defined(FFT_FIXED)
#error FFT_TEXTURE_REVIEW is a PC-only fixed-camera diagnostic
#endif
// Diagnostic output only: the offline texture preview follows actual winning pixels.
static u16 review_faces[PW*PH], review_face;
static u8 review_edges[PW*PH];
#endif
#ifdef FFT_FIXED
#ifdef FFT_CONFIG
#define PVISIBLE (pidle_active ? pidle->visible : vbuf + 6504)
#else
#define PVISIBLE (vbuf + 6504)
#endif
#else
#define PVISIBLE (vbuf + 22100)
#endif

static void perspective_init(void)
{
    pbank = rt_file("fftmesh", RT_NULL); pvalid = 0; pfaces = 0;
#ifdef FFT_FIXED
    plow = 0;
#endif
    if (pbank && (RD16(pbank, 0) > 500 || RD16(pbank, 2) < 2 || RD16(pbank, 2) > 4 || RD16(pbank, 3) > 128)) pbank = RT_NULL;
}

static s16 psine(u8 a) { return (s8)pbank[8 + (a & 63)]; }
static u16 precip(u16 z) { return z < 1024 ? RD16(pbank + 72, z) : 0; }

static u8 pview_yaw(void)
{
#ifdef FFT_CINEMA
    if (st.cphase) return st.cyaw;
#endif
    return st.pyaw;
}

static void perspective_enter(u8 kind)
{
    static const u8 direction[4] = {16, 48, 32, 0};
#ifdef FFT_FIXED
    kind = 1;
#endif
    st.perspective = kind;
    st.pyaw = st.pbase = direction[st.unit[U_RAMZA].face];
    st.ppitch = 3; st.pside = 0; st.pdist = 56;
#ifdef FFT_FIXED
    st.ppitch = st.pzoom ? 6 : 1; st.pside = 6; st.pdist = st.pzoom ? 80 : 40;
#endif
    pvalid = 0;
#ifndef FFT_FIXED
    memset(PSCALES, 0, NUNIT*sizeof(PScale));
    memset(PFRAMEKEY, 0, NUNIT*sizeof(const u8 *));
#endif
}

static void perspective_keys(void)
{
    st.tick++;                              // idle poses, with no tactical update or CT draw
    if (input_pressed(K_ESC | K_B | K_C)) {
#if defined(FFT_FIXED) && defined(FFT_CAMERA_MOTION)
        st.ptrans = 2; st.ptrans_t = 16; pvalid = 0;
        return;
#endif
        st.perspective = 0; pvalid = 0; vbuf_rot = 0xFF;
#ifdef FFT_FIXED
        scene_rot = 0xFF;
#endif
        W->bg_ok = 0; W->status_i = 0;
        return;
    }
#ifdef FFT_FIXED
    if (input_pressed(K_F5)) st.pyaw ^= 32;
    if (input_pressed(K_MODE)) {
        st.pzoom = st.pdist != 80;
#ifdef FFT_CAMERA_MOTION
        st.pfrom = !st.pzoom;
#endif
        st.pdist = st.pzoom ? 80 : 40; st.ppitch = st.pzoom ? 6 : 1;
#ifdef FFT_CAMERA_MOTION
        st.ptrans = 3; st.ptrans_t = 0;
#endif
        pvalid = 0;
    }
    if (input_pressed(K_D)) {
#ifdef FFT_CONFIG
        st.pwire ^= 1;
#else
        st.psolid = (st.psolid+1) % 3;
#endif
        pvalid = 0;
    }
    if (input_pressed(K_ENTER | K_F1)) st.pyaw = st.pbase;
    return;
#else
    if (input_pressed(K_MODE)) perspective_enter(st.perspective % 3 + 1);
    if (input_pressed(K_ENTER)) { perspective_enter(st.perspective); return; }
    if (st.perspective == 1) return;
    if ((st.tick & 1) == 0) {
        s16 yaw = (s16)((st.pyaw - st.pbase + 32) & 63) - 32;
        if (input_held(K_LEFT)) {
            if (st.perspective == 3) st.pyaw = (st.pyaw - 1) & 63;
            else if (input_held(K_D)) { if (yaw > -8) st.pyaw = (st.pyaw - 1) & 63; }
            else if (st.pside > -24) st.pside -= 2;
        }
        if (input_held(K_RIGHT)) {
            if (st.perspective == 3) st.pyaw = (st.pyaw + 1) & 63;
            else if (input_held(K_D)) { if (yaw < 8) st.pyaw = (st.pyaw + 1) & 63; }
            else if (st.pside < 24) st.pside += 2;
        }
        if (st.perspective == 3) {
            if (input_held(K_UP) && st.ppitch < 12) st.ppitch++;
            if (input_held(K_DOWN) && st.ppitch > -5) st.ppitch--;
            if (input_held(K_F1) && st.pdist > 40) st.pdist -= 2;
            if (input_held(K_F5) && st.pdist < 112) st.pdist += 2;
        }
    }
#endif
}

static PVertex ptransform(s16 x, s16 y, s16 z)
{
    PVertex v;
    s16 dx = x - pcamx, dy = y - pcamy, dz = z - pcamz, f;
#ifdef FFT_FIXED
    if (plow == 1) { dx = x-ptpx; dy = y-ptpy; dz = z-ptpz; }
#endif
    v.x = (s16)((muls16(dx, pc) + muls16(dz, ps)) >> 7);
    f = (s16)((muls16(dx, ps) - muls16(dz, pc)) >> 7);
    v.y = (s16)((muls16(dy, pcp) + muls16(f, psp)) >> 7);
    v.z = (s16)((muls16(f, pcp) - muls16(dy, psp)) >> 7);
#ifdef FFT_FIXED
    if (plow == 1) {
        v.z = (s16)((muls16(256-ptalpha, 160)+muls16(ptalpha, v.z+ptdepth)) >> 8);
        v.x += ptfx+divs32_16(muls16(ptcx-80, v.z), 96);
        v.y += ptfy+divs32_16(muls16(48-ptcy, v.z), 96);
    }
#endif
    return v;
}

static PScreen pproject(PVertex v)
{
    PScreen p;
    u16 r = precip(v.z);
#ifdef FFT_FIXED
    p.x = 80 + (s16)((muls16(v.x, (s16)r) * 48) >> 15);
    p.y = 48 - (s16)((muls16(v.y, (s16)r) * 48) >> 15);
#else
    p.x = 40 + (s16)((muls16(v.x, (s16)r) * 48) >> 16);
    p.y = 24 - (s16)((muls16(v.y, (s16)r) * 48) >> 16);
#endif
    p.q = (s16)(r >> 4);                   // 4096/z, linear across a projected triangle
    if (p.q > 255) p.q = 255;
#ifdef FFT_FIXED
    if (plow) { p.x >>= 1; p.y >>= 1; }
#endif
    return p;
}

static inline __attribute__((always_inline)) void ppixel(s16 x, s16 y, u8 colour)
{
#ifdef FFT_FIXED
    if (plow) {
        u16 off = y*10+((u16)x >> 3); u8 bit = 0x80 >> (x & 7);
        if (colour & 1) PTHALF[0][off] |= bit; else PTHALF[0][off] &= ~bit;
        if (colour & 2) PTHALF[1][off] |= bit; else PTHALF[1][off] &= ~bit;
        return;
    }
#endif
#ifdef FFT_TEXTURE_REVIEW
    if (review_face) review_faces[y*PW+x] = review_face;
    else review_edges[y*PW+x] = 1;
#endif
#ifdef FFT_FIXED
    u16 off;
#ifdef FFT_CONFIG
    if(pidle_active) {
        if(y<8 || y>=93) return;
        off=(y-8)*20+((u16)x>>3);
    } else
#endif
        off=y*30+((u16)x>>3);
    u8 bit = 0x80 >> (x & 7);
    if (colour & 1) ((u8 *)rt_light)[off] |= bit; else ((u8 *)rt_light)[off] &= ~bit;
    if (colour & 2) ((u8 *)rt_dark)[off] |= bit; else ((u8 *)rt_dark)[off] &= ~bit;
#else
    u16 off = y * 10 + ((u16)x >> 3);
    u8 bit = 0x80 >> (x & 7);
    if (colour & 1) half[0][off] |= bit; else half[0][off] &= ~bit;
    if (colour & 2) half[1][off] |= bit; else half[1][off] &= ~bit;
#endif
}

// Scan-convert clipped convex polygons. Divisions occur per edge and span, never per pixel.
// q = reciprocal depth: affine interpolation is perspective correct (8-bit quantization).
#ifdef FFT_CINEMA
// Accumulate winning pixels into a byte before touching the colour/water planes. The
// depth test remains exact; this removes repeated variable shifts and plane RMW per pixel.
static void cspan(s16 y, s16 x, s16 end, s32 q, s16 step, u8 shade)
{
    u8 *depth =
#ifdef FFT_CONFIG
        pidle_active ? pidle->depth+(y-8)*160+x :
#endif
        PDEPTH+y*(plow ? 80 : 160)+x;
    u8 *owner =
#ifdef FFT_CONFIG
        pidle_active ? RT_NULL :
#endif
        POWNER+y*160;
    u16 off = y*(plow ? 10 : 30)+((u16)x >> 3), water = y*20+((u16)x >> 3);
    u8 *l = plow ? PTHALF[0] : rt_light, *d = plow ? PTHALF[1] : rt_dark;
#ifdef FFT_CONFIG
    if(pidle_active) off=water=(y-8)*20+((u16)x>>3);
#endif
    while (x < end) {
        u8 bit = 0x80 >> (x & 7), mask = 0;
        do {
            s16 z = (s16)(q >> 7);
            if (z > *depth) {
                *depth = (u8)z; mask |= bit;
                if (!plow
#ifdef FFT_CONFIG
                    && !pidle_active
#endif
                    ) owner[x] = powner_group;
#ifdef FFT_TEXTURE_REVIEW
                review_faces[y*PW+x] = review_face;
#endif
            }
            depth++; q += step; x++; bit >>= 1;
        } while (bit && x < end);
        if (mask) {
            if (shade & 1) l[off] |= mask; else l[off] &= ~mask;
            if (shade & 2) d[off] |= mask; else d[off] &= ~mask;
            if (!plow) { if (shade & 4) PCACHE[water] |= mask; else PCACHE[water] &= ~mask; }
        }
        off++; water++;
    }
}
#endif

static void praster(PScreen *p, u8 n, u8 shade)
{
    s16 left[PH], right[PH], lq[PH], rq[PH], i, y, lo = PH, hi = 0;
    s16 width = PW, height = PH;
#ifdef FFT_FIXED
    if (plow) { width = 80; height = 50; }
#endif
    for (i = 0; i < n; i++) {
        if (p[i].y < lo) lo = p[i].y;
        if (p[i].y > hi) hi = p[i].y;
    }
    if (lo < 0) lo = 0;
    if (hi > height) hi = height;
    if (lo >= hi) return;
    for (y = lo; y < hi; y++) { left[y] = 32767; right[y] = -32767; }
    for (i = 0; i < n; i++) {
        PScreen a = p[i], b = p[i+1 < n ? i+1 : 0];
        s16 ya, yb, dx, dq;
        s32 x, q;
        if (a.y == b.y) continue;
        if (a.y > b.y) { PScreen t = a; a = b; b = t; }
        ya = a.y < lo ? lo : a.y; yb = b.y > hi ? hi : b.y;
        if (ya >= yb) continue;
#ifdef FFT_FIXED
        dx = divs32_16((s32)(b.x-a.x)*128, b.y-a.y);
        x = (s32)a.x*128 + muls16(dx, ya-a.y) + (dx >> 1) + 64;
#else
        dx = divs32_16((s32)(b.x-a.x)*256, b.y-a.y);
        x = (s32)a.x*256 + muls16(dx, ya-a.y) + (dx >> 1) + 128;
#endif
        dq = divs32_16((s32)(b.q-a.q)*128, b.y-a.y);
        q = (s32)a.q*128 + muls16(dq, ya-a.y) + (dq >> 1);
        for (y = ya; y < yb; y++, x += dx, q += dq) {
            s16 xx = (s16)(x >> (PW == 160 ? 7 : 8)), qq = (s16)q;
            if (xx < left[y]) { left[y] = xx; lq[y] = qq; }
            if (xx > right[y]) { right[y] = xx; rq[y] = qq; }
        }
    }
#ifdef FFT_CONFIG
    if (pidle_active) {
        memcpy(pidle->spans[0]+lo,left+lo,(hi-lo)*sizeof(s16));
        memcpy(pidle->spans[1]+lo,right+lo,(hi-lo)*sizeof(s16));
        memcpy(pidle->spans[2]+lo,lq+lo,(hi-lo)*sizeof(s16));
        memcpy(pidle->spans[3]+lo,rq+lo,(hi-lo)*sizeof(s16));
        pidle->row=lo; pidle->end=hi; pidle->shade=shade;
        pfaces++; return;
    }
#endif
    for (y = lo; y < hi; y++) {
        s16 a = left[y], b = right[y], x0, x1, step;
        s32 q;
#ifndef FFT_CINEMA
        s16 x;
        u8 *depth = PDEPTH + y*width;
#endif
        if (a >= b || b <= 0 || a >= width) continue;
        step = divs32_16((s32)rq[y] - lq[y], b-a);
        x0 = a < 0 ? 0 : a; x1 = b > width ? width : b;
        q = lq[y] + muls16(step, x0-a);
#ifdef FFT_CINEMA
        cspan(y, x0, x1, q, step, shade);
#else
        for (x = x0; x < x1; x++, q += step) {
            s16 d = (s16)(q >> 7);
            if (d > depth[x]) {
                depth[x] = (u8)d; ppixel(x, y, shade & 3);
#ifdef FFT_FIXED
                if (!plow)
                { u16 off = y*20+((u16)x >> 3); u8 bit = 0x80 >> (x & 7);
                  POWNER[y*PW+x] = powner_group;
                  if (shade & 4) PCACHE[off] |= bit; else PCACHE[off] &= ~bit; }
#endif
            }
        }
#endif
    }
    pfaces++;
}

static s16 pdistance(PVertex v, u8 plane)
{
    if (!plane) return v.z - P_NEAR;
    if (plane == 1) return 5*v.z + 6*v.x;
    if (plane == 2) return 5*v.z - 6*v.x;
    if (plane == 3) return v.z - 2*v.y;
    return 13*v.z + 24*v.y;
}

static void ptriangle(PVertex *in, u8 shade)
{
    PVertex buf[2][12];
    u8 i, plane, n = 3, k = 0;
    memcpy(buf[0], in, 3*sizeof(PVertex));
    for (plane = 0; plane < 5; plane++, k ^= 1) {
        u8 count = 0;
        for (i = 0; i < n; i++) {
            PVertex a = buf[k][i], b = buf[k][i+1 < n ? i+1 : 0];
            s16 da = pdistance(a, plane), db = pdistance(b, plane);
            if (da >= 0) buf[k^1][count++] = a;
            if ((da >= 0) != (db >= 0)) {
                PVertex c;
                c.x = a.x + divs32_16(muls16(b.x-a.x, da), da-db);
                c.y = a.y + divs32_16(muls16(b.y-a.y, da), da-db);
                c.z = a.z + divs32_16(muls16(b.z-a.z, da), da-db);
                if (c.z < P_NEAR) c.z = P_NEAR;
                buf[k^1][count++] = c;
            }
        }
        n = count;
        if (n < 3) return;
    }
    { PScreen p[12]; for (i = 0; i < n; i++) p[i] = pproject(buf[k][i]); praster(p, n, shade); }
}

#ifdef FFT_TEXTURE_PREVIEW
#include "texture_preview.inc"
#endif

// Shared camera setup; detailed and coarse transition renderers use identical motion.
static void pcamera(void)
{
    const u8 *vp = pbank+2120;
    u16 nv = RD16(pbank,0), nf = RD16(pbank,1);
    s16 distance = st.pdist, pitch = st.ppitch;
    const Unit *ramza = &st.unit[U_RAMZA];
    u8 yaw = st.pyaw, ax = ramza->x, az = ramza->z;
    s16 ah = map_tiles[az*MAP_W+ax].stand, lateral = 12+st.pside;
#ifdef FFT_FIXED
    const u8 *extra = vp+nv*6+nf*8+RD16(pbank, 3)*8;
    const u8 *materials = RD16(pbank, 2) >= 3 ? extra+2+RD16(extra, 0)*4 : RT_NULL;
    s16 eye = st.pdist == 80 ? 76 : 28;
    u8 motion = st.ptrans, moment = st.ptrans_t;
    plow = 0;
#ifdef FFT_CINEMA
    if (st.cphase) {
        distance = st.cdist; pitch = st.cpitch; yaw = st.cyaw;
        ax = st.cax; az = st.caz; ah = st.cah; lateral = 12+st.cside; eye = st.ceye;
        motion = st.cphase == 1 ? 1 : st.cphase == 3 ? 2 : 0;
        moment = ctween[st.cframe];
    }
#endif
#endif
    ps = psine(yaw); pc = psine(yaw+16);
    psp = psine((u8)pitch); pcp = psine((u8)(pitch+16));
#ifdef FFT_FIXED
    if (st.ptrans == 3) {
        s16 a = pease[st.ptrans_t], from = st.pfrom ? 80 : 40;
        s16 pitch = st.pfrom ? 6 : 1, height = st.pfrom ? 76 : 28;
        distance = from+(s16)(muls16(st.pdist-from, a) >> 8);
        eye = height+(s16)(muls16(eye-height, a) >> 8);
        psp = psine((u8)pitch)+(s16)(muls16(psp-psine((u8)pitch), a) >> 8);
        pcp = psine((u8)(pitch+16))+(s16)(muls16(pcp-psine((u8)(pitch+16)), a) >> 8);
    }
#endif
    pcamx = ax*28+14; pcamz = az*28+14; pcamy = ah*6+28;
    pcamx += -(s16)(muls16(ps, distance) >> 7)
        + (s16)(muls16(pc, lateral) >> 7);
    pcamz += (s16)(muls16(pc, distance) >> 7)
        + (s16)(muls16(ps, lateral) >> 7);
#ifdef FFT_FIXED
    pcamy += eye-28;
    fixed_cutaway(materials ? materials+nf : RT_NULL);
    if (motion == 1 || motion == 2) {
        PVertex foot = ptransform(ax*28+14, ah*6, az*28+14);
        Feet iso;
        s16 ix, iz, start_yaw = (56-16*st.rot) & 63, delta = ((yaw-start_yaw+32) & 63)-32;
        view_of(st.rot, ax, az, &ix, &iz);
        iso.x = scene_x(st.rot, ix, iz); iso.y = scene_y(ix, iz)+HH-ah*(HU/2);
        ptalpha = pease[moment]; ptdepth = foot.z;
        ptfx = (s16)(muls16(foot.x, ptalpha) >> 8); ptfy = (s16)(muls16(foot.y, ptalpha) >> 8);
        ptcx = 80+(s16)(muls16(iso.x-st.camx-80, 256-ptalpha) >> 8);
        ptcy = 48+(s16)(muls16(iso.y-st.camy-48, 256-ptalpha) >> 8);
        ptpx = ax*28+14; ptpy = ah*6; ptpz = az*28+14;
        yaw = (start_yaw+(s16)(muls16(delta, ptalpha) >> 8)) & 63;
        delta = 6+(s16)(muls16(pitch-6, ptalpha) >> 8);
        ps = psine((u8)yaw); pc = psine((u8)(yaw+16));
        psp = psine((u8)delta); pcp = psine((u8)(delta+16));
        plow = 1; memset(PTHALF, 0, 1000);
    } else if (st.ptrans == 3) {
        plow = 2; memset(PTHALF, 0, 1000);
    }
#endif
 }

#ifdef FFT_ULTRA_TRANSITION
#include "ultra_transition.inc"
#endif

static void pvertex(u16 i)
{
    const u8 *vp=pbank+2120+i*6;
    u8 c,code=0;
    PVERT[i]=ptransform((s16)RD16(vp,0),(s16)RD16(vp,1),(s16)RD16(vp,2));
    for(c=0;c<5;c++) if(pdistance(PVERT[i],c)<0) code|=1<<c;
    PCODES[i]=code;
    if(!code
#ifdef FFT_CONFIG
       && !pidle_active
#endif
       ) PPROJECT[i]=pproject(PVERT[i]);
}

static void pface(u16 i)
{
    const u8 *fp=pbank+2120+RD16(pbank,0)*6+i*8;
    PVertex v[3]; u8 c,both=31,either=0,shade=fp[6]; u16 ix[3];
#ifdef FFT_TEXTURE_REVIEW
    review_face=i+1;
#endif
#ifdef FFT_FIXED
    const u8 *extra=pbank+2120+RD16(pbank,0)*6+RD16(pbank,1)*8+RD16(pbank,3)*8;
    if(pgroup && PHIDDEN[pgroup[i]]) return;
    powner_group=pgroup ? pgroup[i] : 0;
    if(RD16(pbank,2)>=3) shade=extra[2+RD16(extra,0)*4+i];
#endif
    if(!PVISIBLE[fp[7]]) return;
    for(c=0;c<3;c++) { ix[c]=RD16(fp,c); both&=PCODES[ix[c]]; either|=PCODES[ix[c]]; }
    if(both) return;
    if(!either) {
        PScreen p[3]; for(c=0;c<3;c++) p[c]=
#ifdef FFT_CONFIG
            pidle_active ? pproject(PVERT[ix[c]]) :
#endif
            PPROJECT[ix[c]];
        praster(p,3,shade);
    } else {
        for(c=0;c<3;c++) v[c]=PVERT[ix[c]];
        ptriangle(v,shade);
    }
}

static void pgeometry(void)
{
    const u8 *vp = pbank+2120;
    u16 nv = RD16(pbank,0), nf = RD16(pbank,1), i;
#ifdef FFT_FIXED
    u8 redraw = 0;
#endif
    pcamera();
    { const u8 *plane = vp + nv*6 + nf*8;
      u16 count = RD16(pbank, 3);
      for (i = 0; i < count; i++, plane += 8) {
          s16 nx = (s16)RD16(plane, 0), ny = (s16)RD16(plane, 1), nz = (s16)RD16(plane, 2);
          PVISIBLE[i] = nx*pcamx + ny*pcamy + nz*pcamz > (s16)RD16(plane, 3);
#ifdef FFT_FIXED
          if (plow == 1) PVISIBLE[i] = 1; // depth resolves two-sided entry/exit geometry
#endif
      } }
#ifdef FFT_FIXED
    draw_clear();
    memset(PCACHE, 0, 2000);                 // visible water, temporary during geometry only
    memset(POWNER, 0, 16000);
#else
    memset(half, 0, sizeof half);
#endif
    memset(PDEPTH, 0, PW*PH); pfaces = 0;
#ifdef FFT_TEXTURE_REVIEW
    memset(review_faces, 0, sizeof review_faces); memset(review_edges, 0, sizeof review_edges);
#endif
    // Quiet white sky; a light horizon helps the original silhouette read on the LCD.
    for (i = 0; i < nv; i++) pvertex(i);
#ifdef FFT_FIXED
    for (;;) {
#endif
    for (i = 0; i < nf; i++) pface(i);
#ifdef FFT_FIXED
        if (plow || st.psolid || redraw || !fixed_obstruction()) break;
        // Rebuild once only if actual foreground pixels mask a protected view region.
        // Unselected architecture keeps its fill, depth and ordinary occlusion.
        redraw = 1; draw_clear(); memset(PCACHE, 0, 2000);
        memset(PDEPTH, 0, 16000); memset(POWNER, 0, 16000); pfaces = 0;
#ifdef FFT_TEXTURE_REVIEW
        memset(review_faces, 0, sizeof review_faces); memset(review_edges, 0, sizeof review_edges);
#endif
    }
#endif
#ifndef FFT_FIXED
    memcpy(PCACHE, half, sizeof half);
#else
#ifdef FFT_TEXTURE_REVIEW
    review_face = 0;
#endif
    if (!plow) fixed_edges();
#ifdef FFT_TEXTURE_PREVIEW
#ifdef FFT_CONFIG
    if (!st.pwire)
#endif
        preview_texture(0);
#endif
    if (plow) {
        static const u8 expand[16] = {0,3,12,15,48,51,60,63,192,195,204,207,240,243,252,255};
        s16 y, x; u8 plane;
        for (plane = 0; plane < 2; plane++) {
            u8 *dst = plane ? rt_dark : rt_light;
            for (y = 0; y < 50; y++) for (x = 0; x < 10; x++) {
                u8 b = PTHALF[plane][y*10+x];
                dst[y*60+x*2] = dst[y*60+30+x*2] = expand[b >> 4];
                dst[y*60+x*2+1] = dst[y*60+30+x*2+1] = expand[b & 15];
            }
        }
        // Expand backwards in-place, keeping full-resolution actor depth tests unchanged.
        for (y = 49; y >= 0; y--) for (x = 79; x >= 0; x--) {
            u8 q = PDEPTH[y*80+x]; u16 off = y*320+x*2;
            PDEPTH[off] = PDEPTH[off+1] = PDEPTH[off+160] = PDEPTH[off+161] = q;
        }
    }
#endif
    vbuf_rot = 0xFF;                        // transformed scratch replaced the unpacked view
}

#ifndef FFT_FIXED
static void psprite(u8 who)
{
    const Unit *u = &st.unit[who];
    PVertex v;
    PScreen p;
    const u8 *g;
    s16 h, w, x0, y0, x, y, sy = 0, sy_step, sx_step;
    PScale *cache = PSCALES + who;
    u8 rot = (u8)(4 - ((st.pyaw + 8) >> 4)) & 3;
    if (u->x >= MAP_W || u->z >= MAP_H) return;
    v = ptransform(u->x*28+14, map_tiles[u->z*MAP_W+u->x].stand*6, u->z*28+14);
    if (v.z < P_NEAR) return;
    p = pproject(v);
    h = (s16)((muls16(34, (s16)precip(v.z))*48) >> 16);
    if (h < 3) return;
    if (h > 52) h = 52;
    w = (h*16) / UNIT_SH;
    if (w < 2) w = 2;
    x0 = p.x - (w >> 1); y0 = p.y - (h*UNIT_FOOT)/UNIT_SH;
    if (x0 >= 80 || x0+w <= 0 || y0 >= 50 || y0+h <= 0) return;
    g = unit_frame(rot, who);
    if (cache->frame != g || cache->h != h || cache->w != w) {
        cache->frame = g; cache->h = (u8)h; cache->w = (u8)w;
        sx_step = (16*256) / w; sy_step = (UNIT_SH*256) / h;
        for (y = 0; y < h; y++, sy += sy_step) {
            u16 mask = RD16(g, 2*UNIT_SH+(sy >> 8));
            u16 l = RD16(g, sy >> 8), d = RD16(g, UNIT_SH+(sy >> 8));
            u32 ml = 0, md = 0, mm = 0, out = 0x80000000UL;
            s16 sx = 0;
            for (x = 0; x < w; x++, sx += sx_step, out >>= 1) {
                u16 bit = 0x8000U >> (sx >> 8);
                if (!(mask & bit)) mm |= out;
                if (l & bit) ml |= out;
                if (d & bit) md |= out;
            }
            cache->row[0][y] = ml; cache->row[1][y] = md; cache->row[2][y] = mm;
        }
    }
    for (y = 0; y < h; y++) {
        s16 yy = y0+y;
        u32 mask, l, d, bit = 0x80000000UL;
        if ((u16)yy >= 50) continue;
        mask = cache->row[2][y]; l = cache->row[0][y]; d = cache->row[1][y];
        // Build a visible row, then update at most five bytes on each colour plane.
        for (x = 0; x < w; x++, bit >>= 1) {
            s16 xx = x0+x;
            if ((u16)xx >= 80 || p.q+1 < PDEPTH[yy*80+xx]) mask &= ~bit;
        }
        l &= mask; d &= mask;
        for (x = 0; x < w; x += 8) {
            s16 xx = x0+x, byte = xx >> 3, shift = xx & 7;
            u8 m = (u8)(mask >> 24), a = (u8)(l >> 24), b = (u8)(d >> 24);
            u16 off = yy*10;
            if ((u16)byte < 10) {
                half[0][off+byte] = (half[0][off+byte] & ~(m >> shift)) | (a >> shift);
                half[1][off+byte] = (half[1][off+byte] & ~(m >> shift)) | (b >> shift);
            }
            if (shift && (u16)(byte+1) < 10) {
                half[0][off+byte+1] = (half[0][off+byte+1] & ~(m << (8-shift))) | (a << (8-shift));
                half[1][off+byte+1] = (half[1][off+byte+1] & ~(m << (8-shift))) | (b << (8-shift));
            }
            mask <<= 8; l <<= 8; d <<= 8;
        }
    }
}
static void perspective_render(void)
{
    u8 camera_key[6], i, j, order_u[NUNIT];
    u8 changed;
    s16 depths[NUNIT], y, x;
#if defined(RT_CYCLES) && defined(FFT_CAPTURE)
    u32 started = BENCH_CYCLES, geometry_done, sprites_done, present_done;
#endif
    if (!pbank) { draw_rect(0, 0, 160, 100, C_WHITE); draw_text(4, 40, "missing fftmesh", F_SMALL, C_BLACK); return; }
    camera_key[0] = st.pyaw; camera_key[1] = (u8)st.ppitch; camera_key[2] = (u8)st.pside;
    camera_key[3] = st.pdist; camera_key[4] = st.unit[U_RAMZA].x; camera_key[5] = st.unit[U_RAMZA].z;
    changed = !pvalid || memcmp(camera_key, pold, 6);
    ZB(15);
    if (changed) {
        pgeometry(); memcpy(pold, camera_key, 6); pvalid = 1;
    } else memcpy(half, PCACHE, sizeof half);
    ZE(15);
#if defined(RT_CYCLES) && defined(FFT_CAPTURE)
    geometry_done = BENCH_CYCLES;
#endif
    for (i = 0; i < NUNIT; i++) {
        const u8 *frame = unit_frame((u8)(4-((st.pyaw+8)>>4)) & 3, i);
        if (PFRAMEKEY[i] != frame) changed = 1;
        PFRAMEKEY[i] = frame;
    }
    if (!changed) {
        memcpy(rt_light, PFRAME, 3000); memcpy(rt_dark, PFRAME+3000, 3000);
#if defined(RT_CYCLES) && defined(FFT_CAPTURE)
        sprites_done = geometry_done; present_done = BENCH_CYCLES;
#endif
        goto frame_done;
    }
    ZB(16);
    for (i = 0; i < NUNIT; i++) {
        const Unit *u = &st.unit[i];
        depths[i] = ptransform(u->x*28+14, 0, u->z*28+14).z;
        for (j = i; j && depths[order_u[j-1]] < depths[i]; j--) order_u[j] = order_u[j-1];
        order_u[j] = i;
    }
    for (i = 0; i < NUNIT; i++) psprite(order_u[i]);
    ZE(16);
#if defined(RT_CYCLES) && defined(FFT_CAPTURE)
    sprites_done = BENCH_CYCLES;
#endif
    ZB(17);
    for (y = 0; y < 50; y++) for (x = 0; x < 10; x++) {
        u16 l = dbl[half[0][y*10+x]], d = dbl[half[1][y*10+x]];
        u8 *dl = rt_light + y*60, *dd = rt_dark + y*60;
        WR16(dl, x, l); WR16(dl+30, x, l);
        WR16(dd, x, d); WR16(dd+30, x, d);
    }
    ZE(17);
#if defined(RT_CYCLES) && defined(FFT_CAPTURE)
    present_done = BENCH_CYCLES;
#endif
    draw_rect(0, 0, 160, 8, C_WHITE);
    draw_text(2, 0, st.perspective == 1 ? "1 FIXED  MODE:next" : st.perspective == 2 ? "2 LATERAL  ALPHA:look" : "3 ORBIT  F1/F5:zoom", F_SMALL, C_BLACK);
    draw_rect(0, 93, 160, 7, C_WHITE);
    draw_text(2, 93, "Ramza  |  SHIFT:back  ENTER:reset", F_SMALL, C_BLACK);
    memcpy(PFRAME, rt_light, 3000); memcpy(PFRAME+3000, rt_dark, 3000);
frame_done:
#ifdef RT_CYCLES
    BENCH_VALUE(0x70000000UL | pfaces);
#endif
#if defined(RT_CYCLES) && defined(FFT_CAPTURE)
    { u32 ended = BENCH_CYCLES;
      BENCH_VALUE(0x72000000UL | rt_frame);
      BENCH_VALUE(geometry_done-started); BENCH_VALUE(sprites_done-geometry_done);
      BENCH_VALUE(present_done-sprites_done); BENCH_VALUE(ended-started);
      BENCH_SHOT(rt_dark); }
#endif
}
#else
#include "fixed.inc"
#ifdef FFT_CINEMA
#include "cinema.inc"
#endif
#endif
