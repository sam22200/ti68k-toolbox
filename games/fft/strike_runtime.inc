// A fresh target query for the stored tile; never follow recoil automatically.
// Only ordinary living-target Attack is supported by this bounded entry.
#if MOD_CORE
#include "strike_target.inc"
#endif
static void strike_launch(u8 target)
{
    CoreAction *c=&st.action;AbId ability=st.abil;u8 hand=W->weapon_panel.resume?W->weapon_panel.hand:1;
    if(c->phase!=CTX_IMPACT&&c->phase!=CTX_APPLIED)return;
    c->phase=c->phase==CTX_IMPACT?CTX_ROOT:CTX_SECONDARY;
    action_pose_start(c->actor,target);st.hand=hand;st.abil=ability;
    if(ability!=NO_ABIL&&(abilities[ability].flags&AF_CHARGE))charge_roll(c->actor,target,ability);
    else attack_roll(c->actor,target);
    if(W){W->hold=W->frz=W->frz_h=0;W->fc_key=0;}
}
