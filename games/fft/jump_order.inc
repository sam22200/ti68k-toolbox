// Cold Jump order: called after COMMAND returns, never evict an executing image.
static u8 jump_order(u8 a,u8 t,AbId ab)
{
    if(!action_available(a,t)||!can_use(a,ab)||!st.unit[t].hp||
       !jump_reaches(a,UNIT_CELL(&st.unit[a]),UNIT_CELL(&st.unit[t])))return 4;
    st.pending[a].state=DEFER_AIRBORNE;st.pending[a].ability=JUMP_FIRST;
    st.pending[a].tile=UNIT_CELL(&st.unit[t]);
    // One cold unsigned divide; Speed0 is clamped to1 for liveness.
    st.pending[a].ticks=50U/(st.unit[a].sp?st.unit[a].sp:1U);
    if(!st.pending[a].ticks)st.pending[a].ticks=1;
    status_transparent_remove(a);unit_status_action(&st.unit[a],4);
    st.moved=st.acted=1;st.mode=M_MENU;st.menu=MN_WAIT;return 2;
}
