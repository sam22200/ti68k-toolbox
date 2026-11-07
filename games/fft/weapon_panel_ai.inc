// Scalar rows remain immutable while a second cache adds same-panel collateral.
static u8 weapon_panel_ai(void)
{
    u8 a=st.act,row=0,t=W->ai_panel_at,left=4;u16 bit=1;
    if(!W->ai_panel_left)return 1;
    while(!(W->ai_panel_left&bit)){bit<<=1;row++;}
    do{
        u8 j; s16 value=W->ai_weapon_bonus[row][t];
        for(j=0;j<NUNIT;j++)if(j!=t&&UNIT_CELL(&st.unit[j])==UNIT_CELL(&st.unit[t])&&magic_target(j)==MAGIC_VALID){
            const Unit *u=&st.unit[j];u8 kind=W->ai_weapon_kind[j];s16 gain;
            gain=worth(u,magic_hp(kind,u->hp,u->max_hp,W->ai_weapon_value[row][j]),u->status)-worth(u,u->hp,u->status);
            if((u->team==TEAM_ENEMY)!=(st.unit[a].team==TEAM_ENEMY))gain=-gain;
            value+=gain+W->ai_weapon_bonus[row][j];
        }
        W->ai_panel_bonus[row][t]=value;
        if(++t==NUNIT){W->ai_panel_left&=(u16)~bit;t=0;break;}
    }while(--left);
    W->ai_panel_at=t;return !W->ai_panel_left;
}
