// The campaign save (save.inc): a chest's item joins the inventory, the party keeps what it drew.
#include "gariland.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>

// A KO is temporary until the countdown becomes a crystal/chest. Exercise both the
// first party creation and subsequent victories, including an undeployed roster member.
static void check_ko_party(void)
{
    static const u8 death[] = { 3, 2, 1, 0, DC_CRYSTAL, DC_CHEST, DC_GONE };
    u8 k, i, recruit = 0;
    for (k = 0; k < sizeof death; k++) {
        Save first, back;
        Unit before;
        u8 slot = 0, lost = DC_OBJ(death[k]);
        remove("ffsave.sav");
        sw_init(61);
        for (i = 0; i < NUNIT; i++) if (st.unit[i].team == TEAM_PLAYER && i != U_RAMZA) { recruit = i; break; }
        assert(i < NUNIT);
        before = st.unit[recruit];
        for (i = 0; i < recruit; i++) if (st.unit[i].team == TEAM_PLAYER) slot++;
        st.unit[recruit].hp = 0; st.unit[recruit].dc = death[k];
        save_campaign(); assert(rt_load(SAVE_NAME, &back, sizeof back));
        assert(back.n == 5 - lost);
        if (!lost) {
            for (i = 0; i < 5; i++)
                assert(((u32)back.unit[slot].raw[i][0] << 16 | (u16)back.unit[slot].raw[i][1] << 8 |
                        back.unit[slot].raw[i][2]) == before.raw[i]);
            party_load();
            assert(st.unit[recruit].hp == before.max_hp && st.unit[recruit].mp == before.max_mp);
            assert(st.unit[recruit].dc == 3 && !st.unit[recruit].status);
        }

        // A loaded party uses slot identities; compaction must retain the other units
        // and the member who was not deployed, with all their learned abilities/JP.
        remove("ffsave.sav");
        sw_init(61); save_campaign(); assert(rt_load(SAVE_NAME, &first, sizeof first));
        first.unit[first.n] = first.unit[first.n - 1]; first.unit[first.n++].name = 254;
        assert(save_profile_set(&first,8,1));assert(save_profile_set(&first,9,2));
        assert(rt_save(SAVE_NAME, &first, sizeof first));
        party_load();
        slot = st.unit[recruit].slot;
        st.unit[recruit].hp = 0; st.unit[recruit].dc = death[k];
        save_campaign(); assert(rt_load(SAVE_NAME, &back, sizeof back));
        assert(back.n == first.n - lost);
        assert(!memcmp(back.profiles,first.profiles,sizeof back.profiles));
        for (i = 0; i < back.n; i++)
            assert(!memcmp(&back.unit[i], &first.unit[i + (lost && i >= slot)], sizeof back.unit[i]));
        if (!lost) {
            party_load();
            assert(st.unit[recruit].hp == before.max_hp && st.unit[recruit].dc == 3);
        }
    }
}

static void check_hp_roundtrip(void)
{
    u8 original;
    for (original = 0; original < 2; original++) {
        Unit *r;
        u16 hp = original ? 52 : 51;
        remove("ffsave.sav");
        sw_init(16); r = &st.unit[U_RAMZA];
        assert(r->raw[0] == 508530UL && r->max_hp == 51); // the native victory fixture's seed
        if (original) r->raw[0] = 0x07E86DUL;            // original Gariland oracle.h, same equipment
        unit_stats(r); assert(r->max_hp == hp);
        save_campaign();
        memset(r->raw, 0, sizeof r->raw); r->max_hp = r->hp = 0;
        party_load(); assert(r->max_hp == hp && r->hp == hp);
    }
}

static void check_gil_roundtrip(void)
{
    Save s;u8 enemy;
    remove("ffsave.sav");sw_init(61);
    assert(qlong(battle_save.money)==2000);save_campaign();
    assert(rt_load(SAVE_NAME,&s,sizeof s)&&qlong(s.money)==2000);
    for(enemy=0;enemy<NUNIT;enemy++)if(st.unit[enemy].team==TEAM_ENEMY)break;
    assert(enemy<NUNIT);
    // Until victory, both gains and losses exist only in the battle allocation.
    steal_gil_commit(U_RAMZA,enemy,600);
    assert(qlong(battle_save.money)==2600);
    assert(rt_load(SAVE_NAME,&s,sizeof s)&&qlong(s.money)==2000);
    save_campaign();assert(rt_load(SAVE_NAME,&s,sizeof s)&&qlong(s.money)==2600);
    save_campaign();assert(rt_load(SAVE_NAME,&s,sizeof s)&&qlong(s.money)==2600);
    sw_init(61);assert(qlong(battle_save.money)==2600);
    steal_gil_commit(enemy,U_RAMZA,600);save_campaign();
    assert(rt_load(SAVE_NAME,&s,sizeof s)&&qlong(s.money)==2000);
    steal_gil_commit(enemy,U_RAMZA,9801);save_campaign();
    assert(rt_load(SAVE_NAME,&s,sizeof s)&&qlong(s.money)==0);
    qput(s.money,99999998UL);assert(rt_save(SAVE_NAME,&s,sizeof s));sw_init(61);
    steal_gil_commit(U_RAMZA,enemy,600);save_campaign();
    assert(rt_load(SAVE_NAME,&s,sizeof s)&&qlong(s.money)==99999999UL);
    // First campaign save after a scripted battle uses the same starting funds.
    remove("ffsave.sav");sw_init(61);steal_gil_commit(U_RAMZA,enemy,600);save_campaign();
    assert(rt_load(SAVE_NAME,&s,sizeof s)&&qlong(s.money)==2600);
    remove("ffsave.sav");
}

