#include "cast_return.inc"
// Effect-stage entry only. Initial command range/binding/CT dispatch is separate.
static u8 batch_prepare(CastBatch *b,u8 ordered)
{
    const Ability *ability;const u8 *raw;s16 mask;u16 bit;u8 a,t;
    if(!b||b->phase!=CAST_BATCH_EMPTY||!W||!W->surface)return 0;
    a=b->actor;
    if(!action_available(a,a)||b->ability>=512||!can_use(a,b->ability))return 0;
    if(b->follow==MATH_BIND&&!math_filters(a,b->ref))return 0;
    ability=&abilities[b->ability];
    raw=cj_range(common,b->ability);if(b->follow==MATH_BIND?!math_spell(b->ability):!cast_area_supported(ability,raw))return 0;
    if(st.unit[a].x>=MAP_W||st.unit[a].z>=MAP_H||UNIT_CELL(&st.unit[a])>=MAP_N||st.unit[a].dc==DC_GONE)return 0;
    mask=cast_targets(a,b->ref,b->ability,b->follow);
    if(mask<0){
        // An accepted cast pays before target setup, including a vanished centre.
        // The public isolated entry still rejects malformed references purely.
        if(!ordered||mask!=TARGETS_INVALID)return 0;
        mask=0;
    }
    // All validation precedes payment/RNG/action ownership/output changes.
    mime_record(a,b->follow==MATH_BIND?UNIT_CELL(&st.unit[a]):area_resolution_cell(b->follow,b->ref),b->ability,b->follow,b->ref);
    act_start(a,a);st.abil=b->ability;st.hit=0;
    st.kx=st.unit[a].x;st.kz=st.unit[a].z;
    st.unit[a].mp-=skill_cost(a,b->ability);
    b->mask=(u16)mask;
    for(t=0;t<NUNIT;t++){b->victim[t].amount=0;b->victim[t].hit=0;b->victim[t].effect=AB_NULL;}
    {
        u16 todo=b->mask;u8 next=0;
        while(todo){
            if(raw[4]&4){
                u8 j,best=255;t=0;
                for(j=0,bit=1;j<NUNIT;j++,bit<<=1)if(todo&bit){
                    s16 dx=(s16)st.unit[j].x-st.unit[a].x,dz=(s16)st.unit[j].z-st.unit[a].z;
                    u8 d=(u8)((dx<0?-dx:dx)+(dz<0?-dz:dz));
                    if(d<best){best=d;t=j;}
                }
            }else for(t=next;!(todo&((u16)1<<t));t++){}
            next=t+1; // radial batches scan slots once; only lines need distance ranking
            bit=(u16)1<<t;todo&=(u16)~bit;
            magic_prepare(a,t,b->ability,&b->victim[t]);
            // One linear victim reuses the existing frozen scalar knockback.
            if((raw[8]==0x31||raw[8]==0x2D)&&(raw[4]&4)&&!(b->mask&(b->mask-1))&&st.crit&&b->victim[t].hit&&
               (b->victim[t].effect&EFFECT_KIND_MASK)==AB_DAMAGE){
                st.dmg=b->victim[t].amount;knockback(a,t);b->victim[t].amount=st.dmg;
            }
        }
    }
    if(draw_out_formula(ability->formula)){
        u8 any=0;
        for(t=0;t<NUNIT;t++)if(b->victim[t].hit)any=1;
        if(!any)st.crit=0; // source no-hit finalizer cancels every katana loss
    }
    cast_return(b);
    b->phase=CAST_BATCH_READY;
    if(b==&W->cast){
        AreaImpact *q=&W->area_impact;
        q->centre=b->follow==MATH_BIND?UNIT_CELL(&st.unit[a]):area_resolution_cell(b->follow,b->ref);
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
