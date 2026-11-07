#ifndef FFT_WEAPON_SKILL_AMOUNT_H
#define FFT_WEAPON_SKILL_AMOUNT_H
// Non-Charge sword skills and their cross splash share the same scalar.
static u16 weapon_skill_amount(u8 a,u8 t,AbId ab)
{
 const Unit *u=&st.unit[a];u8 w=hand_weapon(u,0);
 s16 xa=zodiac_xa(u,&st.unit[t],unit_status_xa(&st.unit[t],u->pa));
 return (u16)(xa*((w==255?1:items[w].wp)+abilities[ab].y));
}
#endif