static void check_permanent_stats(void)
{
    static const s8 changes[]={-70,-8,-7,-4,-3,0,3,4,7,8,30};
    static const u8 expected[]={53,68,69,69,70,70,70,71,71,72,77};
    u8 i,j;Save a,b;Unit *u;
    for(i=0;i<sizeof changes;i++){
        remove("ffsave.sav");sw_init(61);save_campaign();party_load();
        u=&st.unit[U_RAMZA];u->brave=70;u->faith=30;
        W->permanent_base[U_RAMZA][0]=70;W->permanent_base[U_RAMZA][1]=30;
        u->brave=(u8)(70+changes[i]);u->faith=(u8)(30-changes[i]/2);
        u->raw[0]-=123; // future raw-stat changes must survive an existing-party save
        save_campaign();assert(rt_load(SAVE_NAME,&a,sizeof a));
        assert(a.unit[u->slot].brave==expected[i]);
        assert(a.unit[u->slot].faith==30+(-changes[i]/2)/4);
        assert(a.unit[u->slot].raw[0][0]==(u8)(u->raw[0]>>16));
        assert(a.unit[u->slot].raw[0][1]==(u8)(u->raw[0]>>8));
        assert(a.unit[u->slot].raw[0][2]==(u8)u->raw[0]);
        save_campaign();assert(rt_load(SAVE_NAME,&b,sizeof b));
        assert(!memcmp(&a.unit,&b.unit,sizeof a.unit));
        party_load();assert(u->brave==expected[i]&&u->faith==a.unit[u->slot].faith);
    }
    // First creation and a subsequently undeployed member use their own bases.
    remove("ffsave.sav");sw_init(61);u=&st.unit[U_RAMZA];
    j=W->permanent_base[U_RAMZA][0];u->brave=(u8)(j+4);
    save_campaign();assert(rt_load(SAVE_NAME,&a,sizeof a)&&a.unit[0].brave==j+1);
    remove("ffsave.sav");
}

static void check_away_compaction(void)
{
    Save a,b;u8 actor,slot,k;
    remove("ffsave.sav");sw_init(61);save_campaign();assert(save_load(&a));
    slot=a.n;a.unit[slot]=a.unit[slot-1];a.n++;save_generic_set(&a,slot,1);
    a.missions.dispatches.n=1;
    {PropositionJob *j=&a.missions.dispatches.job[0];
     j->flags=PROP_SENT|PROP_RETURNED;j->id=30;j->days=j->elapsed=9;j->location=6;j->n=1;
     j->unit[0]=slot;j->unit[1]=j->unit[2]=255;
     prop_set_status(&a.missions.dispatches,30,PROP_PROGRESS|PROP_ATTEMPTED);}
    // A frozen report may be left pending while another battle is played.
    {PropositionReport *r=&a.missions.report;
     r->ready=1;r->id=30;r->location=6;r->n=1;r->unit[0]=r->speaker=slot;
     r->unit[1]=r->unit[2]=255;r->result=PROP_FAILURE;prop_put_word(r->jp[0],123);}
    a.poach[17]=7;assert(save_world_valid(&a));assert(rt_save(SAVE_NAME,&a,sizeof a));party_load();
    for(actor=0;actor<NUNIT;actor++)assert(st.unit[actor].slot!=slot);
    for(actor=0;actor<NUNIT;actor++)if(unit_party_member(&st.unit[actor])&&actor!=U_RAMZA&&st.unit[actor].slot<slot)break;
    assert(actor<NUNIT);st.unit[actor].hp=0;st.unit[actor].dc=DC_GONE;save_campaign();assert(save_load(&b));
    assert(b.n==a.n-1&&b.missions.dispatches.job[0].unit[0]==slot-1);
    assert(b.missions.report.unit[0]==slot-1&&b.missions.report.speaker==slot-1);
    assert(prop_word(b.missions.report.jp[0])==123&&b.poach[17]==7&&save_generic(&b,slot-1));
    assert(!memcmp(&b.unit[slot-1],&a.unit[slot],sizeof(SaveUnit)));
    for(k=b.n;k<PARTY_MAX;k++)assert(!save_generic(&b,k));
    remove("ffsave.sav");
}

