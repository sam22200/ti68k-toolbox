// Shared formula37 amount: caller owns its random multiplier and knockback.
static u16 dash_amount(u8 a,u8 t,u16 k)
{
    s16 xa=unit_status_xa(&st.unit[t],st.unit[a].pa);
    u16 d=(u16)(k*zodiac_xa(&st.unit[a],&st.unit[t],xa));
    return d>999?999:d;
}
