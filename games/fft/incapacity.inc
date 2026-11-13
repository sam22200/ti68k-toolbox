#include "objective_rules.h"
// Ordinary elimination plus explicitly authored settled HP-script goals.
static u8 battle_outcome(void)
{
    u8 i,foes=0,blue=0;const Unit *u;u16 ko=0,removed=0;
    if(W->goal)for(i=0,u=st.unit;i<NUNIT;i++,u++){
        if(!u->hp)ko|=(u16)1<<i;
        if(DC_OBJ(u->dc)||u->dc==DC_GONE)removed|=(u16)1<<i;
    }
    for(i=0,u=st.unit;i<NUNIT;i++,u++)if(u->hp&&unit_on_field(u)&&
        !(core_status_current(&u->statuses,0)&0x60)&&
        !(core_status_current(&u->statuses,1)&0xc5)){
        if(unit_home_team(u)==TEAM_ENEMY)foes++;else blue++;
    }
    // Preserve the current campaign's mandatory-Ramza permanent-loss contract.
    if(W->goal)return core_objective_result(RD16(W->goal,1),RD16(W->goal,2),ko,removed,
        DC_OBJ(st.unit[U_RAMZA].dc),!blue?END_DEFEAT:foes?END_NONE:END_VICTORY);
    if(DC_OBJ(st.unit[U_RAMZA].dc)||!blue)return END_DEFEAT;
    return foes?END_NONE:END_VICTORY;
}
