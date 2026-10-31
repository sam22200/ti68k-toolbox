#ifndef FFT_CLEANSE_RULES_H
#define FFT_CLEANSE_RULES_H
// A compiler-validated cancellation set has at most16 requested statuses.
// Pack only its removable members, in source bit order, into the existing u16
// effect quantity. Commit decodes the frozen subset, never the whole live set.
static u16 core_cleanse_pack(const CoreStatus *s,const u8 *request)
{
    u16 bit=1,value=0;u8 b,m,remove;
    for(b=0;b<5;b++){
        remove=core_status_removable(s,b);
        for(m=128;m;m>>=1)if(request[b]&m){if(remove&m)value|=bit;bit<<=1;}
    }
    return value;
}
static void core_cleanse_unpack(const u8 *request,u16 value,u8 *remove)
{
    u8 b,m;
    for(b=0;b<5;b++){
        remove[b]=0;
        for(m=128;m;m>>=1)if(request[b]&m){if(value&1)remove[b]|=m;value>>=1;}
    }
}
#endif
