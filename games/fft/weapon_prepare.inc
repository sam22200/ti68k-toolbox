// A rejected follow-up/response has no new animation; the root packet survives.
static void weapon_obstructed(void)
{
    st.hit=st.push=0;st.dmg=st.self=0;st.effect_kind=AB_NULL;
    if(st.action.phase==CTX_SECONDARY){st.action.left=0;reaction_return();}
    else if(st.hand){st.action.phase=CTX_IMPACT;st.act_t=ACT_END-1;}
}
// One geometry decision selects a panel; all targetable occupants belong to it.
static u16 weapon_panel_mask(u8 cell)
{
    u8 cells[UNIT_CAPACITY],i;u16 eligible=0;
    for(i=0;i<NUNIT;i++){
        const Unit *u=&st.unit[i];cells[i]=UNIT_CELL(u);
        if(unit_on_field(u)&&!unit_airborne(u))eligible|=(u16)1<<i;
    }
    return core_weapon_panel(cells,eligible,cell,NUNIT);
}
static u8 weapon_recipient(u8 a,u8 t)
{
    u8 flags=cj_weapon(common,hand_weapon(&st.unit[a],0))[1];
    if(flags&192)return melee_target(a,t);
    if(flags&16)return arc_target(a,t);
    return direct_target(a,t);
}
static void weapon_prepare(u8 a,u8 t)
{
    WeaponPanel *p=&W->weapon_panel;
    if(!p->resume){
        p->todo=0;
        if(weapon_order(st.abil)){
            u8 found=weapon_recipient(a,t);
            if(found==255){weapon_obstructed();return;}
            st.tgt=t=found;p->todo=weapon_panel_mask(UNIT_CELL(&st.unit[t]));
        }
    }
#ifdef FFT_MODULE
    W->nature_pending|=32;
#else
    weapon_freeze();
#endif
}
// Pure geometry/count output for NUMBERS and its recipient card. No action,
// payment, equipment, CT, live position or RNG mutation.
static void weapon_preview(u8 a,u8 t,AbId ab)
{
    u16 mask;u8 i;
    W->preview_target=t;W->preview_count=1;
    if(!weapon_order(ab))return;
    t=weapon_recipient(a,t);W->preview_target=t;
    W->preview_count=0;if(t==255)return;
    mask=weapon_panel_mask(UNIT_CELL(&st.unit[t]));
    for(i=0;i<NUNIT;i++)if(mask&((u16)1<<i)){
        if(!W->preview_count)W->preview_target=i;
        W->preview_count++;
    }
}
