// Pure magical HP scalars. Weather/Float use source order; human/unmounted scope.
#include "weather_rules.h"
#ifndef FFT_PRODUCTION_CORE   // injection API; chapters set their source weather at load
u8 battle_weather(u8 strength,u8 flags)
{
    if(strength>WEATHER_MAX_STRENGTH||(flags&~3U))return 0;
    st.weather=core_weather(strength,flags);
    if(W){W->fc_key=0;if(st.ai==AI_THINK)W->ai_k=0;}
    return 1;
}
#endif
u8 skill_kind(u8 a,u8 t,AbId ab)
{
    const Ability *b;(void)a;
    if(ab==NO_ABIL)return AB_DAMAGE;
    if(ab==ABIL_DEFEND)return AB_DEFEND;
    if(ab>=NABIL)return AB_NULL;
    b=&abilities[ab];
    if(restore_ability(ab))return restore_kind(t,ab);
    if(magic_status_formula(b->formula))return b->kind;
    if(b->formula==12||b->formula==76||b->formula==13||b->formula==14||b->formula==16||b->formula==0x23||b->formula==0x35)return magic_heal(a,t,ab,2);
    if((u8)(b->formula-0x2D)<=1){          // Holy/Mighty Sword: the primary weapon's element
        u8 w=hand_weapon(&st.unit[a],0),e=w==255?0:cj_weapon(common,w)[6];
        if(st.affinity[t][1]&e)return AB_NULL;
        return st.affinity[t][0]&e?AB_HEAL:AB_DAMAGE;
    }
    if(magic_damage_formula(b->formula)||b->formula==9||b->formula==0x24||b->formula==0x31||b->formula==99){
        if((st.affinity[t][1]&b->element)||((b->element&8)&&(core_status_current(&st.unit[t].statuses,2)&0x40)))return AB_NULL;
        if(st.affinity[t][0]&b->element)return AB_HEAL;
    }
    return b->kind;
}
static s16 magic_third(s16 n)
{
    // Exact floor(n/3) in this bounded positive XA domain, using verified MULS16.
    return (s16)(muls16(n,21846)>>16);
}
static u8 magic_faith(const Unit *u)
{
    u8 bits=core_status_current(&u->statuses,4);
    if(bits&0x40)return 0;                // Innocent wins when both bits exist.
    return bits&0x80?100:u->faith;
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
// The target's Half then Weak affinities (source modify_damage_for_element), all formulas.
static s16 magic_half_weak(u8 t,u8 element,s16 raw)
{
    if(st.affinity[t][2]&element)raw/=2;
    if(st.affinity[t][3]&element)raw=(s16)((u16)raw<<1);
    return raw;
}
static s16 magic_element_value(u8 t,u8 element,s16 raw)
{
    if((u8)(st.weather-3)<=1){
        if(element&128)raw=magic_quarter(raw,3);
        if(element&64)raw=magic_quarter(raw,5);
    }else if((u8)(st.weather-6)<=1&&(element&32))raw=magic_quarter(raw,5);
    return magic_half_weak(t,element,raw);
}
u16 magic_amount(u8 a,u8 t,u16 spec)
{
    const Unit *ua=&st.unit[a],*ut=&st.unit[t];u8 element=(u8)(spec>>8);
    s16 raw;u16 faith;
    if((st.affinity[t][1]&element)||((element&8)&&(core_status_current(&ut->statuses,2)&0x40)))return 0;
    raw=(s16)muls16(magic_xa(a,t,element,1,8),(u8)spec&127);
    raw=magic_element_value(t,element,raw);
    faith=spec&128?10000:(u16)(magic_faith(ua)*magic_faith(ut));   // bit7: formula 0x4E, no Faith
    return (u16)divs32_16(muls16(raw,(s16)faith),10000);
}

// Source formula0a/0b/0c/0d, validated status/healing records. Pure query shared
// by executing images: 0 amount or packed status byte/mask,1 chance,2 category.
u16 magic_heal(u8 a,u8 t,AbId ab,u8 query)
{
    const Ability *b=&abilities[ab];
    const Unit *ua,*ut;
    s16 xa,x;u16 n;
    if(jump_ability(ab))return attack_weapon_damage(a,t,st.unit[a].eq[EQ_WEAPON],0,0,0,255);
    if(b->formula==99){
        if(st.affinity[t][1]&b->element)return 0;
        if(query)return 100;
        xa=(s16)muls16(magic_xa(a,t,0,1,99),b->y);
        if(st.affinity[t][2]&b->element)xa/=2;
        if(st.affinity[t][3]&b->element)xa=(s16)((u16)xa<<1);
        return xa>999?999:(u16)xa;
    }
    if(query==3||(query&128))return magic_status_value(t,ab,(query&128)?(query&127)+1:0);
    ua=&st.unit[a];ut=&st.unit[t];
    if(performance_formula(b->formula)){
        if(query==1)return cj_range(common,ab)[9];
        if(b->kind==AB_STATUS)return magic_status_value(t,ab,0);
        if(b->kind==AB_QUICK)return ab==92?255:256;
        if(b->kind==AB_SPEED||b->kind==AB_BREAK)return (u16)(s16)(s8)b->y;
        if(ab<93)return ua->ma+b->y;
        n=(u16)divs32_16(muls16(ua->pa,ua->brave),100);
        return ua->pa+(n?n:1); // unarmed XA/YA selected by the source ordinary-ability setup
    }
    if(draw_out_formula(b->formula)){
        if(query==4)return ua->job==SOURCE_JOB_MIME||
            (unit_home_team(ua)==TEAM_ENEMY?ua->level>=b->lv:battle_save.inv[ab-0x26]!=0)?ab:0;
        if(query==1)return 100;
        if(b->formula==0x23&&query!=2)return restore_cap((u16)(ua->ma*b->y));
    }
    if(b->formula==0x24||(u8)(b->formula-0x20)<=1){
        if(query==4){
            u16 area=(u16)MAP_W*MAP_H;u8 surface;
            if(!W||!W->surface||!(W->surface_flags&SURFACE_TERRAIN)||ua->x>=MAP_W||ua->z>=MAP_H||UNIT_CELL(ua)>=MAP_N)return 0;
            surface=W->surface[(area<<3)+((W->grid_row[ua->z]+ua->x)<<1)+node_layer(UNIT_CELL(ua))];
            return cj_section(common,CJ_BATTLE)[CJ_EXECUTABLES*12U+8+20+256+surface];
        }
        if(query)return 100;
        xa=(s16)muls16(magic_xa(a,t,b->element,1,0x24),(b->formula==0x24?(ua->pa+b->y)>>1:b->y));
        xa=magic_element_value(t,b->element,xa);
        return xa>999?999:(u16)xa;
    }
    if(!query&&b->kind==AB_STEAL_GIL){
        u32 funds=qlong(battle_save.money);n=(u16)(ua->sp*ua->level);
        return ut->team!=TEAM_ENEMY&&funds<n?(u16)funds:n;
    }
    // Quick; Golem (a marker, its pool is the target's max HP at commit); Foxbird's Brave loss
    if(!query&&(b->formula==18||b->formula==20||b->formula==97))return b->formula==18?255:b->formula==20?1:b->y;
    // Talk: Persuade's CT 0 (high byte keeps it non-zero); Brave/Faith carry a signed loss.
    if(!query&&speech_formula(b->formula)&&b->kind!=AB_STATUS)return b->kind==AB_QUICK?0x100:(u16)(s16)(s8)b->y;
    if(!query&&b->formula==14)return restore_cap(st.unit[t].max_hp);
    if(!query&&b->formula==0x1A)return b->y;   // Ruin: the stat's loss X
    if(!query&&(magic_status_formula(b->formula)||b->kind==AB_CLEANSE))return magic_status_value(t,ab,0);
    if(query==2)return core_status_current(&ut->statuses,0)&0x10?(b->formula==16||b->formula==14?AB_HEAL:AB_DAMAGE):b->kind;
    if(!query&&(b->formula==13||b->formula==0x35||b->formula==0x1B||magic_percent_formula(b->formula))){
        n=(u16)divs32_16(muls16((s16)(b->formula==15||b->formula==0x1B?ut->max_mp:ut->max_hp),b->y)+99,100);
        if(b->formula==9)n=(u16)magic_element_value(t,b->element,(s16)n);
    }else{
        // Golem (formula 20) targets its caster: same sign (neutral zodiac), no
        // target Faith, so the shared path is (MA[+AttackUP]+X)*caster Faith/100.
        // Punch Art has no Faith term.
        u16 faith=b->formula==76||b->formula==81||punch_formula(b->formula)||(u8)(b->formula-0x29)<=1||b->formula==0x50||b->formula==0x3D?10000:(u16)(magic_faith(ua)*(b->formula==20?100:magic_faith(ut)));
        xa=magic_xa(a,t,b->formula==9?b->element:0,
            b->formula==10||b->formula==97||b->formula==0x50||b->formula==0x3D||(u8)(b->formula-0x1A)<=1||(b->formula==14&&!(core_status_current(&ut->statuses,0)&16))||magic_percent_formula(b->formula),b->formula);
        if(b->formula==13||b->formula==14||b->formula==0x35||b->formula==0x1B||magic_status_formula(b->formula)||magic_percent_formula(b->formula)){
            const u8 *r=cj_range(common,ab);x=r[b->formula==0x1A?10:9];   // Ruin 0x1A: MA + Y
            if(ua->zodiac!=12&&ut->zodiac!=12)x=zodiac_xa(ua,ut,x);
            n=(u16)divs32_16(muls16(xa+x,faith),10000);
            return n>100?100:n;
        }
        if(query)return 100;
        n=(u16)divs32_16(muls16((s16)muls16(xa,b->y),faith),10000);
    }
    return n>999?999:n;
}
