// Shared pure presentation eligibility. A response never inherits the root FX.
static u8 area_impact_root(void)
{
    return st.mode==M_ACT&&W->cast.phase!=CAST_BATCH_EMPTY&&
        st.atk==W->cast.actor&&st.abil==W->cast.ability&&
        (st.action.phase!=CTX_IDLE&&st.action.phase!=CTX_DRAIN);
}
static u8 area_impact_harm(u8 i)
{
    const CastVictim *v=&W->cast.victim[i];
    u8 kind=v->effect&EFFECT_KIND_MASK,harm=effect_damage(kind);
    if(!v->hit||kind==AB_NULL)return 0;
    if((s16)v->amount<0)harm=!harm;
    return harm&&v->amount!=0;
}
