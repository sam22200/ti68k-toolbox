// Target-centered attack camera. Tactical resolution remains in fft.c.
static u8 put_n(char *s, u8 n, u16 v);
static u8 put_s(char *s, u8 n, const char *v);
static const u8 *cimages;
#ifndef __m68k__
#include <stdio.h>
#ifdef FFT_CAMERA_MOTION
static u8 cimage_pc[12032];
#endif
#endif

static void cinema_assets(void)
{
    cimages = RT_NULL;
#ifdef FFT_CAMERA_MOTION
#ifdef __m68k__
    { u16 size; const u8 *p = rt_file("fftshot", &size);
      if (p && size >= 12028 && RD16(p, 0) == 0x4353 && RD16(p, 1) == 2) cimages = p; }
#else
    { FILE *f = fopen("fftshot.bin", "rb");
      if (f) {
          unsigned n = (unsigned)fread(cimage_pc, 1, sizeof cimage_pc, f); fclose(f);
          if (n == 12028 && RD16(cimage_pc, 0) == 0x4353 && RD16(cimage_pc, 1) == 2) cimages = cimage_pc;
      } }
#endif
#endif // Historical motion builds only; fixed inspection has no attack-image bank.
}

// Reviewed endpoint textures are prepared offline for these exact camera descriptors.
// A mismatching shot keeps its own flat geometry; textures never change the depth image.
static u8 ctexture(u8 side)
{
#ifdef FFT_CAMERA_MOTION
    const u8 *p; u8 n;
#endif
#ifdef FFT_CONFIG
    if (st.pwire) return 0;
#endif
#ifdef FFT_TEXTURE_PREVIEW
    if (preview_ready) return 1;
#endif
#ifndef FFT_CAMERA_MOTION
    (void)side; return 0;
#else
    if (!cimages || st.cphase != 2 || plow) return 0;
    p = cimages+4;
    for (n = 0; n < 2; n++, p += 6012)
        if (p[0] == st.cax && p[1] == st.caz && p[2] == st.cah && p[3] == st.cyaw
            && p[4] == st.cdist && p[5] == st.ceye && p[6] == st.cpitch
            && p[7] == (u8)st.cside && p[8] == st.psolid) {
            memcpy(scene_l+side*6000, p+12, 6000); return 1;
        }
    return 0;
#endif
}
// Keep the current isometric azimuth when the actors separate well on screen.
// Head-on shots may turn at most 45 degrees, toward the nearest side of the action.
// Capture the original target tile: knockback must not move the camera during impact.
static void cinema_begin(void)
{
    const Unit *a = &st.unit[st.atk], *t = &st.unit[st.tgt];
    s16 dx = (s16)t->x-a->x, dz = (s16)t->z-a->z, x = abs(dx), z = abs(dz), dir;
    s16 span = x > z ? x : z, offset, start = (56-16*st.rot) & 63, delta;
    if (x > 2*z) dir = dx > 0 ? 16 : 48;
    else if (z > 2*x) dir = dz > 0 ? 32 : 0;
    else dir = dx > 0 ? (dz > 0 ? 24 : 8) : (dz > 0 ? 40 : 56);
    st.cyaw = (u8)start;
    offset = (s16)((muls16(dx, psine(start+16))+muls16(dz, psine(start))));
    if (x+z <= 1 || abs(offset)*2 < span*127) {
        delta = (((dir+16)-start+16) & 31)-16; // nearest of the two lateral views
        if (delta < -8) delta = -8;
        if (delta > 8) delta = 8;
        st.cyaw = (start+delta) & 63;
    }
    if (x+z <= 1) { st.cdist = 48; st.ceye = 40; st.cpitch = 5; }
    else { st.cdist = (u8)(80+span*8); st.ceye = (u8)(76+span*4); st.cpitch = 6; }
    offset = (s16)((muls16(-dx*14, psine(st.cyaw+16))+muls16(-dz*14, psine(st.cyaw))) >> 7);
    st.cside = (s8)(offset-12);
    st.cax = t->x; st.caz = t->z; st.cah = map_tiles[t->z*MAP_W+t->x].stand;
    st.ciso_x = st.camx; st.ciso_y = st.camy; st.ciso_rot = st.rot;
    st.cphase = 1; st.cframe = 0; st.perspective = 1;
    W->frz = W->hold = 0; pvalid = 0;
}

