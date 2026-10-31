// Cold start/reset and facing. ACTION owns this in the native chapter build.
static void action_pose_start(u8 a,u8 t)
{
    Unit *ua=&st.unit[a],*ut=&st.unit[t];
    s16 dx=(s16)ut->x-ua->x,dz=(s16)ut->z-ua->z;
    st.atk=a;st.tgt=t;st.act_t=0;st.mode=M_ACT;
    st.crit=st.push=0;st.dmg=st.self=0;st.gain=st.lvup=st.jpg=0;
#ifdef FFT_CHAPTER
    st.hand=0;
    st.effect_kind=AB_DAMAGE;
#endif
    // Issued actions can change the resources used by a cached forecast.
    if(W){W->num_ok=0;W->fc_key=0;}
    if(dx||dz)
        ua->face=(dx<0?-dx:dx)>(dz<0?-dz:dz)?(dx>0?0:1):(dz>0?2:3);
}
static void act_start(u8 a,u8 t)
{
#ifdef FFT_CHAPTER
    if(a>=NUNIT||t>=NUNIT||core_action_begin(&st.action,a,NUNIT)!=CTX_OK)return;
    st.primary.running=st.primary.reward=0;
#endif
    action_pose_start(a,t);
}
