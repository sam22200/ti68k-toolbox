// Equipment breaks 0x25 (CORE_EQUIP_BREAKS.md): the slot Head/Armor/Shield/Weapon Break hits,
// as the source's hardcoding picks it: the helmet, the armour, the left hand's shield (a shield
// is never in the right hand natively), or the weapon of the higher required level (the right on
// ties). 255: nothing to break, the ability becomes the ordinary Attack. The equipment steals
// (0x6e..0x72, CORE_STEAL_EQUIP.md) share it, plus the accessory; for them 255 is a failure.
// Required levels follow Charge's labels in FCJ's battle section (one byte per item).
#ifndef FFT_EQUIP_BREAK_INC
#define FFT_EQUIP_BREAK_INC
#define ITEM_LEVEL(i) (charge_ticks[28+(i)])
static u8 equip_break_slot(const Unit *u,AbId ab)
{
    u8 r=u->eq[EQ_WEAPON],l=u->eq[EQ_SHIELD],shield=l!=255&&!items[l].wp;
    if((u8)(ab-0xa0)<4)ab=(AbId)(ab==0xa0?0x8b:ab==0xa1?0x8a:ab==0xa2?0x8d:0x72);   // Mighty Sword: Armor, Head, Weapon, Accessory
    if(ab==0x72)return u->eq[EQ_ACC]!=255?EQ_ACC:255;            // Steal Accessory
    if(ab<0x8a)ab+=0x1c;                                          // Steal Helmet..Weapon (0x26)
    if(ab<0x8c){ab-=0x8a;return u->eq[ab]!=255?(u8)ab:255;}   // EQ_HEAD, EQ_BODY
    if(ab==0x8c)return shield?EQ_SHIELD:255;
    if(shield)l=255;
    if(r==255)return l==255?255:EQ_SHIELD;
    return l!=255&&ITEM_LEVEL(l)>ITEM_LEVEL(r)?EQ_SHIELD:EQ_WEAPON;
}
// (PA' + zodiac(WP + Y))%: punch_strike adds the attacker's WP passed as its roll; a steal's
// (SP' + zodiac(X))% adds nothing
static u16 equip_break_percent(u8 a,u8 t,AbId ab)
{
    u8 w=hand_weapon(&st.unit[a],0);
    return charge_query(a,t,ab,0,w!=255&&abilities[ab].formula==0x25?items[w].wp:0);
}
#endif