static u8 cinema_eligible(void)
{
#ifdef FFT_CAMERA_MOTION
    return st.cenabled && pbank && !st.perspective && !st.cphase && st.mode == M_ACT
        && !st.act_t && (st.abil == NO_ABIL || (st.abil < NABIL && act_kind() == AB_DAMAGE));
#else
    return 0; // Automatic perspective shots were rejected after native timing review.
#endif
}

static void cinema_update(void)
{
    st.tick++;
    if (st.cphase == 1) {
        if (st.cframe < 9) st.cframe++;
        else { st.cphase = 2; st.cframe = 0; }
        pvalid = 0;
    } else if (st.cphase == 2) {
        if (st.deny) st.deny--;
        W->upd_k = 0;
        // Two logical updates per displayed frame; never skip an impact/reward event.
        // Stop immediately on exit: AI/end handling must not run a second time.
        attack_step();
        if (st.mode == M_ACT) attack_step();
        if (st.mode != M_ACT) { st.cphase = 3; st.cframe = 9; pvalid = 0; }
    } else if (st.cframe) {
        st.cframe--; pvalid = 0;
    } else {
        st.cphase = st.cframe = st.perspective = 0; plow = 0; pvalid = 0;
        st.camx = st.ciso_x; st.camy = st.ciso_y; st.rot = st.ciso_rot;
        scene_rot = vbuf_rot = 0xFF; W->bg_ok = 0; W->status_i = 0;
        W->cur_k = W->fc_key = W->win_k[0] = 0;
        W->frz = W->hold = 0;
    }
}

static void cinema_render(void) { perspective_render(); }

static PVertex cfeet(u8 who)
{
    const Unit *u = &st.unit[who];
    PVertex f;
    f.x = u->x*28+14; f.z = u->z*28+14;
    f.y = u->x < MAP_W && u->z < MAP_H ? map_tiles[u->z*MAP_W+u->x].stand*6 : 0;
    if (st.mode == M_WALK && who == st.sel && st.path_i+1 < st.path_n) {
        const u8 *a = st.path[st.path_i], *b = st.path[st.path_i+1];
        u8 t = st.walk_t, hop = 0, q;
        s16 ha = map_tiles[a[1]*MAP_W+a[0]].stand*6;
        s16 hb = map_tiles[b[1]*MAP_W+b[0]].stand*6;
        if (step_frames(st.path_i) == CLIMB_FRAMES) { hop = walk_h[t]*2; t += WALK_FRAMES; }
        q = walk_q[t];
        f.x = a[0]*28+14+(s16)(muls16(((s16)b[0]-a[0])*28, q) >> 6);
        f.z = a[1]*28+14+(s16)(muls16(((s16)b[1]-a[1])*28, q) >> 6);
        f.y = ha+(s16)(muls16(hb-ha, q) >> 6)+hop;
    }
    return f;
}

