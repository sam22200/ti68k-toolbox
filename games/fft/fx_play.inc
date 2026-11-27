// Combat-effect presentation: step 2 of campaign/COMBAT_FX.md -- the schedule, the single
// impact and the reusable primitives. It draws; it decides nothing.
//
// The spec asks for an effect scheduler and a result applied exactly once. The engine
// already has both, so this binds to them instead of adding a second clock:
//
//   spec phase      engine            what happens here
//   anticipation    act_t < ACT_WIND  nothing yet
//   focus           ACT_WIND          dim, the caster stays bright
//   animation       ACT_BLOW          dim, the target stays bright, the primitive plays
//   impact          ACT_HIT           the flash; the combat core applies the result here
//   result/return   ACT_END           dimming off, the picture returns by itself
//
// The single impact needs no work: the result is applied by the combat core at ACT_HIT and
// nothing below reads or writes HP, MP, CT or the RNG. Skipping or speeding the animation
// moves act_t, not the result -- which is what the spec's acceptance test asks for, and
// what check_combat_fx.c proves.
//
// Every primitive is a pure function of act_t and two screen points, so there is no effect
// state at all: nothing in Work, nothing allocated, nothing to reset, and the PC and the
// 68000 compute the same picture. That matters more than it looks: Work cannot grow without
// invalidating every built module image (MOD_SIG, see COMBAT_FX.md).

#define FX_FLASH 4                      // frames of impact flash from ACT_HIT

// One pixel, white or black, clipped. The plane idiom is draw_glints()'s.
static void fx_dot(s16 x, s16 y, u8 white)
{
    u16 o;
    u8 b;
    if ((u16)y >= RT_H || (u16)x >= RT_W) return;
    o = (u16)(y * RT_PBYTES + (x >> 3));
    b = (u8)(0x80 >> (x & 7));
    if (white) { ((u8 *)rt_light)[o] &= (u8)~b; ((u8 *)rt_dark)[o] &= (u8)~b; }
    else       { ((u8 *)rt_light)[o] |= b;      ((u8 *)rt_dark)[o] |= b; }
}

// A ground ring: the 2:1 isometric diamond the map's own tiles use, so it sits on the
// ground instead of floating. One division for the whole ring, then 8.8 accumulation.
// Haste, Slow, Protect, Shell, Stop, MBarrier and Cure are all this with another radius
// and cadence, which is why it is the first primitive.
static void fx_ring(s16 cx, s16 cy, s16 r, u8 white)
{
    s16 i, h = (s16)(r >> 1);
    s32 dy, step;
    if (r < 2) return;
    step = ((s32)h << 8) / r;
    for (i = 0, dy = 0; i <= r; i += 2, dy += 2 * step) {
        s16 d = (s16)(h - (s16)(dy >> 8));
        fx_dot((s16)(cx + i), (s16)(cy + d), white); fx_dot((s16)(cx + i), (s16)(cy - d), white);
        fx_dot((s16)(cx - i), (s16)(cy + d), white); fx_dot((s16)(cx - i), (s16)(cy - d), white);
    }
}

// A burst: eight spokes leaving a point, in the same 2:1 ratio. Explosions and the
// convergence-then-blast family.
static void fx_burst(s16 cx, s16 cy, s16 r, u8 white)
{
    static const s8 dir[8][2] = { {2,0}, {-2,0}, {0,1}, {0,-1}, {2,1}, {2,-1}, {-2,1}, {-2,-1} };
    s16 k, i;
    for (k = 0; k < 8; k++)
        for (i = r >> 1; i <= r; i++)
            fx_dot((s16)(cx + dir[k][0] * i), (s16)(cy + dir[k][1] * i), white);
}

// A slash: a straight sweep from the attacker towards the target, its leading part only,
// so it reads as motion rather than as a drawn line. The physical attack and the Holy
// Sword family are this with another length and tilt.
static void fx_slash(s16 x0, s16 y0, s16 x1, s16 y1, u8 phase, u8 white)
{
    s16 dx = (s16)(x1 - x0), dy = (s16)(y1 - y0), i;
    for (i = 0; i < 6; i++) {
        s16 t = (s16)(phase * 4 + i * 2);           // 0..64 along the sweep
        if (t > 64) break;
        fx_dot((s16)(x0 + (s16)(((s32)dx * t) >> 6)), (s16)(y0 + (s16)(((s32)dy * t) >> 6)), white);
        fx_dot((s16)(x0 + (s16)(((s32)dx * t) >> 6)), (s16)(y0 + (s16)(((s32)dy * t) >> 6) - 1), white);
    }
}

// The schedule: what the running action asks for this frame. Called before the dimming,
// because the dimming needs to know who stays bright. It reads the action the engine is
// already running and nothing else.
static void fx_schedule(const Feet *f, const u8 *vis)
{
    u8 t;
    (void)f; (void)vis;
    if (st.mode != M_ACT) { if (W->fx_sel) fx_focus(FX_NO_UNIT, 0); return; }
    t = st.act_t;
    if (t < ACT_WIND || t >= ACT_END) { if (W->fx_sel) fx_focus(FX_NO_UNIT, 0); return; }
    fx_focus(t < ACT_BLOW ? st.atk : st.tgt, 1);
}

// The animation, after the dimming and the focused unit, so it reads over both.
static void fx_play(const Feet *f, const u8 *vis)
{
    u8 t = st.act_t, atk = st.atk, tgt = st.tgt;
    s16 ax, ay, bx, by;
    if (st.mode != M_ACT || t < ACT_WIND || t >= ACT_END) return;
    if (atk >= NUNIT || tgt >= NUNIT || !vis[atk] || !vis[tgt]) return;   // off screen: nothing to draw
    ax = (s16)(f[atk].x - st.camx); ay = (s16)(f[atk].y - st.camy);
    bx = (s16)(f[tgt].x - st.camx); by = (s16)(f[tgt].y - st.camy);
    if (t < ACT_BLOW)                                   // the caster gathers
        fx_ring(ax, (s16)(ay + UNIT_FOOT), (s16)(2 * (ACT_BLOW - t)), 1);
    else if (t < ACT_HIT)                               // the sweep reaches the target
        fx_slash(ax, (s16)(ay + UNIT_FOOT / 2), bx, (s16)(by + UNIT_FOOT / 2), (u8)(t - ACT_BLOW), 1);
    else if (t < ACT_HIT + FX_FLASH)                    // the impact
        fx_burst(bx, (s16)(by + UNIT_FOOT / 2), (s16)(3 + 4 * (t - ACT_HIT)), 1);
    else                                                // the ring settles and fades out
        fx_ring(bx, (s16)(by + UNIT_FOOT), (s16)(4 + 2 * (t - ACT_HIT - FX_FLASH)), 0);
}
