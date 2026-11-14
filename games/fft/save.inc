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
#define save battle_save

#if MOD_PART(MOD_PARTY)
static void funds_load(void)
{
    if(!save_load(&save)){memset(&save,0,sizeof save);qput(save.money,2000);}
}
#endif

#if MOD_PART(MOD_SAVE)
#if !MOD_CORE
#include "reward_value.h"
#include "reaction_policy_runtime.inc"
#include "magic_commit.inc"
#include "batch_commit.inc"
#endif
#include "chapter_validate.h"

static void be16(u8 *p, u16 v) { p[0] = (u8)(v >> 8); p[1] = (u8)v; }
static void learn(u8 *bits, u8 p) { bits[p >> 3] |= (u8)(0x80 >> (p & 7)); }

#if MOD_CORE
#include "reward_prepare.inc"
#endif

static void save_rewards(void)
{
    u32 money=qlong(save.money)+st.reward_base+st.reward_bonus;
    u8 i;
    for(i=0;i<st.reward_n;i++) {
        u8 item=st.reward_items[i];u16 count=save_owned_count(&save,item);
        if(count>=99)money+=qword(qpart(flow_bank,Q_PRICES)+2*item);
        else save.inv[item]++;
    }
    if(money>99999999UL)money=99999999UL;
    qput(save.money,money);save.flow=4;save.deploy_n=0;
    memset(save.deploy_battle,0,sizeof save.deploy_battle);
}

// Source post-battle rule: keep one quarter of the net change, truncating
// signed changes toward zero. The original also resets the live value to it.
static u8 permanent_stat(u8 base,u8 current)
{
    return current>=base?(u8)(base+((u16)(current-base)>>2)):
                         (u8)(base-((u16)(base-current)>>2));
}
static void save_permanent_stats(SaveUnit *s,Unit *u,u8 who,u8 slot)
{
    u8 k;
    s->brave=permanent_stat(W->permanent_base[who][0],u->brave);
    s->faith=permanent_stat(W->permanent_base[who][1],u->faith);
    // A second settlement cannot reapply the same Brave/Faith change.
    W->permanent_base[who][0]=u->brave=s->brave;
    W->permanent_base[who][1]=u->faith=s->faith;
    save_raw_set(&save,slot,(u->raw[0]&RAW_CURRENT)!=0);
    for(k=0;k<5;k++){
        s->raw[k][0]=(u8)(u->raw[k]>>16);s->raw[k][1]=(u8)(u->raw[k]>>8);s->raw[k][2]=(u8)u->raw[k];
    }
}

static void save_unit_progress(SaveUnit *s,Unit *u,u8 i,u8 slot)
{
    u8 c,k;
    c = (u8)(s->job - 0x4A);
    if (c >= CAREER_N) c = 0;
    s->level = u->level; s->exp = u->exp;
    save_permanent_stats(s,u,i,slot);
    for(k=0;k<EQ_N;k++)s->eq[k]=u->eq[k]==255?0:u->eq[k];
    be16(s->jp[c], u->jp); be16(s->tjp[c], u->tjp);
}

// the party the battle took from the save (party_load): the records keep every job's JP and
// abilities; the battle changes a unit's level, EXP and its job's JP; the units lost (a crystal
// or a chest) leave the party
static void party_update(void)
{
    Unit *u = st.unit;
    u8 i, k, n = 0, lost[PARTY_MAX];
    memset(lost, 0, sizeof lost);
    for (i = 0; i < NUNIT; i++, u++) {
        SaveUnit *s;
        if (u->slot >= save.n) continue;
        if(!unit_party_member(u)){
            if(u->allegiance&ALLEGIANCE_INVITED)lost[u->slot]=1;
            continue;
        }
        if (DC_OBJ(u->dc)) { lost[u->slot] = 1; continue; }
        s = &save.unit[u->slot];
        save_unit_progress(s,u,i,u->slot);
    }
    for (k = 0; k < save.n; k++) {
        if(lost[k]){lost[k]=255;continue;}
        lost[k]=n;
        save_raw_set(&save,n,save_raw_current(&save,k));
        if (n != k) memcpy(&save.unit[n], &save.unit[k], sizeof save.unit[0]);
        n++;
    }
    for(i=0;i<NUNIT;i++)if(st.unit[i].slot<save.n)st.unit[i].slot=lost[st.unit[i].slot];
    for (k = n; k < save.n; k++){memset(&save.unit[k],0,sizeof save.unit[0]);save_raw_set(&save,k,0);}
    save.n = n;
}

