// The campaign save (save.inc): a chest's item joins the inventory, the party keeps what it drew.
#include "gariland.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
int main(void)
{
    u8 i,before,id;Save s;
    if(access("ffgi.bin",R_OK))assert(!chdir("data"));
    remove("ffsave.sav");
    sw_init(61);assert(sw_step(0));
    st.unit[1].hp=0;st.unit[1].dc=DC_CHEST;
    pick_up(U_RAMZA,1);id=items[st.pick].id;assert(st.nfound==1);
    st.mode=M_BROWSE;
    save_campaign();assert(rt_load(SAVE_NAME,&s,sizeof s)&&!memcmp(s.magic,SAVE_MAGIC,4));
    before=0;for(i=0;i<NBAG;i++)if(cdesc[CD(BAG_ITEM)+i]==id)before=st.bag[i];
    assert(s.inv[id]==before+1);
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
    puts("Gariland: the campaign save keeps a chest's item, the party's draws and its flag; the party loads back as it was");
    return 0;
}
