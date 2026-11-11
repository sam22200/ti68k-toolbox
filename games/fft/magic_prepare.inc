// Shared execution preparation. HP, MP, status and rewards are untouched.
// Source evasion and the successful formula/proc draw still advance battle RNG.
static void magic_prepare(u8 a,u8 t,AbId ab,CastVictim *v)
{
    const Ability *b=&abilities[ab];
#if MOD_PART(MOD_TARGETS)
    if(draw_out_formula(b->formula)&&st.unit[a].job!=SOURCE_JOB_MIME)
        st.crit=(u16)(muls16((s16)rand15(),100)>>15)<items[ab-0x26].wp; // last target's katana roll
#endif
    if(magic_reflects_from(a,t,ab)){v->effect=AB_REFLECT;v->hit=0;v->amount=0;return;}
    v->effect=skill_kind(a,t,ab);v->amount=0;
    v->hit=magic_hit(a,t,ab,1);
    if(v->hit==2){v->hit=1;v->effect=AB_CATCH;v->amount=b->bag;return;}
    if(v->hit&&(v->effect!=AB_NULL||b->formula==0x31)){
        if(b->formula==14&&v->effect==AB_DEATH&&!magic_heal(a,t,ab,3)){v->hit=0;v->effect=AB_NULL;return;}
        u16 amount;
        if(b->formula==0x2D||b->formula==0x2F){   // Holy/Dark Sword: 4% critical (knockback), amount, Holy's status at 100%
            u8 crit=(((u32)rand15()*100>>15)<4);u16 critical=crit?rand15():0;
            amount=charge_query(a,t,ab,(u8)(crit<<2),critical);
            if(b->flags&AF_AREA)st.crit=crit;   // the batch pushes a sole victim (batch_prepare)
            else if(crit){st.dmg=amount;knockback(a,t);amount=st.dmg;}   // lethal test, fall damage
            if(b->formula==0x2D&&magic_heal(a,t,ab,3))v->effect|=EFFECT_STATUS;
        }else
        if(b->formula==0x31){   // Punch Art: the 4% critical, then its amount draw
            u8 crit=(((u32)rand15()*100>>15)<4);u16 critical=crit?rand15():0;
            if(b->element)st.crit=crit;
            amount=charge_query(a,t,ab,0,0x8000|critical);
            if(b->element&&v->effect!=AB_NULL)rand15(); // original conditional proc, empty status set
        }else
#if MOD_PART(MOD_TARGETS)
        if(performance_formula(b->formula)&&b->kind==AB_STATUS){
            static const u8 ids[]={18,25,26,27,28,10,11,12,22,24,29,30,35};
            u8 k=(u8)(muls16((s16)rand15(),ab==91?5:8)>>15);
            amount=magic_heal(a,t,ab,128|ids[(ab==91?0:5)+k]);
            if(!amount)v->hit=0;
        }else
#endif
        amount=magic_quantity(a,t,ab);
        if(magic_damage_formula(b->formula)||b->formula==9)rand15(); // Cure/Raise have no formula08 proc draw.
#if MOD_PART(MOD_TARGETS)
        if(b->formula==0x20&&(u16)(muls16((s16)rand15(),100)>>15)<19){
            u8 choice=ab==82?((rand15()>>14)?3:2):1;
            u16 packed=magic_heal(a,t,ab,ab==82?(128|(choice==2?SID_CONFUSION:SID_SENTENCE)):3);
            if(packed){amount|=(u16)choice<<10;v->effect|=EFFECT_STATUS;}
        }
#endif
        v->amount=amount;
        if(b->formula==0x24&&(u16)(muls16((s16)rand15(),100)>>15)<19&&magic_heal(a,t,ab,3))
            v->effect|=EFFECT_STATUS;
        if(magic_status_formula(b->formula)&&!amount)v->hit=0;
    }else v->hit=0;
#if MOD_PART(MOD_TARGETS)
    // The source invalid-target finalizer clears the actor's staged katana
    // result too, even when earlier victims succeeded.
    if(draw_out_formula(b->formula)&&magic_target(t)==MAGIC_BLOCKED)st.crit=0;
#endif
    if(!magic_applicable(t,ab)||v->effect==AB_NULL)v->hit=0;
    if(!v->hit){v->amount=0;v->effect=AB_NULL;}
    else if((b->element&128)&&(core_status_removable(&st.unit[t].statuses,2)&128))
        v->effect|=EFFECT_OIL;
}
