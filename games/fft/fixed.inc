// Native fixed shoulder: geometry and depth -> two backgrounds and per-actor masks.
// Lazy masked poses are byte-aligned draw commands; the animation loop has no Z tests.
typedef struct { s16 x, y; u16 mask; u8 xb, top, bytes, rows, w, h, q; } FRect;
typedef struct { FRect rect[NUNIT]; u16 glint[4][128]; u8 count[4], actors[NUNIT], valid; } FView;
typedef struct {
    u16 frame, off;
#ifdef FFT_CINEMA
    u8 bank, flash;
#endif
} FSlot;
typedef struct {
    FView view[2]; FSlot slot[2][NUNIT][5]; u16 masks, used, misses, resets;
#ifdef FFT_CINEMA
    PVertex feet[NUNIT];
    u8 positioned, textured;
#endif
} FWork;
typedef char FixedWorkFitsStatus[sizeof(FWork) <= sizeof(W->status) ? 1 : -1];
#define FW ((FWork *)W->status)
#define FMASK (vbuf + 40000)
#define FPOOL vbuf
#define FPOOL_BYTES 24000
#define FMASK_BYTES 12320
#ifdef FFT_CINEMA
static void cpositions(u8 side);
static void chud(void);
static void cweapon(u8 side, u8 who);
static void cprojectile(void);
static u8 ctexture(u8 side);
#endif

// Camera review prototype: actual winning pixels select cutaway, never a box's proximity.
// This reader only locates components. Their archived bounds are not a visibility rule.
static void fixed_cutaway(const u8 *src)
{
    pgroup = pegroup = RT_NULL;
    memset(PHIDDEN, st.psolid == 2 ? 1 : 0, 128); PHIDDEN[0] = 0;
    if (RD16(pbank, 2) != 4) return;
    pgroup = src+2+RD16(src, 0)*12; pegroup = pgroup+RD16(pbank, 1);
}

// Require three real foreground samples from the same architectural component.
// Floor/canal pixels, sky, peripheral objects and scenery behind the target do not count.
static void fixed_probe(s16 cx, s16 cy, s16 sx, s16 sy, u16 limit)
{
    s16 x, y;
    memset(PCOUNTS, 0, 128);
    for (y = -1; y <= 1; y++) for (x = -1; x <= 1; x++) {
        s16 px = cx+x*sx, py = cy+y*sy;
        u16 off;
        u8 group;
        if ((u16)px >= 160 || py < 8 || py >= 93) continue;
        off = py*160+px; group = POWNER[off];
        if (group && PDEPTH[off] > limit && ++PCOUNTS[group] >= 3) PHIDDEN[group] = 1;
    }
}

static u8 fixed_obstruction(void)
{
#ifdef FFT_CINEMA
    const Unit *u = &st.unit[st.cphase ? st.tgt : U_RAMZA];
    PVertex v = ptransform(u->x*28+14, map_tiles[u->z*MAP_W+u->x].stand*6, u->z*28+14);
#else
    const Unit *u = &st.unit[U_RAMZA];
    PVertex v = ptransform(u->x*28+14, map_tiles[u->z*MAP_W+u->x].stand*6, u->z*28+14);
#endif
    PScreen p;
    s16 h, w, top, step;
    u16 i;
    if (!pgroup || v.z < P_NEAR) return 0;
    p = pproject(v);
    h = (s16)((muls16(34, (s16)precip(v.z))*48) >> 15);
    if (h > 104) h = 104;
    w = (h*16)/UNIT_SH; top = p.y-(h*UNIT_FOOT)/UNIT_SH;
    step = w >> 2; if (!step) step = 1;
    fixed_probe(p.x, top+(h >> 1), step, (h >> 2), p.q+1);
    fixed_probe(80, 48, 6, 8, (precip(v.z+28) >> 4)+1);
    // Clear genuinely nearer foreground across the floor beside the shoulder subject.
    // A peripheral building behind/beside him must not be selected just for its size.
    fixed_probe(p.x >> 1, p.y+8, p.x >> 2, 8, p.q+1);
    for (i = 1; i < 128; i++) if (PHIDDEN[i]) return 1;
    return 0;
}

