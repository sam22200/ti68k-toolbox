// Formula08 uses a signed halfword, then caps positive quantities only.
static u16 magic_quantity(u8 a,u8 t,AbId ab)
{
    const Ability *b=&abilities[ab];
    if(b->formula==0x31||b->formula==0x2D||b->formula==0x2F)return charge_query(a,t,ab,0,0);   // Spin Fist, Holy/Dark Sword: no critical in forecasts
    if(!magic_damage_formula(b->formula))return magic_heal(a,t,ab,0);
    s16 value=(s16)magic_amount(a,t,((u16)b->element<<8)|b->y|(b->formula==0x4E?128:0));
    return value>999?999:(u16)value;
}
