// Cold result capture, before presentation hides the enemies.
static void prepare_rewards(void)
{
    u8 i;u16 levels=0,modifiers=0;
    if(st.reward_ready||!flow_bank)return;
    st.reward_n=0;st.reward_base=st.reward_bonus=0;
    // Result fields are captured before presentation hides enemies. KO bodies,
    // crystals and chests remain present; a removed/picked-up unit does not.
    for(i=0;i<NUNIT;i++) {
        const Unit *u=&st.unit[i];
        if(unit_original_enemy(u))levels+=u->lv0;
        if(u->x>=MAP_W||u->dc==DC_GONE)continue;
        modifiers+=u->money_modifier;
        if(u->war_trophy&&st.reward_n<NUNIT)st.reward_items[st.reward_n++]=u->war_trophy;
    }
    st.reward_base=(u32)levels*100;st.reward_bonus=(u32)modifiers*100;
    st.reward_ready=1;st.reward_page=0;
}

