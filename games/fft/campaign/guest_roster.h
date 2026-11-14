#ifndef FFT_GUEST_ROSTER_H
#define FFT_GUEST_ROSTER_H
#include "save_types.h"
// Cold save operations only. IDs are chapter-independent, authored identities;
// neither encounter positions nor job/sprite/name indexes are persistent keys.
static u16 save_guest_id(const Save *s,u8 i)
{return (u16)s->guest_id[i][0]<<8|s->guest_id[i][1];}
static u8 save_guests_valid(const Save *s)
{
    u8 i,j;
    if(s->n>PARTY_MAX||s->guest_reserved||(s->guest_raw&0xf0))return 0;
    for(i=0;i<GUEST_MAX;i++){
        u16 id=save_guest_id(s,i);
        if(!id){if(s->guest_raw&(1U<<i))return 0;continue;}
        for(j=0;j<i;j++)if(save_guest_id(s,j)==id)return 0;
    }
    return 1;
}
static u8 save_guest_find(const Save *s,u16 id)
{
    u8 i;if(id)for(i=0;i<GUEST_MAX;i++)if(save_guest_id(s,i)==id)return i;
    return 255;
}
static void save_guest_clear(Save *s,u8 i)
{
    if(i>=GUEST_MAX)return;
    memset(s->guest_id[i],0,2);memset(&s->guest[i],0,sizeof s->guest[i]);
    save_raw_set(s,(u8)(PARTY_MAX+i),0);
}
// A full normal roster must not discard the guest or any of its equipment.
// Story code commits the returned in-memory Save with its flags in one write.
static u8 save_guest_recruit(Save *s,u16 id)
{
    u8 i=save_guest_find(s,id),n=s->n;
    if(i==255||n>=PARTY_MAX)return 255;
    memcpy(&s->unit[n],&s->guest[i],sizeof s->unit[0]);
    save_raw_set(s,n,save_raw_current(s,(u8)(PARTY_MAX+i)));s->n++;
    save_guest_clear(s,i);return n;
}
static u8 save_guest_remove(Save *s,u16 id)
{u8 i=save_guest_find(s,id);if(i==255)return 0;save_guest_clear(s,i);return 1;}
// Source ownership includes equipment on all20 formation slots, also reserves.
static u16 save_owned_count(const Save *s,u8 item)
{
    u16 count=s->inv[item];u8 i,k;
    for(i=0;i<s->n;i++)for(k=0;k<5;k++)if(s->unit[i].eq[k]==item)count++;
    for(i=0;i<GUEST_MAX;i++)if(save_guest_id(s,i))
        for(k=0;k<5;k++)if(s->guest[i].eq[k]==item)count++;
    return count;
}
#endif