static u16 fixed_edge_range(u16 first,u16 last)
{
    const u8 *src = pbank+2120+RD16(pbank, 0)*6+RD16(pbank, 1)*8+RD16(pbank, 3)*8;
    u16 i, count, effort=0;
    if (RD16(pbank, 2) < 3) return 2;
    count = RD16(src, 0); src += 2;
    if(last>count) last=count;
    src+=first*4;
    for (i = first; i < last; i++, src += 4) {
        PVertex a = PVERT[RD16(src, 0)], b = PVERT[RD16(src, 1)];
        PScreen pa, pb;
        s16 dx, dy, sx, sy, err, x, y, length, qstep, steps;
        s32 q;
        u8 plane, reject = 0, style = pegroup ? PHIDDEN[pegroup[i]] : 0;
        for (plane = 0; plane < 5; plane++) {
            s16 da = pdistance(a, plane), db = pdistance(b, plane);
            if (!plane && style) { da -= 8; db -= 8; } // clip only near edge fragments
            if (da < 0 && db < 0) { reject = 1; break; }
            if ((da < 0) != (db < 0)) {
                PVertex c;
                c.x = a.x+divs32_16(muls16(b.x-a.x, da), da-db);
                c.y = a.y+divs32_16(muls16(b.y-a.y, da), da-db);
                c.z = a.z+divs32_16(muls16(b.z-a.z, da), da-db);
                if (c.z < P_NEAR) c.z = P_NEAR;
                if (da < 0) a = c; else b = c;
            }
        }
        if (reject) { effort+=2; continue; }
        pa = pproject(a); pb = pproject(b);
        dx = abs(pb.x-pa.x); dy = abs(pb.y-pa.y);
        sx = pa.x < pb.x ? 1 : -1; sy = pa.y < pb.y ? 1 : -1;
        err = dx-dy; length = dx > dy ? dx : dy;
        effort+=24+((u16)length>>4);
        qstep = length ? divs32_16((s32)(pb.q-pa.q)*128, length) : 0;
        q = (s32)pa.q*128; x = pa.x; y = pa.y;
        for (steps = length+1; steps; steps--, q += qstep) {
            s16 e = 2*err;
            if ((!style || !(steps & 2)) && (u16)x < 160 && (u16)y < 100
#ifdef FFT_CONFIG
                && (!pidle_active || (y>=8 && y<93))
                && (q >> 7)+1 >= (pidle_active ? pidle->depth[(y-8)*160+x] : PDEPTH[y*160+x])
#else
                && (q >> 7)+1 >= PDEPTH[y*160+x]
#endif
                ) ppixel(x, y, C_BLACK);
            if (e > -dy) { err -= dy; x += sx; }
            if (e < dx) { err += dx; y += sy; }
        }
    }
    return effort;
}

static void fixed_edges(void) { fixed_edge_range(0,65535U); }

#ifdef FFT_CONFIG
#include "prefetch.inc"
#endif

