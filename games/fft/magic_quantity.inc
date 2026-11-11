// Formula08 uses a signed halfword, then caps positive quantities only.
static u16 magic_quantity(u8 a,u8 t,AbId ab)
{
    const Ability *b=&abilities[ab];
    if(b->formula==0x31)return charge_query(a,t,ab,0,0);   // Spin Fist: the Punch Art strike, no critical
    if(companion_formula(b->formula))return special(a,t,ab,COMPANION_AMOUNT);
    if(b->formula!=8)return magic_heal(a,t,ab,0);
    s16 value=(s16)magic_amount(a,t,((u16)b->element<<8)|b->y);
    return value>999?999:(u16)value;
}
