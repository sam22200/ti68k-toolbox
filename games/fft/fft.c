// Final Fantasy Tactics, Magic City Gariland: isometric tech demo (README.md for the design).
// World tiles (x, z) are rotated into view tiles (u, v) for each of the four orientations; the
// scene of each orientation is the game's own textured map drawn on the PC (tools/extract.py)
// with the depth of its pixels, four ZX0-packed data files read in place. The current one is
// unpacked (during the turn that reaches it) and copied into a buffer larger than the screen
// (the reach rings are drawn there); every frame copies the camera's window and draws the
// units through a cover mask made from that depth.
// The rotation itself is animated with polygons projected at intermediate angles, each face in
// the mean grey of its pixels in the views (map.h).
#include "fft.h"
#include <stdlib.h>
#include <string.h>

#ifdef RT_CYCLES                        // zones under ti-cycles: 3 view copy, 4 units, 5 cursor
#include "../../tools/m68kbench/bench.h" // and HUD, 6 scene, 7 rotation frame, 8 view unpacking
#define ZB(n) BENCH_BEGIN(n)
#define ZE(n) BENCH_END(n)
#else
#define ZB(n)
#define ZE(n)
#endif

// Planes are big-endian 16-bit words (MSB = leftmost pixel): native on the TI, composed on
// the PC. RMW16: word = word & ~k | v.
#ifdef __m68k__
#define RD16(p, i) (((const u16 *)(p))[i])
#define WR16(p, i, v) (((u16 *)(p))[i] = (v))
#define RD32(p, i) (*(const u32 *)((const u8 *)(p) + 2 * (i)))
#define WR32(p, i, v) (*(u32 *)((u8 *)(p) + 2 * (i)) = (v))
#define RMW16(p, k, v) (*(u16 *)(p) = (*(u16 *)(p) & ~(k)) | (v))
// 16 x 16 -> 32 multiply and 32 / 16 division in one instruction (ti68k-performance.md §4,
// verified): plain C widens shared shorts and calls __mulsi3 / __divsi3
static inline long muls16(short a, short b)
{ long r; asm("muls.w %2,%0" : "=d"(r) : "0"(a), "dmi"(b)); return r; }
static inline short divs32_16(long n, short d)
{ asm("divs.w %1,%0" : "+d"(n) : "dmi"(d)); return (short)n; }
#else
#define RD16(p, i) ((u16)((p)[2 * (i)] << 8 | (p)[2 * (i) + 1]))
#define WR16(p, i, v) ((p)[2 * (i)] = (u8)((v) >> 8), (p)[2 * (i) + 1] = (u8)(v))
#define RD32(p, i) ((u32)RD16(p, i) << 16 | RD16(p, (i) + 1))
#define WR32(p, i, v) (WR16(p, i, (u16)((v) >> 16)), WR16(p, (i) + 1, (u16)(v)))
#define RMW16(p, k, v) do { u16 o_ = (u16)((p)[0] << 8 | (p)[1]); o_ = (u16)((o_ & ~(k)) | (v)); \
                            (p)[0] = (u8)(o_ >> 8); (p)[1] = (u8)o_; } while (0)
static long muls16(short a, short b) { return (long)a * b; }
static short divs32_16(long n, short d) { return (short)(n / d); }
#endif

#ifdef __m68k__
#include "../../lib/unpack68k.h"         // zx0_asm: ~42-73 cycles per output byte
#define zx0_unpack zx0_asm
#else
// ZX0 v2 (einar-saukas/ZX0), the PC's copy of lib/unpack68k.s (experiments/compress/unpack.c)
static int zx0_bit(const u8 **s, u16 *bb)
{
    u16 t;
    if (*bb == 0x8000) *bb = (u16)(*(*s)++ << 8 | 0x80);
    t = *bb; *bb = (u16)(*bb << 1);
    return (t & 0x8000) != 0;
}
static u16 zx0_gamma(const u8 **s, u16 *bb)
{
    u16 x = 1;
    while (!zx0_bit(s, bb)) { x += x; if (zx0_bit(s, bb)) x++; }
    return x;
}
static u8 *zx0_unpack(const u8 *s, u8 *d)
{
    u16 dist = 1, n, bb = 0x8000;
    for (;;) {
        for (n = zx0_gamma(&s, &bb); n; n--) *d++ = *s++;     // literals
        if (!zx0_bit(&s, &bb)) {                                // repeat the last offset
            for (n = zx0_gamma(&s, &bb); n; n--, d++) *d = d[-dist];
            if (!zx0_bit(&s, &bb)) continue;
        }
        for (;;) {                                              // new offset
            u16 lo;
            n = 1;
            while (!zx0_bit(&s, &bb)) { n += n; if (!zx0_bit(&s, &bb)) n++; }
            if (n == 256) return d;
            lo = *s++;
            dist = (u16)((n << 7) - (lo >> 1));
            n = 1;
            if (!(lo & 1)) do { n += n; if (zx0_bit(&s, &bb)) n++; } while (!zx0_bit(&s, &bb));
            for (n++; n; n--, d++) *d = d[-dist];
            if (!zx0_bit(&s, &bb)) break;
        }
    }
}
#endif

State st;
u8 *scene_l, *scene_d;
u8 scene_rot;
// the four views (fftv0..3, ZX0, read in place), unpacked one at a time into vbuf: light and
// dark planes, then twice (the frontmost pixels of each byte, then the next ones) their view
// diagonal and their mask (SC_PLANE each)
#define VIEW_BYTES (6UL * SC_PLANE)          // 52,320: above a 16-bit int
static const u8 *views[4];
static const u8 *ugfx;                  // the units' frames (fftu, units.h), read in place
static u8 *vbuf, vbuf_rot;

// Work memory, allocated with the scene (globals and statics are stored in the program file
// on the TI: -mno-bss, and the TI-89 refuses programs above 24 KB)
#define TW (RT_W / 2)                   // the rotation frames: half resolution
#define TH (RT_H / 2)
#define COVER_H UNIT_SH                // the shadow lies inside the sprite's box
#define GLINTS 250                      // glints per phase at most
typedef struct {
    s16 span_l[TH + 8], span_r[TH + 8];
    u8 queue[MAP_W * MAP_H], from[MAP_W * MAP_H];
    u8 half[2][TW / 8 * TH];
    u16 dbl[256];                       // byte -> its bits doubled
    u8 order[MAP_W * MAP_H], count[256];
    struct { s16 x, y; } glint[4][GLINTS];  // the water's glints of each phase (scene pixels)
    u8 glints[4];
    s16 key[MAP_W * MAP_H], gx[MAP_W + 1][MAP_H + 1], gy[MAP_W + 1][MAP_H + 1];
} Work;
static Work *W;
#define span_l (W->span_l)
#define span_r (W->span_r)
#define queue (W->queue)
#define bfs_from (W->from)
#define half (W->half)
#define dbl (W->dbl)
#define order (W->order)
#define key (W->key)
#define gx (W->gx)
#define gy (W->gy)

