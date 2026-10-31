// Shared execution preparation. HP, MP, status and rewards are untouched.
// Source evasion and the successful formula/proc draw still advance battle RNG.
static void magic_prepare(u8 a,u8 t,AbId ab,CastVictim *v)
{
    const Ability *b=&abilities[ab];
    if(magic_reflects(t,ab)){v->effect=AB_REFLECT;v->hit=0;v->amount=0;return;}
    v->effect=skill_kind(a,t,ab);v->amount=0;
    v->hit=magic_hit(a,t,ab,1);
    if(v->hit&&v->effect!=AB_NULL){
        if(b->formula==14&&v->effect==AB_DEATH&&!magic_heal(a,t,ab,3)){v->hit=0;v->effect=AB_NULL;return;}
        u16 amount;
        if(b->formula==0x31){   // Spin Fist: the 4% critical, then its amount draw (no knockback in an area)
            u16 critical=(((u32)rand15()*100>>15)<4)?rand15():0;
            amount=charge_query(a,t,ab,0,0x8000|critical);
        }else amount=magic_quantity(a,t,ab);
        if(b->formula==8||b->formula==9)rand15(); // Cure/Raise have no formula08 proc draw.
        v->amount=amount;
        if(magic_status_formula(b->formula)&&!amount)v->hit=0;
    }else v->hit=0;
    if(!magic_applicable(t,ab))v->hit=0;
    if(!v->hit){v->amount=0;v->effect=AB_NULL;}
    else if((b->element&128)&&(core_status_removable(&st.unit[t].statuses,2)&128))
        v->effect|=EFFECT_OIL;
}
