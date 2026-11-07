// Source Reflect is a primary pre-formula miss, not magical evasion.
#ifndef FFT_REFLECT_RULES_H
#define FFT_REFLECT_RULES_H
enum { REACTION_REFLECT=0x1b8,MAGIC_ABSENT=0,MAGIC_VALID=1,MAGIC_BLOCKED=2,REACTION_ABANDON=0x1c1 };
static u8 magic_reflects_from(u8 a,u8 t,AbId ab)
{
    const u8 *r;
    if(st.unit[a].set==21||st.action.phase>=CTX_SECONDARY||!(core_status_current(&st.unit[t].statuses,4)&2))return 0;
    r=cj_range(common,ab);return r&&(r[5]&128);
}
#ifndef FFT_PRODUCTION_CORE
static u8 magic_reflects(u8 t,AbId ab){return magic_reflects_from(st.act,t,ab);}
#endif
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
// The same single-recipient bounce for execution and hypothetical AI origins.
// No live action/status mutation and no second reflection at the endpoint.
static u8 reflect_recipient(u8 t,s16 ox,s16 oz)
{
    const Unit *u=&st.unit[t];s16 x=((s16)u->x<<1)-ox,z=((s16)u->z<<1)-oz;
    const u8 *p;u16 cell;u8 layer,found;
    if((x==u->x&&z==u->z)||(u16)x>=MAP_W||(u16)z>=MAP_H||!W->surface)return 255;
    p=W->surface+((u16)(W->grid_row[u->z]+u->x)<<3)+4*node_layer(UNIT_CELL(u));
    layer=core_reflect_layer(W->surface+((u16)(W->grid_row[z]+x)<<3),p[0],W->surface_flags&SURFACE_MOVE_UPPER);
    if(!layer)return 255;cell=node_at(x,z,layer-1);if(cell>=MAP_N)return 255;
    found=unit_at_cell((u8)cell);return found?found-1:255;
}
#ifndef magic_applicable
static u8 magic_applicable(u8 t,AbId ab)
{
    const Unit *u=&st.unit[t];u8 validity=magic_target(t);
    if(!validity)return 0;
    if(abilities[ab].kind==AB_CLEANSE){
        const u8 *r=cj_section(common,CJ_STATUS)+6*abilities[ab].y;
        if(!u->hp||(core_status_current(&u->statuses,0)&0x20))return 0;
        if(unit_petrified(u)&&!(r[2]&0x80))return 0;
        return !(core_status_current(&u->statuses,3)&1)||(r[4]&1);
    }
    if(abilities[ab].formula!=13&&abilities[ab].formula!=0x35)return validity==MAGIC_VALID;
    if(unit_petrified(u)||(core_status_current(&u->statuses,3)&1))return 0;
    if(core_status_current(&u->statuses,0)&0x10)return validity==MAGIC_VALID;
    return !u->hp&&!(u->statuses.innate[0]&0x20);
}
#endif
#endif