// ---------------------------------------------------------------- the map in four orientations
u8 view_w(u8 rot) { return rot & 1 ? MAP_H : MAP_W; }
u8 view_h(u8 rot) { return rot & 1 ? MAP_W : MAP_H; }

void view_of(u8 rot, s16 x, s16 z, s16 *u, s16 *v)
{
    switch (rot) {
    case 0: *u = x; *v = z; break;
    case 1: *u = MAP_H - 1 - z; *v = x; break;
    case 2: *u = MAP_W - 1 - x; *v = MAP_H - 1 - z; break;
    default: *u = z; *v = MAP_W - 1 - x; break;
    }
}

void world_of(u8 rot, s16 u, s16 v, s16 *x, s16 *z)
{
    switch (rot) {
    case 0: *x = u; *z = v; break;
    case 1: *x = v; *z = MAP_H - 1 - u; break;
    case 2: *x = MAP_W - 1 - u; *z = MAP_H - 1 - v; break;
    default: *x = MAP_W - 1 - v; *z = u; break;
    }
}

const Tile *tile_at(s16 x, s16 z)
{
    if ((u16)x >= MAP_W || (u16)z >= MAP_H) return RT_NULL;
    return &map_tiles[z * MAP_W + x];
}

// view corner c of a tile is world corner (c - rot) & 3 (both numbered (0,0) (1,0) (1,1) (0,1))
u8 corner_h(u8 rot, s16 u, s16 v, u8 c)
{
    s16 x, z;
    if ((u16)u >= view_w(rot) || (u16)v >= view_h(rot)) return 0;
    world_of(rot, u, v, &x, &z);
    return map_tiles[z * MAP_W + x].c[(c - rot) & 3];
}

s16 scene_x(u8 rot, s16 u, s16 v) { return (s16)((u - v + view_h(rot)) * HW); }
s16 scene_y(s16 u, s16 v) { return (s16)(SC_OY + (u + v) * HH); }

u8 unit_at(s16 x, s16 z)
{
    u8 i;
    for (i = 0; i < NUNIT; i++)
        if (st.unit[i].x == x && st.unit[i].z == z) return i + 1;
    return 0;
}

// ---------------------------------------------------------------- drawing targets
// Every drawing call takes scene coordinates; the target maps them to its own pixels (origin
// ox, oy) and clips to pixels [x0, x1) and rows [y0, y1).
typedef struct { u8 *l, *d; s16 stride, ox, oy, x0, x1, y0, y1; } Dst;
static Dst D;

static void dst_scene(void)
{
    D.l = scene_l; D.d = scene_d; D.stride = SC_BYTES; D.ox = D.oy = 0;
    D.x0 = 0; D.x1 = SC_W; D.y0 = 0; D.y1 = SC_H;
}

static void dst_screen(s16 camx, s16 camy)
{
    D.l = rt_light; D.d = rt_dark; D.stride = RT_PBYTES; D.ox = camx; D.oy = camy;
    D.x0 = 0; D.x1 = RT_W; D.y0 = 0; D.y1 = RT_H;
}

// A masked sprite of up to 24 columns (u32 rows, bit 31 = column 0): dest = dest & ~m | src & m,
// written as three 16-bit words per plane and row, each clipped
static void blit24(s16 x, s16 y, s16 rows, const u32 *l, const u32 *d, const u32 *m)
{
    s16 w, sh, r = 0, k;
    u16 ck[3];
    u8 *pl, *pd;
    x -= D.ox; y -= D.oy;
    if (x >= D.x1 || x + 24 <= D.x0) return;
    if (y < D.y0) { r = D.y0 - y; y = D.y0; }
    if (y + rows - r > D.y1) rows = D.y1 - y + r;
    if (r >= rows) return;
    w = x >> 4; sh = x & 15;
    for (k = 0; k < 3; k++) {
        s16 lo = (w + k) << 4, a = lo < D.x0 ? D.x0 : lo, b = lo + 16 > D.x1 ? D.x1 : lo + 16;
        ck[k] = a < b ? (u16)((0xFFFFU >> (a - lo)) & (0xFFFFU << (lo + 16 - b))) : 0;
    }
    pl = D.l + y * D.stride + 2 * w; pd = D.d + y * D.stride + 2 * w;
    for (; r < rows; r++, pl += D.stride, pd += D.stride) {
        u32 mm = m[r] >> sh, ll = l[r] >> sh, dd = d[r] >> sh;
        u16 k0 = ck[0] & (u16)(mm >> 16), k1 = ck[1] & (u16)mm;
        if (k0) { RMW16(pl, k0, (u16)(ll >> 16) & k0); RMW16(pd, k0, (u16)(dd >> 16) & k0); }
        if (k1) { RMW16(pl + 2, k1, (u16)ll & k1); RMW16(pd + 2, k1, (u16)dd & k1); }
        if (sh > 8) {
            u16 k2 = ck[2] & (u16)(m[r] << (16 - sh));
            if (k2) {
                RMW16(pl + 4, k2, (u16)(l[r] << (16 - sh)) & k2);
                RMW16(pd + 4, k2, (u16)(d[r] << (16 - sh)) & k2);
            }
        }
    }
}

// Convex polygon (n <= 4 points, target coordinates) filled with an 8x8 pattern aligned to the
// target. Pixel (x, y) is inside when its centre is. Spans in 16-bit words.

static const u16 lmask[16] = { 0xFFFF, 0x7FFF, 0x3FFF, 0x1FFF, 0x0FFF, 0x07FF, 0x03FF, 0x01FF,
    0x00FF, 0x007F, 0x003F, 0x001F, 0x000F, 0x0007, 0x0003, 0x0001 };   // pixels >= x & 15

