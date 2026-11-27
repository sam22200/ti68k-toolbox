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

// A horizontal run, in whole bytes wherever it can. A solid shape filled with fx_dot
// would cost a call per pixel and the layer has 8,910 cycles of margin (COMBAT_FX.md);
// per row this touches one byte per eight pixels instead, on both planes.
static void fx_span(s16 x0, s16 x1, s16 y, u8 white, u8 pat)
{
    u8 *lp, *dp, m0, m1;
    s16 b0, b1;
    if ((u16)y >= RT_H) return;
    if (x0 < 0) x0 = 0;
    if (x1 > RT_W - 1) x1 = RT_W - 1;
    if (x1 < x0) return;
    lp = (u8 *)rt_light + (u16)(y * RT_PBYTES);
    dp = (u8 *)rt_dark + (u16)(y * RT_PBYTES);
    b0 = (s16)(x0 >> 3); b1 = (s16)(x1 >> 3);
    m0 = (u8)((0xFFu >> (x0 & 7)) & pat);
    m1 = (u8)((0xFFu << (7 - (x1 & 7))) & pat);
    if (b0 == b1) {
        u8 m = (u8)(m0 & m1);
        if (white) { lp[b0] &= (u8)~m; dp[b0] &= (u8)~m; }
        else       { lp[b0] |= m;      dp[b0] |= m; }
        return;
    }
    if (white) { lp[b0] &= (u8)~m0; dp[b0] &= (u8)~m0; }
    else       { lp[b0] |= m0;      dp[b0] |= m0; }
    for (b0++; b0 < b1; b0++) {
        if (white) { lp[b0] &= (u8)~pat; dp[b0] &= (u8)~pat; }
        else       { lp[b0] |= pat;      dp[b0] |= pat; }
    }
    if (white) { lp[b1] &= (u8)~m1; dp[b1] &= (u8)~m1; }
    else       { lp[b1] |= m1;      dp[b1] |= m1; }
}

// Night Sword's own shape, read off EFFECT/E173.BIN rather than invented: its frame 0 is
// a 40x136 atlas cell whose quad spans y -26..110 about the anchor -- a tall spike, point
// up, base at the target. At the project's 0.6 sprite scale that is 24x82, which is how
// large it is in the original: about three and a half times a unit's height.
//
// This is a SOLID shape, and that is the whole point. A one-pixel halo around a scatter of
// one-pixel dots is four blacks for one white and reads worse, not better (COMBAT_FX.md);
// around a silhouette it is the project's ordinary visibility rule and it works. The edge
// is drawn black, the body white, the body after the edge so the two never fight.
//
// The half-width accumulates in 8.8 instead of dividing once a row: 82 divisions would
// cost more than the fill itself.
// E173's own height is 136, which is 82 rows at the project's 0.6 sprite scale. The
// budget pays 28 of them at about 1,220 cycles a row: H=32 measures 354,778 and H=36
// measures 359,566, which passes with 434 cycles left -- not a margin, a bet that this
// scenario reaches the worst frame. 28 keeps the margin the layer had before the asm.
#define FX_SPIKE_H 24                   // the pillar, by measurement: see the sweep below
#define FX_SPIKE_W 12                   // 40 * 0.6 / 2, its half-width at the base
// It comes down, holds, and thins away -- the three the original shows, in its own
// proportions: the wisp reaches the ground about twelve of its frames after it appears,
// flares, and is gone twelve more later. Twelve of its frames is eight of our ticks.
//
// The thinning is in WIDTH, not in height: at wash+54 the original still has a tall
// column, only a narrower and sparkier one. It does not retract towards the sky.
//
// Eight each, so the three are shifts and not divisions. This runs on every frame of a
// wash and a 32-bit divide would cost about 900 cycles of the 1,220 a row already costs.
// Past the third phase it returns at once: the tail of a long hold then draws nothing,
// where before it paid 28 rows a frame to hold a frozen picture.
// Stasis Sword, read frame by frame off the capture rather than off the atlas picture.
// Bright blue pixels tracked through the wash put the effect in x 83..143, y 0..169 of a
// 256x240 frame -- it starts at the very TOP of the screen, not above the target.
//
//   +24  a faceted shard of ice appears at the top of the screen, point down
//   +28  it falls, elongated
//   +32  it reaches the target AND a pillar of light erupts upward from the target
//   +36  the pillar holds: white at the foot, blue going up, sparks running along it
//   +56  more shards keep falling into it; then it thins away
//
// **There are two movements, not one.** Shards coming down, and a pillar going up. The
// two earlier versions of this each drew one cone: first rising, then -- correcting the
// direction -- falling. Both replaced the whole sequence with half of it, and the second
// lost the pillar, which is the part that carries the effect, to gain the shard.
#define FX_ICE_FALL 5                   // ticks: the blocks close on the target
#define FX_ICE_RISE 4                   // the pillar grows out of it
#define FX_ICE_HOLD 8                   // it stands, shards still falling
#define FX_ICE_GONE 6                   // it thins away
// The original's bright core is about 20 px of a 256-wide frame, which is 12 of our 160
// -- a half-width of 6, roughly one unit across. FX_SPIKE_W is 12, and at that the foot
// read as a white bar rather than a column.
#define FX_PILLAR_W 6
#define FX_SHARD_H 14
#define FX_SHARD_W 3

