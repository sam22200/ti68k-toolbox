// Ordinary magical HP radial footprints. No damage, payment, RNG or image calls.
// Mask bits are stable native actor IDs; source strike ordering is a separate contract.
static u16 area_resolution_cell(u8 follow,u8 ref)
{
    if(follow){
        const Unit *u;if(ref>=NUNIT)return MAP_N;u=&st.unit[ref];
        if(u->x>=MAP_W||u->z>=MAP_H)return MAP_N;
        return UNIT_CELL(u)<MAP_N?UNIT_CELL(u):MAP_N;
    }
    return ref<MAP_N?ref:MAP_N;
}
s16 cast_targets(u8 a,u8 ref,AbId ab,u8 follow)
{
    const u8 *r,*col;u16 centre,mask=0,bit;u8 i,cx,cz,rh,aoe,flags,cell;
    const Unit *u;
    if(a>=NUNIT||follow>1||!W||!W->surface)return TARGETS_INVALID;
    u=&st.unit[a];
    if(u->x>=MAP_W||u->z>=MAP_H||UNIT_CELL(u)>=MAP_N||u->dc==DC_GONE)return TARGETS_INVALID;
    r=cj_range(common,ab);if(!r)return TARGETS_INVALID;
    // Unsupported shape/family never silently becomes a radial spell.
    if((!magic_formula(r[8])&&r[8]!=0x31)||(r[3]&32)||(r[4]&14)||(r[6]&32))return TARGETS_UNSUPPORTED;
    centre=area_resolution_cell(follow,ref);
    if(centre>=MAP_N)return TARGETS_INVALID;
    cx=TILE_X[centre];cz=TILE_Z[centre];aoe=r[1];flags=r[4];
    col=W->surface+((u16)(W->grid_row[cz]+cx)<<3)+4*node_layer((u8)centre);
    if(col[3]&128)return TARGETS_INVALID;
    rh=(u8)core_area_height(col);
    for(i=0,bit=1;i<NUNIT;i++,bit<<=1){
        s16 dx,dz;u8 layer;
        if(!magic_target(i))continue; // KO/Petrify/Wall remain present, not effect-eligible.
        u=&st.unit[i];cell=UNIT_CELL(u);layer=node_layer(cell);
        if((flags&1)&&cell==UNIT_CELL(&st.unit[a]))continue;
        if(aoe!=255){
            dx=(s16)u->x-cx;if(dx<0)dx=-dx;dz=(s16)u->z-cz;if(dz<0)dz=-dz;
            if(dx+dz>aoe)continue;
            col=W->surface+((u16)(W->grid_row[u->z]+u->x)<<3);
            if(core_area_layer(col,rh,r[2],flags&32,W->surface_flags&SURFACE_MOVE_UPPER)!=layer+1)continue;
        }
        mask|=bit;
    }
    if(flags&192){
        u16 todo=mask,result=0;
        // Source team filters mutate occupied panels, not individual actors.
        // Overlapping KO/live occupants therefore share a result, in slot order.
        for(i=0,bit=1;i<NUNIT;i++,bit<<=1)if(todo&bit){
            u16 group=0,b;u8 j,available=1,marked=0;
            cell=UNIT_CELL(&st.unit[i]);
            for(j=i,b=bit;j<NUNIT;j++,b<<=1)if((mask&b)&&UNIT_CELL(&st.unit[j])==cell)group|=b;
            todo&=(u16)~group;
            for(j=i,b=bit;j<NUNIT;j++,b<<=1)if((group&b)&&available){
                u8 same=j==a||((st.unit[a].team==TEAM_ENEMY)==(st.unit[j].team==TEAM_ENEMY));
                if(flags&(same?128:64))marked=1;else available=0;
            }
            // A later rejected occupant clears range but does not clear an earlier mark.
            if(marked)result|=group;
        }
        mask=result;
    }
    return (s16)mask;
}
