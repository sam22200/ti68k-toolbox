// Cold binding after encounter generation and player deployment. Work keeps
// only stable keys, never duplicate roster records or borrowed save pointers.
#include "campaign/guest_roster.h"
#include "guest_bank.h"
static u8 guest_load(u8 encounter)
{
    u16 size,n;u8 i,used=0,result=1;char route=chapter_route(encounter);
    const u8 *bank=resource_file("ffgu",&size);
    memset(W->guest_key,0,sizeof W->guest_key);
    if(!bank)return 1;
    if(!guest_bank_valid(bank,size))return 0;
    for(i=0;i<GUEST_MAX;i++)if(save_guest_id(&battle_save,i))used|=(u8)1<<i;
    for(n=0;n<RD16(bank,3);n++){
        const u8 *e=bank+8+4*n;u16 identity=RD16(e,1);u8 slot;
        if(e[0]!=(u8)route)continue;
        i=e[1];if(i>=NUNIT||unit_defs[i].team!=TEAM_GUEST||st.unit[i].team!=TEAM_GUEST)return 0;
        slot=save_guest_find(&battle_save,identity);
        if(slot==255){for(slot=0;slot<GUEST_MAX&&((used>>slot)&1);slot++);if(slot==GUEST_MAX)return 0;}
        else result|=2; // restore only when at least one saved guest is present
        used|=(u8)1<<slot;st.unit[i].slot=(u8)(PARTY_MAX+slot);W->guest_key[i]=identity;
    }
    return result;
}
