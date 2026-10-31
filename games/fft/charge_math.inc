// Source Charge adds power to the selected hand's base XA before statuses/zodiac.
// Pure queries never alter equipment, status, inventory, CT or RNG.
#define charge_amount(a,t,ab,hand,awake,critical,roll) \
    charge_query(a,t,ab,(u8)((hand)|((awake)<<1)|((critical)<<2)),roll)
#if MOD_CORE || MOD_PART(MOD_COMMAND) || MOD_PART(MOD_FORMULA)
static void charge_roll(u8 a,u8 t,AbId ability)
{
    st.hit=roll_hit(a,t);if(!st.hit)return;
    st.crit=(u8)(((u32)rand15()*100>>15)<4);
    st.dmg=charge_amount(a,t,ability,st.hand,0,st.crit,st.crit?rand15():0);
    if(st.crit)knockback(a,t);
}
#endif
