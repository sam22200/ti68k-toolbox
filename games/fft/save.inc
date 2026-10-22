// The campaign save (campaign/save.h) written after a victory: the chapter's story flag
// (descriptor WON), and when its player's units are the campaign's party (PARTY), those still
// with it (not a crystal or a chest) and the bag. FFT's ids throughout (JOB_ID, SET_POS,
// BAG_ITEM, the supports'): the save is every chapter's. Only what a battle unit keeps is
// written: its own job's JP and abilities, the support it fights with.
#include "campaign/save.h"
static Save save;                       // (a module's: in its image, there until the program exits)

static void be16(u8 *p, u16 v) { p[0] = (u8)(v >> 8); p[1] = (u8)v; }

static void save_party(void)
{
    const Unit *u = st.unit;
    const u8 *pos = cdesc + CD(SET_POS), *bag = cdesc + CD(BAG_ITEM);
    u8 i, k;
    save.n = 0;
    memset(save.unit, 0, sizeof save.unit);
    for (i = 0; i < NUNIT; i++, u++) {
        SaveUnit *s;
        u8 c;
        if (u->team != TEAM_PLAYER || DC_OBJ(u->dc) || save.n == PARTY_MAX) continue;
        s = &save.unit[save.n++];
        s->job = cdesc[CD(JOB_ID) + u->job];
        c = (u8)(s->job - 0x4A);           // its career row (another job: the Squire's)
        if (c >= CAREER_N) c = 0;
        s->gfx = u->gfx; s->type = (u8)(i == U_RAMZA ? 2 : u->female); s->name = u->name;
        s->level = u->level; s->exp = u->exp; s->brave = u->brave; s->faith = u->faith;
        s->zodiac = u->zodiac;
        for (k = 0; k < 5; k++) {
            s->raw[k][0] = (u8)(u->raw[k] >> 16); s->raw[k][1] = (u8)(u->raw[k] >> 8); s->raw[k][2] = (u8)u->raw[k];
        }
        for (k = 0; k < EQ_N; k++) s->eq[k] = u->eq[k] == 0xFF ? 0 : items[u->eq[k]].id;
        if (i != U_RAMZA) be16(s->support, u->defend ? CD(DEFEND_ID) : CD(EQUIP_CHANGE_ID));
        be16(s->jp[c], u->jp); be16(s->tjp[c], u->tjp);
        for (k = 0; k < SET_N; k++) if (u->learned & (1 << k)) {
            u8 p = pos[u->set * SET_N + k];
            s->learned[c][p >> 3] |= (u8)(0x80 >> (p & 7));
        }
    }
    for (k = 0; k < NBAG; k++) save.inv[bag[k]] = st.bag[k];
}

static void save_campaign(void)
{
    u8 w = (u8)CD(WON);
    if (!rt_load(SAVE_NAME, &save, sizeof save) || memcmp(save.magic, SAVE_MAGIC, 4)) {
        memset(&save, 0, sizeof save);
        memcpy(save.magic, SAVE_MAGIC, 4);
    }
    if (w--) save.flags[w >> 3] |= (u8)(1 << (w & 7));
    if (CD(PARTY)) save_party();
    rt_save(SAVE_NAME, &save, sizeof save);
}
#if !MOD_CORE
u16 mod_entry(u8 op, u8 a) { (void)op; (void)a; save_campaign(); return 0; }
#endif
