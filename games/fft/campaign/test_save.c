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
    remove("ffsave.sav");
    puts("Gariland: the campaign save keeps a chest's item, the party's draws and its flag");
    return 0;
}
