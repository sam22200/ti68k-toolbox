// Geometry cached the receiver for each reflected occupant at this origin.
// Conditional19% and spell hit chance are already included in ai_proc_gain.
static s16 __attribute__((noinline)) weapon_reflect_score(u8 target,u8 row,u8 origin)
{
    u8 i,hand,cell=UNIT_CELL(&st.unit[target]);s16 gain=0;
    if(!W->ai_reflect_hands)return 0;
    for(i=0;i<NUNIT;i++)if(UNIT_CELL(&st.unit[i])==cell&&st.unit[i].hp&&(core_status_current(&st.unit[i].statuses,4)&2)){
        u8 receiver=(W->ai_reflect[origin][i>>1]>>((i&1)<<2))&15;
        if(receiver&&magic_hp(W->ai_weapon_kind[i],st.unit[i].hp,st.unit[i].max_hp,W->ai_weapon_value[row][i]))
            for(hand=0;hand<2;hand++)if(W->ai_reflect_hands&(1<<hand))gain+=W->ai_proc_gain[hand][receiver-1];
    }
    return gain;
}
