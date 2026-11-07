// Source Charge adds power to the selected hand's base XA before statuses/zodiac.
// Pure queries never alter equipment, status, inventory, CT or RNG.
#define charge_amount(a,t,ab,hand,awake,critical,roll) \
    charge_query(a,t,ab,(u8)((hand)|((awake)<<1)|((critical)<<2)),roll)
#if MOD_CORE || MOD_PART(MOD_COMMAND) || MOD_PART(MOD_COMBAT) || MOD_PART(MOD_FORMULA) || MOD_PART(MOD_RESPONSE)
static void charge_roll(u8 a,u8 t,AbId ability)
{
    st.abil=ability;attack_roll(a,t);
}
#endif