static void fprepare(u8 side)
{
    FView *view = &FW->view[side];
    u16 i, needed = 0;
#ifdef FFT_CONFIG
    if(!pidle_take())
#endif
        pgeometry();
    // Sparse screen-space current. Both pixels must belong to the final visible water.
    // This mask follows winning polygons, so banks/roofs cannot sparkle through water.
    memset(view->count, 0, sizeof view->count);
    { u16 x, y;
      for (y = 10; !plow && y < 92; y += 4) for (x = 0; x < 158; x += 2) {
          u16 off = y*20+(x >> 3); u8 bits = 0xC0 >> (x & 7);
          u8 phase = ((x >> 1)+(y >> 2)) & 3, n = view->count[phase];
          u16 screen = y*30+(x >> 3);
          if ((PCACHE[off] & bits) == bits && (((u8 *)rt_light)[screen] & bits) == bits
              && !(((u8 *)rt_dark)[screen] & bits) && n < 128) {
              view->glint[phase][n] = (y*30+(x >> 3))*4+((x & 7) >> 1); view->count[phase]++;
          }
      } }
    memcpy(scene_l+side*6000, rt_light, 3000);
    memcpy(scene_l+side*6000+3000, rt_dark, 3000);
#ifdef FFT_CINEMA
    FW->positioned = 0;
    if (st.cphase) FW->view[!side].valid = 0;
    FW->textured = ctexture(side);
#endif
    memset(FW->slot, 0, sizeof FW->slot); FW->used = 0; // geometry reused the pose pool
    for (i = 0; i < NUNIT; i++) {
        const Unit *u = &st.unit[i];
        FRect *r = &view->rect[i];
        PVertex v;
        PScreen p;
        s16 h, w, xa, xb, ya, yb;
        memset(r, 0, sizeof *r);
        if (u->x >= MAP_W || u->z >= MAP_H) continue;
        v = ptransform(u->x*28+14, map_tiles[u->z*MAP_W+u->x].stand*6, u->z*28+14);
        if (v.z < P_NEAR) continue;
        p = pproject(v);
        if (plow) { p.x *= 2; p.y *= 2; }
        h = (s16)((muls16(34, (s16)precip(v.z))*48) >> 15);
        if (plow == 1) h += (s16)(muls16(6, 256-ptalpha) >> 8);
        if (h < 4) continue;
        if (h > 104) h = 104;
        w = (h*16)/UNIT_SH;
        r->x = p.x-(w >> 1); r->y = p.y-(h*UNIT_FOOT)/UNIT_SH;
        xa = r->x < 0 ? 0 : r->x; xb = r->x+w > 160 ? 160 : r->x+w;
        ya = r->y < 8 ? 8 : r->y; yb = r->y+h > 93 ? 93 : r->y+h;
        if (xa >= xb || ya >= yb) continue;
        r->xb = (u8)(xa >> 3); r->top = (u8)ya;
        r->bytes = (u8)(((xb+7) >> 3)-r->xb); r->rows = (u8)(yb-ya);
        r->w = (u8)w; r->h = (u8)h; r->q = (u8)p.q;
        needed += r->bytes*r->rows;
    }
    if (FW->masks+needed > FMASK_BYTES) {
        FW->view[0].valid = FW->view[1].valid = 0; FW->masks = 0;
    }
    for (i = 0; i < NUNIT; i++) {
        FRect *r = &view->rect[i];
        u16 y, x;
        u8 *dst;
        r->mask = FW->masks;
        FW->masks += r->bytes*r->rows;
        dst = FMASK+r->mask;
        for (y = 0; y < r->rows; y++) for (x = 0; x < r->bytes; x++) {
            u16 k, xx = r->xb*8+x*8, yy = r->top+y;
            u8 out = 0, bit = 0x80;
            for (k = 0; k < 8; k++, bit >>= 1)
                if ((u16)(xx+k-r->x) < r->w && r->q+1 >= PDEPTH[yy*160+xx+k]) out |= bit;
            *dst++ = out;
        }
        { u16 j;
          for (j = i; j && view->rect[view->actors[j-1]].q > r->q; j--)
              view->actors[j] = view->actors[j-1];
          view->actors[j] = (u8)i; }
    }
    view->valid = 1;
    scene_rot = vbuf_rot = 0xFF;             // both scene and view scratch were repurposed
}

