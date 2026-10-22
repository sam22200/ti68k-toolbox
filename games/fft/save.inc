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

// the party the battle took from the save (party_load): the records keep every job's JP and
// abilities; the battle changes a unit's level, EXP and its job's JP; the units lost (a crystal
// or a chest) leave the party
static void party_update(void)
{
    const Unit *u = st.unit;
    u8 i, k, n = 0, lost[PARTY_MAX];
    memset(lost, 0, sizeof lost);
    for (i = 0; i < NUNIT; i++, u++) {
        SaveUnit *s;
        u8 c;
        if (u->team != TEAM_PLAYER || u->slot >= save.n) continue;
        if (DC_OBJ(u->dc)) { lost[u->slot] = 1; continue; }
        s = &save.unit[u->slot];
        c = (u8)(s->job - 0x4A);
        if (c >= CAREER_N) c = 0;
        s->level = u->level; s->exp = u->exp;
        be16(s->jp[c], u->jp); be16(s->tjp[c], u->tjp);
    }
    for (k = 0; k < save.n; k++) if (!lost[k]) {
        if (n != k) memcpy(&save.unit[n], &save.unit[k], sizeof save.unit[0]);
        n++;
    }
    for (k = n; k < save.n; k++) memset(&save.unit[k], 0, sizeof save.unit[0]);
    save.n = n;
}

static void save_party(void)
{
    const Unit *u = st.unit;
    const u8 *pos = cdesc + CD(SET_POS), *bag = cdesc + CD(BAG_ITEM), *rsm = cdesc + CD(RSM_POS);
    u8 i, k;
    if (st.party_in) { party_update(); goto bag; }
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
bag:
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
    if (CD(PARTY) & 1 || st.party_in) save_party();
    rt_save(SAVE_NAME, &save, sizeof save);
}

// The player's units from the campaign save (descriptor PARTY bit 2; the test door 70): the
// chapter's player units are the deployment slots, Ramza's for the save's Ramza, the others in
// order for the rest of the party (FFT's roster order; a deployment choice is not modelled);
// slots past the party leave the battle. FFT's ids map to the chapter's skill sets (SET_ID,
// SET_POS: a job whose set the chapter has keeps the abilities it learned there); the bag from
// the inventory.
static u8 party_next(u8 n, u8 ramza)          // the save's next unit (from n) that is / is not Ramza
{
    while (n < save.n && (save.unit[n].type == 2) != ramza) n++;
    return n;
}

static void party_load(void)
{
    const u8 *pos = cdesc + CD(SET_POS), *bag = cdesc + CD(BAG_ITEM);
    u8 i, k, r, o;
    if (!rt_load(SAVE_NAME, &save, sizeof save) || memcmp(save.magic, SAVE_MAGIC, 4) || !save.n) return;
    st.party_in = 1;
    r = party_next(0, 1); o = party_next(0, 0);
    for (i = 0; i < NUNIT; i++) {
        Unit *u = &st.unit[i];
        const SaveUnit *s;
        u8 c, set = 0xFF, n;
        if (u->team != TEAM_PLAYER) continue;
        if (i == U_RAMZA) n = r;
        else { n = o; o = party_next((u8)(o + 1), 0); }
        if (n >= save.n) { u->hp = 0; u->dc = DC_GONE; u->x = 0xFF; u->slot = 0xFF; continue; }
        s = &save.unit[n]; u->slot = n;
        u->job = s->job; u->female = s->type == 1; u->name = s->name;
        u->gfx = s->gfx != 0xFF ? s->gfx : u->female ? UG_SQUIRE_F : UG_SQUIRE_M;
        u->level = u->lv0 = s->level; u->exp = s->exp; u->brave = s->brave; u->faith = s->faith;
        u->zodiac = s->zodiac;
        for (k = 0; k < 5; k++) u->raw[k] = (u32)s->raw[k][0] << 16 | (u16)s->raw[k][1] << 8 | s->raw[k][2];
        for (k = 0; k < EQ_N; k++) u->eq[k] = s->eq[k] ? s->eq[k] : 0xFF;
        unit_stats(u);
        c = (u8)(s->job - 0x4A);
        if (c >= CAREER_N) c = 0;
        u->jp = (u16)(s->jp[c][0] << 8 | s->jp[c][1]); u->tjp = (u16)(s->tjp[c][0] << 8 | s->tjp[c][1]);
        for (k = 0; k < NSET; k++) if (cdesc[CD(SET_ID) + k] == job_sets[s->job]) set = k;
        u->learned = 0;
        if (set != 0xFF) {
            u->set = set;
            for (k = 0; k < SET_N; k++) {
                u8 p = pos[set * SET_N + k];
                if (skill_sets[set].abil[k] != 0xFF && s->learned[c][p >> 3] & (0x80 >> (p & 7)))
                    u->learned |= (u8)(1 << k);
            }
        }
        u->defend = (u16)(s->support[0] << 8 | s->support[1]) == CD(DEFEND_ID);
        u->status = 0; u->kos = 0; u->gexp = u->gjp = 0; u->dc = 3; u->ct = 0;
    }
    for (k = 0; k < NBAG; k++) st.bag[k] = save.inv[bag[k]];
}
#if !MOD_CORE
u16 mod_entry(u8 op, u8 a) { (void)a; if (op) party_load(); else save_campaign(); return 0; }
#endif
