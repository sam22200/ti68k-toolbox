// RESPONSE owns endpoint/formula setup, using resident pure-math callbacks.
// kx/kz in the saved root packet retain the original cast origin across Counters.
static u8 reflect_launch(void)
{
    CoreAction *c=&st.action;Unit *u=&st.unit[c->actor];
    s16 x,z;u16 cell;u8 layer,target,face=u->face;const u8 *p;CastVictim v;
    if(c->phase!=CTX_SECONDARY||st.primary.running)return 0;
    st.primary.running=1;st.abil=st.primary.abil;
    x=((s16)u->x<<1)-st.primary.kx;z=((s16)u->z<<1)-st.primary.kz;
    target=255;
    if((u->x!=st.primary.kx||u->z!=st.primary.kz)&&x>=0&&z>=0&&x<MAP_W&&z<MAP_H&&W->surface){
        p=W->surface+((u16)(W->grid_row[u->z]+u->x)<<3)+4*node_layer(UNIT_CELL(u));
        layer=core_reflect_layer(W->surface+((u16)(W->grid_row[z]+x)<<3),p[0],W->surface_flags&SURFACE_MOVE_UPPER);
        if(layer){cell=node_at(x,z,layer-1);if(cell<MAP_N){target=unit_at_cell((u8)cell);target=target?target-1:255;}}
    }
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
