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

// Effects are NOT depth tested. seen(), the per-pixel test the water glints use, is
// compiled out when the chapter's water bank is in (FFT_WATER_BANK), so a depth-aware
// dot would do nothing in the configuration that actually ships. Sorting effects behind
// the scenery is left undone rather than half done; campaign/COMBAT_FX.md says so.

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

// A beam: the whole line from the caster to the target, held while it travels.
static void fx_beam(s16 x0, s16 y0, s16 x1, s16 y1, u8 white)
{
    s16 dx = (s16)(x1 - x0), dy = (s16)(y1 - y0), i;
    for (i = 0; i <= 32; i++)
        fx_dot((s16)(x0 + (s16)(((s32)dx * i) >> 5)), (s16)(y0 + (s16)(((s32)dy * i) >> 5)), white);
}

// A column coming down onto the target: Bolt, Holy and Raise.
static void fx_column(s16 cx, s16 cy, u8 t, u8 white)
{
    s16 y, top = (s16)(cy - 44 + 4 * t);
    if (top < cy - 44) top = (s16)(cy - 44);
    for (y = top; y < cy; y += 2) { fx_dot(cx, y, white); fx_dot((s16)(cx + 1), y, white); }
}

// Motes rising around a unit: Cure, Raise, and the drain back to the caster. Scattered by
// a hash of the index and the frame, never by the RNG: the picture must not consume it.
static void fx_motes(s16 cx, s16 cy, u8 t, u8 n, u8 white)
{
    u8 i;
    for (i = 0; i < n; i++) {
        u16 h = (u16)(i * 2654U + t * 97U);
        s16 dx = (s16)(((h >> 3) & 15) - 8);
        s16 up = (s16)((t + i * 3) & 15);
        fx_dot((s16)(cx + dx), (s16)(cy - up), white);
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

// What plays at each phase, and whether the picture dims for it.
//
// The engine already classifies the action -- `st.effect_kind` carries the AB_*
// category act_kind() resolves -- so this is a table, not a second classifier.
// Five bytes a row, const, in the module image: no RAM, and an ability moves to
// another look by editing one row.
//
// The DIM column is measured on the original, with `tools/effect_watch.py`, and
// it does not follow the outcome category. Heal and Antidote both strip a status
// and the original stages them oppositely:
//
//     no dimming   Dash, Throw Stone, Attack, Heal      (skills, physical acts)
//     dimming      Wish 3.03 s, Potion 1.63 s, Antidote 1.63 s,
//                  Eye Drop 1.97 s, Echo Grass 1.40 s, Phoenix Down 1.47 s
//
// So what decides the staging is the *kind of action* -- a physical skill plays
// in full light, an item or an incantation gets the stage -- and this table is
// keyed on the outcome. Six rows below are measured; the rest are marked NOT
// OBSERVED and default to dimming because most of them are cast. Do not read an
// unmarked row as evidence.
enum { FXP_NONE, FXP_RING, FXP_SLASH, FXP_BURST, FXP_BEAM, FXP_COLUMN, FXP_MOTES };
#define FX_KINDS 32                     // EFFECT_KIND_MASK + 1
#define FX_DIM 4                        // the fifth byte of a row
#define D_NO 0                          // measured: plays in full light
#define D_YES 1                         // measured: the picture dims
#define D_GUESS 1                       // NOT OBSERVED, dimming assumed
static const u8 fx_table[FX_KINDS][5] = {   // gather, travel, impact, settle, dim
    [AB_DAMAGE]          = { FXP_RING,   FXP_SLASH,  FXP_BURST,  FXP_RING,  D_NO    },  // Dash, Stone, Attack
    [AB_CURE]            = { FXP_MOTES,  FXP_NONE,   FXP_RING,   FXP_RING,  D_NO    },  // Heal
    [AB_HEAL]            = { FXP_MOTES,  FXP_NONE,   FXP_MOTES,  FXP_RING,  D_YES   },  // Potion
    [AB_CLEANSE]         = { FXP_MOTES,  FXP_NONE,   FXP_RING,   FXP_RING,  D_YES   },  // Antidote, Echo Grass
    [AB_REVIVE]          = { FXP_MOTES,  FXP_COLUMN, FXP_COLUMN, FXP_RING,  D_YES   },  // Phoenix Down
    [AB_WISH]            = { FXP_MOTES,  FXP_BEAM,   FXP_MOTES,  FXP_NONE,  D_YES   },  // Wish
    [AB_HP_DAMAGE_MP_HEAL] = { FXP_RING, FXP_BEAM,   FXP_BURST,  FXP_MOTES, D_GUESS },
    [AB_MP_DAMAGE]       = { FXP_RING,   FXP_BEAM,   FXP_BURST,  FXP_NONE,  D_GUESS },
    [AB_HP_MP_HEAL]      = { FXP_MOTES,  FXP_NONE,   FXP_MOTES,  FXP_RING,  D_GUESS },
    [AB_MP_HEAL]         = { FXP_MOTES,  FXP_NONE,   FXP_MOTES,  FXP_NONE,  D_GUESS },
    [AB_RAISE]           = { FXP_MOTES,  FXP_COLUMN, FXP_COLUMN, FXP_RING,  D_GUESS },
    [AB_STATUS]          = { FXP_RING,   FXP_BEAM,   FXP_RING,   FXP_RING,  D_GUESS },
    [AB_DEFEND]          = { FXP_RING,   FXP_NONE,   FXP_RING,   FXP_RING,  D_GUESS },
    [AB_GOLEM]           = { FXP_RING,   FXP_NONE,   FXP_RING,   FXP_RING,  D_GUESS },
    [AB_REFLECT]         = { FXP_RING,   FXP_NONE,   FXP_RING,   FXP_RING,  D_GUESS },
    [AB_BRAVE]           = { FXP_MOTES,  FXP_NONE,   FXP_MOTES,  FXP_NONE,  D_GUESS },
    [AB_FAITH]           = { FXP_MOTES,  FXP_NONE,   FXP_MOTES,  FXP_NONE,  D_GUESS },
    [AB_SPEED]           = { FXP_RING,   FXP_NONE,   FXP_RING,   FXP_RING,  D_GUESS },
    [AB_QUICK]           = { FXP_RING,   FXP_NONE,   FXP_RING,   FXP_RING,  D_GUESS },
    [AB_DEATH]           = { FXP_COLUMN, FXP_BEAM,   FXP_BURST,  FXP_NONE,  D_GUESS },
    [AB_BREAK]           = { FXP_RING,   FXP_SLASH,  FXP_BURST,  FXP_NONE,  D_NO    },  // like a weapon blow
    [AB_EQUIP_BREAK]     = { FXP_RING,   FXP_SLASH,  FXP_BURST,  FXP_NONE,  D_NO    },
    [AB_STEAL_EQUIP]     = { FXP_NONE,   FXP_SLASH,  FXP_MOTES,  FXP_NONE,  D_NO    },
    [AB_STEAL_GIL]       = { FXP_NONE,   FXP_SLASH,  FXP_MOTES,  FXP_NONE,  D_NO    },
    [AB_STEAL_EXP]       = { FXP_NONE,   FXP_SLASH,  FXP_MOTES,  FXP_NONE,  D_NO    },
    [AB_CATCH]           = { FXP_NONE,   FXP_NONE,   FXP_RING,   FXP_NONE,  D_NO    },
};

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
    {   // the original stages an item or an incantation and leaves a physical act
        // in full light (tools/effect_watch.py), so the row decides, not the phase
        u8 kind = (u8)(st.effect_kind & EFFECT_KIND_MASK);
        if (kind >= FX_KINDS || !fx_table[kind][FX_DIM]) { if (W->fx_sel) fx_focus(FX_NO_UNIT, 0); return; }
        fx_focus(t < ACT_BLOW ? st.atk : st.tgt, 1);
    }
}

// One primitive, placed for this phase. Separated from the schedule so the table says
// what plays and this says where, and neither has to know the other's business.
static void fx_draw(u8 which, u8 t, s16 ax, s16 ay, s16 bx, s16 by)
{
    switch (which) {
    case FXP_RING:   fx_ring(bx, (s16)(by + UNIT_FOOT), (s16)(4 + 2 * (t & 15)), 1); break;
    case FXP_SLASH:  fx_slash(ax, (s16)(ay + UNIT_FOOT / 2), bx, (s16)(by + UNIT_FOOT / 2), t, 1); break;
    case FXP_BURST:  fx_burst(bx, (s16)(by + UNIT_FOOT / 2), (s16)(3 + 4 * (t & 7)), 1); break;
    case FXP_BEAM:   fx_beam(ax, (s16)(ay + UNIT_FOOT / 2), bx, (s16)(by + UNIT_FOOT / 2), 1); break;
    case FXP_COLUMN: fx_column(bx, (s16)(by + UNIT_FOOT), t, 1); break;
    case FXP_MOTES:  fx_motes(bx, (s16)(by + UNIT_FOOT), t, 10, 1); break;
    default: break;
    }
}

// The animation, after the dimming and the focused unit, so it reads over both.
static void fx_play(const Feet *f, const u8 *vis)
{
    u8 t = st.act_t, atk = st.atk, tgt = st.tgt, phase, kind;
    s16 ax, ay, bx, by;
    if (st.mode != M_ACT || t < ACT_WIND || t >= ACT_END) return;
    if (atk >= NUNIT || tgt >= NUNIT || !vis[atk] || !vis[tgt]) return;   // off screen: nothing to draw
    ax = (s16)(f[atk].x - st.camx); ay = (s16)(f[atk].y - st.camy);
    bx = (s16)(f[tgt].x - st.camx); by = (s16)(f[tgt].y - st.camy);
    phase = t < ACT_BLOW ? 0 : t < ACT_HIT ? 1 : t < ACT_HIT + FX_FLASH ? 2 : 3;
    kind  = (u8)(st.effect_kind & EFFECT_KIND_MASK);
    if (kind >= FX_KINDS) return;
    fx_draw(fx_table[kind][phase],
            (u8)(phase == 0 ? ACT_BLOW - t : phase == 1 ? t - ACT_BLOW
                 : phase == 2 ? t - ACT_HIT : t - ACT_HIT - FX_FLASH),
            ax, ay, bx, by);
}