// rows ymin.. of the target from span_l/span_r, an 8 x 8 pattern of 16-bit rows aligned to
// the target's pixels (the rotation frames)
static void fill_spans(s16 ymin, s16 rows, const u16 *pl, const u16 *pd)
{
    u8 *ql = D.l + ymin * D.stride, *qd = D.d + ymin * D.stride;
    const s16 *sa = span_l, *sb = span_r;
    s16 stride = D.stride, py8 = (ymin + D.oy) & 7;
    for (; rows > 0; rows--, ql += stride, qd += stride, py8 = (py8 + 1) & 7) {
        s16 xa = *sa++, xb = *sb++, n;
        u16 rl, rd, k, kr;
        u8 *al, *ad;
        if (xa < D.x0) xa = D.x0;
        if (xb > D.x1) xb = D.x1;
        if (xa >= xb) continue;
        rl = pl[py8]; rd = pd[py8];
        al = ql + ((xa >> 3) & ~1); ad = qd + ((xa >> 3) & ~1);
        k = lmask[xa & 15];
        kr = (u16)(0xFFFFU << (15 - ((xb - 1) & 15)));
        n = ((xb - 1) >> 4) - (xa >> 4);
        if (!n) k &= kr;
        RMW16(al, k, rl & k); RMW16(ad, k, rd & k);
        if (!n) continue;
        for (al += 2, ad += 2; --n > 0; al += 2, ad += 2) { WR16(al, 0, rl); WR16(ad, 0, rd); }
        RMW16(al, kr, rl & kr); RMW16(ad, kr, rd & kr);
    }
}

static void fill_poly(const s16 *px, const s16 *py, u8 n, const u16 *pl, const u16 *pd)
{
    s16 ymin = 32767, ymax = -32768, y, i, rows;
    for (i = 0; i < n; i++) {
        s16 yy = py[i] - D.oy;
        if (yy < ymin) ymin = yy;
        if (yy > ymax) ymax = yy;
    }
    if (ymin < D.y0) ymin = D.y0;
    if (ymax > D.y1) ymax = D.y1;
    rows = ymax - ymin;
    if (rows <= 0) return;
    {
        s16 *a = span_l, *b = span_r;
        for (y = rows; y > 0; y--) { *a++ = 32767; *b++ = -32768; }
    }
    for (i = 0; i < n; i++) {
        s16 j = i + 1 < n ? i + 1 : 0;
        s16 x0 = px[i] - D.ox, y0 = py[i] - D.oy, x1 = px[j] - D.ox, y1 = py[j] - D.oy;
        s16 ya, yb, *sl, *sr, dx8;
        s32 x, dx;
        if (y0 == y1) continue;
        if (y0 > y1) { s16 t = x0; x0 = x1; x1 = t; t = y0; y0 = y1; y1 = t; }
        ya = y0 < ymin ? ymin : y0;
        yb = y1 > ymax ? ymax : y1;
        if (ya >= yb) continue;
        dx8 = divs32_16((s32)(x1 - x0) << 8, y1 - y0);            // 8.8 slope: |dx| < 128
        dx = (s32)dx8 << 8;
        x = ((s32)x0 << 16) + (muls16(dx8, (s16)(2 * (ya - y0) + 1)) << 7) + 0x8000;  // row centre
        sl = span_l + (ya - ymin); sr = span_r + (ya - ymin);
        for (y = yb - ya; y > 0; y--, x += dx, sl++, sr++) {
            s16 xi = (s16)(x >> 16);
            if (xi < *sl) *sl = xi;
            if (xi > *sr) *sr = xi;
        }
    }
    fill_spans(ymin, rows, pl, pd);
}

static u8 reach_hi(u8 world) { return st.mode == M_TARGET && st.reach[world] != 0xFF; }

// pixels of the scene in front of view diagonal depth inside a box (byte columns, rows):
// restored from the view (a reach ring drawn behind a building), but for a pattern of them:
// a hidden ring stays visible over the building, set apart from the seen ones
static const u8 hide_pat[2] = { 0xCC, 0x33 };   // 2-pixel dashes (picked over the whole ring
                                                // and a checkerboard)
static void uncover(s16 x, s16 y, s16 w, s16 rows, u8 depth)
{
    const u8 *vl = vbuf, *vd = vl + SC_PLANE, *t1 = vd + SC_PLANE, *m1 = t1 + SC_PLANE;
    const u8 *t2 = m1 + SC_PLANE, *m2 = t2 + SC_PLANE;
    s16 b0 = x >> 3, b1 = (x + w - 1) >> 3, r, b;
    if (b0 < 0) b0 = 0;
    if (b1 >= SC_BYTES) b1 = SC_BYTES - 1;
    if (y < 0) { rows += y; y = 0; }
    if (y + rows > SC_H) rows = SC_H - y;
    for (r = 0; r < rows; r++) {
        u16 o = (u16)((y + r) * SC_BYTES + b0);
        for (b = b0; b <= b1; b++, o++) {
            u8 m = ((t1[o] > depth ? m1[o] : 0) | (t2[o] > depth ? m2[o] : 0)) & ~hide_pat[(y + r) & 1];
            if (!m) continue;
            scene_l[o] = (u8)((scene_l[o] & ~m) | (vl[o] & m));
            scene_d[o] = (u8)((scene_d[o] & ~m) | (vd[o] & m));
        }
    }
}

// The scene of orientation rot with the current reach rings: the view copied from its file,
// then a dotted ring on each reachable tile, behind what stands in front of it
static u8 scene_seq;                    // hl_seq it was made for
static u8 hl_seq;                       // bumped when the highlights may have changed

static void unpack_view(u8 rot)
{
    if (vbuf_rot == rot) return;
    zx0_unpack(views[rot], vbuf);
    vbuf_rot = rot;
}

// is scene pixel (x, y) in front of everything nearer than view diagonal depth (not covered)?
static u8 seen(s16 x, s16 y, u8 depth)
{
    const u8 *t1 = vbuf + 2 * SC_PLANE, *m1 = t1 + SC_PLANE, *t2 = m1 + SC_PLANE, *m2 = t2 + SC_PLANE;
    u16 o = (u16)(y * SC_BYTES + (x >> 3));
    u8 b = (u8)(0x80 >> (x & 7));
    return !((t1[o] > depth && m1[o] & b) || (t2[o] > depth && m2[o] & b));
}

// The water's glints of orientation rot: 2-pixel dashes on a lattice of the scene's pixels
// (every other row, one even column in 8, staggered by row) that slides 2 px right each phase
// (a current), on the water tiles' tops where the view shows them (not under a bank or a roof)
static void make_glints(u8 rot)
{
    u8 i;
    memset(W->glints, 0, sizeof W->glints);
    for (i = 0; i < MAP_W * MAP_H; i++) {
        s16 u, v, x, y, r, px;
        if (!map_tiles[i].water) continue;
        view_of(rot, i % MAP_W, i / MAP_W, &u, &v);
        x = scene_x(rot, u, v) - HW; y = scene_y(u, v) - map_tiles[i].stand * (HU / 2);
        for (r = 1; r < 11; r++) {
            s16 py = y + r, k = r < 6 ? r : 11 - r;
            if ((py & 1) || (u16)py >= SC_H) continue;
            for (px = (x + 10 - 2 * k + 1) & ~1; px + 1 < x + 14 + 2 * k; px += 2) {
                u8 ph = (u8)(((px >> 1) + 3 * (py >> 1)) & 3), n;
                if (px < 0 || px + 1 >= SC_W) continue;
                n = W->glints[ph];
                if (n >= GLINTS || !seen(px, py, (u8)(u + v)) || !seen(px + 1, py, (u8)(u + v))) continue;
                W->glint[ph][n].x = px; W->glint[ph][n].y = py;
                W->glints[ph] = (u8)(n + 1);
            }
        }
    }
}

