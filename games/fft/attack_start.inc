// Shared ordinary-strike construction; no executing image loads another.
u8 attack_sequence(u8 a,u8 t,u8 count)
{
    if(!count||count>2||!action_available(a,t))return 0;
    st.pending[a].state=0;unit_status_action(&st.unit[a],0);
    if(hamedo_begin(a,t,NO_ABIL))return 1;
#ifdef FFT_CHAPTER
    mime_record(a,UNIT_CELL(&st.unit[t]),NO_ABIL,0,0);
#endif
    act_start(a,t);
    st.action.left=count-1;st.action.cell=UNIT_CELL(&st.unit[t]);
    st.abil=NO_ABIL;attack_roll(a,t);return 1;
}
