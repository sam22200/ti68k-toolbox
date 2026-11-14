#ifndef FFT_SAVE_WORLD_H
#define FFT_SAVE_WORLD_H
#include "guest_roster.h"
// Pilot anchor observed in the original Gariland deployment fixture. Legacy
// saves cannot reconstruct travel they never recorded. No time passes on load.
static void save_world_init(Save *s)
{
    memset(&s->world,0,sizeof s->world);s->world.month=s->world.day=1;
    s->world_rng[0]=0x25;s->world_rng[1]=0x45;s->world_rng[2]=0xf4;s->world_rng[3]=0x91;s->world_pad=0;
}
static void save_init(Save *s)
{
    memset(s,0,sizeof *s);memcpy(s->magic,SAVE_MAGIC,4);save_world_init(s);
    s->money[2]=7;s->money[3]=208;
}
static u8 save_world_valid(const Save *s)
{
    const WorldParty *w=&s->world;u8 i;
    if(!save_guests_valid(s)||w->reserved||s->world_pad||w->era>99||!w->month||w->month>12||
       !w->day||w->day>world_month_days[w->month-1]||
       !(s->world_rng[0]|s->world_rng[1]|s->world_rng[2]|s->world_rng[3]))return 0;
    for(i=0;i<WB_SLOTS;i++){
        u8 egg=w->egg[i],left=egg&15,total=egg>>4;
        u16 born=(u16)w->birthday[i][0]<<8|w->birthday[i][1];
        const SaveUnit *u=i<PARTY_MAX?&s->unit[i]:&s->guest[i-PARTY_MAX];
        u8 present=i<PARTY_MAX?i<s->n:save_guest_id(s,(u8)(i-PARTY_MAX))!=0;
        if(!present){if(egg||born||world_egg_color(w,i))return 0;continue;}
        if(born>365)return 0;
        if(egg&&(total<2||!left||left>total||born||u->type!=3||u->job<94||u->job>=142))return 0;
    }
    return 1;
}
// Only eggs can be dismissed here. Every moved record retains its raw and
// world metadata. Stale deployment indices are invalidated together.
static u8 save_dismiss_egg(Save *s,u8 slot)
{
    u8 i;
    if(slot>=s->n||!save_egg(s,slot))return 0;
    for(i=slot;i+1<s->n;i++){
        memcpy(&s->unit[i],&s->unit[i+1],sizeof s->unit[0]);
        save_raw_set(s,i,save_raw_current(s,(u8)(i+1)));
        world_party_move_slot(&s->world,i,(u8)(i+1));
    }
    i=--s->n;memset(&s->unit[i],0,sizeof s->unit[0]);save_raw_set(s,i,0);world_party_clear_slot(&s->world,i);
    s->deploy_n=0;memset(s->deploy_battle,0,sizeof s->deploy_battle);memset(s->deploy,0,sizeof s->deploy);
    return 1;
}
#endif
