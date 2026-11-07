// Persistent Songs/Dances reuse each actor's existing deferred slot.
static u8 performance_order(u8 a,u8 ref,AbId ab,u8 follow)
{
    // Validated performances are global, centred only on the caster's own tile.
    if(a>=NUNIT||follow>1||ab>=512||!performance_formula(abilities[ab].formula)||
       !action_available(a,a)||!can_use(a,ab)||!W||!W->surface||
       cast_target_cell(follow,ref)!=UNIT_CELL(&st.unit[a])||!map_tiles[UNIT_CELL(&st.unit[a])].walk)return 0;
    st.pending[a].ability=ab;st.pending[a].state=DEFER_PERFORMING;
    st.pending[a].ticks=cj_range(common,ab)[12]&127;
    unit_status_action(&st.unit[a],1);st.acted=1;
    st.mode=M_MENU;st.menu=st.moved?MN_WAIT:MN_MOVE;return 2;
}
static u8 performance_pending(u8 a)
{
    AbId ab=st.pending[a].ability;CastBatch *b=&W->cast;
    st.pending[a].ticks=cj_range(common,ab)[12]&127; // repeat even if this pulse is restricted
    if(!action_available(a,a)||!can_use(a,ab))return 0;
    st.act=a;st.ai=AI_OFF;
    b->actor=a;b->ability=ab;b->ref=UNIT_CELL(&st.unit[a]);b->follow=0;b->phase=CAST_BATCH_EMPTY;
    st.open_resolving=1;return CAST_AREA_DISPATCH; // return before loading TARGETS
}
