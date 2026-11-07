#ifndef FFT_EQUIPMENT_RULES_H
#define FFT_EQUIPMENT_RULES_H
// Native equipment uses255 for empty; saves/management convert their0 at the edge.
// Slots retain the shared head/body/accessory/right/left order.
// Native shield storage remains the left slot; right-shield layouts are gated.
static u8 core_equipment_category(const u8 *b,u8 job,u8 female,u16 support,u8 type)
{
    static const u8 byte[8]={2,2,0,0,1,1,0,1};
    static const u8 mask[8]={9,16,16,4,16,1,2,32};
    u8 cats[4],i;
    if(job>=160||type>=32)return 0;
    memcpy(cats,cj_section(b,CJ_BATTLE)+CJ_EQUIPMENT+(u16)job*8,4);
    for(i=0;i<8;i++)if(cj_support(b,job,support,454+i))cats[byte[i]]|=mask[i];
    if(female){cats[2]|=66;cats[3]|=1;}
    return (cats[type>>3]&(128>>(type&7)))!=0;
}
static u8 core_equipment_allowed(const u8 *b,u8 job,u8 female,u16 support,const u8 *eq,u8 slot,u8 id)
{
    static const u8 slots[5]={32,16,8,128,128|64};
    const u8 *it,*other;u8 opposite;
    if(slot>=5)return 0;
    if(id==255)return 1;
    it=cj_equipment_item(b,id);
    if(!(it[1]&slots[slot])||!core_equipment_category(b,job,female,support,it[0]))return 0;
    if(slot<3)return 1;
    opposite=eq[7-slot];if(opposite==255)return 1;
    other=cj_equipment_item(b,opposite);
    if((it[1]&128)&&(other[1]&128))
        return cj_two_swords(b,job,support)&&(it[2]&other[2]&8)!=0;
    if((it[1]&64)&&(other[1]&64))return 0;
    return !((it[2]|other[2])&1); // forced two-handed weapon forbids a shield
}
static u8 core_two_hands(const u8 *b,u8 job,u16 support,const u8 *eq,u8 weapon)
{
    u8 flags;
    if(weapon==255||(eq[3]!=255&&eq[4]!=255))return 0;
    flags=cj_equipment_item(b,weapon)[2];
    return !(flags&1)&&(flags&4)&&cj_support(b,job,support,476);
}
#endif
