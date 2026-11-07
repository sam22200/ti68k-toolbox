// RESPONSE owns endpoint/formula setup, using resident pure-math callbacks.
// kx/kz in the saved root packet retain the original cast origin across Counters.
static u8 reflect_launch(void)
{
    CoreAction *c=&st.action;Unit *u=&st.unit[c->actor];
    u8 target,face=u->face;CastVictim v;
    if(c->phase!=CTX_SECONDARY||st.primary.running)return 0;
    st.primary.running=1;st.abil=st.primary.abil;
    target=reflect_recipient(c->actor,st.primary.kx,st.primary.kz);
    // Source empty reflection has no attack/Miss animation on the reflector.
    if(target>=NUNIT){reaction_return();return 0;}
    action_pose_start(c->actor,target);u->face=face;
    st.abil=st.primary.abil;st.effect_kind=AB_NULL;st.hit=0;
    if(target<NUNIT&&magic_target(target)){
        magic_prepare(c->root,target,st.abil,&v);
        st.hit=v.hit;st.dmg=v.amount;st.effect_kind=v.effect;
    }
    if(W){W->num_ok=W->hold=W->frz=W->frz_h=0;W->fc_key=0;}
    return 1;
}
