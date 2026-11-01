// The campaign save (campaign/save.h) written after a victory: the chapter's story flag
// (descriptor WON), and when its player's units are the campaign's party (PARTY), those still
// with it (not a crystal or a chest), the bag and the chests' items. FFT's ids throughout (a
// unit's job, equipment, skill set and action bits already are; BAG_ITEM and RSM_POS map the
// remaining chapter records): the save is every chapter's. A unit keeps
// what it was drawn with (battle_units): its job's JP and abilities, a generic one's Squire JP
// and abilities when it is another job, the reactions and supports it learned (Equip Change:
// always), the support it fights with.
#include "campaign/save.h"
// rt_save retains its source until native teardown. The save tail therefore
// belongs to the core's longer-lived battle allocation, never Work/scene/cache
// memory released on victory or storage inside an evictable code overlay.
#define save (*(Save *)((u8 *)&st+sizeof(State)))

#if MOD_PART(MOD_SAVE)
#include "chapter_validate.h"
static void be16(u8 *p, u16 v) { p[0] = (u8)(v >> 8); p[1] = (u8)v; }
static void learn(u8 *bits, u8 p) { bits[p >> 3] |= (u8)(0x80 >> (p & 7)); }

static void prepare_rewards(void)
{
    u8 i;u16 levels=0,modifiers=0;
    if(st.reward_ready||!flow_bank)return;
    st.reward_n=0;st.reward_base=st.reward_bonus=0;
    // Result fields are captured before presentation hides enemies. KO bodies,
    // crystals and chests remain present; a removed/picked-up unit does not.
    for(i=0;i<NUNIT;i++) {
        const Unit *u=&st.unit[i];
        if(u->team==TEAM_ENEMY)levels+=u->lv0;
        if(u->x>=MAP_W||u->dc==DC_GONE)continue;
        modifiers+=u->money_modifier;
        if(u->war_trophy&&st.reward_n<NUNIT)st.reward_items[st.reward_n++]=u->war_trophy;
    }
    st.reward_base=(u32)levels*100;st.reward_bonus=(u32)modifiers*100;
    st.reward_ready=1;st.reward_page=0;
}

static void save_rewards(void)
{
    u32 money=qlong(save.money)+st.reward_base+st.reward_bonus;
    u8 i,j,k;
    for(i=0;i<st.reward_n;i++) {
        u8 item=st.reward_items[i];u16 count=save.inv[item];
        for(j=0;j<save.n;j++)for(k=0;k<EQ_N;k++)if(save.unit[j].eq[k]==item)count++;
        if(count>=99)money+=qword(qpart(flow_bank,Q_PRICES)+2*item);
        else save.inv[item]++;
    }
    if(money>99999999UL)money=99999999UL;
    qput(save.money,money);save.flow=4;save.deploy_n=0;
    memset(save.deploy_battle,0,sizeof save.deploy_battle);
}

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
    const u8 *rsm = cdesc + CD(RSM_POS);
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
        be16(s->reaction,st.reaction[i]);
        for (k = 0; k < 5; k++) {
            s->raw[k][0] = (u8)(u->raw[k] >> 16); s->raw[k][1] = (u8)(u->raw[k] >> 8); s->raw[k][2] = (u8)u->raw[k];
        }
        for (k = 0; k < EQ_N; k++) s->eq[k] = u->eq[k] == 0xFF ? 0 : u->eq[k];
        be16(s->jp[c], u->jp); be16(s->tjp[c], u->tjp);
        for (k = 0; k < SET_N; k++) if (u->learned & (1U << k)) learn(s->learned[c], k);
        if(st.support[i])be16(s->support,st.support[i]);
        if (i == U_RAMZA) continue;
        be16(s->support,st.support[i]?st.support[i]:(u->defend ? CD(DEFEND_ID) : CD(EQUIP_CHANGE_ID)));
        if (c) {                           // another job: the Squire's drawn too
            be16(s->jp[0], u->sq_jp); be16(s->tjp[0], u->sq_tjp);
            for (k = 0; k < SET_N; k++)
                if (u->sq_learned & (1U << k)) learn(s->learned[0], k);
        }
        for (k = 0; k < NRSM_PRE; k++) if (u->rsm & (1 << k)) learn(s->learned[0], rsm[k]);
        if (u->rsm & 0x80) learn(s->learned[0], rsm[NRSM_PRE]);
        learn(s->learned[1], (u8)CD(EQUIP_CHANGE_POS));
    }
bag:
    for (k = 0; k < BAG_MAX; k++) save.inv[0xF0 + k] = st.bag[k];
    for (k = 0; k < st.nfound; k++) {
        u8 *n = &save.inv[items[st.found[k]].id];
        if (*n < 99) ++*n;
    }
}

static void save_campaign(void)
{
    u8 w = (u8)CD(WON);
    u8 already;
    if (!save_load(&save)) {
        memset(&save, 0, sizeof save);
        memcpy(save.magic, SAVE_MAGIC, 4);
        qput(save.money,2000);
    }
    already=w?(u8)(save.flags[(w-1)>>3]>>((w-1)&7)&1):0;
    if (w--) save.flags[w >> 3] |= (u8)(1 << (w & 7));
    if (CD(LOCATION)) save.loc = (u8)CD(LOCATION);      // the battle's place on the world map
    if (CD(PARTY) & 1 || st.party_in) save_party();
    if(flow_bank&&CD(LOCATION)==7&&st.reward_ready&&!already)save_rewards();
    rt_save(SAVE_NAME, &save, sizeof save);
}

