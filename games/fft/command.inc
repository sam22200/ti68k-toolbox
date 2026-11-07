#if defined(FFT_MODULE) && MOD_PART(MOD_COMMAND)
static u8 command_effect(u8 a,u8 t,AbId ab)
{
    st.formula.actor=a;st.formula.target=t;st.formula.ability=ab;
    return CAST_EFFECT_DISPATCH;
}
#else
#include "command_effect.inc"
#endif


static u8 command_skill(u8 a, u8 t, AbId ab)
{
#ifdef FFT_CHAPTER
    if (!action_available(a,t)||!can_use(a, ab)) return 4; // rejected before mutating the resident request
#endif
    const Ability *b = &abilities[ab];
#ifdef FFT_CHAPTER
    if(b->formula==0x25&&(st.dmg=equip_break_slot(&st.unit[t],ab))==255)return 3;   // the default Attack
    if(!st.open_resolving&&jump_ability(ab))return 5;
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

