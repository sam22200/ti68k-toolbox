// The campaign save (campaign/save.h) written after a victory: the chapter's story flag
// (descriptor WON), and when its player's units are the campaign's party (PARTY), those still
// with it (not a crystal or a chest), the bag and the chests' items. FFT's ids throughout (a
// unit's job and equipment already are; SET_POS, BAG_ITEM, RSM_POS, the supports'): the save is
// every chapter's. A unit keeps
// what it was drawn with (battle_units): its job's JP and abilities, a generic one's Squire JP
// and abilities when it is another job, the reactions and supports it learned (Equip Change:
// always), the support it fights with.
#include "campaign/save.h"
static Save save;                       // (a module's: in its image, there until the program exits)

static void be16(u8 *p, u16 v) { p[0] = (u8)(v >> 8); p[1] = (u8)v; }
static void learn(u8 *bits, u8 p) { bits[p >> 3] |= (u8)(0x80 >> (p & 7)); }

static void save_party(void)
{
    const Unit *u = st.unit;
    const u8 *pos = cdesc + CD(SET_POS), *bag = cdesc + CD(BAG_ITEM), *rsm = cdesc + CD(RSM_POS);
    u8 i, k;
    save.n = 0;
    memset(save.unit, 0, sizeof save.unit);
    for (i = 0; i < NUNIT; i++, u++) {
        SaveUnit *s;
        u8 c;
        if (u->team != TEAM_PLAYER || DC_OBJ(u->dc) || save.n == PARTY_MAX) continue;
        s = &save.unit[save.n++];
        s->job = u->job;
        c = (u8)(s->job - 0x4A);           // its career row (another job: the Squire's)
        if (c >= CAREER_N) c = 0;
        s->gfx = u->gfx; s->type = (u8)(i == U_RAMZA ? 2 : u->female); s->name = u->name;
        s->level = u->level; s->exp = u->exp; s->brave = u->brave; s->faith = u->faith;
        s->zodiac = u->zodiac;
        for (k = 0; k < 5; k++) {
            s->raw[k][0] = (u8)(u->raw[k] >> 16); s->raw[k][1] = (u8)(u->raw[k] >> 8); s->raw[k][2] = (u8)u->raw[k];
        }
        for (k = 0; k < EQ_N; k++) s->eq[k] = u->eq[k] == 0xFF ? 0 : u->eq[k];
        be16(s->jp[c], u->jp); be16(s->tjp[c], u->tjp);
        for (k = 0; k < SET_N; k++) if (u->learned & (1 << k)) learn(s->learned[c], pos[u->set * SET_N + k]);
        if (i == U_RAMZA) continue;
        be16(s->support, u->defend ? CD(DEFEND_ID) : CD(EQUIP_CHANGE_ID));
        if (c) {                           // another job: the Squire's drawn too
            be16(s->jp[0], u->sq_jp); be16(s->tjp[0], u->sq_tjp);
            for (k = 0; k < SET_N; k++)
                if (u->sq_learned & (1 << k)) learn(s->learned[0], pos[CD(SET_BASIC) * SET_N + k]);
        }
        for (k = 0; k < NRSM_PRE; k++) if (u->rsm & (1 << k)) learn(s->learned[0], rsm[k]);
        if (u->rsm & 0x80) learn(s->learned[0], rsm[NRSM_PRE]);
        learn(s->learned[1], (u8)CD(EQUIP_CHANGE_POS));
    }
    for (k = 0; k < NBAG; k++) save.inv[bag[k]] = st.bag[k];
    for (k = 0; k < st.nfound; k++) {
        u8 *n = &save.inv[items[st.found[k]].id];
        if (*n < 99) ++*n;
    }
}

static void save_campaign(void)
{
    u8 w = (u8)CD(WON);
    if (!rt_load(SAVE_NAME, &save, sizeof save) || memcmp(save.magic, SAVE_MAGIC, 4)) {
        memset(&save, 0, sizeof save);
        memcpy(save.magic, SAVE_MAGIC, 4);
    }
    if (w--) save.flags[w >> 3] |= (u8)(1 << (w & 7));
    if (CD(LOCATION)) save.loc = (u8)CD(LOCATION);      // the battle's place on the world map
    if (CD(PARTY)) save_party();
    rt_save(SAVE_NAME, &save, sizeof save);
}
#if !MOD_CORE
u16 mod_entry(u8 op, u8 a) { (void)op; (void)a; save_campaign(); return 0; }
#endif
