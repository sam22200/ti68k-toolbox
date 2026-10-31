// Pure effect-stage query, not command eligibility or future-position prediction.
// Caller-owned output never aliases the dedicated live Work packet.
static u8 batch_forecast(CastBatch *b)
{
    const Ability *ability;const u8 *raw;s16 mask;u16 bit;u8 t;
    if(!b||!W||b==&W->cast||!W->surface||
       (b->phase!=CAST_BATCH_EMPTY&&b->phase!=CAST_BATCH_FORECAST)||
       b->actor>=NUNIT||b->ability>=512)return 0;
    ability=&abilities[b->ability];raw=cj_range(common,b->ability);
    if(!cast_area_supported(ability,raw))return 0;
    mask=cast_targets(b->actor,b->ref,b->ability,b->follow);
    if(mask<0)return 0;
    // Every rejection precedes writes. Fill spare slots too, for a canonical result.
    b->mask=(u16)mask;
    for(t=0,bit=1;t<UNIT_CAPACITY;t++,bit<<=1){
        CastVictim *v=&b->victim[t];
        if(b->mask&bit)magic_forecast(b->actor,t,b->ability,v);
        else {v->amount=0;v->hit=0;v->effect=AB_NULL;}
    }
    cast_return(b);
    b->phase=CAST_BATCH_FORECAST;return 1;
}
