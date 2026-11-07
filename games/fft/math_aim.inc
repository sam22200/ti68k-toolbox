// Global Math reach for the shared manual preview/AI presentation.
static void math_aim(void)
{
        s16 mask=math_targets(st.act,st.sub&15);u8 i;
        memset(st.reach,255,sizeof st.reach);
        for(i=0;i<NUNIT;i++)if(mask&((u16)1<<i))st.reach[UNIT_CELL(&st.unit[i])]=1;
        hl_seq++;W->win_k[0]=0;st.mode=M_CAST_RANGE;return;
}