static void fdraw(u8 side, u8 who)
{
    const FRect *r = &FW->view[side].rect[who];
    const u8 *g, *src;
    FSlot *slot;
    u16 y, x, bytes, frame;
#ifdef FFT_CINEMA
    u8 bank, flash;
#endif
    u8 pose;
    if (!r->rows) return;
    g = unit_frame((u8)(4-((pview_yaw()+8)>>4)) & 3, who);
#ifdef FFT_CINEMA
    bank = !DC_OBJ(st.unit[who].dc) && armed_pose(who) != 0;
    frame = (u16)(g-(bank ? wgfx : ugfx))+1;
    flash = unit_flash(who);
#else
    frame = (u16)(g-ugfx)+1;
#endif
    pose = idle_seq[(u8)(st.tick+11*who) & 31];
    slot = &FW->slot[side][who][pose];
    bytes = r->bytes*r->rows*3;
#ifdef FFT_CINEMA
    if (slot->frame != frame || slot->bank != bank || slot->flash != flash) {
#else
    if (slot->frame != frame) {
#endif
        u8 *out;
        const u8 *mask = FMASK+r->mask;
        s16 sy_step = (UNIT_SH*256)/r->h, sx_step = (16*256)/r->w;
        s16 sy = (r->top-r->y)*sy_step;
        if (FW->used+bytes > FPOOL_BYTES) {
            memset(FW->slot, 0, sizeof FW->slot); FW->used = 0; FW->resets++;
        }
#ifdef FFT_CINEMA
        slot->bank = bank; slot->flash = flash;
#endif
        slot->frame = frame; slot->off = FW->used;
        out = FPOOL+FW->used; FW->used += bytes; FW->misses++;
        for (y = 0; y < r->rows; y++, sy += sy_step) {
            u16 row = (u16)sy >> 8;
            u16 l = RD16(g, row), d = RD16(g, UNIT_SH+row), m = RD16(g, 2*UNIT_SH+row);
#ifdef FFT_CINEMA
            { u16 inside = RD16(g, 3*UNIT_SH+row);
              if (flash) { l &= ~inside; d &= ~inside; if (flash == 1) l |= inside; } }
#endif
            s16 sx = (r->xb*8-r->x)*sx_step;
            for (x = 0; x < r->bytes; x++) {
                u8 a = 0, b = 0, opaque = 0, bit = 0x80, visible = *mask++;
                u16 k;
                for (k = 0; k < 8; k++, sx += sx_step, bit >>= 1) {
                    s16 xx = sx >> 8;
                    u16 source;
                    if ((u16)xx >= 16 || !(visible & bit)) continue;
                    source = 0x8000U >> xx;
                    if (m & source) continue;
                    opaque |= bit;
                    if (l & source) a |= bit;
                    if (d & source) b |= bit;
                }
                *out++ = (u8)~opaque; *out++ = a; *out++ = b;
            }
        }
    }
    src = FPOOL+slot->off;
    for (y = 0; y < r->rows; y++) {
        u8 *l = rt_light+(r->top+y)*30+r->xb, *d = rt_dark+(r->top+y)*30+r->xb;
        for (x = r->bytes; x; x--) {
            u8 mask = *src++;
            *l = (*l & mask) | *src++; l++;
            *d = (*d & mask) | *src++; d++;
        }
    }
}

static void perspective_render(void)
{
    u8 side = st.pyaw != st.pbase, i;
#ifdef FFT_ULTRA_TRANSITION
    if (ultra_motion()) { ultra_render(); return; }
#endif
#ifdef FFT_CINEMA
    if (st.cphase) side = 0;
#endif
#if defined(RT_CYCLES) && defined(FFT_CAPTURE)
    u32 started = BENCH_CYCLES, geometry_done, sprites_done;
#endif
    if (!pbank) { draw_clear(); draw_text(2, 40, "missing fftmesh", F_SMALL, C_BLACK); return; }
    if (!pvalid) {
#ifdef FFT_CONFIG
        pidle_complete(); // finish while its transformed vertices still occupy Status
#endif
        memset(FW, 0, sizeof *FW); pvalid = 1;
    }
    ZB(15);
    if (!FW->view[side].valid) fprepare(side);
#ifdef FFT_CINEMA
    if (!plow && st.cphase) cpositions(side);
#endif
    ZE(15);
#if defined(RT_CYCLES) && defined(FFT_CAPTURE)
    geometry_done = BENCH_CYCLES;
#endif
    ZB(17);
    memcpy(rt_light, scene_l+side*6000, 3000);
    memcpy(rt_dark, scene_l+side*6000+3000, 3000);
    ZE(17); ZB(16);
    { FView *view = &FW->view[side]; u8 phase = (st.tick >> 3) & 3;
      const u16 *src = view->glint[phase]; u16 n = view->count[phase];
      while (n--) { u16 p = *src++;
          u16 off = p >> 2; u8 mask = (u8)~(0xC0 >> ((p & 3)*2));
          ((u8 *)rt_light)[off] &= mask; ((u8 *)rt_dark)[off] &= mask;
      } }
    for (i = 0; i < NUNIT; i++) {
#ifdef FFT_CINEMA
        if (!plow && st.cphase) cweapon(side, FW->view[side].actors[i]);
#endif
        fdraw(side, FW->view[side].actors[i]);
    }
#ifdef FFT_CINEMA
    if (!plow && st.cphase) cprojectile();
#endif
    ZE(16);
#if defined(RT_CYCLES) && defined(FFT_CAPTURE)
    sprites_done = BENCH_CYCLES;
#endif
#ifdef FFT_CINEMA
    if (st.cphase) chud();
    else {
#endif
    draw_rect(0, 0, 160, 8, C_WHITE);
    draw_text(2, 0,
#ifdef FFT_CONFIG
        st.pwire ? "FLAT" :
#endif
        st.psolid == 1 ? "SOLID" : st.psolid == 2 ? "DOTTED" : "AUTO", F_SMALL, C_BLACK);
    draw_text(42, 0, side ? "BACK F5:front" : "FRONT F5:back", F_SMALL, C_BLACK);
    draw_rect(0, 93, 160, 7, C_WHITE);
    draw_text(2, 93, "MODE:zoom AL:wall SHIFT:map", F_SMALL, C_BLACK);
#ifdef FFT_CINEMA
    }
#endif
#if defined(RT_CYCLES) && defined(FFT_CAPTURE)
    { u32 ended = BENCH_CYCLES;
      BENCH_VALUE(0x72000000UL | rt_frame); BENCH_VALUE(geometry_done-started);
      BENCH_VALUE(sprites_done-geometry_done); BENCH_VALUE(0); BENCH_VALUE(ended-started);
      BENCH_VALUE(0x74000000UL | FW->used); BENCH_VALUE(0x75000000UL | FW->masks);
      BENCH_VALUE(0x76000000UL | FW->misses); BENCH_VALUE(0x77000000UL | FW->resets);
      BENCH_SHOT(rt_dark); }
#endif
}
