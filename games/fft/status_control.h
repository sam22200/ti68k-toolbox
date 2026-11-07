// Cached source control policy. Context bits0..1 remain separate from derived bits.
#ifndef FFT_STATUS_CONTROL_H
#define FFT_STATUS_CONTROL_H
static u8 core_status_no_evade(const CoreStatus *s)
{
    return (core_status_current(s,0)&9)||(core_status_current(s,1)&0x10)||
        (core_status_current(s,3)&2)||(core_status_current(s,4)&0x14);
}
static u8 core_status_controls(const CoreStatus *s)
{
    u8 b0=core_status_current(s,0),b1=core_status_current(s,1);
    u8 b3=core_status_current(s,3),b4=core_status_current(s,4),flags=0;
    if((b0&0x40)||(b1&0x81)||(b3&2)||(b4&0x10))flags|=STATUS_CT_FROZEN;
    if(b0&4)flags|=STATUS_AIRBORNE;
    if(b4&8)flags|=STATUS_NO_MOVE;
    if(b4&4)flags|=STATUS_NO_ACT;
    if(core_status_no_evade(s))flags|=STATUS_NO_EVADE;
    if((b1&0x14)|(core_status_current(s,2)&0x0c)|(b4&0x20))flags|=STATUS_NO_CONTROL;
    return flags;
}
static u8 core_status_end_ct(const CoreStatus *s,u8 ct,u8 moved,u8 acted)
{
    u16 value=ct;u8 b4=core_status_current(s,4);
    if(!moved&&!(b4&8))value+=20;
    if(!acted&&!(b4&4)&&!(core_status_current(s,0)&1))value+=20;
    return value>60?60:(u8)value;
}
#endif
