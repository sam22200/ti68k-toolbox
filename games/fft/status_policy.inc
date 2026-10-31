#ifndef FFT_STATUS_POLICY_IMPL
#define FFT_STATUS_POLICY_IMPL
// Pure bitmap stage, source order retained. rules:40 raw16-byte rows, then
// formation immunity and rider masks (5 bytes each), read in place from Flash.
#include "status_lifetime.h"
static u16 core_status_filter(const CoreStatus *s,CoreStatusDelta *d,
                             u8 flags,u8 removal_only,const u8 *rules)
{
    u8 current[5],byte,mask,j;const u8 *row;u16 total=0;
    for(byte=0;byte<5;byte++){
        current[byte]=core_status_current(s,byte);
        if(flags&STATUS_RIDER)d->add[byte]&=rules[40*16+5+byte];
    }
    if(!removal_only&&(d->add[2]&2)&&(current[2]&2)){
        d->add[2]&=(u8)~2;d->remove[2]|=2; // Frog toggles instead of stacking.
    }
    for(byte=0;byte<5;byte++){
        if(!removal_only){
            d->add[byte]&=(u8)~(s->immune[byte]|(byte<3?current[byte]:s->innate[byte]));
            if(flags&STATUS_IMMORTAL)d->add[byte]&=(u8)~rules[40*16+byte];
        }
        d->remove[byte]&=core_status_removable(s,byte);
    }
    if(!removal_only){
        row=rules;
        for(byte=0;byte<5;byte++){
            if(!d->add[byte]){row+=8*STATUS_ROW;continue;}
            for(mask=128;mask;mask>>=1,row+=16)
                if(d->add[byte]&mask)for(j=0;j<5;j++)if(current[j]&row[11+j]){
                    d->add[byte]&=(u8)~mask;break;
                }
        }
        row=rules;
        for(byte=0;byte<5;byte++){
            if(!d->add[byte]){row+=8*STATUS_ROW;continue;}
            for(mask=128;mask;mask>>=1,row+=16)
                if(d->add[byte]&mask)for(j=0;j<5;j++)d->add[j]&=(u8)~row[6+j];
        }
    }
    for(byte=0;byte<5;byte++)total+=(u16)d->add[byte]+d->remove[byte];
    return total;
}

// Bitmap/timer commit corresponding to filter/remove/store. Team/control,
// equipment and presentation handlers remain separate.
static u16 core_status_apply(CoreStatus *s,CoreStatusDelta *d,
                            u8 flags,u8 removal_only,const u8 *rules,u8 *ct)
{
    u16 total=core_status_filter(s,d,flags,removal_only,rules);
    u8 byte,mask,j;const u8 *row=rules;
    core_status_remove(s,d->remove,ct);
    for(byte=0;byte<5;byte++){
      if(!d->add[byte]){row+=8*STATUS_ROW;continue;}
      for(mask=128;mask;mask>>=1,row+=16){
        u8 blocked=0;
        if(!(d->add[byte]&mask))continue;
        for(j=0;j<5;j++)if(s->inflicted[j]&row[11+j]){blocked=1;break;}
        if(blocked)continue;
        for(j=0;j<5;j++)d->remove[j]=row[6+j];
        core_status_filter(s,d,flags,removal_only,rules);
        core_status_remove(s,d->remove,ct);
        s->inflicted[byte]|=mask;
        core_status_timer_set(ct,(u8)((u16)(row-rules)>>4),0,rules);
      }
    }
    return total;
}

#endif
