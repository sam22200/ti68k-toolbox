// How much of unit i view rot hides (the AI's camera): the tiles in front of it (greater
// diagonals) whose top face rises over its sprite, from the map's corner heights (the other
// views' depth is not unpacked), at four of the sprite's columns: the rows hidden, 0..4 * VIS_H
// (the tests: as the view's depth says half hidden or not on 558 of 560 tiles and views)
#define VIS_H 22
#define HIDE(j, r) do { s16 r_ = (r); if (r_ > hid[j]) hid[j] = r_; } while (0)
u8 view_hides(u8 rot, u8 i)
{
    static const s8 wdx[4] = { 1, 0, -1, 0 }, wdz[4] = { 0, -1, 0, 1 };   // world step of view +u
    const Unit *un = &st.unit[i];
    s16 u, v, k, d, base, x, z, hid[4] = { 0, 0, 0, 0 };
    s8 sx = (s8)(wdx[rot] + wdx[(rot + 3) & 3]), sz = (s8)(wdz[rot] + wdz[(rot + 3) & 3]);   // +u +v
    u8 j, n = 0, c0 = (u8)(-rot & 3), c1 = (u8)((1 - rot) & 3), c3 = (u8)((3 - rot) & 3);
    view_of(rot, un->x, un->z, &u, &v);
    base = HH - map_tiles[UNIT_CELL(un)].stand * (HU / 2);
    for (k = -1; k <= 1; k++) {                     // the three columns of tiles over the sprite
        d = k ? 1 : 2;                              // (d: diagonals in front), from the nearest:
        world_of(rot, u + ((d + k) >> 1), v + ((d - k) >> 1), &x, &z);   // a step is +u +v
        for (; d * HH <= base + MAXH * HU; d += 2, x += sx, z += sz) {
            const u8 *c;
            s16 h0, top, l, r;
            if ((u16)x >= MAP_W || (u16)z >= MAP_H) continue;
            { const Tile *tile=tile_at(x,z); if(!tile)continue; c=tile->c; }
            h0 = c[c0];                             // its top face's upper edges: from the left
            top = base - d * HH + h0 * HU;          // corner up to the top one (rows above the
            l = (s16)c[c3] - h0 - 1;                // feet), down to the right one; the sprite's
            r = (s16)c[c1] - h0 - 1;                // columns -5 -2 2 5 against the tile's centre
            if (!k) {
                HIDE(0, top + (l * 5 >> 1)); HIDE(1, top + l); HIDE(2, top + r); HIDE(3, top + (r * 5 >> 1));
            } else if (k > 0) {                     // the tile to the right (centre +HW)
                HIDE(2, top + l * 5); HIDE(3, top + (l * 7 >> 1));
            } else {
                HIDE(0, top + (r * 7 >> 1)); HIDE(1, top + r * 5);
            }
        }
    }
    for (j = 0; j < 4; j++) n += (u8)(hid[j] > VIS_H ? VIS_H : hid[j]);
    return n;
}

#ifndef __m68k__
// the same rows read from the current view's depth (the tests; 0xFF: the view not unpacked)
u8 view_hides_now(u8 i)
{
    static const s8 col[4] = { -5, -2, 2, 5 };
    Feet f;
    u8 j, n = 0;
    s16 y;
    if (vbuf_rot != st.rot) return 0xFF;
    f = unit_feet(st.rot, i);
    for (j = 0; j < 4; j++) {
        s16 x = f.x + col[j];
        u8 bit = (u8)(0x80 >> (x & 7));
        for (y = f.y - VIS_H; y < f.y; y++) {
            const u8 *t; u16 x0, w, o = (u16)(x >> 3);
            if ((u16)y >= SC_H) continue;
            DEPTH_ROW(y, t, x0, w);
            if ((u16)(o - x0) >= w) continue;
            t += o;
            if ((t[0] > f.depth && t[DEPTH_STRIDE] & bit) || (t[2 * DEPTH_STRIDE] > f.depth && t[3 * DEPTH_STRIDE] & bit)) n++;
        }
    }
    return n;
}
#endif

// An AI unit's plan made (the frame not drawn): the view it will act in. When, from the tile it
// acts from, it or its target is more than half hidden: the view hiding the least of the worse
// of them (FFT turns the camera after the move; the player's view back on the player's turn)
static void ai_camera(void)
{
    Unit *a = &st.unit[st.act];
    u8 o=unit_at_cell(st.ai_tgt),home=UNIT_CELL(a),r,h,g,bh=0xFF;
    st.ai_view = 0xFF;
    if (st.acted || st.ai_ab == AI_NONE || st.ai_ab == ABIL_DEFEND) return;
    unit_place(a,st.ai_o);
    for (r = 0; r < 4; r++) {
        u8 q = (st.rot + r) & 3;
        h = view_hides(q, st.act);
        if (o && o - 1 != st.act && (g = view_hides(q, o - 1)) > h) h = g;
        if (!r && h <= 2 * VIS_H) break;
        if (h < bh) { bh = h; st.ai_view = q; }
    }
    unit_place(a,home);
    if (st.ai_view == st.rot) st.ai_view = 0xFF;
}