static void cpositions(u8 side)
{
    FView *view = &FW->view[side];
    u16 i, j;
    for (i = 0; i < NUNIT; i++) {
        PVertex f = cfeet((u8)i), v;
        FRect rect, *r = &view->rect[i];
        PScreen p;
        s16 h, w, xa, xb, ya, yb;
        u16 y, x;
        u8 *out;
        if (FW->positioned && !memcmp(&f, &FW->feet[i], sizeof f)) continue;
        FW->feet[i] = f; memset(&rect, 0, sizeof rect); rect.mask = i*768;
        if (st.unit[i].x >= MAP_W || st.unit[i].z >= MAP_H) goto ready;
        v = ptransform(f.x, f.y, f.z);
        if (v.z < P_NEAR) goto ready;
        p = pproject(v);
        h = (s16)((muls16(34, (s16)precip(v.z))*48) >> 15);
        if (h < 4) goto ready;
        if (h > 104) h = 104;
        w = (h*16)/UNIT_SH;
        rect.x = p.x-(w >> 1); rect.y = p.y-(h*UNIT_FOOT)/UNIT_SH;
        xa = rect.x < 0 ? 0 : rect.x; xb = rect.x+w > 160 ? 160 : rect.x+w;
        ya = rect.y < 8 ? 8 : rect.y; yb = rect.y+h > 93 ? 93 : rect.y+h;
        if (xa >= xb || ya >= yb) goto ready;
        rect.xb = (u8)(xa >> 3); rect.top = (u8)ya;
        rect.bytes = (u8)(((xb+7) >> 3)-rect.xb); rect.rows = (u8)(yb-ya);
        rect.w = (u8)w; rect.h = (u8)h; rect.q = (u8)p.q;
ready:
        if (FW->positioned && !memcmp(r, &rect, sizeof rect)) continue;
        *r = rect;
        memset(FW->slot[side][i], 0, sizeof FW->slot[side][i]);
        out = FMASK+r->mask;
        // A clipped 64-pixel sprite needs at most 9*85 = 765 mask bytes.
        for (y = 0; y < r->rows; y++) for (x = 0; x < r->bytes; x++) {
            u16 k, xx = r->xb*8+x*8, yy = r->top+y;
            u8 bits = 0, bit = 0x80;
            for (k = 0; k < 8; k++, bit >>= 1)
                if ((u16)(xx+k-r->x) < r->w && r->q+1 >= PDEPTH[yy*160+xx+k]) bits |= bit;
            *out++ = bits;
        }
    }
    FW->positioned = 1; FW->masks = NUNIT*768;
    for (i = 0; i < NUNIT; i++) {
        for (j = i; j && view->rect[view->actors[j-1]].q > view->rect[i].q; j--)
            view->actors[j] = view->actors[j-1];
        view->actors[j] = (u8)i;
    }
}

// Original blade atlas, scaled with its body and tested against the same fixed depth.
static void cweapon(u8 side, u8 who)
{
    const FRect *r = &FW->view[side].rect[who];
    const u8 *g;
    s16 x, y, h, w, bx, by, sy, sx, ys, xs;
    u8 phase = armed_pose(who), dir, k;
    if (phase < 2 || !r->rows) return;
    dir = unit_direction((u8)(4-((pview_yaw()+8)>>4)) & 3, who);
    k = (st.unit[who].gfx >= UG_CHEMIST_M ? 24 : 0)
        +(items[st.unit[who].eq[EQ_WEAPON]].knife ? 0 : 12)+(dir >> 1)*6+(dir & 1)*3+phase-2;
    g = wgfx+WEAPON_OFF+(u32)k*WEAPON_FB;
    h = (WEAPON_SH*r->h)/UNIT_SH; w = r->w*2;
    bx = r->x+(r->w >> 1)+(weapon_x[k]*r->w)/16;
    by = r->y+((UNIT_FOOT-WEAPON_FOOT)*r->h)/UNIT_SH;
    ys = (WEAPON_SH*256)/h; xs = (32*256)/w;
    for (y = 0, sy = 0; y < h; y++, sy += ys) {
        u16 row = (u16)sy >> 8;
        u32 l = RD32(g, row*2), d = RD32(g, (WEAPON_SH+row)*2), m = RD32(g, (2*WEAPON_SH+row)*2);
        if (by+y < 8 || by+y >= 93) continue;
        for (x = 0, sx = 0; x < w; x++, sx += xs) {
            u32 bit = 0x80000000UL >> ((u16)sx >> 8);
            s16 xx = bx+x;
            if ((u16)xx < 160 && !(m & bit) && r->q+1 >= PDEPTH[(by+y)*160+xx])
                ppixel(xx, by+y, (l & bit ? 1 : 0)+(d & bit ? 2 : 0));
        }
    }
}

