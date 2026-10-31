// Ordinary elimination only. Source importance3/script objectives remain separate.
static u8 battle_outcome(void)
{
    u8 i,foes=0,blue=0;const Unit *u;
    for(i=0,u=st.unit;i<NUNIT;i++,u++)if(u->hp&&unit_on_field(u)&&
        !(core_status_current(&u->statuses,0)&0x60)&&
        !(core_status_current(&u->statuses,1)&0xc5)){
        if(u->team==TEAM_ENEMY)foes++;else blue++;
    }
    // Preserve the current campaign's mandatory-Ramza permanent-loss contract.
    if(DC_OBJ(st.unit[U_RAMZA].dc)||!blue)return END_DEFEAT;
    return foes?END_NONE:END_VICTORY;
}
