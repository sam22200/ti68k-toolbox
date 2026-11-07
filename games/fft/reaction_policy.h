// First supported reaction families; source IDs remain16-bit.
#ifndef FFT_REACTION_POLICY_H
#define FFT_REACTION_POLICY_H
enum { REACTION_PA_SAVE=0x1a6,REACTION_MA_SAVE=0x1a7,REACTION_SPEED_SAVE=0x1a8,
       REACTION_HP_RESTORE=0x1af,REACTION_MP_RESTORE=0x1b0,REACTION_CRITICAL_QUICK=0x1b1,
       REACTION_SUNKEN_STATE=0x1a9,REACTION_BRAVE_UP=0x1ad,REACTION_FAITH_UP=0x1ae,
       REACTION_CAUTION=0x1aa,REACTION_DRAGON_SPIRIT=0x1ab,REACTION_REGENERATOR=0x1ac,
       REACTION_MEATBONE=434,REACTION_COUNTER_MAGIC=435,REACTION_TACKLE=0x1b4,REACTION_FLOOD=0x1b5,REACTION_ABSORB_MP=0x1b6,
       REACTION_GILGAME=0x1b7,REACTION_DISTRIBUTE=444,REACTION_MP_SWITCH=445,REACTION_SPLIT=446,
       REACTION_WEAPON_GUARD=447,REACTION_CATCH=450,REACTION_BLADE_GRASP=451,REACTION_ARROW_GUARD=452,REACTION_HAMEDO=453,
       REACTION_POTION=0x1b9,REACTION_COUNTER=0x1ba };
#define reaction_stat(id) ((u16)((id)-REACTION_PA_SAVE)<3U)
#define reaction_self(id) ((u16)((id)-REACTION_SUNKEN_STATE)<6U)
#define reaction_status(id) ((u16)((id)-REACTION_SUNKEN_STATE)<4U)
#define reaction_pre(id) ((u16)((id)-REACTION_SUNKEN_STATE)<3U||(id)==REACTION_BRAVE_UP)
#define reaction_resource(id) (reaction_critical(id)||(id)==REACTION_ABSORB_MP||(id)==REACTION_GILGAME)
#define reaction_mp(id) ((id)==REACTION_FAITH_UP||(id)==REACTION_ABSORB_MP)
#define reaction_typed(id) ((u16)((id)-REACTION_SUNKEN_STATE)<=REACTION_SPLIT-REACTION_SUNKEN_STATE)
// The pending slot is opaque to CoreAction: high bit freezes an eight-bit MP cost.
#define reaction_id(slot) ((slot)&0x8000U?REACTION_ABSORB_MP:(slot)&0x4000U?REACTION_GILGAME:(slot)&0x2000U?REACTION_SPLIT:(slot)&0x1000U?REACTION_DISTRIBUTE:(slot)&0x0800U?REACTION_COUNTER_MAGIC:(slot))
#define reaction_counter(id) ((id)==REACTION_COUNTER||(u16)((id)-REACTION_TACKLE)<2U||reaction_pre(id))
#define reaction_critical(id) ((u16)((id)-REACTION_HP_RESTORE)<4U)
static u8 reaction_chance(u8 brave,u16 roll)
{
    return ((u32)roll*100U>>15)<brave;
}
static u8 reaction_potion(const u8 *bag,u8 enemy)
{
    u8 i;if(enemy)return 0;
    for(i=0;i<3;i++)if(bag[i])return i;
    return 255;
}
typedef struct {
    u16 dmg;
    union {u16 self,effect_mp;};        // same tag and layout as the live scalar payload
    u16 abil;
    u8 atk,tgt,act_t,hit,crit,push,kx,kz,push_cell,gain,lvup,jpg,running,reward,hand,effect_kind;
} ActionPacket;
typedef char ActionPacketSize[(sizeof(ActionPacket)==22)?1:-1];
#endif
