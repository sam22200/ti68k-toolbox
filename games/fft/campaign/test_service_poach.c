#include "flow_battle.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    u8 item;u32 after;const u8 *poach;Save before,back;Unit *a,*t;u16 charge;
    sw_init(200);assert(W&&common);a=&st.unit[U_RAMZA];t=&st.unit[10];
    poach=cj_section(common,CJ_BATTLE)+CJ_DISCOVERY;
    for(charge=0;charge<512;charge++)if(abilities[charge].flags&AF_CHARGE)break;
    assert(charge<512);
    for(st.abil=NO_ABIL;;st.abil=charge) {
        t->job=94;t->hp=0;t->dc=0;t->status_flags=0;t->x=st.unit[9].x;t->z=st.unit[9].z;
        a->job=74;st.support[U_RAMZA]=471;st.atk=U_RAMZA;st.tgt=10;st.hit=1;st.effect_kind=AB_DAMAGE;
        before=battle_save;rng32=1;item=poach[(rand15()>>7)<31];after=rng32;rng32=1;
        nature_hit_commit();assert(t->dc==DC_GONE&&rng32==after);
        assert(battle_save.poach[item]==before.poach[item]+1&&!memcmp(battle_save.inv,before.inv,256));
        before=battle_save;nature_hit_commit();assert(rng32==after&&!memcmp(&before,&battle_save,sizeof before));
        if(st.abil==charge)break;
    }
    before=battle_save;save_campaign();
    assert(save_load(&back)&&!memcmp(back.poach,before.poach,sizeof before.poach));
    save_campaign();assert(save_load(&back)&&!memcmp(back.poach,before.poach,sizeof before.poach));
    puts("Poach/Charge: one source roll, separate persistent stock, no bag receipt or duplicate");return 0;
}
