// First supported reaction families; source IDs remain16-bit.
#ifndef FFT_REACTION_POLICY_H
#define FFT_REACTION_POLICY_H
enum { REACTION_POTION=0x1b9,REACTION_COUNTER=0x1ba };
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