// Blocks of ice, converging. This is what E163's animation says and what one shard
// could never say: of its 42 records, 16..24 are the SAME 40x48 cell over nine frames
// with its quad corners shearing and closing in -- x -16..9 down to -2..2 -- and 28..38
// are another doing the same. That convergence is the effect. Around it sit three 16x16
// cells and a handful of 8x8 ones: distinct pieces, not one mass.
//
// `effect_rip.py --frames` printed only the first six records until today, which is why
// three versions of this were built from a seventh of what the disc says.
#define FX_BLOCKS 3
#define FX_FALL_SHIFT 2                 // FX_ICE_FALL - 1 == 4: the convergence is a shift
static const s8 fx_blk[FX_BLOCKS][2] = { { -13, -58 }, { 11, -72 }, { -4, -82 } };

// One block: a solid diamond with its outline. Solid, not dithered -- these read as ice
// only if they have an edge and a body, and they are small enough to pay for it.
static void fx_block(s16 cx, s16 cy, s16 size)
{
    s16 r;
    for (r = (s16)-size; r <= size; r++) {
        s16 y = (s16)(cy + r), w = (s16)(size - (r < 0 ? -r : r));
        if ((u16)y >= RT_H) continue;
        fx_dot((s16)(cx - w - 1), y, 0);
        fx_dot((s16)(cx + w + 1), y, 0);
        fx_span((s16)(cx - w), (s16)(cx + w), y, 1, 0xFFu);
    }
}

// They start spread above the target and close on it: k runs FX_ICE_FALL-1 down to 0, so
// at the last tick every block is on the target and the pillar takes over.
static void fx_blocks(s16 cx, s16 base, u8 t)
{
    u8 i;
    s16 k = (s16)(FX_ICE_FALL - 1 - (s16)t);
    for (i = 0; i < FX_BLOCKS; i++)
        fx_block((s16)(cx + ((fx_blk[i][0] * k) >> FX_FALL_SHIFT)),
                 (s16)(base + ((fx_blk[i][1] * k) >> FX_FALL_SHIFT)), (s16)(4 - i));
}

// The pillar: out of the target and upward, widest at the foot and tapering, solid for
// the first rows and a checkerboard above -- the original is white at the base and blue
// as it climbs, and four greys say that with density. Sparks are a hash of the row and
// the frame, never the RNG: the picture must not consume it.
static void fx_pillar(s16 cx, s16 base, s16 h, s16 wmax, u8 t)
{
    s16 r;
    s32 w8 = (s32)wmax << 8, step = ((s32)wmax << 7) / FX_SPIKE_H;   // tapers to half
    for (r = 0; r < h; r++, w8 -= step) {
        s16 y = (s16)(base - r);
        s16 w = (s16)(w8 >> 8);
        if ((u16)y >= RT_H) break;
        if (w < 1) break;
        fx_dot((s16)(cx - w - 1), y, 0);
        fx_dot((s16)(cx + w + 1), y, 0);
        fx_span((s16)(cx - w), (s16)(cx + w), y, 1,
                r < 5 ? 0xFFu : (u8)((y & 1) ? 0xAAu : 0x55u));      // white at the foot
    }
    {   // Three sparks climbing it. The height they are at is masked, not taken modulo
        // h: a modulo is a division, three of them a frame, and this runs on every frame
        // of a wash. Clamped instead of wrapped, so a spark rests at the top rather than
        // jumping back to the foot.
        u8 i;
        for (i = 0; i < 3; i++) {
            u16 hsh = (u16)(i * 2654U + t * 97U);
            s16 up = (s16)((t * 2 + i * 9) & 31);
            if (up >= h) up = (s16)(h - 1);
            fx_dot((s16)(cx + ((hsh >> 4) & 7) - 3), (s16)(base - up), 1);
        }
    }
}

