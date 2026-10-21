// The common actor bank's frame cache (actors.h).
// A logical frame (units.h / actors.h) of the common actor bank, decoded into its
// unit's slot of the frame cache: at most one ordinary decode a frame (actor_budget).
static const u8 *actor_frame(u16 logical, u8 slot)
{
    u16 frame_key = ACTOR_KEY(logical), id = frame_key & 0x7FFF, best = 0;
    u8 e, victim = 0;
    for (e = 0; e < AC_N && ac_key[e] != frame_key; e++) ;
    if (e == AC_N) {
        // Animation alone can wait a frame (its pose still cached); a new actor never
        // borrows another's appearance. One ordinary miss per frame bounds decompression.
        e = W->actor_ent[slot];
        if (!W->actor_budget && W->actor_key[slot] != 0xFFFF && slot < ACTOR_UNITS && ac_key[e] == W->actor_key[slot]) {
            ac_age[e] = W->ac_now;
            return (const u8 *)acache[e];
        }
        if (W->actor_budget) W->actor_budget--;
        ZB(20);
        // The least recently drawn entry: never one drawn this frame (48 > 13 actors).
        for (e = 0; e < AC_N; e++) {
            u16 unused = (u16)(W->ac_now - ac_age[e]);   // (not "idle": a TIGCC ROM call)
            if (ac_key[e] == 0xFFFF) { victim = e; break; }
            if (unused >= best) { best = unused; victim = e; }
        }
        e = victim;
        {   // frames are numbered across ffca then ffcb (tools/common_actors.py)
            const u8 *b = abank[0]; u16 r = id;
            if (r >= abank_n0) { b = abank[1]; r -= abank_n0; }
            b += 2 + 2 * RD16(b, 0) + RD16(b + 2, r);
#ifdef __m68k__
            if (frame_key & 0x8000) {           // every 16-pixel row of the four planes; the
                u16 tmp[UNIT_FB / 2];           // routine writes each row backwards: never in place
                zx0_unpack(b, (u8 *)tmp);
                SpriteX8_MIRROR_H_R(UNIT_FB / 2, (const u8 *)tmp, 2, (u8 *)acache[e]);
            } else
#endif
            zx0_unpack(b, (u8 *)acache[e]);
        }
#ifndef __m68k__
        if (frame_key & 0x8000) {
            static const u8 rev[16] = {0,8,4,12,2,10,6,14,1,9,5,13,3,11,7,15};
            u8 *p = (u8 *)acache[e], *end = p + UNIT_FB;
            for (; p < end; p += 2) {
                u8 a = p[0], c = p[1];
                p[0] = (u8)(rev[c & 15] << 4 | rev[c >> 4]);
                p[1] = (u8)(rev[a & 15] << 4 | rev[a >> 4]);
            }
        }
#endif
        ac_key[e] = frame_key;
        W->actor_misses++;
        ZE(20);
    }
    ac_age[e] = W->ac_now;
    W->actor_key[slot] = frame_key; W->actor_ent[slot] = e;
    return (const u8 *)acache[e];
}

// Forget every slot's pose and every cached frame (a new battle or chapter scene).
static void actor_reset(void)
{
    memset(W->actor_key, 255, sizeof W->actor_key);
    memset(ac_key, 255, AC_N * 2);
    W->actor_budget = 255; W->actor_misses = W->actor_next = 0;
}
