/* Mode 7 - Demo 2 (David Coz, 2005): the 3D objects along the road (flat shaded triangles).
 * Reconstructed from the binary (see mode7.h). Original addresses in the comments.
 *
 * At load time the models (5 of them, up to 16 vertices and 10 triangles) are placed on the
 * track by the level's instance table into one world vertex / face list, and every face is
 * registered in the 16x16 grid of 256-unit cells its vertices fall in ("space partitioning").
 * Each frame: the cells in a cone in front of the camera, their faces (once each), their
 * vertices projected once (a per-vertex cache), and the triangles filled in 4 greys. A face
 * with a vertex behind the eye is skipped ("clipping"). World coordinates here are the
 * camera's divided by 4 (one texel of the near texture). */
#include "mode7.h"

/* one muls.w / one divs.w (optimisation: GCC calls __mulsi3 / the ROM for long operands) */
static inline long muls16(short a, short b)
{ long r; asm("muls.w %2,%0" : "=d"(r) : "0"(a), "dmi"(b)); return r; }
static inline short divs32_16(long n, short d)      /* the quotient must fit 16 bits */
{ asm("divs.w %1,%0" : "+d"(n) : "dmi"(d)); return (short)n; }

/* 0x2872: sizes from the level table, the pointers to the camera, the buffers; 0 if out of memory */
short Init3D(World *w, short level, short *px, short *py, unsigned char *pangle)
{
    w->nobj = level_info[level][0];
    w->nfaces = level_info[level][2];
    w->nverts = level_info[level][1];
    w->px = px;
    w->py = py;
    w->pangle = pangle;
    w->focal = 43;
    w->cam_z = 20;
    w->cx = 64;
    w->cy = 40;
    w->level = level;                     /* added: the original never sets it (0 on the stack) */
    w->verts_h = w->faces_h = w->cell_faces_h = w->drawn_h = w->cache_h = H_NULL;
    if (!(w->verts_h = HeapAlloc(w->nverts * 6))) return 0;
    if (!(w->faces_h = HeapAlloc((long)w->nfaces << 3))) return 0;
    if (!(w->cell_faces_h = HeapAlloc(0x1400))) return 0;   /* sic: 160 cells of 16 faces, not 256 */
    if (!(w->drawn_h = HeapAlloc(w->nfaces))) return 0;
    return (w->cache_h = HeapAlloc(w->nverts * 2)) != H_NULL;
}

/* 0x3c72 */
void Free3D(World *w)
{
    if (w->cell_faces_h) HeapFree(w->cell_faces_h);
    if (w->drawn_h) HeapFree(w->drawn_h);
    if (w->cache_h) HeapFree(w->cache_h);
    if (w->verts_h) HeapFree(w->verts_h);
    if (w->faces_h) HeapFree(w->faces_h);
}

/* 0x4502: every instance's model, turned by its angle around its first vertex, moved to (x, y) */
static void BuildWorld(World *w)
{
    short nv = 0, nf = 0, i, j, k;
    for (i = 0; i < w->nobj; i++) {
        const Instance *in = &instances[w->level][i];
        const Model *m = &models[in->type];
        short ox = in->x, oy = in->y, base = nv;
        long c = cos128[in->angle], s = sin128[in->angle];
        short x0 = m->v[0].x, y0 = m->v[0].y;
        for (j = 0; j < m->nverts; j++) {
            long dx = (short)(m->v[j].x - x0), dy = (short)(m->v[j].y - y0);
            short rx = (dx * c + dy * s) >> 7;
            short ry = (dy * c - dx * s) >> 7;
            w->verts[nv].x = x0 + rx + ox;
            w->verts[nv].y = y0 + ry + oy;
            w->verts[nv].z = m->v[j].z;
            nv++;
        }
        for (k = 0; k < m->nfaces; k++) {
            w->faces[nf].v[0] = m->f[k].v[0] + base;
            w->faces[nf].v[1] = m->f[k].v[1] + base;
            w->faces[nf].v[2] = m->f[k].v[2] + base;
            w->faces[nf].color = m->f[k].color;
            nf++;
        }
    }
}

/* 0x2982: each face into the cells of its vertices (once per cell); prints the face count */
static void BuildCells(World *w)
{
    char buf[200];
    short f, k;
    memset(w->cell_count, 0, 256);
    sprintf(buf, "%d", w->nfaces);
    DrawStr(0, 0, buf, A_REPLACE);
    for (f = 0; f < w->nfaces; f++)
        for (k = 0; k <= 2; k++) {
            Vertex *v = &w->verts[w->faces[f].v[k]];
            short cell = ((long)(v->y >> 6) << 4) + (v->x >> 6);
            signed char *cnt = &w->cell_count[cell];
            if (*cnt <= 0 || w->cell_faces[((long)cell << 4) + *cnt - 1] != f) {
                w->cell_faces[((long)cell << 4) + *cnt] = f;
                (*cnt)++;
            }
        }
}

