#include "cast_return.inc"
// Effect-stage entry only. Initial command range/binding/CT dispatch is separate.
static u8 batch_prepare(CastBatch *b,u8 ordered)
{
    const Ability *ability;const u8 *raw;s16 mask;u16 bit;u8 a,t;
    if(!b||b->phase!=CAST_BATCH_EMPTY||!W||!W->surface)return 0;
    a=b->actor;
    if(!action_available(a,a)||b->ability>=512||!can_use(a,b->ability))return 0;
    ability=&abilities[b->ability];
    raw=cj_range(common,b->ability);if(!cast_area_supported(ability,raw))return 0;
    if(st.unit[a].x>=MAP_W||st.unit[a].z>=MAP_H||UNIT_CELL(&st.unit[a])>=MAP_N||st.unit[a].dc==DC_GONE)return 0;
    mask=cast_targets(a,b->ref,b->ability,b->follow);
    if(mask<0){
        // An accepted cast pays before target setup, including a vanished centre.
        // The public isolated entry still rejects malformed references purely.
        if(!ordered||mask!=TARGETS_INVALID)return 0;
        mask=0;
    }
    // All validation precedes payment/RNG/action ownership/output changes.
    act_start(a,a);st.abil=b->ability;st.hit=0;
    st.kx=st.unit[a].x;st.kz=st.unit[a].z;
    st.unit[a].mp-=skill_cost(a,b->ability);
    b->mask=(u16)mask;
    for(t=0,bit=1;t<NUNIT;t++,bit<<=1){
        CastVictim *v=&b->victim[t];v->amount=0;v->hit=0;v->effect=AB_NULL;
        if(b->mask&bit)magic_prepare(a,t,b->ability,v);
    }
    cast_return(b);
    b->phase=CAST_BATCH_READY;
    if(b==&W->cast){
        AreaImpact *q=&W->area_impact;
        q->centre=area_resolution_cell(b->follow,b->ref);
        q->ready=0;q->next=0;q->oil_ready=0;
    }
    return 1;
}
static u8 batch_owned_prepare(void)
{
    u8 result=batch_prepare(&W->cast,1);
    if(!result)st.open_resolving=0;
    return result;
}
