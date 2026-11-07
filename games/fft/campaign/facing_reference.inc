// Previous resident arrows, immutable regression reference.
// Index of the screen-space arrow for the active unit's world facing.
static u8 wait_arrow(void)
{
    static const s8 fdx[4] = { 1, -1, 0, 0 }, fdz[4] = { 0, 0, 1, -1 };
    const Unit *un = &st.unit[st.act];
    s16 u0, v0, du, dv;
    u8 sel;
    view_of(st.rot, 1, 1, &u0, &v0);
    view_of(st.rot, 1 + fdx[un->face], 1 + fdz[un->face], &du, &dv);
    du -= u0; dv -= v0;
    sel = dv < 0 ? 0 : du > 0 ? 1 : dv > 0 ? 2 : 3;
#ifdef FFT_SOURCE_CAMERA
    sel = 3 - sel;                      // scene_x reflects horizontal screen directions
#endif
    return sel;
}

// the facing arrows around the active unit at Wait, the one it faces black
static void draw_arrows(void)
{
    static const s8 ax[4] = { 13, 13, -13, -13 }, ay[4] = { -6, 7, 7, -6 };
    Feet f = unit_feet(st.rot, st.act);
    u8 g, r, sel = wait_arrow(), lm[8], mk[8];
    RtSprite sp;
    sp.w = 8; sp.h = 8; sp.light = sp.dark = lm; sp.mask = mk;
    for (g = 0; g < 4; g++) {
        for (r = 0; r < 8; r++) {
            u8 fi = W->arrow[g][0][r], o = W->arrow[g][1][r];
            lm[r] = g == sel ? fi : (u8)(o & ~fi);
            mk[r] = (u8)~o;
        }
        draw_sprite(f.x - st.camx + ax[g] - 4, f.y - st.camy + ay[g] - 4, &sp);
    }
}