#ifdef BENCH
/* benchmark scene: n "mountains" (a vertical triangle split in two faces, dark and black, facing
 * a camera that looks towards +x), base b, height h, at (x[i], y[i]) in 3D units */
void BenchScene(World *w, short n, const short *x, const short *y, short b, short h)
{
    short i;
    for (i = 0; i < n; i++) {
        Vertex *v = w->verts + 4 * i;
        Face *f = w->faces + 2 * i;
        v[0].x = v[1].x = v[2].x = v[3].x = x[i];
        v[0].y = y[i] - b / 2; v[1].y = y[i]; v[2].y = y[i] + b / 2; v[3].y = y[i];
        v[0].z = v[2].z = v[3].z = 0; v[1].z = h;
        f[0].v[0] = 4 * i; f[0].v[1] = 4 * i + 1; f[0].v[2] = 4 * i + 3; f[0].color = 2;
        f[1].v[0] = 4 * i + 1; f[1].v[1] = 4 * i + 2; f[1].v[2] = 4 * i + 3; f[1].color = 3;
    }
    w->nverts = 4 * n;
    w->nfaces = 2 * n;
    BuildCells(w);
}
#endif

/* 0x322e (locked: see DerefAll) */
void Deref3D(World *w)
{
    w->verts = HLock(w->verts_h);
    w->faces = HLock(w->faces_h);
    w->cell_faces = HLock(w->cell_faces_h);
    w->drawn = HLock(w->drawn_h);
    w->cache = HLock(w->cache_h);
    BuildWorld(w);
    BuildCells(w);
}

/* 0x26b8: the non-empty cells of the 8x8 cells around the camera that are in view. Each cell
 * centre is turned into the camera frame (depth, side, in 1/8 cell), incrementally; a cell is
 * kept close to the eye (within 13 + depth sideways, any cell nearer than 0xC40) or ahead,
 * nearer than 0xC40 (squared), inside a cone that widens with the distance. */
static void VisibleCells(World *w)
{
    short cx = *w->px, cy = *w->py;
    short px = cx >> 4, py = cy >> 4;
    short xmin = (cx >> 8) - 4, xmax = (cx >> 8) + 4;
    short ymin = (cy >> 8) - 4, ymax = (cy >> 8) + 4;
    unsigned char a;
    short c, s, dcx, dsx, u, v, x, y;

#ifdef ORIGINAL
    memset(w->unused4d0, 0, 256);         /* never read (the optimised build drops it) */
#endif
    if (xmin < 0) xmin = 0;
    if (xmax > 15) xmax = 15;
    if (ymin < 0) ymin = 0;
    if (ymax > 15) ymax = 15;
    a = 192 - *w->pangle;
    c = cos128[a];
    s = sin128[a];
    dcx = c << 4;
    dsx = (-s) << 4;
    {
        short dx = (xmin << 4) - px + 8, dy = (ymin << 4) - py + 8;
        u = c * dx + (-s) * dy;
        v = s * dx + c * dy;
    }
    w->nvis = 0;
    for (x = xmin; x < xmax; x++) {
        short u0 = u, v0 = v;
        for (y = ymin; y < ymax; y++) {
            if (w->cell_count[((long)y << 4) + x]) {
                short depth = u >> 7, side = v >> 7, aside = side < 0 ? -side : side;
                short lim = 19, d2;
                if (depth > -5) {
                    short m = depth > 8 ? 8 : depth;
                    if (m + 13 > aside) {
                        if ((short)(depth * depth + side * side) >= 0xC40) goto next;
                        goto add;
                    }
                }
                if (depth < 0) goto next;
                d2 = depth * depth + side * side;
                if (d2 > 0xC40) goto next;
                if (d2 <= 0x7FF) {
                    lim = 21;
                    if (d2 <= 0x3FF) {
                        lim = 24;
                        if (d2 <= 0xFF) {
                            lim = 28;
                            if (d2 <= 0x63) lim = 32;
                        }
                    }
                }
                if (aside > (short)(lim * depth) >> 4) goto next;
add:
                w->vis[w->nvis][0] = x;
                w->vis[w->nvis][1] = y;
                w->nvis++;
            }
next:
            u += dsx;
            v += dcx;
        }
        u = u0 + dcx;
        v = v0 - dsx;
    }
}

