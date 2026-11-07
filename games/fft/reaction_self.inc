// EQUIPMENT constructs typed self effects using the common status filter.
// The resident enters this image after RESPONSE selection has returned.
static void reaction_self_launch(u8 kind)
{
    CoreAction *c=&st.action;u8 who=c->actor;Unit *u=&st.unit[who];
    if(c->phase!=CTX_SECONDARY||st.primary.running||kind!=6)return;
    st.primary.running=1;action_pose_start(who,who);
    if(W){W->hold=W->frz=W->frz_h=0;W->fc_key=0;}
    st.abil=c->reaction[who];st.hit=!(core_status_current(&u->statuses,3)&1);
    if(st.abil>=REACTION_BRAVE_UP){
        st.effect_kind=st.abil==REACTION_BRAVE_UP?AB_BRAVE:AB_FAITH;st.dmg=253;
    }else{
        CoreStatusDelta d;u8 byte,mask;
        byte=st.abil==REACTION_CAUTION?0:st.abil==REACTION_REGENERATOR?3:2;
        mask=byte==0?2:byte==3?64:st.abil==REACTION_SUNKEN_STATE?16:32;
        memset(&d,0,sizeof d);d.add[byte]=mask;
        core_status_filter(&u->statuses,&d,u->status_flags,0,cj_status_rules(common));
        st.effect_kind=AB_STATUS;st.dmg=d.add[byte]?((u16)byte<<8)|d.add[byte]:0;
    }
    if(!st.hit)st.dmg=0;
}