// this frame's glints on the screen, white (a phase every 8 frames)
static void draw_glints(void)
{
    u8 ph = (u8)((st.tick >> 3) & 3), n = W->glints[ph], i;
    for (i = 0; i < n; i++) {
        s16 x = W->glint[ph][i].x - st.camx, y = W->glint[ph][i].y - st.camy, k;
        if ((u16)y >= RT_H || x < 0 || x + 1 >= RT_W) continue;
        for (k = x; k <= x + 1; k++) {
            u16 o = (u16)(y * RT_PBYTES + (k >> 3));
            u8 b = (u8)~(0x80 >> (k & 7));
            ((u8 *)rt_light)[o] &= b; ((u8 *)rt_dark)[o] &= b;
        }
    }
}

static void ensure_scene(u8 rot)
{
    u8 i;
    if (scene_rot == rot && scene_seq == hl_seq) return;
    unpack_view(rot);
    if (scene_rot != rot) make_glints(rot);
    memcpy(scene_l, vbuf, 2 * SC_PLANE);
    scene_rot = rot; scene_seq = hl_seq;
    dst_scene();
    for (i = 0; i < MAP_W * MAP_H; i++) {
        s16 u, v, x, y;
        if (!reach_hi(i)) continue;
        view_of(rot, i % MAP_W, i / MAP_W, &u, &v);
        x = scene_x(rot, u, v) - HW; y = scene_y(u, v) - map_tiles[i].stand * (HU / 2);
        blit24(x, y, 12, hi_l, hi_d, hi_m);
        uncover(x, y, 24, 12, (u8)(u + v));
    }
}

u16 scene_checksum(void)
{
    u16 a = 0, b = 0, i;
    for (i = 0; i < (u16)(SC_BYTES * SC_H); i++) {
        a = (a + scene_l[i]) % 255; b = (b + a) % 255;
        a = (a + scene_d[i]) % 255; b = (b + a) % 255;
    }
    return (u16)(b << 8 | a);
}

// ---------------------------------------------------------------- reach and paths
static const s8 dir_x[4] = { 1, -1, 0, 0 }, dir_z[4] = { 0, 0, 1, -1 };

static void bfs(u8 who, u8 *from)
{
    u8 qh = 0, qt = 0, i;
    Unit *un = &st.unit[who];
    for (i = 0; i < MAP_W * MAP_H; i++) st.reach[i] = 0xFF;
    st.reach[un->z * MAP_W + un->x] = 0;
    queue[qt++] = un->z * MAP_W + un->x;
    while (qh < qt) {
        u8 c = queue[qh++], x = c % MAP_W, z = c / MAP_W, d;
        const Tile *t = &map_tiles[c];
        if (st.reach[c] >= MOVE) continue;
        for (d = 0; d < 4; d++) {
            s16 nx = x + dir_x[d], nz = z + dir_z[d];
            const Tile *n = tile_at(nx, nz);
            u8 o, nc;
            s16 dh;
            if (!n || !n->walk) continue;
            nc = (u8)(nz * MAP_W + nx);
            if (st.reach[nc] != 0xFF) continue;
            dh = (s16)n->stand - t->stand;
            if (dh > 2 * JUMP || dh < -2 * JUMP) continue;
            o = unit_at(nx, nz);
            if (o && st.unit[o - 1].team != un->team) continue;     // enemies block, allies pass
            st.reach[nc] = st.reach[c] + 1;
            if (from) from[nc] = c;
            queue[qt++] = nc;
        }
    }
}

void compute_reach(u8 who)
{
    u8 i;
    hl_seq++;
    bfs(who, RT_NULL);
    for (i = 0; i < NUNIT; i++)                                     // can't stop on a unit
        if (i != who) st.reach[st.unit[i].z * MAP_W + st.unit[i].x] = 0xFF;
}

u8 make_path(u8 who, u8 tx, u8 tz)
{
    u8 c = tz * MAP_W + tx, n, i;
    bfs(who, bfs_from);
    n = st.reach[c];
    if (n == 0xFF || n >= PATH_MAX) { st.path_n = 0; return 0; }
    for (i = n + 1; i-- > 0;) {
        st.path[i][0] = c % MAP_W; st.path[i][1] = c / MAP_W;
        c = bfs_from[c];
    }
    st.path_n = n + 1;                                             // path[0] = the start
    compute_reach(who);
    return st.path_n;
}

// ---------------------------------------------------------------- units on screen
// a unit's feet: scene position of its tile centre at its standing height, interpolated
// while walking (t / WALK_FRAMES of the way to the next tile, with a hop on height changes);
// depth: the diagonal it is drawn after (both tiles' while walking)
typedef struct { s16 x, y; u8 depth; } Feet;

static Feet unit_feet(u8 rot, u8 i)
{
    Feet f;
    s16 u, v;
    const Unit *un = &st.unit[i];
    view_of(rot, un->x, un->z, &u, &v);
    f.x = scene_x(rot, u, v);
    f.y = scene_y(u, v) + HH - map_tiles[un->z * MAP_W + un->x].stand * (HU / 2);
    f.depth = (u8)(u + v);
    if (st.mode == M_WALK && st.sel == i && st.path_i + 1 < st.path_n) {
        const u8 *a = st.path[st.path_i], *b = st.path[st.path_i + 1];
        s16 u2, v2, x2, y2, t = st.walk_t, hop;
        s16 sa = map_tiles[a[1] * MAP_W + a[0]].stand, sb = map_tiles[b[1] * MAP_W + b[0]].stand;
        view_of(rot, a[0], a[1], &u, &v);
        view_of(rot, b[0], b[1], &u2, &v2);
        f.x = scene_x(rot, u, v); f.y = scene_y(u, v) + HH - sa * (HU / 2);
        x2 = scene_x(rot, u2, v2); y2 = scene_y(u2, v2) + HH - sb * (HU / 2);
        hop = sa != sb ? (t * (WALK_FRAMES - t)) * 2 : 0;            // up to 8 px at mid-step
        f.x += (x2 - f.x) * t / WALK_FRAMES;
        f.y += (y2 - f.y) * t / WALK_FRAMES - hop;
        f.depth = (u8)(u + v > u2 + v2 ? u + v : u2 + v2);
    }
    return f;
}