static void save_new_unit(SaveUnit *s,Unit *u,u8 i,u8 slot)
{
    const u8 *rsm=cdesc+CD(RSM_POS);u8 c,k;
    memset(s,0,sizeof *s);
    s->job = u->job;
    c = (u8)(s->job - 0x4A);           // its career row (another job: the Squire's)
    if (c >= CAREER_N) c = 0;
    s->gfx = u->gfx; s->type = unit_monster(u)?3:(u8)(i == U_RAMZA && u->team == TEAM_PLAYER ? 2 : u->female); s->name = u->name;
    s->level = u->level; s->exp = u->exp; save_permanent_stats(s,u,i,slot);
    s->zodiac = u->zodiac;
    be16(s->reaction,st.reaction[i]);
    if(st.movement[i])be16(s->movement,(u16)(485+st.movement[i]));
    for (k = 0; k < EQ_N; k++) s->eq[k] = u->eq[k] == 0xFF ? 0 : u->eq[k];
    be16(s->jp[c], u->jp); be16(s->tjp[c], u->tjp);
    for (k = 0; k < SET_N; k++) if (u->learned & (1U << k)) learn(s->learned[c], k);
    if(st.support[i])be16(s->support,st.support[i]);
    if ((i == U_RAMZA&&u->team==TEAM_PLAYER)||unit_monster(u)) return;
    be16(s->support,st.support[i]?st.support[i]:(u->defend ? CD(DEFEND_ID) : W->guest_key[i]?0:CD(EQUIP_CHANGE_ID)));
    if (c) {                           // another job: the Squire's drawn too
        be16(s->jp[0], u->sq_jp); be16(s->tjp[0], u->sq_tjp);
        for (k = 0; k < SET_N; k++)
            if (u->sq_learned & (1U << k)) learn(s->learned[0], k);
    }
    for (k = 0; k < NRSM_PRE; k++) if (u->rsm & (1 << k)) learn(s->learned[0], rsm[k]);
    if (u->rsm & 0x80) learn(s->learned[0], rsm[NRSM_PRE]);
    if(!W->guest_key[i])learn(s->learned[1], (u8)CD(EQUIP_CHANGE_POS));
}
static void guests_update(void)
{
    u8 i;
    for(i=0;i<NUNIT;i++)if(W->guest_key[i]){
        Unit *u=&st.unit[i];u8 g=save_guest_find(&save,W->guest_key[i]);
        if(g==255){
            for(g=0;g<GUEST_MAX&&save_guest_id(&save,g);g++);
            if(g==GUEST_MAX)continue; // cold binding rejects this before a battle starts
            be16(save.guest_id[g],W->guest_key[i]);
            save_new_unit(&save.guest[g],u,i,(u8)(PARTY_MAX+g));
        }else save_unit_progress(&save.guest[g],u,i,(u8)(PARTY_MAX+g));
        u->slot=(u8)(PARTY_MAX+g);
    }
}

