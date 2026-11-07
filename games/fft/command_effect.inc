static u8 command_effect(u8 a,u8 t,AbId ab)
{
    const Ability *b=&abilities[ab];
    if(hamedo_begin(a,t,ab))return 1;
#ifdef FFT_CHAPTER
    mime_record(a,UNIT_CELL(&st.unit[t]),ab,0,0);
    // Deferred callers reach this point only when resolving; arming costs no MP.
    st.unit[a].mp-=skill_cost(a,ab);
#endif
    act_start(a, t);
    st.abil = ab;
    if (b->flags & AF_ITEM &&
#ifdef FFT_CHAPTER
        !MIME_ACTIVE(a) &&
#endif
#ifdef FFT_CHAPTER
        unit_home_team(&st.unit[a])!=TEAM_ENEMY
#else
        st.unit[a].team==TEAM_PLAYER
#endif
    ) {
#ifdef FFT_CHAPTER
        if(b->formula==99)battle_save.inv[b->bag]--;
        else
#endif
        st.bag[b->bag]--;
        if (W) W->win_k[0] = 0;                    // the item list shows the count
    }
#ifdef FFT_CHAPTER
    if(unit_monster(&st.unit[a])&&ab==265){attack_roll(a,t);st.abil=ab;return 1;}
    if(magic_formula(b->formula)){
        st.kx=st.unit[a].x;st.kz=st.unit[a].z; // saved original-caster origin for Reflect
        CastVictim v;magic_prepare(a,t,ab,&v);
        st.effect_kind=v.effect;st.hit=v.hit;st.dmg=v.amount;
        return 1;
    }
    if(jump_ability(ab)){
        st.hit=magic_target(t)==MAGIC_VALID;
        if(st.hit)st.dmg=skill_amount(a,t,ab,0);
        return 1;
    }
    if(b->flags&AF_CHARGE){
        st.action.left=hand_count(a)-1;st.action.cell=UNIT_CELL(&st.unit[t]);
        charge_roll(a,t,ab);return 1;
    }
#endif
#ifdef FFT_CHAPTER
    if(restore_ability(ab)){restore_prepare(t,ab);return 1;}
    if(b->kind==AB_CLEANSE)st.effect_kind=AB_CLEANSE;
    if(b->formula==0x26&&equip_break_slot(&st.unit[t],ab)==255){st.hit=0;return 1;}   // nothing to steal: forced failure
    if((u8)(b->formula-0x25)<=3||(u8)(b->formula-0x2b)<=1){
        // Steal Exp, breaks: physical evasion, then the percent roll (none at 100), then the effect.
        u16 c;st.hit=magic_hit(a,t,ab,1);if(!st.hit)return 1;
        c=b->formula<=0x26?equip_break_percent(a,t,ab):charge_query(a,t,ab,0,0);
        if(c<100&&(u16)((u32)rand15()*100>>15)>=c){st.hit=0;return 1;}
        if(b->formula==0x2b){st.dmg=b->y;return 1;}            // a break: the stat loss (commit)
        if(b->formula==0x2c){st.dmg=skill_amount(a,t,ab,0);return 1;}   // Magic Break: the MP lost
        if(b->formula<=0x26){st.hit=st.support[t]!=SUPPORT_MAINTENANCE;st.dmg=equip_break_slot(&st.unit[t],ab);return 1;}
        st.dmg=skill_amount(a,t,ab,0);st.hit=st.dmg!=0;return 1;
    }
    if((u8)(b->formula-0x31)<=1){
        // Punch Art strikes: physical evasion, a 4% critical with knockback, then
        // Repeating Fist's 1..X hits (source draw order).
        st.hit=magic_hit(a,t,ab,1);if(!st.hit)return 1;
        st.crit=(u8)(((u32)rand15()*100>>15)<4);
        st.dmg=skill_amount(a,t,ab,0x8000|(st.crit?rand15():0));
        if(b->formula==0x32){
            u16 n=st.dmg*(u16)(((u32)rand15()*cj_range(common,ab)[9]>>15)+1);
            st.dmg=n>999?999:n;
        }
        if(st.crit)knockback(a,t);
        return 1;
    }
#endif
    st.hit = skill_effect_hit(a, t, ab) == 100;
    if (b->flags & AF_EVADE) st.hit = magic_hit(a, t, ab, 1);
    if (!st.hit) return 1;
    st.dmg = skill_amount(a, t, ab,
#ifdef FFT_CHAPTER
        b->kind==AB_CLEANSE?0:
#endif
        rand15());
    if (b->kind == AB_CURE) st.dmg = status_curable(&st.unit[t],b->y);   // the statuses removed (shown)
    if (b->kind == AB_WISH) st.self = st.unit[a].max_hp / 5;
    if (b->kind == AB_DAMAGE
#ifdef FFT_CHAPTER
        && !(b->flags & AF_WEAPON) && b->formula!=8
#endif
    ) {
        u16 r = (u16)((u32)rand15() * st.unit[a].brave >> 15);
        if (((u32)rand15() * st.unit[t].brave >> 15) < r) knockback(a, t);
    }
    return 1;
}