/* 0x2b56: the faces of the visible cells, each once */
static void VisibleFaces(World *w)
{
    short n = 0, i, k;
    BZ(Z_CELLS); VisibleCells(w); EZ(Z_CELLS);
    BZ(Z_FACES);
    memset(w->drawn, 0, w->nfaces);
    for (i = 0; i < w->nvis; i++) {
        short x = w->vis[i][0], y = w->vis[i][1];
        short cell = ((long)y << 4) + x;
        short cnt = w->cell_count[cell];
        for (k = 0; k < cnt; k++) {
            short f = w->cell_faces[((long)cell << 4) + k];
            if (!w->drawn[f]) {
                w->drawn[f] = 1;
                w->visfaces[n++] = f;
            }
        }
    }
    w->nvisfaces = n;
    EZ(Z_FACES);
}

#ifdef ORIGINAL
/* 0x35a0: one span on both planes, colour bit 0 = light, bit 1 = dark */
static void HLine2(short xa, short xb, short y, short color, void *p0, void *p1)
{
    HLine(p0, xa, xb, y, (color & 1) ? 1 : 0);
    HLine(p1, xa, xb, y, (color & 2) ? 1 : 0);
}
#endif

/* 0x341c: flat triangle, x in 11.5 fixed point, clipped to the 128x100 view */
static void FillTri(short x0, short y0, short x1, short y1, short x2, short y2,
                         short color, unsigned char *vs)
{
    short t, s02, s12, s01, fa, fb;
#ifdef ORIGINAL
    short y, xa, xb;
#endif
    if (y1 == y0 && y2 == y0) return;
    if (x1 == x0 && x2 == x1) return;
    if (!(y0 > y1)) { t = x0; x0 = x1; x1 = t; t = y0; y0 = y1; y1 = t; }
    if (!(y0 > y2)) { t = x0; x0 = x2; x2 = t; t = y0; y0 = y2; y2 = t; }
    if (!(y1 > y2)) { t = x1; x1 = x2; x2 = t; t = y1; y1 = y2; y2 = t; }
    /* now y0 >= y1 >= y2: vertex 2 is the top one; 999 marks a horizontal edge */
    s02 = (y0 == y2) ? 999 : (short)((x0 - x2) << 5) / (short)(y0 - y2);
    s12 = (y1 == y2) ? 999 : (short)((x1 - x2) << 5) / (short)(y1 - y2);
    s01 = (y0 == y1) ? 999 : (short)((x0 - x1) << 5) / (short)(y0 - y1);
    fa = x2 << 5;                         /* the long edge 2 -> 0 */
    fb = (s12 == 999) ? x1 << 5 : x2 << 5;               /* 2 -> 1, then 1 -> 0 */
    if (y2 > 100) y2 = 100;
    if (y1 > 100) y1 = 100;
    if (y0 > 100) y0 = 100;
#ifdef ORIGINAL
    for (y = y2; y < y1; y++, fa += s02, fb += s12)
        if (y >= 0) {
            xa = fa >> 5; if (xa < 0) xa = 0; if (xa > 127) xa = 127;
            xb = fb >> 5; if (xb < 0) xb = 0; if (xb > 127) xb = 127;
            HLine2(xa, xb, y, color, vs, vs + 0xF00);
        }
    for (y = y1; y < y0; y++, fa += s02, fb += s01)
        if (y >= 0) {
            xa = fa >> 5; if (xa < 0) xa = 0; if (xa > 127) xa = 127;
            xb = fb >> 5; if (xb < 0) xb = 0; if (xb > 127) xb = 127;
            HLine2(xa, xb, y, color, vs, vs + 0xF00);
        }
#else
    /* optimised: the scanline loops in asm (TriSpans: the span on both planes at once, the
     * edges stepped in registers), the same spans as the original's C loop calling HLine twice */
    {
        long e;
        color = ((color & 1) << 1) | ((color >> 1) & 1);   /* dark plane first (mode7.h) */
        BZ(Z_SPANS); e = TriSpans(vs, y2, y1, fa, fb, s02, s12, color);
        TriSpans(vs, y1, y0, e >> 16, e, s02, s01, color); EZ(Z_SPANS);
    }
#endif
}

