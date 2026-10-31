// Source Reflect is a primary pre-formula miss, not magical evasion.
#ifndef FFT_REFLECT_RULES_H
#define FFT_REFLECT_RULES_H
enum { REACTION_REFLECT=0x1b8,MAGIC_ABSENT=0,MAGIC_VALID=1,MAGIC_BLOCKED=2,REACTION_ABANDON=0x1c1 };
static u8 magic_reflects(u8 t,AbId ab)
{
    const u8 *r;
    if(st.action.phase>=CTX_SECONDARY||!(core_status_current(&st.unit[t].statuses,4)&2))return 0;
    r=cj_range(common,ab);return r&&(r[5]&128);
}
// Raw source heights choose the endpoint layer, independently of AoE tolerance.
// A distance tie chooses the higher height, then upper if heights are equal.
static u8 core_reflect_layer(const u8 *p,u8 height,u8 upper)
{
    s16 lo,hi;
    if(p[3]&128)return upper&&!(p[7]&128)?2:0;
    if(!upper||(p[7]&128))return 1;
    lo=(s16)p[0]-height;if(lo<0)lo=-lo;
    hi=(s16)p[4]-height;if(hi<0)hi=-hi;
    return hi<lo||(hi==lo&&p[4]>=p[0])?2:1;
}
static u8 magic_applicable(u8 t,AbId ab)
{
    const Unit *u=&st.unit[t];u8 validity=magic_target(t);
    if(abilities[ab].kind==AB_CLEANSE){
        const u8 *r=cj_section(common,CJ_STATUS)+6*abilities[ab].y;
        if(!validity||!u->hp||(core_status_current(&u->statuses,0)&0x20))return 0;
        if(unit_petrified(u)&&!(r[2]&0x80))return 0;
        return !(core_status_current(&u->statuses,3)&1)||(r[4]&1);
    }
    if(abilities[ab].formula!=13&&abilities[ab].formula!=0x35)return validity==MAGIC_VALID;
    if(!validity||unit_petrified(u)||(core_status_current(&u->statuses,3)&1))return 0;
    if(core_status_current(&u->statuses,0)&0x10)return validity==MAGIC_VALID;
    return !u->hp&&!(u->statuses.innate[0]&0x20);
}
#endif
