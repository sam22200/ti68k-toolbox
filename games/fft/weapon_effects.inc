// Pure weapon packets. No owner load, state mutation or random draw in forecasts.
#ifndef FFT_WEAPON_EFFECTS_INC
#define FFT_WEAPON_EFFECTS_INC
static u8 weapon_kind_hand(u8 a,u8 t,u8 hand)
{
    const u8 *w=cj_weapon(common,hand_weapon(&st.unit[a],hand));u8 f=w[2],e=w[6];
    if(f==6||f==7)return core_status_current(&st.unit[t].statuses,0)&16?(f==6?AB_DAMAGE:AB_HEAL):(f==6?AB_HEAL:AB_DAMAGE);
    if(f==3)e=0;
    if((st.affinity[t][1]&e)||((e&8)&&(core_status_current(&st.unit[t].statuses,2)&64)))return AB_NULL;
    return st.affinity[t][0]&e?AB_HEAL:AB_DAMAGE;
}
static u8 weapon_kind(u8 a,u8 t,AbId ab)
{
    u8 kind=weapon_kind_hand(a,t,0);(void)ab;
    return kind==AB_NULL&&hand_count(a)==2?weapon_kind_hand(a,t,1):kind;
}
static u8 weapon_faith(const Unit *u)
{
    u8 bits=core_status_current(&u->statuses,4);return bits&64?0:bits&128?100:u->faith;
}
static s16 weapon_third(s16 n){return (s16)(muls16(n,21846)>>16);}
static s16 weapon_element(u8 t,u8 e,s16 n,u8 weather)
{
    if(weather){
        if((u8)(st.weather-3)<=1){if(e&128)n=(s16)(muls16(n,3)/4);if(e&64)n=(s16)(muls16(n,5)/4);}
        else if((u8)(st.weather-6)<=1&&(e&32))n=(s16)(muls16(n,5)/4);
    }
    if(st.affinity[t][2]&e)n/=2;
    if(st.affinity[t][3]&e)n=(s16)((u16)n<<1);
    return n;
}
static u16 weapon_value(u8 a,u8 t,u8 weapon,u8 charge,u8 awake,u8 critical,u16 amplitude,u16 base,u16 tier)
{
    const u8 *w=cj_weapon(common,weapon);u8 f=w[2],e=w[6];s16 n;
    if(f==4){
        const Unit *u=&st.unit[a],*v=&st.unit[t];u8 spell=e&128?16:e&64?20:24;u16 pct=(u16)(muls16((s16)tier,100)>>15);
        s16 xa=w[4]+charge;
        spell+=pct<10?2:pct<40?1:0;
        if(st.affinity[a][4]&e)xa=(s16)((u16)(xa*5)>>2);
        if(cj_support(common,u->job,st.support[a],SUPPORT_MAGIC_ATTACK_UP))xa=weapon_third(xa<<2);
        if(cj_support(common,v->job,st.support[t],SUPPORT_MAGIC_DEFENSE_UP))xa=weapon_third(xa<<1);
        if(core_status_current(&v->statuses,3)&16)xa=weapon_third(xa<<1);
        if(core_status_current(&v->statuses,2)&6)xa+=xa>>1;
        if(u->zodiac!=12&&v->zodiac!=12)xa=zodiac_xa(u,v,xa);
        n=weapon_element(t,e,(s16)muls16(xa,abilities[spell].y),1);
        n=(s16)divs32_16(muls16(n,(s16)((u16)weapon_faith(u)*weapon_faith(v))),10000);
    }else{
        u8 mode=awake|(f==6?2:0);
        if((f==1||f==2)&&(st.affinity[a][4]&e))base|=32768;
        n=(s16)weapon_amount(a,t,weapon,critical,amplitude,mode,charge,base);
        if(f==1||f==2)n=weapon_element(t,e,n,0);
    }
    return charge?(u16)n:n>999?999:(u16)n;
}
static u16 weapon_forecast(u8 a,u8 t,AbId ab,u8 hand,u8 awake)
{
    u8 charge=ab<512&&(abilities[ab].flags&AF_CHARGE)?abilities[ab].y:0;
    return weapon_value(a,t,hand_weapon(&st.unit[a],hand),charge,awake,0,0,16384,16384);
}
static u16 weapon_total(u8 a,u8 t,AbId ab)
{
    u8 kind=weapon_kind(a,t,ab);u16 n;
    if(kind==AB_NULL||unit_petrified(&st.unit[t]))return 0;
    if(kind==AB_DAMAGE&&st.golem[st.unit[t].team&1])return 0;
    n=weapon_kind_hand(a,t,0)==AB_NULL?0:weapon_forecast(a,t,ab,0,0);
    if(hand_count(a)==2&&((s16)n<0||kind!=AB_DAMAGE||n<st.unit[t].hp)){
        u8 other=weapon_kind_hand(a,t,1);u16 m=weapon_forecast(a,t,ab,1,weapon_kind_hand(a,t,0)==AB_DAMAGE);
        if(other==kind)n+=m;
        else if(other!=AB_NULL)n-=m;
    }
    return n;
}
#if (MOD_CORE && !defined(FFT_MODULES)) || (defined(FFT_MODULE) && MOD_PART(MOD_WEAPON))
static u8 melee_target(u8 a,u8 t);
static u8 melee_cache(void);
#include "weapon_plan.inc"
#endif
#ifdef FFT_MODULE
static
#endif
u16 attack_prediction(u8 a,u8 t){return weapon_total(a,t,NO_ABIL);}
#endif
