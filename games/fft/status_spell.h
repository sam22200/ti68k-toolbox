#ifndef FFT_STATUS_SPELL_H
#define FFT_STATUS_SPELL_H
#include "cleanse_rules.h"
// Source all-or-nothing filter specialized to a validated single-byte request.
// This is a pure resident query: no image loading and no mutation of the unit.
static u16 magic_status_value(u8 t,AbId ab)
{
    const CoreStatus *s=&st.unit[t].statuses;
    const u8 *r=cj_section(common,CJ_STATUS)+6*abilities[ab].y,*rules=cj_status_rules(common),*p;
    u8 b,m,bit,k,j,current[5];
    if(abilities[ab].kind==AB_CLEANSE)return core_cleanse_pack(s,r+1);
    if(r[0]!=128)return 0;
    for(b=0;b<5&&!r[b+1];b++){}if(b==5)return 0;m=r[b+1];
    for(j=0;j<5;j++)current[j]=core_status_current(s,j);
    if(st.unit[t].status_flags&STATUS_RIDER)m&=rules[645+b];
    // Frog on a Frog toggles it off (core_status_filter); an innate Frog stays.
    if(b==2&&(m&current[2]&2))return (u16)(core_status_removable(s,2)&2)*0x101;
    m&=(u8)~(s->immune[b]|(b<3?current[b]:s->innate[b]));
    if(st.unit[t].status_flags&STATUS_IMMORTAL)m&=(u8)~rules[640+b];
    for(bit=128,k=0;bit;bit>>=1,k++)if(m&bit){
        p=rules+((u16)((b<<3)+k)<<4);
        for(j=0;j<5;j++)if(current[j]&p[11+j]){m&=(u8)~bit;break;}
    }
    for(bit=128,k=0;bit;bit>>=1,k++)if(m&bit)m&=(u8)~rules[((u16)((b<<3)+k)<<4)+6+b];
    return m?((u16)b<<8)|m:0;
}
#endif
