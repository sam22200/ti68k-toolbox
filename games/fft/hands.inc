// Compact equipment keeps one item per physical hand: a weapon or a shield.
// No swapping live equipment to evaluate the other hand (stats/evade stay stable).
#ifdef FFT_MODULE
#define hand_weapon M.hand_weapon      // the resident copy (ModCtx), not one per image
#else
static u8 hand_weapon(const Unit *u,u8 hand)
{
    u8 right=u->eq[EQ_WEAPON],left=u->eq[EQ_SHIELD];
    if(UNIT_FROG(u))return 255;        // source: both hands empty, Two Swords still strikes twice
    if(left!=255&&!items[left].wp)left=255;
    if(right==255)return hand?255:left;
    return hand?left:right;
}
#endif
static u8 weapon_reach(const Unit *u)
{
    u8 weapon=hand_weapon(u,0);return weapon!=255?items[weapon].reach:1;
}
static u8 hand_count(u8 actor)
{
    const Unit *u=&st.unit[actor];u8 a,b;
    if(!cj_two_swords(common,u->job,st.support[actor]))return 1;
    a=hand_weapon(u,0);b=hand_weapon(u,1);
    if(a==255&&b==255)return 2;
    return a!=255&&b!=255&&(items[a].pad&items[b].pad&HAND_DUAL)?2:1;
}
static u8 current_weapon(u8 actor)
{
    return hand_weapon(&st.unit[actor],st.mode==M_ACT&&st.atk==actor?st.hand:0);
}