static void save_party(void)
{
    Unit *u = st.unit;
    u8 i, k;
    if (st.party_in) party_update();
    else {save.n=0;memset(save.unit,0,sizeof save.unit);memset(save.raw_current,0,sizeof save.raw_current);}
    for (i = 0; i < NUNIT; i++, u++) {
        SaveUnit *s;
        if(!unit_party_member(u)||DC_OBJ(u->dc)||save.n==PARTY_MAX)continue;
        if(st.party_in&&(u->slot!=255||!(u->allegiance&ALLEGIANCE_INVITED)))continue;
        if(st.party_in)u->slot=save.n;
        s = &save.unit[save.n++];
        save_new_unit(s,u,i,(u8)(save.n-1));
    }
    for (k = 0; k < BAG_MAX; k++) save.inv[0xF0 + k] = st.bag[k];
    for (k = 0; k < st.nfound; k++) {
        u8 *n = &save.inv[items[st.found[k]].id];
        if (*n < 99) ++*n;
    }
    for (k = 0; k < st.nstolen; k++) {      // steals: at 99 owned (worn included), price / 4 gil
        u8 item = items[st.stolen[k]].id; u16 count = save_owned_count(&save,item);
        if (count < 99) save.inv[item]++;
        else if (flow_bank) {
            u32 m = qlong(save.money) + (qword(qpart(flow_bank, Q_PRICES) + 2 * item) >> 2);
            qput(save.money, m > 99999999UL ? 99999999UL : m);
        }
    }
}

static void save_campaign(void)
{
    u8 w = (u8)CD(WON);
    u8 already,inventory[256+SAVE_DISCOVERY_BYTES];
    u32 funds=qlong(save.money);       // save_load refreshes roster/flags, not the live balance
    memcpy(inventory,save.inv,256);memcpy(inventory+256,save.discovered,SAVE_DISCOVERY_BYTES);
    if (!save_load(&save)) {
        memset(&save, 0, sizeof save);
        memcpy(save.magic, SAVE_MAGIC, 4);
    }
    qput(save.money,funds);memcpy(save.inv,inventory,256);memcpy(save.discovered,inventory+256,SAVE_DISCOVERY_BYTES);
    already=w?(u8)(save.flags[(w-1)>>3]>>((w-1)&7)&1):0;
    if (w--) save.flags[w >> 3] |= (u8)(1 << (w & 7));
    if (CD(LOCATION)) save.loc = (u8)CD(LOCATION);      // the battle's place on the world map
    guests_update();
    if (CD(PARTY) & 1 || st.party_in) save_party();
    if(flow_bank&&st.reward_ready&&!already&&(CD(PARTY)&1||st.party_in))save_rewards();
    if(rt_save(SAVE_NAME, &save, sizeof save)){
        u8 k;
        // Commit the live receipt too: a repeated exit must neither add finds/
        // steals twice nor overwrite newly awarded bag items with pre-result counts.
        for(k=0;k<BAG_MAX;k++)st.bag[k]=save.inv[0xF0+k];
        st.nfound=st.nstolen=0;
    }
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
    while (n < save.n && ((save.unit[n].type == 2) != ramza || save.unit[n].type==3)) n++;
    return n;
}

static void saved_unit_load(u8 i,const SaveUnit *s,u8 slot)
{
    Unit *u=&st.unit[i];u8 c,k;u->slot=slot;
    u->job = s->job; u->female = s->type == 1; u->name = s->name;
    u->gfx = s->gfx != 0xFF ? s->gfx : unit_monster(u)?255:u->female ? UG_SQUIRE_F : UG_SQUIRE_M;
    u->level = u->lv0 = s->level; u->exp = s->exp; u->brave = s->brave; u->faith = s->faith;
    W->permanent_base[i][0]=u->brave;W->permanent_base[i][1]=u->faith;
    u->zodiac = s->zodiac;
    for (k = 0; k < 5; k++) u->raw[k] = (u32)s->raw[k][0] << 16 | (u16)s->raw[k][1] << 8 | s->raw[k][2];
    if(save_raw_current(&save,slot))u->raw[0]|=RAW_CURRENT;
    for (k = 0; k < EQ_N; k++) u->eq[k] = s->eq[k] ? s->eq[k] : 0xFF;
    {u16 id=(u16)(s->movement[0]<<8|s->movement[1]);
     st.movement[i]=(u16)(id-486)<24U?(u8)(id-485):0;}
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
static void guests_restore(void)
{
    u8 i;
    for(i=0;i<NUNIT;i++)if(W->guest_key[i]){
        u8 g=save_guest_find(&save,W->guest_key[i]);
        if(g<GUEST_MAX)saved_unit_load(i,&save.guest[g],(u8)(PARTY_MAX+g));
    }
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
            if(p[0]>=save.n||save.unit[p[0]].type==3||(mask&(1U<<p[0]))){valid=0;break;}
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
        u8 n;
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
        saved_unit_load(i,s,n);
    }
    for (k = 0; k < BAG_MAX; k++) st.bag[k] = save.inv[0xF0 + k];
    monster_refresh();
}
#endif
#if !MOD_CORE && MOD_PART(MOD_PARTY)
#include "loot.inc"
#endif

