#ifdef FFT_CHAPTER
#include "stat_text.h"
#endif
// One compact prediction line centered above the arrow; both character names stay visible.
// The preview uses the combat formulas directly; no separate target window is needed.
u8 forecast_text(u8 a, u8 t, AbId ab, char *s)
{
#if defined(FFT_CHAPTER) && !defined(FFT_MODULE)
    weapon_preview(a,t,ab);t=W->preview_target;
#endif
    if(t>=NUNIT){u8 n=put_s(s,0,"Miss 0%");s[n]=0;return n;}
#if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define forecast_status_names CONTOUR_REF_st_names
#else
    static const char *const forecast_status_names[4]={"Defend","Poison","Dark","Silence"};
#endif
#ifdef FFT_CHAPTER
    if (ab < 512 && abilities[ab].formula == 0x25 && equip_break_slot(&st.unit[t], ab) == 255) ab = NO_ABIL;
#endif
    const Ability *b = ab == NO_ABIL ? RT_NULL : &abilities[ab];
    u8 hit = b ? skill_hit(a, t, ab) : hit_chance(a, t), n = 0, j;
#ifdef FFT_CHAPTER
    u8 kind=weapon_order(ab)?weapon_kind(a,t,ab):b&&(magic_formula(b->formula)||restore_ability(ab))?skill_kind(a,t,ab):b?b->kind:AB_DAMAGE;
    if(kind==AB_NULL||(b&&restore_ability(ab)&&!hit)||(b&&magic_formula(b->formula)&&!magic_applicable(t,ab)))n=put_s(s,n,"Miss");
    else if(kind==AB_QUICK||kind==AB_GOLEM)n=put_s(s,n,kind==AB_GOLEM?"Golem":skill_amount(a,t,ab,0)>>8?"CT 0":"Quick");
    else if(kind==AB_BRAVE||kind==AB_FAITH||kind==AB_SPEED||(kind==AB_BREAK&&performance_formula(b->formula)))n=stat_text(s,kind,ab,abilities[ab].y,1);
    else if(kind==AB_STATUS||kind==AB_CLEANSE)n=put_s(s,n,hit?"Status":"Miss");
    else if(kind==AB_MP_DAMAGE||(kind==AB_BREAK&&ab==0x8e)){n=put_s(s,n,"-");n=put_n(s,n,skill_amount(a,t,ab,0));n=put_s(s,n,"MP");}
    else if(kind==AB_EQUIP_BREAK||kind==AB_STEAL_EQUIP||kind==AB_STEAL_GIL)n=put_s(s,n,kind==AB_EQUIP_BREAK?"Break":"Steal");
    else if(kind==AB_STEAL_EXP){n=put_s(s,n,"EXP-");n=put_n(s,n,skill_amount(a,t,ab,0));}
    else if(kind==AB_MP_HEAL){
        const Unit *u=&st.unit[t];n=put_s(s,n,"+");
        n=put_n(s,n,action_hp(AB_HEAL,u->mp,u->max_mp,skill_amount(a,t,ab,0))-u->mp);n=put_s(s,n,"MP");
    }
    else if(kind==AB_HP_MP_HEAL||kind==AB_HP_DAMAGE_MP_HEAL){
        const Unit *u=&st.unit[t];u16 amount=skill_amount(a,t,ab,0);
        if(kind==AB_HP_DAMAGE_MP_HEAL&&amount>=u->hp)n=put_s(s,n,"KO");
        else {s[n++]=kind==AB_HP_MP_HEAL?'+':'-';n=put_n(s,n,kind==AB_HP_MP_HEAL?action_hp(AB_HEAL,u->hp,u->max_hp,amount)-u->hp:amount);n=put_s(s,n,"HP");}
        n=put_s(s,n," +");n=put_n(s,n,action_hp(AB_HEAL,u->mp,u->max_mp,restore_cap(u->max_mp))-u->mp);
        n=put_s(s,n,"MP");
    }
    else
#endif
    if (b && b->kind == AB_CURE) {
        if (hit) {
            for (j = 0; !(status_curable(&st.unit[t],b->y) & (1 << j)); j++) ;
            s[n++] = '-'; n = put_s(s, n, forecast_status_names[j]);
        } else n = put_s(s, n, "Miss");
    } else {                                   // a sure KO (the least damage left): "KO";
        const Unit *ut = &st.unit[t];          // a heal up to the HP the target can take
        u8 harm =
#ifdef FFT_CHAPTER
            effect_damage(kind);
#else
            !b || b->effect_damage(kind);
#endif
        u16 v = b ? skill_amount(a,t,ab,(b->flags&AF_CHARGE)?FORMULA_FORECAST:0) : attack_prediction(a,t);
#ifdef FFT_CHAPTER
        if((weapon_order(ab)||(b&&magic_damage_formula(b->formula)))&&(s16)v<0){harm=!harm;v=(u16)(0U-v);}
#endif
        if (harm && ut->hp && v >= ut->hp) n = put_s(s, n, "KO");
        else {
            if (!harm && (!b||b->kind != AB_REVIVE)) v = action_hp(AB_HEAL,ut->hp,ut->max_hp,v)-ut->hp;
            s[n++] = harm ? '-' : '+';
            n = put_n(s, n, v);
            n = put_s(s, n, "HP");
        }
    }
    s[n++] = ' '; n = put_n(s, n, hit); s[n++] = '%';
#ifdef FFT_CHAPTER
    if(W->preview_count>1){s[n++]=' ';s[n++]='x';n=put_n(s,n,W->preview_count);}
#endif
    s[n] = 0;
    return n;
}

