#ifndef FFT_STATUS_SPELL_H
#define FFT_STATUS_SPELL_H
#include "cleanse_rules.h"
// Source all-or-nothing filter specialized to a validated single-byte request.
// This is a pure resident query: no image loading and no mutation of the unit.
static u16 magic_status_value(u8 t,AbId ab,u8 choice)
{
    const CoreStatus *s=&st.unit[t].statuses;
    const u8 *r=cj_section(common,CJ_STATUS)+6*((abilities[ab].formula==0x24||draw_out_formula(abilities[ab].formula))?cj_range(common,ab)[11]:abilities[ab].y),*rules=cj_status_rules(common),*p;
    u8 b,m,bit,k,j,current[5];
    if((ab==109||ab==116)&&(cj_allegiance_immune(common,st.unit[t].job,st.unit[t].eq)&(ab==109?2:1)))return 0;
    if(abilities[ab].kind==AB_CLEANSE)return core_cleanse_pack(s,r+1);
    if(!choice&&r[0]!=128&&!(r[0]==64&&performance_formula(abilities[ab].formula)))return 0;
    for(j=0;j<5;j++)current[j]=core_status_current(s,j);
    for(b=choice?(choice-1)>>3:0;b<5;b++){
        m=choice?(u8)(128>>((choice-1)&7)):r[b+1];if(!m)continue;
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
        if(m)return ((u16)b<<8)|m;
        if(choice||r[0]!=64)return 0;
    }
    return 0;
}
#endif
