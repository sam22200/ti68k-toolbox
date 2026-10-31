// Pure target-state transitions shared by commit, AI and healing previews.
// Formula/RNG, legality, costs, rewards and actor-side effects are separate.
#ifndef FFT_ACTION_EFFECT_H
#define FFT_ACTION_EFFECT_H
enum { EFFECT_KIND_MASK=63,EFFECT_RETURN=64,EFFECT_OIL=128 };
static u8 effect_damage(u8 kind){return kind==AB_DAMAGE||(u8)(kind-AB_HP_DAMAGE_MP_HEAL)<=1;}
static u16 action_hp(u8 kind,u16 hp,u16 max_hp,u16 amount)
{
    if(effect_damage(kind))return amount>=hp?0:hp-amount;
    if(kind==AB_HEAL||kind==AB_WISH||kind==AB_REVIVE||kind==AB_HP_MP_HEAL)
        return amount>=max_hp-hp?max_hp:hp+amount;
    return hp;
}
static u8 action_status(u8 kind,u8 status,u8 mask)
{
    if(kind==AB_CURE)return status&(u8)~mask;
    if(kind==AB_DEFEND)return status|ST_DEFEND;
    return status;
}
#ifdef FFT_CHAPTER
// Preserve source signed halfword results without changing their event category.
static u16 magic_hp(u8 kind,u16 hp,u16 max_hp,u16 amount)
{
    if(kind==AB_DEATH)return 0;
    if((s16)amount<0){kind=kind==AB_DAMAGE?AB_HEAL:AB_DAMAGE;amount=(u16)(0U-amount);}
    return action_hp(kind,hp,max_hp,amount);
}
#endif
#endif
