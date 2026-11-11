// Magical XA and element stages shared by the resident's formula08 (magic_math.inc) and the
// companion image's formula 0x4E (companion.inc): one source, each image its own copy.
#ifndef FFT_MAGIC_STAGES_H
#define FFT_MAGIC_STAGES_H
static s16 magic_third(s16 n)
{
    // Exact floor(n/3) in this bounded positive XA domain, using verified MULS16.
    return (s16)(muls16(n,21846)>>16);
}
// Signed /4 toward zero before narrowing, including quotients beyond s16.
// DIVS.W would overflow at raw32767*5/4; shift the wide verified MULS result.
static s16 magic_quarter(s16 raw,u8 factor)
{
    s32 product=muls16(raw,factor);if(product<0)product+=3;
    return (s16)(product>>2);
}
// Punch Art (formula 0x31..0x35) uses PA, Attack UP x4/3 and Martial Arts x3/2
// (no weapon takes part). Secret Fist0x50 starts at MA, uses the physical
// supports/target modifiers and no Faith. These immediate commands' source
// Berserk/Frog attacker XA is unreachable:
// Frog may only Attack or Frog, Berserk only Attack, and Punch Art has no CT.
static s16 magic_xa(u8 a,u8 t,u8 element,u8 hostile,u8 f)
{
    const Unit *ua=&st.unit[a],*ut=&st.unit[t];u8 punch=punch_formula(f)||(u8)(f-0x31)<=1,physical=punch||f==0x50||f==99;
    s16 xa=f==99?ua->sp:punch?ua->pa:ua->ma;
    if(f!=99&&(st.affinity[a][4]&element))xa=(s16)(((u16)xa*5)>>2);   // Earth Slash carries Earth; the other Punch Art passes no element
    if(f!=99&&(u8)(f-0x29)>1&&st.support[a]==(physical?SUPPORT_ATTACK_UP:SUPPORT_MAGIC_ATTACK_UP))xa=magic_third(xa<<2);   // Talk/Steal Heart: plain MA
    if(f!=99&&physical&&st.support[a]==SUPPORT_MARTIAL_ARTS)xa+=xa>>1;
    if(hostile){
        if(st.support[t]==(physical?SUPPORT_DEFENSE_UP:SUPPORT_MAGIC_DEFENSE_UP))xa=magic_third(xa<<1);
        if(core_status_current(&ut->statuses,3)&(physical?32:16))xa=magic_third(xa<<1);
        if(physical&&((core_status_current(&ut->statuses,0)&8)||(core_status_current(&ut->statuses,4)&16)))xa+=xa>>1;
        if(core_status_current(&ut->statuses,2)&6)xa+=xa>>1;
    }
    return ua->zodiac!=12&&ut->zodiac!=12?zodiac_xa(ua,ut,xa):xa;
}
static s16 magic_element_value(u8 t,u8 element,s16 raw)
{
    if((u8)(st.weather-3)<=1){
        if(element&128)raw=magic_quarter(raw,3);
        if(element&64)raw=magic_quarter(raw,5);
    }else if((u8)(st.weather-6)<=1&&(element&32))raw=magic_quarter(raw,5);
    if(st.affinity[t][2]&element)raw/=2;
    if(st.affinity[t][3]&element)raw=(s16)((u16)raw<<1);
    return raw;
}
#endif