// Cover mask of a unit: the pixels in front of it (the view's depth: diagonals greater than
// its own) in its sprite's 16 x COVER_H box, one bit per pixel
static void cover(u16 *cm, s16 fx, s16 fy, u8 depth)
{
    const u8 *t1 = vbuf + 2 * SC_PLANE, *m1 = t1 + SC_PLANE, *t2 = m1 + SC_PLANE, *m2 = t2 + SC_PLANE;
    s16 bx = fx - 8, by = fy - UNIT_FOOT, b0 = bx >> 3, sh = 8 - (bx & 7), r, k;
    for (r = 0; r < COVER_H; r++) {
        s16 y = by + r;
        u32 m = 0;
        if ((u16)y < SC_H) {
            u16 o = (u16)(y * SC_BYTES + b0);
            for (k = 0; k < 3; k++, o++) {
                m <<= 8;
                if ((u16)(b0 + k) >= SC_BYTES) continue;
                if (t1[o] > depth) m |= m1[o];
                if (t2[o] > depth) m |= m2[o];
            }
        }
        cm[r] = (u16)(m >> sh);
    }
}

// FFT's battle idle (TYPE1.SEQ 6 / 7: frames 11 10 9 10 11 12 13 12 for 6 8 10 8 ticks at
// 60 Hz) and walk (8 / 9: the same frames for 2 4 6 4 ticks), in our frames (~30 per second)
static const u8 idle_seq[32] = { 2, 2, 2, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1,
    2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 3, 3, 3, 3 };
static const u8 walk_seq[16] = { 2, 1, 1, 0, 0, 0, 1, 1, 2, 3, 3, 4, 4, 4, 3, 3 };

// a unit's frame (units.h) seen from orientation rot: its facing turned into the view, front
// frames looking down-left (+v), back ones up-left (-u), mirrored for +u and -v as in FFT
static const u8 *unit_frame(u8 rot, u8 i, u8 *mirror)
{
    static const s8 fdx[4] = { 1, -1, 0, 0 }, fdz[4] = { 0, 0, 1, -1 };
    const Unit *un = &st.unit[i];
    s16 u0, v0, du, dv;
    u8 t = (u8)(st.tick + 11 * i), k;                  // the units out of step
    view_of(rot, 0, 0, &u0, &v0);
    view_of(rot, fdx[un->face], fdz[un->face], &du, &dv);
    du -= u0; dv -= v0;
    k = st.mode == M_WALK && i == st.sel ? walk_seq[t & 15] : idle_seq[t & 31];
    if (du < 0 || dv < 0) k += 5;
    *mirror = du > 0 || dv < 0;
    return ugfx + (u16)(un->gfx * UNIT_FRAMES + k) * (6 * UNIT_SH);
}

#define REV16(w) ((u16)(rev8[(u8)(w)] << 8 | rev8[(u16)(w) >> 8]))

// the opaque pixels of row r (1 .. UNIT_SH - 2) of a sprite mask whose four neighbours are opaque
static u16 erode(const u16 *mk, u8 r)
{
    u16 o = (u16)~mk[r];
    return o & (u16)(o << 1) & (o >> 1) & (u16)~mk[r - 1] & (u16)~mk[r + 1];
}

// a unit and its shadow, both through the cover mask cm (ExtGraph: dest = dest & mask | data,
// mask 1 = transparent, so the data is cleared under the cover too), the unit's contour drawn
// over the cover, an enemy's diamond above it; cm RT_NULL: the unit alone, uncovered (the
// rotation frames)
static void draw_unit_covered(s16 sx, s16 sy, u8 rot, u8 i, const u16 *cm)
{
    RtSprite s;
    u16 mk[COVER_H], ml[COVER_H], md[COVER_H], any = 0;
    u8 r, mir;
    const u8 *g = unit_frame(rot, i, &mir);
    s.w = 16;
    if (cm) {
        for (r = 0; r < 3; r++) {
            u16 c = cm[UNIT_FOOT - 1 + r];
            mk[r] = shadow_gfx[2][r] | c; ml[r] = shadow_gfx[0][r] & ~c; md[r] = shadow_gfx[1][r] & ~c;
        }
        s.h = 3;
        s.light = ml; s.dark = md; s.mask = mk;
        draw_sprite(sx - 8, sy - 1, &s);
    }
    for (r = 0; r < UNIT_SH; r++) {
        u16 l = RD16(g, r), d = RD16(g, UNIT_SH + r), k = RD16(g, 2 * UNIT_SH + r);
        if (mir) { l = REV16(l); d = REV16(d); k = REV16(k); }
        mk[r] = k; ml[r] = l; md[r] = d;
        if (cm) any |= cm[r];
    }
    if (any) {          // covered: only the contour stays (the white outline and the black
        u8 r1 = UNIT_SH - 3;    // line inside it), the sprite eroded twice and taken away,
        u16 a, b, c;            // on the covered rows only (rows 0, 1 and the last two: outline)
        for (r = 2; r < r1 && !cm[r]; r++) ;
        while (r1 > r && !cm[r1]) r1--;
        a = erode(mk, r - 1); b = erode(mk, r);
        for (; r <= r1; r++, a = b, b = c) {
            u16 h;
            c = erode(mk, r + 1);       // before mk[r] changes (it reads rows r .. r + 2)
            h = cm[r] & b & (u16)(b << 1) & (b >> 1) & a & c;
            mk[r] |= h; ml[r] &= ~h; md[r] &= ~h;
        }
    }
    s.h = UNIT_SH;
    s.light = ml; s.dark = md; s.mask = mk;
    draw_sprite(sx - 8, sy - UNIT_FOOT, &s);
    if (st.unit[i].team == TEAM_ENEMY) {        // the team at a glance: a diamond above enemies,
        s.h = FOE_H;                            // never covered (it shows a hidden one too)
        s.light = foe_gfx[0]; s.dark = foe_gfx[1]; s.mask = foe_gfx[2];
        draw_sprite(sx - 8, sy - UNIT_FOOT - FOE_H + 1, &s);
    }
}

// ---------------------------------------------------------------- the rotation animation
// 32 angles per turn: S = round(6 sqrt2 sin a * 256), view r is at 45 + 90 r degrees (index
// 4 + 8 r), where the projection equals the cardinal one exactly (6 * 256)
static const s16 sin32[32] = { 0, 424, 831, 1207, 1536, 1806, 2007, 2130, 2172, 2130, 2007,
    1806, 1536, 1207, 831, 424, 0, -424, -831, -1207, -1536, -1806, -2007, -2130, -2172, -2130,
    -2007, -1806, -1536, -1207, -831, -424 };

