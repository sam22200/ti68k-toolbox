// Shared direct trace: generic boolean query or weapon recipient (255=terrain).
#ifndef FFT_DIRECT_ROOT
#define FFT_DIRECT_ROOT
static u16 isqrt(u32 v)
{
    u16 r = 0, b = 0x100;
    while (b) { if ((u32)muls16((s16)(r | b), (s16)(r | b)) <= v) r |= b; b >>= 1; }
    return r;
}
#endif

#ifndef FFT_DIRECT_NAME
#define FFT_DIRECT_NAME ai_clear
#define FFT_DIRECT_DEFAULT
#endif
static u8 FFT_DIRECT_NAME(u8 a, u8 t)
{
    const Unit *ua = &st.unit[a], *ut = &st.unit[t];
    s16 x0 = (s16)(ua->x * 28 + 14), z0 = (s16)(ua->z * 28 + 14), x1 = (s16)(ut->x * 28 + 14), z1 = (s16)(ut->z * 28 + 14);
    s16 lx = (x0 < x1 ? x0 : x1) - 8, hx = (x0 < x1 ? x1 : x0) + 8, lz = (z0 < z1 ? z0 : z1) - 8, hz = (z0 < z1 ? z1 : z0) + 8;
    s32 x = (s32)x0 << 16, z = (s32)z0 << 16, y, dx, dy, dz;
    s16 ox[UNIT_CAPACITY], oz[UNIT_CAPACITY], oy[UNIT_CAPACITY]; // other standing units
#ifdef FFT_DIRECT_PANEL
    u8 body[UNIT_CAPACITY], id[UNIT_CAPACITY];
#define FFT_DIRECT_REACHED t
#define FFT_DIRECT_BLOCKED 255
#else
#define FFT_DIRECT_REACHED 1
#define FFT_DIRECT_BLOCKED 0
#endif
    u16 l, n, i;                                            // near the line
    u8 k, m = 0;
    for (k = 0; k < NUNIT; k++) {
        const Unit *u = &st.unit[k];
        s16 ux = (s16)(u->x * 28 + 14), uz = (s16)(u->z * 28 + 14);
        if (k == a || k == t
#ifdef FFT_DIRECT_PANEL
            ||!unit_on_field(u)||UNIT_CELL(u)==UNIT_CELL(ut)
#else
            ||!u->hp
#endif
#ifdef FFT_CHAPTER
            ||unit_airborne(u)
#endif
            || ux <= lx || ux >= hx || uz <= lz || uz >= hz) continue;
        ox[m] = ux; oz[m] = uz; oy[m] = (s16)(W->hs[UNIT_CELL(u)] * 6);
#ifdef FFT_DIRECT_PANEL
        id[m]=k;body[m]=!u->hp?12:(core_status_current(&u->statuses,2)&6)?24:36;
#endif
        m++;
    }
    y = ((s32)W->hs[UNIT_CELL(ua)] * 6 + 24) << 16;
#ifdef FFT_DIRECT_PANEL
    if(core_status_current(&ua->statuses,2)&6)y-=8L<<16;
#endif
    dx = ((s32)x1 << 16) - x;
    dz = ((s32)z1 << 16) - z;
    dy = (((s32)W->hs[UNIT_CELL(ut)] * 6 + (ut->hp ? 24 : 12)) << 16) - y;
#ifdef FFT_DIRECT_PANEL
    if(ut->hp&&(core_status_current(&ut->statuses,2)&6))dy-=8L<<16;
#endif
    {   // the whole-unit offsets fit 16 bits: one MULS each, not __mulsi3
        s16 ex = (s16)(dx >> 16), ey = (s16)(dy >> 16), ez = (s16)(dz >> 16);
        l = isqrt((u32)(muls16(ex, ex) + muls16(ey, ey) + muls16(ez, ez)));
    }
    if (!l) return FFT_DIRECT_REACHED;
    n = l >> 1;
    dx = dx / l * 2; dy = dy / l * 2; dz = dz / l * 2;     // a step: 2 units along the line
    for (i = 0; i < n; i++) {
        s16 px, pz, py;
        u8 c;
        x += dx; y += dy; z += dz;
        px = (s16)(x >> 16); pz = (s16)(z >> 16); py = (s16)(y >> 16);
        if (py <= 0 || (u16)px >= MAP_W * 28 || (u16)pz >= MAP_H * 28) continue;
        c = (u8)(TILE_COL[px] + TILE_ROW[pz]);
#ifdef FFT_CHAPTER
        if(W->surface) {
            if(surface_hit_layers(W->surface+((u16)c<<3),py,(W->surface_flags&SURFACE_HIT_UPPER)?2:1))return FFT_DIRECT_BLOCKED;
        } else {
            // Synthetic tactical fixtures without source columns retain their
            // explicit simplified column geometry. Retail chapters carry source columns.
            if(W->grid) {
                if(!(W->grid->present[c>>3]&(1U<<(c&7))))continue;
                c=W->grid->lookup[c];
            }
            if((u16)py<=W->top[c])return FFT_DIRECT_BLOCKED;
        }
#else
        if((u16)py<=W->top[c])return FFT_DIRECT_BLOCKED;
#endif
        for (k = 0; k < m; k++)
            if (py > oy[k] && py < oy[k] +
#ifdef FFT_DIRECT_PANEL
                body[k]
#else
                36
#endif
                && px > ox[k] - 8 && px < ox[k] + 8 && pz > oz[k] - 8 && pz < oz[k] + 8)
#ifdef FFT_DIRECT_PANEL
                return id[k];
#else
                return 0;
#endif
    }
    return FFT_DIRECT_REACHED;
}
#undef FFT_DIRECT_REACHED
#undef FFT_DIRECT_BLOCKED
#ifdef FFT_DIRECT_DEFAULT
#undef FFT_DIRECT_DEFAULT
#undef FFT_DIRECT_NAME
#endif
