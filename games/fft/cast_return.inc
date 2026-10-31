// Source single-panel drains freeze the caster's paired event too. Geometry
// still excludes the caster; the resulting packet mask includes both effects.
static void cast_return(CastBatch *b)
{
    u8 i,a=b->actor;CastVictim *out=&b->victim[a];
    if(!magic_transfer_formula(abilities[b->ability].formula))return;
    for(i=0;i<NUNIT;i++)if(i!=a&&(b->mask&((u16)1<<i))&&b->victim[i].hit){
        const CastVictim *v=&b->victim[i];out->amount=v->amount;out->hit=v->hit;
        out->effect=EFFECT_RETURN|(v->effect==AB_MP_DAMAGE?AB_MP_HEAL:v->effect==AB_DAMAGE?AB_HEAL:AB_DAMAGE);
        b->mask|=(u16)1<<a; // Original last successful target overwrites this slot.
    }
}
