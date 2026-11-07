// A single frozen packet may mix ordinary resources with stat/status outcomes.
static u8 performance_status_commit(void)
{
    u8 k=st.effect_kind&EFFECT_KIND_MASK;
    return k==AB_STATUS||k==AB_SPEED||k==AB_BREAK||k==AB_QUICK;
}
