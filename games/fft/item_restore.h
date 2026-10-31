#ifndef FFT_ITEM_RESTORE_H
#define FFT_ITEM_RESTORE_H
// Scalar resource-item family. No costs, rewards, image loads or allocation.
static u8 restore_ability(AbId ab)
{
    return ab<512&&(abilities[ab].flags&AF_ITEM)&&(u8)(abilities[ab].formula-0x48)<=3;
}
static u8 restore_undead(const Unit *u){return core_status_current(&u->statuses,0)&16;}
static u8 restore_possible(u8 t,AbId ab)
{
    const Unit *u=&st.unit[t];
    return abilities[ab].formula!=0x4b||restore_undead(u)||
        ((!u->hp||(core_status_current(&u->statuses,0)&32))&&!(u->statuses.innate[0]&32));
}
static u8 restore_applicable(u8 t,AbId ab)
{
    const Unit *u=&st.unit[t];u8 dead=!u->hp||(core_status_current(&u->statuses,0)&32);
    if(!magic_target(t)||unit_petrified(u)||(core_status_current(&u->statuses,3)&1))return 0;
    if(dead)return abilities[ab].formula==0x4b&&!restore_undead(u)&&!(u->statuses.innate[0]&32);
    return abilities[ab].formula!=0x4b||restore_undead(u);
}
static u8 restore_kind(u8 t,AbId ab)
{
    if(abilities[ab].formula==0x49)return AB_MP_HEAL;
    if(restore_undead(&st.unit[t]))return abilities[ab].formula==0x4a?AB_HP_DAMAGE_MP_HEAL:AB_DAMAGE;
    return abilities[ab].kind;
}
static u16 restore_cap(u16 n){return n>999?999:n;}
static u16 restore_quantity(u8 t,AbId ab,u16 roll)
{
    const Ability *b=&abilities[ab];const Unit *u=&st.unit[t];
    if(b->formula==0x4a)return restore_cap(u->max_hp);
    if(b->formula==0x4b)return restore_undead(u)?restore_cap(u->hp):
        (u16)((muls16((s16)(roll&32767),b->y)>>15)+1);
    return restore_cap((u16)(b->y*10));
}
// The formula runs before late target validation. A failed Phoenix can still
// consume its one source draw; ordinary Potion/Ether/Elixir consume none.
static void restore_prepare(u8 t,AbId ab)
{
    u16 roll=0;st.effect_mp=0;st.dmg=0;st.hit=0;st.effect_kind=AB_NULL;
    if(!magic_target(t)||!restore_possible(t,ab))return;
    if(abilities[ab].formula==0x4b&&!restore_undead(&st.unit[t]))roll=rand15();
    st.effect_kind=restore_kind(t,ab);st.dmg=restore_quantity(t,ab,roll);
    if(st.effect_kind==AB_HP_MP_HEAL||st.effect_kind==AB_HP_DAMAGE_MP_HEAL)st.effect_mp=restore_cap(st.unit[t].max_mp);
    st.hit=restore_applicable(t,ab);
    if(!st.hit){st.dmg=st.effect_mp=0;st.effect_kind=AB_NULL;}
}
#endif
