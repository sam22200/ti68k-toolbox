#define BN_Y (st.banner == BN_START ? 2 : 34)
static const u8 bn_width[4] = BN_WIDTHS;
static u8 bn_line(u8 kind,u8 line){return kind==BN_START?line:kind;}
// the scenery shaded a grey darker (white light, light dark, dark black) behind a diagonal edge
// x + y = c coming from the top left: before it (in), or past it (out: it leaves the same way)
static void bn_shade1(u8 *l, u8 *d, u32 k)    // a long of both planes (k: its pixels)
{
    u32 L = RD32(l, 0), D = RD32(d, 0);
    WR32(l, 0, (L & ~k) | ((~L | D) & k));
    WR32(d, 0, D | (L & k));
}
static void bn_shade(void)
{
    u8 *l = rt_light, *d = rt_dark, out = st.banner_t >= W->bn_end - DK_N;
    s16 c = (s16)(out ? st.banner_t - (W->bn_end - DK_N) : st.banner_t + 1) * DK_SPEED, y;
    for (y = 0; y < RT_H; y++, l += RT_PBYTES, d += RT_PBYTES) {
        s16 e = c - y, a, b;
        u8 *pl, *pd, *pe;
        if (e < 0) e = 0;
        if (e > RT_W) e = RT_W;
        a = out ? e : 0; b = out ? RT_W : e;
        if (a >= b) continue;
        pl = l + (a >> 5) * 4; pd = d + (a >> 5) * 4; pe = l + (b >> 5) * 4;
        if (a & 31) {                          // the edge's long, then the whole ones
            u32 k = 0xFFFFFFFFUL >> (a & 31);
            if (pl == pe) k &= ~(0xFFFFFFFFUL >> (b & 31));
            bn_shade1(pl, pd, k);
            if (pl == pe) continue;
            pl += 4; pd += 4;
        }
        for (; pl < pe; pl += 4, pd += 4) {
#ifdef __m68k__
            u32 L = *(PlaneWord *)pl, D = *(PlaneWord *)pd;
            *(PlaneWord *)pl = ~L | D; *(PlaneWord *)pd = D | L;
#else
            bn_shade1(pl, pd, 0xFFFFFFFFUL);
#endif
        }
        if (b & 31) bn_shade1(pl, pd, ~(0xFFFFFFFFUL >> (b & 31)));
    }
}

// Reveal word-aligned strips directly from the archived bank. Outlined text is all
// present before its first frame; the complete word always gets BN_HOLD frames.
static void bn_draw(void)
{
    const u8 *bank = UI_SHARED(BANNERS);
    u8 line;
    for (line = 0; line < (st.banner == BN_START ? 2 : 1); line++) {
        u8 k = bn_line(st.banner, line), r;
        const u8 *g = bank + RD16(bank, k), *l = g + 4;
        u16 size = (u16)(g[2] * g[3]);
        const u8 *d = l + size, *m = d + size;
        s16 c = (s16)(st.banner_t - W->bn_t0[line]) * BN_SPEED + ((RT_W - bn_width[k]) >> 1);
        u8 full;
        u16 tail = 0;
        if (st.banner_t <= W->bn_t0[line] || st.banner_t >= W->bn_end - DK_N) continue;
        if (c > RT_W) c = RT_W;
        c -= g[0];
        if (c <= 0) continue;
        full = (u8)(c >> 4);
        if (full >= g[3] / 2) full = g[3] / 2;
        else if (c & 15) tail = (u16)(0xFFFFU << (16 - (c & 15)));
        for (r = 0; r < g[2]; r++, l += g[3], d += g[3], m += g[3]) {
            u8 *sl = (u8 *)rt_light + (BN_Y + g[1] + r) * RT_PBYTES + (g[0] >> 3);
            u8 *sd = (u8 *)rt_dark + (BN_Y + g[1] + r) * RT_PBYTES + (g[0] >> 3);
            s16 w;
            for (w = 0; w < full; w++) {
                u16 km = RD16(m, w);
                if (!km) continue;
                RMW16(sl + 2 * w, km, RD16(l, w) & km);
                RMW16(sd + 2 * w, km, RD16(d, w) & km);
            }
            if (tail) {
                u16 km = RD16(m, w) & tail;
                RMW16(sl + 2 * w, km, RD16(l, w) & km);
                RMW16(sd + 2 * w, km, RD16(d, w) & km);
            }
        }
    }
}
