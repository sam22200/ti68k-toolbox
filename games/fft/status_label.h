#ifndef FFT_STATUS_LABEL_H
#define FFT_STATUS_LABEL_H
// Existing seven-row number renderer; labels fit its32-pixel cache.
static const char *status_label(u16 value)
{
    u8 b=(u8)(value>>8),m=(u8)value;
    if(b==0)return m==2?"Defend":m?"Undead":"Miss";
    if(b==1)return m&128?"Stone":m&32?"Dark":m&16?"Confuse":"Silence";
    if(b==2)return m&16?"Hidden":m&32?"Reraise":m&8?"Berserk":m&4?"Chicken":"Frog";
    if(b==3){
        if(m==48)return "Guard";
        if(m&128)return "Poison";if(m&64)return "Regen";
        if(m&32)return "Protect";if(m&16)return "Shell";
        if(m&8)return "Haste";if(m&4)return "Slow";return "Stop";
    }
    if(b==4){
        if(m&128)return "Faith";if(m&64)return "Innoc.";
        if(m&16)return "Sleep";if(m&8)return "Bind";if(m&4)return "ActOff";
        return "Reflect";
    }
    return "Status";
}
#endif