// A small, depth-aware flight cue for Throw Stone; collision/hit resolution stays unchanged.
static void cprojectile(void)
{
    const Unit *a = &st.unit[st.atk], *t = &st.unit[st.tgt];
    PVertex v; PScreen p; s16 frame, q, x, y, z;
    if (st.cphase != 2 || st.abil >= NABIL || abilities[st.abil].range <= 1
        || st.act_t < ACT_BLOW || st.act_t > ACT_HIT) return;
    frame = st.act_t-ACT_BLOW; q = frame*85; // three flight intervals, 0..255
    x = a->x*28+14+(s16)(muls16(((s16)t->x-a->x)*28, q) >> 8);
    z = a->z*28+14+(s16)(muls16(((s16)t->z-a->z)*28, q) >> 8);
    y = map_tiles[a->z*MAP_W+a->x].stand*6+24;
    y += (s16)(muls16(((s16)map_tiles[t->z*MAP_W+t->x].stand-map_tiles[a->z*MAP_W+a->x].stand)*6, q) >> 8);
    v = ptransform(x, y, z); if (v.z < P_NEAR) return;
    p = pproject(v);
    if (p.x < 2 || p.x >= 158 || p.y < 10 || p.y >= 91 || p.q+1 < PDEPTH[p.y*160+p.x]) return;
    draw_rect(p.x-2, p.y-2, 5, 5, C_WHITE); draw_rect(p.x-1, p.y-1, 3, 3, C_BLACK);
}

typedef struct { s16 x, y; u8 width; } CRewardBox;
static CRewardBox creward_box(u8 characters)
{
    const FRect *r = &FW->view[0].rect[st.atk];
    CRewardBox b;
    b.width = characters*4+6;            // small AMS font is at most four pixels wide
    b.x = r->x+(r->w >> 1)-(b.width >> 1); b.y = r->y-12;
    if (b.x < 1) b.x = 1;
    if (b.x > 159-b.width) b.x = 159-b.width;
    if (b.y < 10) b.y = 10;
    if (b.y > 82) b.y = 82;
    return b;
}

static void chud(void)
{
    char text[60]; u8 n;
    const Unit *t = &st.unit[st.tgt];
    draw_rect(0, 0, 160, 8, C_WHITE);
    draw_text(2, 0, st.abil == NO_ABIL ? "Attack" : abilities[st.abil].name, F_SMALL, C_BLACK);
    draw_rect(0, 93, 160, 7, C_WHITE);
    n = put_s(text, 0, unit_name(st.tgt)); n = put_s(text, n, " HP ");
    n = put_n(text, n, t->hp); text[n] = 0;
    draw_text(2, 93, text, F_SMALL, C_BLACK);
    if (st.cphase == 2 && st.act_t >= ACT_HIT && st.act_t < ACT_HIT+NUM_T) {
        const FRect *r = &FW->view[0].rect[st.tgt];
        s16 x = r->x, y = r->y-10-((st.act_t-ACT_HIT) >> 2);
        u8 width;
        if (x < 2) x = 2;
        if (x > 133) x = 133;
        if (y < 10) y = 10;
        if (y > 83) y = 83;
        if (st.hit) { n = put_n(text, 0, st.dmg); text[n] = 0; } else strcpy(text, "Miss");
        // AMS small digits advance four pixels; "Miss" advances 6+2+3+3.
        width = st.hit ? n*4 : 14;
        draw_rect(x-1, y-1, 27, 10, C_BLACK); draw_rect(x, y, 25, 8, C_WHITE);
        draw_text(x+((25-width) >> 1), y+1, text, F_SMALL, C_BLACK);
    }
    if (st.cphase == 2 && st.gain && st.act_t >= ACT_HIT+GAIN_T) {
        n = put_s(text, 0, st.act_t >= ACT_HIT+JP_T ? "JP +" : "EXP +");
        n = put_n(text, n, st.act_t >= ACT_HIT+JP_T ? st.jpg : st.gain); text[n] = 0;
        { CRewardBox b = creward_box(n);
          draw_rect(b.x-1,b.y-1,b.width+2,10,C_BLACK);
          draw_rect(b.x,b.y,b.width,8,C_WHITE);
          draw_text(b.x+3,b.y+1,text,F_SMALL,C_BLACK); }
    }
}