#ifdef BENCH
/* reference: the C scanline loop that TriSpans replaced (bench scenario 4 compares them) */
void FillTriRef(short x0, short y0, short x1, short y1, short x2, short y2, short color, unsigned char *vs)
{
    short t, y, s02, s12, s01, fa, fb, xa, xb;
#if DARK_FIRST
    color = ((color & 1) << 1) | ((color >> 1) & 1);
#endif
    if (y1 == y0 && y2 == y0) return;
    if (x1 == x0 && x2 == x1) return;
    if (!(y0 > y1)) { t = x0; x0 = x1; x1 = t; t = y0; y0 = y1; y1 = t; }
    if (!(y0 > y2)) { t = x0; x0 = x2; x2 = t; t = y0; y0 = y2; y2 = t; }
    if (!(y1 > y2)) { t = x1; x1 = x2; x2 = t; t = y1; y1 = y2; y2 = t; }
    s02 = (y0 == y2) ? 999 : (short)((x0 - x2) << 5) / (short)(y0 - y2);
    s12 = (y1 == y2) ? 999 : (short)((x1 - x2) << 5) / (short)(y1 - y2);
    s01 = (y0 == y1) ? 999 : (short)((x0 - x1) << 5) / (short)(y0 - y1);
    fa = x2 << 5;
    fb = (s12 == 999) ? x1 << 5 : x2 << 5;
    if (y2 > 100) y2 = 100;
    if (y1 > 100) y1 = 100;
    if (y0 > 100) y0 = 100;
    for (y = y2; y < y1; y++, fa += s02, fb += s12)
        if (y >= 0) {
            xa = fa >> 5; if (xa < 0) xa = 0; if (xa > 127) xa = 127;
            xb = fb >> 5; if (xb < 0) xb = 0; if (xb > 127) xb = 127;
            Span2(vs, xa, xb, y, color);
        }
    for (y = y1; y < y0; y++, fa += s02, fb += s01)
        if (y >= 0) {
            xa = fa >> 5; if (xa < 0) xa = 0; if (xa > 127) xa = 127;
            xb = fb >> 5; if (xb < 0) xb = 0; if (xb > 127) xb = 127;
            Span2(vs, xa, xb, y, color);
        }
}
void FillTriNew(short x0, short y0, short x1, short y1, short x2, short y2, short color, unsigned char *vs)
{ FillTri(x0, y0, x1, y1, x2, y2, color, vs); }
#endif

/* 0x2c94: project the vertices of the visible faces (once each) and fill the triangles */
static void DrawFaces(World *w, unsigned char *vs)
{
    short xs[3], ys[3];
    short camx = *w->px >> 2, camy = *w->py >> 2, focal = w->focal;
    short c = cos128[*w->pangle], s = sin128[*w->pangle];
    short i, k, nproj = 0;

    for (i = 0; i < w->nverts; i++) w->cache[i] = -1;
    for (i = 0; i < w->nvisfaces; i++) {
        short f = w->visfaces[i], clipped = 0;
        for (k = 0; k <= 2; k++) {
            short vi = w->faces[f].v[k], p = w->cache[vi], cl;
            if (p >= 0) {
                xs[k] = w->proj[p].x;
                ys[k] = w->proj[p].y;
                cl = w->clip[p];
            } else {
#ifdef ORIGINAL
                long dx = (short)(w->verts[vi].x - camx), dy = (short)(w->verts[vi].y - camy);
                long dz = (short)(w->verts[vi].z - w->cam_z);
                long side = (c * dx + s * dy) >> 7;
                long depth = (c * dy - s * dx) >> 7;
                w->clip[nproj] = 0;
                if (depth > 10) {
                    w->proj[nproj].x = w->cx + (short)(side * focal / depth);
                    w->proj[nproj].y = w->cy - (short)(dz * focal / depth);
#else
                /* optimised: 16-bit operands (3D coordinates < 1024, |side| and depth < 4096), so
                 * muls.w / divs.w give the original's long results (quotient < 16012); the
                 * original calls __mulsi3 4 times and the ROM's 32-bit division twice */
                short dx = w->verts[vi].x - camx, dy = w->verts[vi].y - camy;
                short dz = w->verts[vi].z - w->cam_z;
                short side = (muls16(c, dx) + muls16(s, dy)) >> 7;
                short depth = (muls16(c, dy) - muls16(s, dx)) >> 7;
                w->clip[nproj] = 0;
                if (depth > 10) {
                    w->proj[nproj].x = w->cx + divs32_16(muls16(side, focal), depth);
                    w->proj[nproj].y = w->cy - divs32_16(muls16(dz, focal), depth);
#endif
                } else {
                    w->clip[nproj] = 1;   /* behind the eye */
                    w->proj[nproj].x = 77;
                    w->proj[nproj].y = 0;
                }
                xs[k] = w->proj[nproj].x;
                ys[k] = w->proj[nproj].y;
                cl = w->clip[nproj];
                w->cache[vi] = nproj++;
            }
            if (cl) { clipped = 1; break; }
        }
        if (!clipped) {
            BZ(Z_FILL);
            FillTri(xs[0], ys[0], xs[1], ys[1], xs[2], ys[2], w->faces[f].color, vs);
            EZ(Z_FILL);
        }
    }
}

/* 0x3692 */
void Render3D(World *w, unsigned char *vs)
{
    VisibleFaces(w);
    BZ(Z_PROJECT); DrawFaces(w, vs); EZ(Z_PROJECT);
}