// Drawn at half resolution (80 x 50, every size halved, a level = 3 px) then doubled onto the
// screen: the rows are what costs (ti-cycles: ~9k per polygon at full size)

static void draw_turn(void)
{
    s16 a = (s16)(4 + 8 * st.rot + st.turn * (s16)(st.turn_t + 1) * (8 / (TURN_FRAMES + 1))) & 31;
    s16 S = sin32[a], C = sin32[(a + 8) & 31];
    s16 pvx, pvy, cu, cv, i, j, x, z;
    u8 n = 0;
    // the pivot (cursor tile centre) stays where it is on screen
    view_of(st.rot, st.cx, st.cz, &cu, &cv);
    pvx = scene_x(st.rot, cu, cv) - st.camx;
    pvy = scene_y(cu, cv) + HH - st.camy;
    memset(half, 0, sizeof half);
    D.l = half[0]; D.d = half[1]; D.stride = TW / 8; D.ox = D.oy = 0;
    D.x0 = 0; D.x1 = TW; D.y0 = 0; D.y1 = TH;
    for (z = 0; z <= MAP_H; z++)                                   // grid corners at height 0
        for (x = 0; x <= MAP_W; x++) {
            s16 x2 = 2 * (x - st.cx) - 1, z2 = 2 * (z - st.cz) - 1;
            gx[x][z] = (pvx >> 1) + (s16)((muls16(x2, C) - muls16(z2, S)) >> 9);
            gy[x][z] = (pvy >> 1) + (s16)((muls16(x2, S) + muls16(z2, C)) >> 10);
        }
    {                                           // counting sort by the centre's depth
        u8 *cnt = W->count;
        memset(cnt, 0, sizeof W->count);
        for (z = 0, i = 0; z < MAP_H; z++)
            for (x = 0; x < MAP_W; x++, i++) {
                s16 k = (gy[x][z] + gy[x + 1][z + 1] + 512) >> 2;
                key[i] = k = k < 0 ? 0 : k > 255 ? 255 : k;
                cnt[k]++;
            }
        for (i = 0, j = 0; i < 256; i++) { u8 c = cnt[i]; cnt[i] = (u8)j; j += c; }
        for (i = 0; i < MAP_W * MAP_H; i++) order[cnt[key[i]]++] = (u8)i;
        n = MAP_W * MAP_H;
    }
    for (i = 0; i < n; i++) {
        u8 o = order[i], f;
        const Tile *t = &map_tiles[o];
        s16 tx = o % MAP_W, tz = o / MAP_W, cx[4], cy[4], px[4], py[4];
        cx[0] = gx[tx][tz]; cx[1] = gx[tx + 1][tz]; cx[2] = gx[tx + 1][tz + 1]; cx[3] = gx[tx][tz + 1];
        cy[0] = gy[tx][tz]; cy[1] = gy[tx + 1][tz]; cy[2] = gy[tx + 1][tz + 1]; cy[3] = gy[tx][tz + 1];
        {                                       // off screen: box of the column down to h 0
            s16 xa = cx[0], xb = cx[0], ya = cy[0], yb = cy[0];
            u8 c;
            for (c = 1; c < 4; c++) {
                if (cx[c] < xa) xa = cx[c];
                if (cx[c] > xb) xb = cx[c];
                if (cy[c] < ya) ya = cy[c];
                if (cy[c] > yb) yb = cy[c];
            }
            if (xb < 0 || xa >= TW || yb < 0 || ya - MAXH * (HU / 2) >= TH) continue;
        }
        cy[0] -= t->c[0] * (HU / 2); cy[1] -= t->c[1] * (HU / 2);
        cy[2] -= t->c[2] * (HU / 2); cy[3] -= t->c[3] * (HU / 2);
        // faces towards the viewer: +x (corners 1, 2) if S > 0, -x (3, 0) if S < 0,
        // +z (2, 3) if C > 0, -z (0, 1) if C < 0; lit when facing left
        for (f = 0; f < 4; f++) {
            static const u8 fa[4] = { 1, 3, 2, 0 }, fb[4] = { 2, 0, 3, 1 };
            static const s8 fdx[4] = { 1, -1, 0, 0 }, fdz[4] = { 0, 0, 1, -1 };
            static const u8 mir[4][4] = { { 1, 0, 3, 2 }, { 1, 0, 3, 2 }, { 3, 2, 1, 0 }, { 3, 2, 1, 0 } };
            s16 facing = f == 0 ? S : f == 1 ? -S : f == 2 ? C : -C, lit;
            const Tile *nt;
            u8 ha, hb, na = 0, nb = 0, k;
            if (facing <= 0) continue;
            nt = tile_at(tx + fdx[f], tz + fdz[f]);
            ha = t->c[fa[f]]; hb = t->c[fb[f]];
            if (nt) { na = nt->c[mir[f][fa[f]]]; nb = nt->c[mir[f][fb[f]]]; }
            if (na >= ha && nb >= hb) continue;
            px[0] = cx[fa[f]]; py[0] = cy[fa[f]];
            px[1] = cx[fb[f]]; py[1] = cy[fb[f]];
            px[2] = cx[fb[f]]; py[2] = cy[fb[f]] + (hb > nb ? (hb - nb) * (HU / 2) : 0);
            px[3] = cx[fa[f]]; py[3] = cy[fa[f]] + (ha > na ? (ha - na) * (HU / 2) : 0);
            lit = f == 0 ? C : f == 1 ? -C : f == 2 ? -S : S;      // normal's screen x < 0
            k = t->look[1 + f] + (lit >= 0);                       // the views' grey, one
            if (k >= SHADES) k = SHADES - 1;                       // darker facing right
            fill_poly(px, py, 4, turn_pat[k][0], turn_pat[k][1]);
        }
        fill_poly(cx, cy, 4, turn_pat[t->look[0]][0], turn_pat[t->look[0]][1]);
    }
    {                                                             // doubled onto the screen
        const u8 *hl = half[0], *hd = half[1];
        u8 *dl = rt_light, *dd = rt_dark;
        for (i = 0; i < TH; i++, dl += 2 * RT_PBYTES, dd += 2 * RT_PBYTES)
            for (j = 0; j < TW / 8; j++) {
                u16 l = dbl[*hl++], d = dbl[*hd++];
                WR16(dl, j, l); WR16(dl + RT_PBYTES, j, l);
                WR16(dd, j, d); WR16(dd + RT_PBYTES, j, d);
            }
    }
   
    // units on top, at their projected feet (no occlusion while turning)
    for (i = 0; i < NUNIT; i++) {
        const Unit *un = &st.unit[i];
        s16 x2 = 2 * (un->x - st.cx), z2 = 2 * (un->z - st.cz), sx, sy;
        if (un->x >= MAP_W) continue;
        sx = pvx + (s16)((muls16(x2, C) - muls16(z2, S)) >> 8);
        sy = pvy + (s16)((muls16(x2, S) + muls16(z2, C)) >> 9) - map_tiles[un->z * MAP_W + un->x].stand * (HU / 2);
        draw_unit_covered(sx, sy, st.rot, (u8)i, RT_NULL);
    }
}

