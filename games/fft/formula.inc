#ifdef FFT_MODULE
#define FFT_FORMULA_FN static
#else
#define FFT_FORMULA_FN
#endif
// One evaluator source, compiled in MOD_FORMULA and locally in MOD_AI. An AI
// candidate never evicts its executing overlay. RNG stays in the caller.
#ifdef FFT_CHAPTER
#include "charge_math.inc"
#if !defined(FFT_MODULE) || MOD_PART(MOD_FORMULA) || MOD_PART(MOD_NUMBERS) || MOD_PART(MOD_COMMAND) || MOD_PART(MOD_COMBAT)
#include "equip_break.inc"    // forecasts and COMMAND only: ACTION commits, the AI never chooses breaks
#endif
#endif
FFT_FORMULA_FN u16 skill_amount(u8 a, u8 t, AbId ab, u16 r);   // Steal Exp: nothing to steal, no hit
#if !defined(FFT_MODULE) || !MOD_PART(MOD_ACTION)   // ACTION keeps only the amount (formula_call)
// Execution has already paid. It must not test the remaining MP a second time.
static u8 skill_effect_hit(u8 a, u8 t, AbId ab)
{
#ifdef FFT_CHAPTER
    if (ab >= NABIL || abilities[ab].kind == 255 || !unit_can_act(&st.unit[a]) || status_blocks_skill(st.unit[a].status,ab,st.unit[a].set)) return 0;
#endif
    const Ability *b = &abilities[ab];
    const Unit *ut = &st.unit[t];
#ifdef FFT_CHAPTER
    if(b->kind==AB_THROW_SELECTOR)return 0;
    if(unit_airborne(ut))return 0;
    if(jump_ability(ab))return magic_target(t)==MAGIC_VALID?100:0;
    if(restore_ability(ab))return restore_applicable(t,ab)?100:0;
    if(b->kind==AB_CLEANSE&&b->formula==0x38)return magic_applicable(t,ab)&&magic_heal(a,t,ab,0)?100:0;
    if(b->formula==14&&!(core_status_current(&ut->statuses,0)&16)&&!magic_heal(a,t,ab,3))return 0;
    if(magic_formula(b->formula)||(b->formula==0x31&&b->element))return magic_applicable(t,ab)&&skill_kind(a,t,ab)!=AB_NULL&&
        (!magic_status_formula(b->formula)||magic_heal(a,t,ab,0))?magic_hit(a,t,ab,0):0;
#endif
#if !defined(FFT_MODULE) || MOD_PART(MOD_FORMULA) || MOD_PART(MOD_NUMBERS)   // forecasts only
    if ((u8)(b->formula - 0x25) <= 1) { // equipment: nothing to break, the Attack's (nothing to
        u16 c, h = magic_hit(a,t,ab,0);     // steal: 0); Maintenance: 0
        if (equip_break_slot(ut, ab) == 255) return b->formula == 0x25 ? (u8)h : 0;
        if (st.support[t] == SUPPORT_MAINTENANCE) return 0;
        c = equip_break_percent(a, t, ab);
        return (u8)(c >= 100 ? h : h * c / 100);
    }
#endif
#if !defined(FFT_MODULE) || !(MOD_PART(MOD_AI) || MOD_PART(MOD_COMMAND)) // AI rejects; COMMAND handles before skill_effect_hit
    if ((u8)(b->formula - 0x27) <= 1 || (u8)(b->formula - 0x2b) <= 1) {   // resources, breaks: evasion x the percent;
        u16 c = charge_query(a, t, ab, 0, 0), h = magic_hit(a,t,ab,0);   // nothing left to steal: no hit
        return (u8)(b->formula-0x27)<=1 && !skill_amount(a,t,ab,0) ? 0 : (u8)(c >= 100 ? h : h * c / 100);
    }
#endif
    if (b->kind == AB_CURE) return status_curable(ut,b->y) ? 100 : 0;
    if (b->kind == AB_REVIVE) return ut->hp ? 0 : 100;
    return b->flags & AF_EVADE ? magic_hit(a,t,ab,0) : 100;
}
FFT_FORMULA_FN u8 skill_hit(u8 a,u8 t,AbId ab)
{
#ifdef FFT_CHAPTER
    if(unit_petrified(&st.unit[t])&&(ab>=NABIL||abilities[ab].kind!=AB_CLEANSE))return 0;
    if(st.unit[a].mp<skill_cost(a,ab))return 0;
#endif
    return skill_effect_hit(a,t,ab);
}
#endif
FFT_FORMULA_FN u16 skill_amount(u8 a, u8 t, AbId ab, u16 r)
{
#ifdef FFT_CHAPTER
    if (ab >= NABIL || abilities[ab].kind == 255) return 0;
#endif
    const Ability *b = &abilities[ab];
    const Unit *ua = &st.unit[a];
    u16 k;
#ifdef FFT_CHAPTER
    if(unit_monster(ua)&&ab==265)return attack_prediction(a,t);
    if(jump_ability(ab))return magic_heal(a,t,ab,0);
    if(restore_ability(ab))return restore_quantity(t,ab,r);
    if(b->kind==AB_CLEANSE)return magic_heal(a,t,ab,0);
    if(magic_formula(b->formula))return magic_quantity(a,t,ab);
#endif
    k = (u16)(((u32)r * b->y >> 15) + 1);
#ifdef FFT_CHAPTER
    // Punch Art strikes share the resident physical evaluator (CORE_PUNCH_ART.md).
    if ((u8)(b->formula - 0x31) <= 1) return charge_query(a, t, ab, 0, r);
#if !defined(FFT_MODULE) || !(MOD_PART(MOD_AI) || MOD_PART(MOD_ACTION))   // formula_call: FORMULA
    if (b->formula == 0x27) return magic_heal(a,t,ab,0);
    if (b->formula == 0x28) {           // the EXP stolen: raw Speed + Y, at most 100 and the target's
        k = ua->sp + b->y; if (k > 100) k = 100;
        return k < st.unit[t].exp ? k : st.unit[t].exp;
    }
    if (b->formula == 0x2C)             // Magic Break: ceil(max MP x Y / 100); 999 x 50 fits a u16
        return (u16)((u16)(st.unit[t].max_mp * b->element + 99) / 100);
#endif
    if (b->formula == 0x36 || b->formula == 0x2B) return b->y;
    if (b->formula == 0x2D || b->formula == 0x30 || (b->formula == 1 && b->flags & AF_CHARGE)) {
        if (b->flags & AF_CHARGE) {
            if(r&FORMULA_FORECAST)return weapon_total(a,t,ab);
            return weapon_forecast(a,t,ab,st.mode==M_ACT&&st.atk==a?st.hand:0,0);
        }
        return weapon_skill_amount(a,t,ab);
    }
#endif
#ifdef FFT_CHAPTER
    if(b->formula==0x37)return dash_amount(a,t,k);
#else
    if(b->kind==AB_DAMAGE){
        u16 d=(u16)(k*zodiac_xa(ua,&st.unit[t],ua->pa));
        return d>999?999:d;
    }
#endif
#ifdef FFT_CHAPTER
    if (b->formula == 0x48) return (u16)(b->y * 10);
    if (b->formula == 0x4B) return k;
    if (b->formula == 0x3C) return (u16)(ua->max_hp / 5 * 2);
#else
    if (b->kind == AB_HEAL) return (u16)(b->y * 10);
    if (b->kind == AB_REVIVE) return k;
    if (b->kind == AB_WISH) return (u16)(ua->max_hp / 5 * 2);
#endif
    return 0;
}
#if !MOD_CORE && MOD_PART(MOD_FORMULA)
u16 mod_entry(u8 op, u8 a)
{
    (void)a;
#ifdef FFT_CHAPTER
    if(op==14){weapon_freeze();return 0;}
#endif
    if(op==9)return charge_amount(st.formula.actor,st.formula.target,st.formula.ability,a&1,(a>>1)&1,(a>>2)&1,st.formula.roll);
    return op ? skill_amount(st.formula.actor, st.formula.target, st.formula.ability, st.formula.roll)
              : skill_hit(st.formula.actor, st.formula.target, st.formula.ability);
}
#endif

#undef FFT_FORMULA_FN