int main(void)
{
    u8 i,before,id;Save s;
    if(access("ffgi.bin",R_OK))assert(!chdir("data"));
    check_ko_party();
    check_hp_roundtrip();
    check_gil_roundtrip();
    check_permanent_stats();
    check_away_compaction();
    remove("ffsave.sav");
    // A crystal teaches EVERY ability it lists, not only the one picked, and only
    // for a skill set the taker has held: a set it never unlocked is the job FFT
    // refuses to teach from. It restores HP and MP either way (loot.inc).
    {
        Unit *a,*b;LearnBits had;u8 set0;
        sw_init(61);assert(sw_step(0));
        a=&st.unit[U_RAMZA];b=&st.unit[1];
        set0=a->set;
        b->hp=0;b->dc=DC_CRYSTAL;b->set=set0;b->learned=(LearnBits)~a->learned;
        had=a->learned;a->hp=1;a->mp=0;
        pick_up(U_RAMZA,1);
        assert(st.pick==PICK_TAUGHT);
        assert(a->learned==(LearnBits)(had|(LearnBits)~had));   // all of them at once
        assert(a->hp==a->max_hp&&a->mp==a->max_mp);
        b->hp=0;b->dc=DC_CRYSTAL;b->set=(u8)(set0+1);           // a set never held
        b->learned=(LearnBits)~a->learned;had=a->learned;a->hp=1;
        pick_up(U_RAMZA,1);
        assert(st.pick==0xFF&&a->learned==had&&a->hp==a->max_hp);
    }
    remove("ffsave.sav");
    sw_init(61);assert(sw_step(0));
    st.unit[1].hp=0;st.unit[1].dc=DC_CHEST;
    pick_up(U_RAMZA,1);id=items[st.pick].id;assert(st.nfound==1);
    st.stolen[st.nstolen++]=0xD1;               // a stolen accessory (CORE_STEAL_EQUIP): the inventory
    st.mode=M_BROWSE;
    before=0;for(i=0;i<NBAG;i++)if(cdesc[CD(BAG_ITEM)+i]==id)before=st.bag[BAG_SLOT(i)];
    save_campaign();assert(rt_load(SAVE_NAME,&s,sizeof s)&&!memcmp(s.magic,SAVE_MAGIC,4));
    assert(s.inv[items[0xD1].id]==(id==items[0xD1].id?2:1));
    assert(s.inv[id]==before+1);
    assert(!st.nfound&&!st.nstolen);
    {Save again;save_campaign();assert(rt_load(SAVE_NAME,&again,sizeof again));assert(!memcmp(&s,&again,sizeof s));}
    assert(s.flags[0]==2&&s.n==5);              // Gariland's flag; Ramza and four recruits
    assert(s.unit[0].job==0x01&&s.unit[0].type==2);
    for(i=1;i<s.n;i++){                        // every recruit: Equip Change, a support equipped
        assert(s.unit[i].learned[1][2]&(0x80>>(CD(EQUIP_CHANGE_POS)&7)));
        assert(s.unit[i].support[0]==1);
    }
    // the party from the save: Gariland's units rebuilt from what it saved are its own
    remove("ffsave.sav");
    sw_init(61);assert(sw_step(0));
    {
        Unit before[NUNIT];
        memcpy(before,st.unit,sizeof before);
        save_campaign();
        for(i=0;i<NUNIT;i++)if(st.unit[i].team==TEAM_PLAYER){Unit *u=&st.unit[i];
            u->job=u->level=u->brave=u->set=u->learned=u->defend=0;memset(u->raw,0,sizeof u->raw);memset(u->eq,0,sizeof u->eq);}
        party_load();assert(st.party_in);
        for(i=0;i<NUNIT;i++){
            const Unit *a=&before[i],*b=&st.unit[i];
            if(a->team!=TEAM_PLAYER)continue;
            assert(a->job==b->job&&a->gfx==b->gfx&&a->level==b->level&&a->exp==b->exp&&a->brave==b->brave&&a->faith==b->faith);
            assert(a->zodiac==b->zodiac&&a->name==b->name&&a->female==b->female&&!memcmp(a->raw,b->raw,sizeof a->raw));
            assert(!memcmp(a->eq,b->eq,sizeof a->eq)&&a->max_hp==b->max_hp&&a->max_mp==b->max_mp&&a->pa==b->pa&&a->ma==b->ma);
            assert(a->sp==b->sp&&a->move==b->move&&a->jump==b->jump&&a->jp==b->jp&&a->tjp==b->tjp);
            assert(a->set==b->set&&a->learned==b->learned&&a->defend==b->defend);
        }
        // a victory with that party updates its records only: the other jobs' JP stay
        {Save s1,s2;assert(rt_load(SAVE_NAME,&s1,sizeof s1));
         st.unit[U_RAMZA].level++;save_campaign();assert(rt_load(SAVE_NAME,&s2,sizeof s2));
         assert(s2.n==s1.n&&s2.unit[0].level==s1.unit[0].level+1&&!memcmp(s2.unit[1].jp,s1.unit[1].jp,sizeof s1.unit[1].jp));}
    }
    remove("ffsave.sav");
    puts("Gariland: KO counters 3..0 survive both save paths; crystals teach their abilities;\n          crystals/chests/gone leave; undeployed records and stats survive");
    return 0;
}
