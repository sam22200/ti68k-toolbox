// COMMAND owns cold cast preparation and due dispatch. No nested image loads.
static u8 command_cast_effect(u8 a,u8 ref,AbId ab,u8 follow)
{
    if(follow==MATH_BIND||(abilities[ab].flags&AF_AREA)){
        CastBatch *b=&W->cast;
        b->actor=a;b->ref=ref;b->ability=ab;b->follow=follow;b->phase=CAST_BATCH_EMPTY;
        return CAST_AREA_DISPATCH; // COMMAND must return before TARGETS is loaded.
    }
    u16 cell=cast_target_cell(follow,ref);u8 t=cell<MAP_N?unit_at_cell((u8)cell):0;
    // Pay at resolution even when the selected centre has no eligible victim.
    // Empty-centre visual effects await the spell presentation/area milestone.
    if(!t||(magic_formula(abilities[ab].formula)?magic_target(t-1)==0:
            (!st.unit[t-1].hp&&abilities[ab].kind!=AB_REVIVE))){
        st.unit[a].mp-=skill_cost(a,ab);return 0;
    }
    cursor_place((u8)cell);return command_effect(a,t-1,ab);
}
static u8 command_cast(u8 a,u8 ref,AbId ab,u8 follow)
{
    const u8 *r=cj_range(common,ab);u8 ticks,result;
    if(!cast_can_order(a,ref,ab,follow))return 0;
    ticks=follow==MATH_BIND?0:core_cast_ticks(r[12],r[5]&4,st.support[a]==SUPPORT_SHORT_CHARGE,
                          st.support[a]==SUPPORT_NON_CHARGE);
    st.pending[a].state=DEFER_NONE;unit_status_action(&st.unit[a],0);
    st.acted=1;
    if(ticks){
        st.pending[a].ability=ab;st.pending[a].tile=ref;
        st.pending[a].state=follow?DEFER_CAST_UNIT:DEFER_CAST_TILE;
        st.pending[a].ticks=ticks;
        unit_status_action(&st.unit[a],8);
        st.mode=M_MENU;st.menu=st.moved?MN_WAIT:MN_MOVE;return 2;
    }
    result=command_cast_effect(a,ref,ab,follow);
    if(result==CAST_AREA_DISPATCH||result==CAST_EFFECT_DISPATCH)return result;
    if(!result){
        st.mode=M_MENU;st.menu=st.moved?MN_WAIT:MN_MOVE;
    }
    return 1;
}
static u8 command_pending(u8 a)
{
    u8 kind=st.pending[a].state,ref=st.pending[a].tile,t,result=1;AbId ab=st.pending[a].ability;
    st.pending[a].state=DEFER_NONE;unit_status_action(&st.unit[a],0);
    if(!action_available(a,a)||!can_use(a,ab))return 0;
    if(kind==DEFER_CHARGING||kind==DEFER_AIRBORNE){
        t=ref<MAP_N?unit_at_cell(ref):0;if(!t||!st.unit[t-1].hp)return 0;
    }else{
        // Cost eligibility is rechecked before looking for a current victim.
        t=0;
        if(kind!=DEFER_CAST_TILE&&kind!=DEFER_CAST_UNIT)return 0;
    }
    st.act=a;st.ai=AI_OFF;
    if(kind!=DEFER_CHARGING&&kind!=DEFER_AIRBORNE){
        result=command_cast_effect(a,ref,ab,kind==DEFER_CAST_UNIT);if(!result)return 0;
    }else{
        cursor_place(ref);result=command_effect(a,t-1,ab);
    }
    st.open_resolving=1;return result;
}