// ---------------------------------------------------------------- camera
static void cam_clamp(s16 *x, s16 *y)
{
    if (*x < 0) *x = 0;
    if (*x > SC_W - RT_W) *x = SC_W - RT_W;
    if (*y < 0) *y = 0;
    if (*y > SC_H - RT_H) *y = SC_H - RT_H;
}

static void cam_target(s16 *tx, s16 *ty)
{
    s16 u, v;
    view_of(st.rot, st.cx, st.cz, &u, &v);
    *tx = scene_x(st.rot, u, v) - RT_W / 2;
    *ty = scene_y(u, v) + HH - map_tiles[st.cz * MAP_W + st.cx].stand * (HU / 2) - RT_H / 2 - 6;
    cam_clamp(tx, ty);
}

static s16 ease(s16 c, s16 t)
{
    s16 d = (t - c) >> 2;
    if (!d) d = t > c ? 1 : t < c ? -1 : 0;
    return c + d;
}

// ---------------------------------------------------------------- game
static u8 *sbuf;

static void
#ifdef __m68k__
__attribute__((__stkparm__))
#endif
release(void)
{
    if (sbuf) free(sbuf);
    if (vbuf) free(vbuf);
    sbuf = vbuf = RT_NULL;
    scene_l = scene_d = RT_NULL;
}

void game_init(void)
{
    u16 i;
    rt_state = &st;
    rt_state_size = sizeof(st);
#ifdef __m68k__
    sbuf = vbuf = RT_NULL;
#endif
    for (i = 0; i < 4; i++) {                  // read in place (archived on the TI): fetched
        char name[6] = "fftv0";                // again each run, the archive may have moved
        name[4] = (char)('0' + i);
        views[i] = rt_file(name, RT_NULL);
    }
    ugfx = rt_file("fftu", RT_NULL);
    if (!sbuf) {                               // two blocks: AMS allocates at most ~64 KB
        vbuf = malloc(VIEW_BYTES);
        sbuf = vbuf ? malloc(2 * SC_PLANE + sizeof(Work)) : RT_NULL;
        if (!sbuf) {                           // not enough memory: a message, ESC quits
            if (vbuf) free(vbuf);
            vbuf = RT_NULL; W = RT_NULL;
            return;
        }
        W = (Work *)(sbuf + 2 * SC_PLANE);
        memset(W, 0, sizeof(Work));
        scene_l = sbuf; scene_d = sbuf + SC_PLANE;
#ifdef __m68k__
        atexit(release);
#else
        (void)release;
#endif
    }
    scene_rot = vbuf_rot = 0xFF;
    for (i = 0; i < 256; i++) {
        u16 d = 0;
        u8 b;
        for (b = 0; b < 8; b++) if (i & (0x80 >> b)) d |= 0xC000 >> (2 * b);
        dbl[i] = d;
    }
}

static void place(u8 i, u8 x, u8 z, u8 gfx, u8 team, u8 face)
{
    st.unit[i].x = x; st.unit[i].z = z; st.unit[i].gfx = gfx; st.unit[i].team = team;
    st.unit[i].face = face;
}

void game_scenario(u16 n)
{
    u8 i;
    for (i = 0; i < sizeof(st); i++) ((u8 *)&st)[i] = 0;
    if (!W) return;
    place(0, 5, 5, UG_RAMZA, TEAM_PLAYER, 2);    // facing +z: the south camera
    place(1, 7, 8, UG_DELITA, TEAM_PLAYER, 2);
    place(2, 1, 11, UG_THIEF, TEAM_ENEMY, 3);    // behind a house from the south (fft_test --find)
    place(3, 4, 5, UG_AGRIAS, TEAM_PLAYER, 2);
    st.cx = 5; st.cz = 5;
    if (n >= 1 && n <= 4) {                       // the thief behind a building, each view
        st.rot = (u8)(n - 1);
        st.cx = st.unit[2].x; st.cz = st.unit[2].z;
    } else if (n == 5) {                          // Ramza selected, his range shown
        st.mode = M_TARGET; st.sel = 0;
        compute_reach(0);
    } else if (n == 6) {                          // in the middle of a rotation
        st.turn = 1; st.turn_t = TURN_FRAMES / 2;
    } else if (n == 7) {                          // Ramza walking
        st.sel = 0;
        if (make_path(0, 7, 7)) { st.mode = M_WALK; st.path_i = 1; st.walk_t = 2; }
    }
    cam_target(&st.camx, &st.camy);
    hl_seq++;
}

static void move_cursor(s16 du, s16 dv)
{
    s16 u, v, x, z;
    view_of(st.rot, st.cx, st.cz, &u, &v);
    u += du; v += dv;
    if ((u16)u >= view_w(st.rot) || (u16)v >= view_h(st.rot)) return;
    world_of(st.rot, u, v, &x, &z);
    st.cx = (u8)x; st.cz = (u8)z;
}

