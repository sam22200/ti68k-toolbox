static u8 command_effect(u8 a,u8 t,AbId ab)
{
    const Ability *b=&abilities[ab];
#ifdef FFT_CHAPTER
    // Deferred callers reach this point only when resolving; arming costs no MP.
    st.unit[a].mp-=skill_cost(a,ab);
#endif
    act_start(a, t);
    st.abil = ab;
#ifdef FFT_CHAPTER
    if(magic_formula(b->formula)){
        st.kx=st.unit[a].x;st.kz=st.unit[a].z; // saved original-caster origin for Reflect
        CastVictim v;magic_prepare(a,t,ab,&v);
        st.effect_kind=v.effect;st.hit=v.hit;st.dmg=v.amount;
        return 0;
    }
    if(b->flags&AF_CHARGE){
        st.action.left=hand_count(a)-1;st.action.cell=UNIT_CELL(&st.unit[t]);
        charge_roll(a,t,ab);return 0;
    }
#endif
    if (b->flags & AF_ITEM && st.unit[a].team == TEAM_PLAYER) {
        st.bag[b->bag]--;
        if (W) W->win_k[0] = 0;                    // the item list shows the count
    }
#ifdef FFT_CHAPTER
    if(restore_ability(ab)){restore_prepare(t,ab);return 0;}
    if(b->kind==AB_CLEANSE)st.effect_kind=AB_CLEANSE;
    if(b->formula==0x28||b->formula==0x2b){
        // Steal Exp, breaks: physical evasion, then the percent roll (none at 100), then the effect.
        u16 c;st.hit=roll_hit(a,t);if(!st.hit)return 0;
        c=charge_query(a,t,ab,0,0);
        if(c<100&&(u16)((u32)rand15()*100>>15)>=c){st.hit=0;return 0;}
        if(b->formula==0x2b){st.dmg=b->y;return 0;}            // a break: the stat loss (commit)
        st.dmg=skill_amount(a,t,ab,0);st.hit=st.dmg!=0;return 0;
    }
    if((u8)(b->formula-0x31)<=1){
        // Punch Art strikes: physical evasion, a 4% critical with knockback, then
        // Repeating Fist's 1..X hits (source draw order).
        st.hit=roll_hit(a,t);if(!st.hit)return 0;
        st.crit=(u8)(((u32)rand15()*100>>15)<4);
        st.dmg=skill_amount(a,t,ab,0x8000|(st.crit?rand15():0));
        if(b->formula==0x32){
            u16 n=st.dmg*(u16)(((u32)rand15()*cj_range(common,ab)[9]>>15)+1);
            st.dmg=n>999?999:n;
        }
        if(st.crit)knockback(a,t);
        return 0;
    }
#endif
    st.hit = skill_effect_hit(a, t, ab) == 100;
    if (b->flags & AF_EVADE) st.hit = roll_hit(a, t);
    if (!st.hit) return 0;
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
    return 0;
}


static u8 command_skill(u8 a, u8 t, AbId ab)
{
#ifdef FFT_CHAPTER
    if (!action_available(a,t)||!can_use(a, ab)) return 0;
#endif
    const Ability *b = &abilities[ab];
#ifdef FFT_CHAPTER
    if (!st.open_resolving) {
        st.pending[a].state = 0;
        unit_status_action(&st.unit[a],0);
        if (b->flags & AF_CHARGE) {
            st.pending[a].state = 1;
            unit_status_action(&st.unit[a],8);
            st.pending[a].ticks = charge_ticks[ab - 0x196];
            st.pending[a].ability = ab;
            st.pending[a].tile = UNIT_CELL(&st.unit[t]);
            st.pending[a].origin = UNIT_CELL(&st.unit[a]);
            st.pending[a].weapon = hand_weapon(&st.unit[a],0);
            st.pending[a].off_weapon = hand_weapon(&st.unit[a],1);
            // Retail Charge commits movement as well as the action.
            st.moved = st.acted = 1;
            st.mode = M_MENU; st.menu = MN_WAIT;
            return 2;
        }
    }
#endif
    return command_effect(a,t,ab);
}

// Defend (a unit whose support ability it is): Defending until its next turn, its evades doubled
static void command_defend(u8 a)
{
#ifdef FFT_CHAPTER
    if(!action_available(a,a))return;
#endif
    act_start(a, a);
    st.abil = ABIL_DEFEND; st.hit = 1;
}

