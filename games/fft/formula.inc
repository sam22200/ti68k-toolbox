// One evaluator source, compiled in MOD_FORMULA and locally in MOD_AI. An AI
// candidate never evicts its executing overlay. RNG stays in the caller.
#ifdef FFT_CHAPTER
#include "charge_math.inc"
#endif
u16 skill_amount(u8 a, u8 t, AbId ab, u16 r);   // Steal Exp: nothing to steal, no hit
// Execution has already paid. It must not test the remaining MP a second time.
static u8 skill_effect_hit(u8 a, u8 t, AbId ab)
{
#ifdef FFT_CHAPTER
    if (ab >= 512 || abilities[ab].kind == 255 || !unit_can_act(&st.unit[a]) || status_blocks_skill(st.unit[a].status,ab,st.unit[a].set)) return 0;
#endif
    const Ability *b = &abilities[ab];
    const Unit *ut = &st.unit[t];
#ifdef FFT_CHAPTER
    if(restore_ability(ab))return restore_applicable(t,ab)?100:0;
    if(b->kind==AB_CLEANSE&&b->formula==0x38)return magic_applicable(t,ab)&&magic_heal(a,t,ab,0)?100:0;
    if(b->formula==14&&!(core_status_current(&ut->statuses,0)&16)&&!magic_heal(a,t,ab,3))return 0;
    if(magic_formula(b->formula))return magic_applicable(t,ab)&&skill_kind(a,t,ab)!=AB_NULL&&
        (!magic_status_formula(b->formula)||magic_heal(a,t,ab,0))?magic_hit(a,t,ab,0):0;
#endif
    if (b->formula == 0x28 || b->formula == 0x2b) {   // Steal Exp, breaks: evasion x the percent;
        u16 c = charge_query(a, t, ab, 0, 0), h = hit_chance(a, t);   // nothing left to steal: no hit
        return b->formula == 0x28 && !skill_amount(a, t, ab, 0) ? 0 : (u8)(c >= 100 ? h : h * c / 100);
    }
    if (b->kind == AB_CURE) return status_curable(ut,b->y) ? 100 : 0;
    if (b->kind == AB_REVIVE) return ut->hp ? 0 : 100;
    return b->flags & AF_EVADE ? hit_chance(a, t) : 100;
}
u8 skill_hit(u8 a,u8 t,AbId ab)
{
#ifdef FFT_CHAPTER
    if(unit_petrified(&st.unit[t])&&(ab>=512||abilities[ab].kind!=AB_CLEANSE))return 0;
    if(st.unit[a].mp<skill_cost(a,ab))return 0;
#endif
    return skill_effect_hit(a,t,ab);
}
u16 skill_amount(u8 a, u8 t, AbId ab, u16 r)
{
#ifdef FFT_CHAPTER
    if (ab >= 512 || abilities[ab].kind == 255) return 0;
#endif
    const Ability *b = &abilities[ab];
    const Unit *ua = &st.unit[a];
    u16 k;
#ifdef FFT_CHAPTER
    if(restore_ability(ab))return restore_quantity(t,ab,r);
    if(b->kind==AB_CLEANSE)return magic_heal(a,t,ab,0);
    if(magic_formula(b->formula))return magic_quantity(a,t,ab);
#endif
    k = (u16)(((u32)r * b->y >> 15) + 1);
#ifdef FFT_CHAPTER
    // Punch Art strikes share the resident physical evaluator (CORE_PUNCH_ART.md).
    if ((u8)(b->formula - 0x31) <= 1) return charge_query(a, t, ab, 0, r);
    if (b->formula == 0x28) {           // the EXP stolen: raw Speed + Y, at most 100 and the target's
        k = ua->sp + b->y; if (k > 100) k = 100;
        return k < st.unit[t].exp ? k : st.unit[t].exp;
    }
    if (b->formula == 0x36 || b->formula == 0x2B || b->formula == 0x2C) return b->y;
    if (b->formula == 0x2D || b->formula == 0x30 || (b->formula == 1 && b->flags & AF_CHARGE)) {
        u8 weapon=hand_weapon(ua,0);
        const Item *w = weapon == 255 ? RT_NULL : &items[weapon];
        s16 pa = ua->pa,xa;
        if (b->flags & AF_CHARGE) {
            u8 forecast=(r&FORMULA_FORECAST)!=0;
            u8 hand=forecast?0:(st.mode==M_ACT&&st.atk==a?st.hand:0);
            u16 damage=charge_amount(a,t,ab,hand,0,0,0);
            if(forecast&&hand_count(a)==2&&damage<st.unit[t].hp)
                damage+=charge_amount(a,t,ab,1,1,0,0);
            return damage;
        }
        xa=zodiac_xa(ua,&st.unit[t],unit_status_xa(&st.unit[t],pa));
        return (u16)(xa * ((w ? w->wp : 1) + b->y));
    }
#endif
#ifdef FFT_CHAPTER
    if (b->formula == 0x37) {
#else
    if (b->kind == AB_DAMAGE) {
#endif
        s16 xa=ua->pa;
#ifdef FFT_CHAPTER
        xa=unit_status_xa(&st.unit[t],xa);
#endif
        u16 d = (u16)(k * zodiac_xa(ua, &st.unit[t], xa));
        return d > 999 ? 999 : d;
    }
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
#include "reward_value.h"
#include "status_commit.inc"
#define FFT_STATUS_BATCH
#define batch_commit batch_status_commit
#define action_magic_commit status_magic_commit
#include "batch_commit.inc"
#undef action_magic_commit
#undef batch_commit
#undef FFT_STATUS_BATCH
#include "strike_runtime.inc"
u16 mod_entry(u8 op, u8 a)
{
    (void)a;
    if(op==12)return a?batch_status_commit(M.cast_batch_out):(batch_status_commit(&W->cast)&2);
    if(op==13)return status_scalar_commit();
    if(op==3)return status_apply_request();
    if(op==7)return strike_target();
    if(op==8){strike_launch(a);return 0;}
    if(op==9)return charge_amount(st.formula.actor,st.formula.target,st.formula.ability,a&1,(a>>1)&1,(a>>2)&1,st.formula.roll);
    return op ? skill_amount(st.formula.actor, st.formula.target, st.formula.ability, st.formula.roll)
              : skill_hit(st.formula.actor, st.formula.target, st.formula.ability);
}
#endif
