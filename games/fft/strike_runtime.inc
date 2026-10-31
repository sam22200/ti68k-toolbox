// A fresh target query for the stored tile; never follow recoil automatically.
// Only ordinary living-target Attack is supported by this bounded entry.
static u8 strike_target(void)
{
    CoreAction *c=&st.action;u8 i;
    if((c->phase!=CTX_IMPACT&&c->phase!=CTX_APPLIED)||!c->left)return 0;
    c->left--;
    if(!st.unit[c->actor].hp||!unit_can_act(&st.unit[c->actor])){c->left=0;return 0;}
    for(i=0;i<NUNIT;i++)if(st.unit[i].hp&&st.unit[i].x<MAP_W&&st.unit[i].z<MAP_H&&
        UNIT_CELL(&st.unit[i])==c->cell&&ranged_reach(c->actor,UNIT_CELL(&st.unit[c->actor]),c->cell))return i+1;
    return 0;
}
static void strike_launch(u8 target)
{
    CoreAction *c=&st.action;AbId ability=st.abil;
    if(c->phase!=CTX_IMPACT&&c->phase!=CTX_APPLIED)return;
    c->phase=c->phase==CTX_IMPACT?CTX_ROOT:CTX_SECONDARY;
    action_pose_start(c->actor,target);st.hand=1;st.abil=ability;
    if(ability!=NO_ABIL&&(abilities[ability].flags&AF_CHARGE))charge_roll(c->actor,target,ability);
    else attack_roll(c->actor,target);
    if(W){W->hold=W->frz=W->frz_h=0;W->fc_key=0;}
}
