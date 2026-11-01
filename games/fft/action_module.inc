#include "action_resolve.inc"
u16 mod_entry(u8 op,u8 a)
{
    if(op==0)return action_commit();
    if(op==1){act_start(st.formula.actor,st.formula.target);return 0;}
    if(op==7){rewards(st.formula.actor,st.formula.target,(u8)st.formula.roll);return 0;}
    if(op==9)return skill_amount(st.formula.actor,st.formula.target,st.formula.ability,st.formula.roll);
    return action_reaction_blocked(st.formula.target);
}
