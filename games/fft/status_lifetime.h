// Source timer storage and staging; HP/presentation are owned by their callers.
#ifndef FFT_STATUS_LIFETIME_H
#define FFT_STATUS_LIFETIME_H
enum { STATUS_TIMERS=16, STATUS_TICK_TIMERS=15,
       STATUS_UPKEEP_HEAL=0x40, STATUS_UPKEEP_DAMAGE=0x80 };
static void core_status_timer_set(u8 *ct,u8 id,u8 removing,const u8 *rules)
{
    if(!ct||id<SID_POISON||id>=STATUS_COUNT)return;
    ct+=id-SID_POISON;
    if(removing)*ct=0;
    else if(id!=SID_SENTENCE||!*ct)*ct=rules[((u16)id<<4)+3];
}
static void core_status_remove(CoreStatus *s,const u8 *remove,u8 *ct)
{
    u8 byte,mask,active;u8 *timer;
    for(byte=0;byte<5;byte++){
        s->inflicted[byte]&=(u8)~remove[byte];
        if(!ct||byte<3||!remove[byte])continue;
        timer=ct+((byte-3)<<3);active=remove[byte];
        for(mask=128;active;mask>>=1,timer++)if(active&mask){
            *timer=0;active^=mask;
        }
    }
}
// Stages removals only. Source byte underflow0->255 is intentional; the
// separate turn-count Death Sentence timer (slot15) does not tick here.
static void core_status_tick(const CoreStatus *s,u8 *ct,u8 *remove)
{
    u8 byte,mask,active;u8 *timer;
    remove[0]=remove[1]=remove[2]=remove[3]=remove[4]=0;
    for(byte=3;byte<5;byte++,ct+=8){
        active=core_status_removable(s,byte);if(byte==4)active&=0xfe;
        timer=ct;
        for(mask=128;active;mask>>=1,timer++)if(active&mask){
            (*timer)--;if(!*timer)remove[byte]|=mask;active^=mask;
        }
    }
}
static u8 core_status_upkeep_kind(const CoreStatus *s,u8 present)
{
    u8 status;
    if(!present||(core_status_current(s,0)&0x64)||(core_status_current(s,1)&0x81))return 0;
    status=core_status_current(s,3);
    if(status&0x80)return STATUS_UPKEEP_DAMAGE; // Poison precedes Regen.
    return status&0x40?STATUS_UPKEEP_HEAL:0;
}
#endif