u8 game_update(void)
{
    s16 tx, ty;
    u8 mode0 = st.mode;
    if (!W || !ugfx || !views[0] || !views[1] || !views[2] || !views[3]) return !input_pressed(K_ESC);
    st.tick++;
    if (st.turn) {                                 // rotating: nothing else moves the view
        if (++st.turn_t >= TURN_FRAMES) {          // done: the cursor stays where it was
            s16 u, v, px, py;
            view_of(st.rot, st.cx, st.cz, &u, &v);
            px = scene_x(st.rot, u, v) - st.camx; py = scene_y(u, v) - st.camy;
            st.rot = (st.rot + st.turn) & 3;
            st.turn = 0; st.turn_t = 0;
            view_of(st.rot, st.cx, st.cz, &u, &v);
            st.camx = scene_x(st.rot, u, v) - px; st.camy = scene_y(u, v) - py;
            cam_clamp(&st.camx, &st.camy);
        }
    } else if (input_pressed(K_F5)) { st.turn = 1; st.turn_t = 0; }
    else if (input_pressed(K_F1)) { st.turn = -1; st.turn_t = 0; }

    if (st.mode == M_WALK) {
        const u8 *a = st.path[st.path_i], *b = st.path[st.path_i + 1];
        st.unit[st.sel].face = b[0] > a[0] ? 0 : b[0] < a[0] ? 1 : b[1] > a[1] ? 2 : 3;
        if (++st.walk_t >= WALK_FRAMES) {
            st.walk_t = 0;
            st.path_i++;
            st.unit[st.sel].x = st.path[st.path_i][0];
            st.unit[st.sel].z = st.path[st.path_i][1];
            st.cx = st.unit[st.sel].x; st.cz = st.unit[st.sel].z;
            if (st.path_i + 1 >= st.path_n) st.mode = M_BROWSE;
        }
    } else if (!st.turn) {
        u32 arrows = rt_keys & (K_UP | K_DOWN | K_LEFT | K_RIGHT);
        if (input_pressed(K_UP | K_DOWN | K_LEFT | K_RIGHT)) st.rep = 0;
        if (arrows && (st.rep == 0 || (st.rep >= 8 && !((st.rep - 8) % 3)))) {
            if (arrows & K_UP) move_cursor(0, -1);
            if (arrows & K_DOWN) move_cursor(0, 1);
            if (arrows & K_LEFT) move_cursor(-1, 0);
            if (arrows & K_RIGHT) move_cursor(1, 0);
        }
        st.rep = arrows ? (u8)(st.rep < 255 ? st.rep + 1 : st.rep) : 0;
        if (input_pressed(K_A | K_ENTER)) {
            u8 o = unit_at(st.cx, st.cz), c = (u8)(st.cz * MAP_W + st.cx);
            if (st.mode == M_BROWSE && o && st.unit[o - 1].team == TEAM_PLAYER) {
                st.mode = M_TARGET; st.sel = o - 1;
                compute_reach(st.sel);
            } else if (st.mode == M_TARGET && st.reach[c] != 0xFF && st.reach[c] > 0
                       && make_path(st.sel, st.cx, st.cz)) {
                st.mode = M_WALK; st.path_i = 0; st.walk_t = 0;
            }
        } else if (input_pressed(K_ESC | K_B)) {
            if (st.mode == M_TARGET) st.mode = M_BROWSE;
            else if (input_pressed(K_ESC)) return 0;
        }
    }
    if (!st.turn) {
        cam_target(&tx, &ty);
        st.camx = ease(st.camx, tx);
        st.camy = ease(st.camy, ty);
    }
    if (st.mode != mode0) hl_seq++;
    return 1;
}

// the camera's window of the scene: 100 rows of 160 px from any pixel position, 32 bits at a
// time (a 32-bit word and the next 16 bits shifted)
static void copy_view(void)
{
    s16 y, i, sh = st.camx & 15, rs = 16 - sh;
    const u8 *sl = scene_l + st.camy * SC_BYTES + ((st.camx >> 4) << 1);
    const u8 *sd = scene_d + st.camy * SC_BYTES + ((st.camx >> 4) << 1);
    u8 *dl = rt_light, *dd = rt_dark;
    for (y = 0; y < RT_H; y++, sl += SC_BYTES, sd += SC_BYTES, dl += RT_PBYTES, dd += RT_PBYTES) {
        if (!sh) {
            for (i = 0; i < RT_W / 16; i += 2) { WR32(dl, i, RD32(sl, i)); WR32(dd, i, RD32(sd, i)); }
        } else {
            for (i = 0; i < RT_W / 16; i += 2) {
                WR32(dl, i, RD32(sl, i) << sh | RD16(sl, i + 2) >> rs);
                WR32(dd, i, RD32(sd, i) << sh | RD16(sd, i + 2) >> rs);
            }
        }
    }
}

static void draw_hud(void)
{
    char s[24];
    const char *name = "";
    u8 o = unit_at(st.cx, st.cz), h = map_tiles[st.cz * MAP_W + st.cx].stand, n = 0;
    static const char *const names[UNIT_GFX_N] = { "Ramza", "Delita", "Agrias", "Thief" };
    static const char dirs[4] = { 'S', 'W', 'N', 'E' };
    if (o) name = names[st.unit[o - 1].gfx];
    s[n++] = dirs[st.rot]; s[n++] = ' '; s[n++] = 'h';
    if (h >= 20) s[n++] = (char)('0' + h / 20);
    s[n++] = (char)('0' + (h / 2) % 10);
    if (h & 1) { s[n++] = '.'; s[n++] = '5'; }
    s[n++] = ' ';
    while (*name && n < 22) s[n++] = *name++;
    s[n] = 0;
    draw_rect(0, RT_H - 7, 4 * n + 2, 7, C_WHITE);
    draw_text(1, RT_H - 6, s, F_SMALL, C_BLACK);
}

void game_render(void)
{
    u8 ord[NUNIT], i, j;
    Feet f[NUNIT];
    s16 u, v;
    if (!W || !ugfx || !views[0] || !views[1] || !views[2] || !views[3]) {
        draw_rect(0, 0, RT_W, RT_H, C_WHITE);
        draw_text(4, 40, W ? "missing fftu, fftv0-fftv3" : "not enough memory (75 KB)", F_SMALL, C_BLACK);
        return;
    }
    if (st.turn) {                                 // the view it turns to, unpacked meanwhile
        ZB(7); draw_turn(); ZE(7);
        ZB(8); unpack_view((st.rot + st.turn) & 3); ZE(8);
        return;
    }
    ZB(6);
    ensure_scene(st.rot);
    ZE(6);
    ZB(3); copy_view(); draw_glints(); ZE(3);
    ZB(4);
    for (i = 0; i < NUNIT; i++) {                  // units back to front
        f[i] = unit_feet(st.rot, i);
        for (j = i; j > 0 && f[ord[j - 1]].depth > f[i].depth; j--) ord[j] = ord[j - 1];
        ord[j] = i;
    }
    for (i = 0; i < NUNIT; i++) {
        u8 k = ord[i];
        u16 cm[COVER_H];
        if (st.unit[k].x >= MAP_W) continue;
        cover(cm, f[k].x, f[k].y, f[k].depth);
        draw_unit_covered(f[k].x - st.camx, f[k].y - st.camy, st.rot, k, cm);
    }
    ZE(4); ZB(5);
    // the cursor, above everything (blinking)
    dst_screen(st.camx, st.camy);
    view_of(st.rot, st.cx, st.cz, &u, &v);
    if (rt_frame & 8 || st.mode == M_TARGET)
        blit24(scene_x(st.rot, u, v) - HW, scene_y(u, v) - (map_tiles[st.cz * MAP_W + st.cx].stand >> 1) * HU,
               12, cur_l, cur_d, cur_m);
    draw_hud();
    ZE(5);
}
