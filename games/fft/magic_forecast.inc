// Current per-victim effect, shared by full footprints and cached AI planning.
static void magic_forecast(u8 a,u8 t,AbId ab,CastVictim *v)
{
    v->amount=0;v->hit=0;v->effect=AB_NULL;
    if(!magic_applicable(t,ab))return;
    if(magic_reflects(t,ab)){v->effect=AB_REFLECT;return;}
    v->effect=skill_kind(a,t,ab);
    if(v->effect!=AB_NULL)v->hit=magic_hit(a,t,ab,0);
    if(abilities[ab].formula==14&&v->effect==AB_DEATH&&!magic_heal(a,t,ab,3))v->hit=0;
    if(!v->hit){v->effect=AB_NULL;return;}
    v->amount=magic_quantity(a,t,ab);
    if(magic_status_formula(abilities[ab].formula)&&!v->amount){v->hit=0;v->effect=AB_NULL;}
    if((abilities[ab].element&128)&&(core_status_removable(&st.unit[t].statuses,2)&128))
        v->effect|=EFFECT_OIL;
}