#if MOD_PART(MOD_PARTY)
// Ordinary starts already load PARTY for unit generation, funds and saved party.
// Keep these cold operations together; no allocator or CT/renderer callback here.
static u8 scenario_prepare(u8 n)
{
    u8 i;
#if !MOD_CORE
    memset(&st,0,sizeof st);
#ifdef FFT_CONFIG
    st.pzoom=st.psolid=1;
#endif
    st.want=st.prot=st.ai_view=255;
#endif
    battle_units();for(i=0;i<NBAG;i++)st.bag[BAG_SLOT(i)]=bag_init[i];
    funds_load();
    if(CD(PARTY)&2||n==70||(flow_bank&&CD(LOCATION)==7))party_load();
    if(flow_bank&&CD(LOCATION)==7){opening_story=qpart(flow_bank,Q_GARILAND);pgfx=rt_file("ffcp",RT_NULL);}
    st.ai_all=n==16;
    if(flow_bank&&CD(LOCATION)==7&&!n){
        st.act=st.sel=U_RAMZA;cursor_place(UNIT_CELL(&st.unit[U_RAMZA]));st.mode=M_BROWSE;return 1;
    }
    return 0;
}
// Cold scripted-start preparation. PARTY already owns funds and fixed_units;
// the resident retains allocation, actor-cache reset and subsequent clock calls.
static void opening_setup(u8 letter)
{
    u8 i;char name[6];
    funds_load();st.want=st.prot=st.ai_view=255;
    name[0]='f';name[1]='f';name[2]=(char)letter;name[3]='s';name[4]='t';name[5]=0;
    opening_story=resource_file(name,RT_NULL);pgfx=rt_file("ffcp",RT_NULL);
    if(opening_story)opening_story+=2;
    fixed_units();for(i=0;i<NBAG;i++)st.bag[BAG_SLOT(i)]=bag_init[i];
    W->dialog_key=0xFFFF;W->bg_ok=W->hold=W->frz=W->status_i=0;
    W->pan_ok[0]=W->pan_ok[1]=0;W->win_k[0]=0;
    W->pan_k[0][0]=W->pan_k[1][0]=W->fc_key=0;scene_rot=vbuf_rot=0xFF;
    st.rot=2;st.act=st.sel=U_RAMZA;cursor_place(UNIT_CELL(&st.unit[0]));st.mode=M_BROWSE;
}
#endif
#if !MOD_CORE
#include "scenario_finish.inc"
u16 mod_entry(u8 op, u8 a)
{
    (void)a;
#if MOD_PART(MOD_SAVE)
    if(op==7)return batch_commit(M.cast_batch_out);
    if(op==8)return batch_commit(&W->cast)&2;
    if(op==4)return chapter_surface_offset(cdesc,M.desc_size,M.interface_size);
    if (op == 0) save_campaign();
#else
    if (op == 15) guests_restore();
    else if (op == 1) party_load();
    else if (op == 2) battle_units();
    else if (op == 6) pick_up(st.formula.actor,a);
    else if (op == 7) banner_setup(a);
    else if (op == 9) unit_stats(M.stats_unit);
    else if (op == 11) funds_load();
    else if (op == 12) opening_setup(a);
    else if (op == 14) scenario_finish(a);
    else if (op == 13) return scenario_prepare(a);
    else fixed_units();
#endif
    return 0;
}
#endif