#endif
#if MOD_PART(MOD_PARTY)
// The player's units from the campaign save (descriptor PARTY bit 2; the test door 70): the
// chapter's player units are the deployment slots, Ramza's for the save's Ramza, the others in
// order for the rest of the party (FFT's roster order; a deployment choice is not modelled);
// slots past the party leave the battle. Skill sets and all 16 action bits retain their
// source positions, including unavailable actions; the bag comes from the inventory.
static u8 party_next(u8 n, u8 ramza)          // the save's next unit (from n) that is / is not Ramza
{
    while (n < save.n && (save.unit[n].type == 2) != ramza) n++;
    return n;
}

static void party_load(void)
{
    u8 i, k, r, o;
    u8 deployed=0,used=0;
    if (!save_load(&save) || !save.n) return;
    if(flow_bank&&qword(save.deploy_battle)==200&&CD(LOCATION)==7&&save.deploy_n&&save.deploy_n<=5) {
        u16 mask=0;u8 valid=1,has_ramza=0;
        for(i=0;i<save.deploy_n;i++) {
            const u8 *p=save.deploy[i];u8 j,legal=0;
            if(p[0]>=save.n||(mask&(1U<<p[0]))){valid=0;break;}
            mask|=(u16)(1U<<p[0]);
            for(j=0;j<25;j++){const u8 *c=qpart(flow_bank,Q_DEPLOY)+4+3*j;if(c[0]&&c[1]==p[1]&&c[2]==p[2])legal=1;}
            for(j=0;j<i;j++)if(save.deploy[j][1]==p[1]&&save.deploy[j][2]==p[2])legal=0;
            if(!legal||p[3]>=4){valid=0;break;}
            if(save.unit[p[0]].type==2)has_ramza=1;
        }
        deployed=valid&&has_ramza;
    }
    st.party_in = 1;
    r = party_next(0, 1); o = party_next(0, 0);
    for (i = 0; i < NUNIT; i++) {
        Unit *u = &st.unit[i];
        const SaveUnit *s;
        u8 c, n;
        const u8 *place=RT_NULL;
        if (u->team != TEAM_PLAYER) continue;
        if(deployed) {
            u8 j;n=255;
            for(j=0;j<save.deploy_n;j++)if(!(used&(1U<<j))&&((save.unit[save.deploy[j][0]].type==2)==(i==U_RAMZA))) {
                place=save.deploy[j];n=place[0];used|=(u8)(1U<<j);break;
            }
        } else if (i == U_RAMZA) n = r;
        else { n = o; o = party_next((u8)(o + 1), 0); }
        if (n >= save.n) { u->hp = 0; u->dc = DC_GONE; u->x = 0xFF; u->slot = 0xFF; memset(st.affinity[i],0,5);continue; }
        s = &save.unit[n]; u->slot = n;
        if(place){unit_place(u,(u8)(place[2]*MAP_W+place[1]));u->face=place[3];}
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
        u->set = job_sets[s->job];
        u->learned = 0;
        for (k = 0; k < SET_N; k++)
            if (s->learned[c][k >> 3] & (0x80 >> (k & 7))) u->learned |= (LearnBits)(1U << k);
        u->defend = (u16)(s->support[0] << 8 | s->support[1]) == CD(DEFEND_ID);
        st.reaction[i]=(u16)(s->reaction[0]<<8|s->reaction[1]);
        st.support[i]=(u16)(s->support[0]<<8|s->support[1]);
        unit_status_seed(u,0); u->kos = 0; u->gexp = u->gjp = 0; u->dc = 3; u->ct = 0;
        u->war_trophy=u->money_modifier=0;
    }
    for (k = 0; k < BAG_MAX; k++) st.bag[k] = save.inv[0xF0 + k];
}
#endif
#if !MOD_CORE && MOD_PART(MOD_PARTY)
#include "loot.inc"
#endif
#if !MOD_CORE && MOD_PART(MOD_SAVE)
#define ugfx (M.chapter_ugfx)
#include "chapter_files.inc"
#undef ugfx
#endif
#if !MOD_CORE
u16 mod_entry(u8 op, u8 a)
{
    (void)a;
#if MOD_PART(MOD_SAVE)
    if(op==6)return chapter_files((char)a);
    if(op==5)return cj_valid(common,M.desc_size);
    if(op==4)return chapter_surface_offset(cdesc,M.desc_size,M.interface_size);
    if (op == 0) save_campaign();
    else prepare_rewards();
#else
    if (op == 1) party_load();
    else if (op == 2) battle_units();
    else if (op == 5) setup_glyphs();
    else if (op == 6) pick_up(st.formula.actor,a);
    else if (op == 7) banner_setup(a);
    else if (op == 8) grid_tables();
    else if (op == 9) unit_stats(M.stats_unit);
    else if (op == 10) chapter_grid();
    else fixed_units();
#endif
    return 0;
}
#endif