static void fx_spike(s16 cx, s16 base, u8 t)
{
    // From off the top of the screen down to the target, not from the middle of it: the
    // tip starts at -FX_SHARD_H, so the shard enters the picture rather than appearing
    // in it, and lands ON the target instead of 14 rows under its feet.
    if (t < FX_ICE_FALL) { fx_blocks(cx, base, t); return; }
    t = (u8)(t - FX_ICE_FALL);
    if (t < FX_ICE_RISE) {                       // the pillar grows out of the impact
        fx_pillar(cx, base, (s16)(((s16)(t + 1) * FX_SPIKE_H) >> 2), FX_PILLAR_W, t);
        return;
    }
    t = (u8)(t - FX_ICE_RISE);
    if (t < FX_ICE_HOLD) {                       // it stands, and shards keep coming
        fx_pillar(cx, base, FX_SPIKE_H, FX_PILLAR_W, t);
        // The original keeps dropping shards into the pillar all through the hold. Ours
        // does not: a second shard is 14 more rows on every frame of it, and that measured
        // 377,886 against the 360,000 limit. The pillar is what carries the effect, so the
        // shards are the part that goes.
        return;
    }
    t = (u8)(t - FX_ICE_HOLD);
    if (t < FX_ICE_GONE)                         // it thins away
        fx_pillar(cx, base, FX_SPIKE_H, (s16)((FX_PILLAR_W * (FX_ICE_GONE - t)) / FX_ICE_GONE), t);
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

// Whether the original stages an ability -- dims and washes the picture for it --
// is **a fact per ability**, not something derivable. Two candidate keys were
// tried and each fell to a counter-example from the measurements:
//
//   st.effect_kind     a Holy Sword and a plain Attack both deal damage, and
//                      the original stages one and not the other
//   abilities[].flags  Dash and Wish both carry 0x00, and Wish dims 3.03 s
//                      while Dash plays in full light. Night Sword dims with
//                      no AF_CAST; nothing observed carries AF_CHARGE
//
// So it is stored the way the original stores it: against the ability. In FFT an
// ability points at an EFFECT file and the file carries its own staging, which is
// why no rule was ever going to fall out of the ability's own fields.
//
// One bit an ability in fx_staged. fx_known says which of those bits is MEASURED
// (tools/effect_stage.py and tools/effect_watch.py) rather than defaulted, so a
// reader can tell a fact from a blank -- the previous table could not, and its
// six "measured" rows were right only by accident of what had been watched.
//
//   staged, measured   Wish 3.03 s, Stasis Sword 1.10 s, Night Sword 1.07..5.47 s,
//                      Potion 1.63 s, Antidote 1.63 s, Eye Drop 1.97 s,
//                      Echo Grass 1.40 s, Phoenix Down 1.47 s
//   plain, measured    Dash, Throw Stone, Heal, and the plain Attack
//
// Everything unmeasured defaults to **not staged**: the dimming costs 55
// kilocycles a frame (campaign/COMBAT_FX.md), and spending that on a guess is
// worse than leaving an effect plain until somebody watches it.
#define FX_ABILITIES 512
static const u8 fx_staged[FX_ABILITIES / 8] = {
    [0x13] = 0x09,                      /* 0x098 Wish, 0x09b Stasis Sword */
    [0x14] = 0x20,                      /* 0x0a5 Night Sword */
    [0x2E] = 0xC1,                      /* 0x170 Potion, 0x176 Antidote, 0x177 Eye Drop */
    [0x2F] = 0x21,                      /* 0x178 Echo Grass, 0x17d Phoenix Down */
};
static const u8 fx_known[FX_ABILITIES / 8] = {
    [0x12] = 0x38,                      /* 0x093 Dash, 0x094 Throw Stone, 0x095 Heal */
    [0x13] = 0x09, [0x14] = 0x20, [0x2E] = 0xC1, [0x2F] = 0x21,
};
#define FX_STAGES(a) ((u16)(a) < FX_ABILITIES && (fx_staged[(a) >> 3] >> ((a) & 7) & 1))

// How long it holds, per ability: the measured second itself, at 30 ticks a second.
//
// It used to be a proportion. The action window was 39 ticks = 1.30 s and six of the
// eight measured abilities outlast that, so literal seconds saturated and the duration
// stopped telling them apart. The window is no longer fixed: a staged ability runs its
// action for as long as its wash lasts (fx_act_end), which is what the original does, so
// the scale hack is gone and these are the seconds off tools/effect_capture.py.
//
// Night Sword is the one to distrust: 1.07, 1.80, 1.83, 2.52 and 5.47 s across two runs.
// 1.83 s is the middle of that spread, not a settled value.
#define FX_HOLD_FULL (ACT_END - ACT_BLOW)       // the whole window: 31 ticks
#define FX_HOLDS 8
static const u16 fx_hold_abil[FX_HOLDS] = { 0x0a5, 0x09b, 0x178, 0x17d, 0x170, 0x176, 0x177, 0x098 };
static const u8  fx_hold_tick[FX_HOLDS] = {    55,    33,    42,    44,    49,    49,    59,    91 };
//   Night Sword 1.83 s, Stasis Sword 1.10 s, Echo Grass 1.40 s, Phoenix Down 1.47 s,
//   Potion 1.63 s, Antidote 1.63 s, Eye Drop 1.97 s, Wish 3.03 s

// Eight entries, scanned when an action is staged. Not cached: Work is full, and growing
// it changes MOD_SIG and makes the core refuse every built module image (fx_focus.inc).
// Eight u16 compares against a 55-kilocycle dimming is not a cost worth a byte of RAM.
static u8 fx_hold_of(u16 a)
{
    u8 i;
    for (i = 0; i < FX_HOLDS; i++)
        if (fx_hold_abil[i] == a) return fx_hold_tick[i];
    return FX_HOLD_FULL;
}

// A staged action runs until its wash is done. The original holds Stasis Sword 1.10 s and
// Wish 3.03 s, while this engine's whole action is 1.30 s -- shorter than one of them --
// which is why the effects read as hurried whatever the hold table says. An ability that
// is not staged keeps exactly the length it always had, and the plain Attack is not
// staged, so the pacing the rest of the suite checks is untouched.
static u8 fx_act_end(void)
{
    u8 e = (u8)(st.gain ? ACT_GAIN_END : ACT_END);
    if (FX_STAGES(st.abil)) {
        u8 h = (u8)(ACT_BLOW + fx_hold_of(st.abil));
        if (h > e) e = h;
    }
    return e;
}

// What plays at each phase, per outcome category. This half is NOT measured: the
// primitives were chosen, and only the staging above comes from the original.
enum { FXP_NONE, FXP_RING, FXP_SLASH, FXP_BURST, FXP_BEAM, FXP_COLUMN, FXP_MOTES, FXP_SPIKE };
#define FX_KINDS 32                     // EFFECT_KIND_MASK + 1
static const u8 fx_table[FX_KINDS][4] = {   // gather, travel, impact, settle
    // The spike does NOT play at the impact: on the original the picture darkens with the
    // blow and the effect erupts about 0.6 s later (tools/effect_capture.py). Phase 2 is
    // the four flash frames, so the spike belongs to phase 3, which starts at ACT_HIT+4.
    [AB_DAMAGE]          = { FXP_RING,   FXP_SLASH,  FXP_NONE,   FXP_SPIKE },   // the sword arts: E173's spike
    // AB_CURE is the STATUS cure, not a heal: Heal (0x095) and Antidote (0x176) both
    // resolve to it with amount 0 -- the Squire's Heal restores no HP at all, it lifts
    // Poison, Darkness and Silence. HP restoration is AB_HEAL (Potion, amount 30). The
    // sparkle is right for it and a solid shape would not be: a cure is not a blow.
    [AB_CURE]            = { FXP_MOTES,  FXP_NONE,   FXP_RING,   FXP_RING  },
    [AB_HEAL]            = { FXP_MOTES,  FXP_NONE,   FXP_MOTES,  FXP_RING  },
    [AB_CLEANSE]         = { FXP_MOTES,  FXP_NONE,   FXP_RING,   FXP_RING  },
    [AB_REVIVE]          = { FXP_MOTES,  FXP_COLUMN, FXP_COLUMN, FXP_RING  },
    [AB_WISH]            = { FXP_MOTES,  FXP_BEAM,   FXP_MOTES,  FXP_NONE  },
    [AB_HP_DAMAGE_MP_HEAL] = { FXP_RING, FXP_BEAM,   FXP_BURST,  FXP_MOTES },
    [AB_MP_DAMAGE]       = { FXP_RING,   FXP_BEAM,   FXP_BURST,  FXP_NONE  },
    [AB_HP_MP_HEAL]      = { FXP_MOTES,  FXP_NONE,   FXP_MOTES,  FXP_RING  },
    [AB_MP_HEAL]         = { FXP_MOTES,  FXP_NONE,   FXP_MOTES,  FXP_NONE  },
    [AB_RAISE]           = { FXP_MOTES,  FXP_COLUMN, FXP_COLUMN, FXP_RING  },
    [AB_STATUS]          = { FXP_RING,   FXP_BEAM,   FXP_RING,   FXP_RING  },
    [AB_DEFEND]          = { FXP_RING,   FXP_NONE,   FXP_RING,   FXP_RING  },
    [AB_GOLEM]           = { FXP_RING,   FXP_NONE,   FXP_RING,   FXP_RING  },
    [AB_REFLECT]         = { FXP_RING,   FXP_NONE,   FXP_RING,   FXP_RING  },
    [AB_BRAVE]           = { FXP_MOTES,  FXP_NONE,   FXP_MOTES,  FXP_NONE  },
    [AB_FAITH]           = { FXP_MOTES,  FXP_NONE,   FXP_MOTES,  FXP_NONE  },
    [AB_SPEED]           = { FXP_RING,   FXP_NONE,   FXP_RING,   FXP_RING  },
    [AB_QUICK]           = { FXP_RING,   FXP_NONE,   FXP_RING,   FXP_RING  },
    [AB_DEATH]           = { FXP_COLUMN, FXP_BEAM,   FXP_BURST,  FXP_NONE  },
    [AB_BREAK]           = { FXP_RING,   FXP_SLASH,  FXP_BURST,  FXP_NONE  },
    [AB_EQUIP_BREAK]     = { FXP_RING,   FXP_SLASH,  FXP_BURST,  FXP_NONE  },
    [AB_STEAL_EQUIP]     = { FXP_NONE,   FXP_SLASH,  FXP_MOTES,  FXP_NONE  },
    [AB_STEAL_GIL]       = { FXP_NONE,   FXP_SLASH,  FXP_MOTES,  FXP_NONE  },
    [AB_STEAL_EXP]       = { FXP_NONE,   FXP_SLASH,  FXP_MOTES,  FXP_NONE  },
    [AB_CATCH]           = { FXP_NONE,   FXP_NONE,   FXP_RING,   FXP_NONE  },
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
    // The wash starts with the BLOW, not with the wind-up. Captured on the original
    // (tools/effect_capture.py, Stasis Sword): the caster draws and raises the blade over
    // some twenty frames in full light, the picture darkens on the frame the swing goes
    // out, and the effect itself erupts about 0.6 s after that. Starting at ACT_WIND dimmed
    // the whole preparation, which the original shows lit -- and it charged FX_DARK_K for
    // eight frames that should not have been paying it.
    if (t < ACT_BLOW || t >= fx_act_end()) { if (W->fx_sel) fx_focus(FX_NO_UNIT, 0); return; }
    // The ability decides, not its outcome: the original's staging is a fact per
    // ability and the table above holds it, measured where it is known.
    if (!FX_STAGES(st.abil)) { if (W->fx_sel) fx_focus(FX_NO_UNIT, 0); return; }
    // ...and it decides for how long, too: the wash ends when the ability's measured
    // hold runs out, which for most of them is before the action does.
    if ((u8)(t - ACT_BLOW) >= fx_hold_of(st.abil)) { if (W->fx_sel) fx_focus(FX_NO_UNIT, 0); return; }
    // The caster stays lit through the swing and the target takes over at the impact:
    // on the original the lit figure is Agrias while the blade travels, and the effect
    // only erupts on the target afterwards.
    fx_focus(t < ACT_HIT ? st.atk : st.tgt, 1);
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
    case FXP_SPIKE:  fx_spike(bx, (s16)(by + UNIT_FOOT), t); break;
    default: break;
    }
}

// The animation, after the dimming and the focused unit, so it reads over both.
static void fx_play(const Feet *f, const u8 *vis)
{
    u8 t = st.act_t, atk = st.atk, tgt = st.tgt, phase, kind;
    s16 ax, ay, bx, by;
    if (st.mode != M_ACT || t < ACT_WIND || t >= fx_act_end()) return;
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
