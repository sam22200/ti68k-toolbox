// Ordinary magical HP radial footprints. No damage, payment, RNG or image calls.
// Mask bits are stable native actor IDs; source strike ordering is a separate contract.
// Same source footprint for victims and the existing manual panel renderer.
static u8 area_cell(u8 a,u16 centre,const u8 *r,u8 cell)
{
    const Unit *actor=&st.unit[a];u8 linear=r[4]&4,x,z,rh;const u8 *col;
    s16 dx,dz;
    x=linear?actor->x:TILE_X[centre];z=linear?actor->z:TILE_Z[centre];
    if((r[4]&1)&&cell==UNIT_CELL(actor))return 0;
    dx=(s16)TILE_X[cell]-x;dz=(s16)TILE_Z[cell]-z;
    if(linear&&!((dx==0&&dz==0&&(TILE_X[centre]!=x||TILE_Z[centre]!=z))||
                 (dz==0&&((dx>0&&TILE_X[centre]>x)||(dx<0&&TILE_X[centre]<x)))||
                 (dx==0&&((dz>0&&TILE_Z[centre]>z)||(dz<0&&TILE_Z[centre]<z)))))return 0;
    if(r[1]==255)return 1;
    if((dx<0?-dx:dx)+(dz<0?-dz:dz)>r[1])return 0;
    col=W->surface+((u16)(W->grid_row[z]+x)<<3)+4*node_layer(linear?UNIT_CELL(actor):(u8)centre);
    if(col[3]&128)return 0;
    rh=(u8)core_area_height(col);
    col=W->surface+((u16)(W->grid_row[TILE_Z[cell]]+TILE_X[cell])<<3);
    return core_area_layer(col,rh,r[2],r[4]&32,W->surface_flags&SURFACE_MOVE_UPPER)==node_layer(cell)+1;
}
static u16 area_resolution_cell(u8 follow,u8 ref)
{
    if(follow){
        const Unit *u;if(ref>=NUNIT)return MAP_N;u=&st.unit[ref];
        if(u->x>=MAP_W||u->z>=MAP_H)return MAP_N;
        return UNIT_CELL(u)<MAP_N?UNIT_CELL(u):MAP_N;
    }
    return ref<MAP_N?ref:MAP_N;
}
// Direct-targeting casts (flags_4 0x20) share the weapons' direct trajectory. TARGETS reads
// the recipient the resident fetched from the weapon planner before this image ran (no
// second copy in the largest images; an unprepared query resolves to nobody); the area AI
// scans many origins inside its image and holds a copy; a one-image build calls the planner's.
#if defined(FFT_MODULE) && MOD_PART(MOD_TARGETS)
#define cast_direct(a,t) (W->direct_from==(a)&&W->direct_to==(t)?W->direct_hit:255)
#elif defined(FFT_MODULE)
#define FFT_DIRECT_NAME cast_direct
#define FFT_DIRECT_PANEL
#include "direct_trajectory.inc"
#undef FFT_DIRECT_PANEL
#undef FFT_DIRECT_NAME
#else
static u8 direct_target(u8 a,u8 t);
#define cast_direct direct_target
#endif
s16 cast_targets(u8 a,u8 ref,AbId ab,u8 follow)
{
    const u8 *r,*col;u16 centre,mask=0,bit;u8 i,cx,cz,flags,cell;
    const Unit *u;
    if(a>=NUNIT||(follow>1&&follow!=MATH_BIND)||!W||!W->surface)return TARGETS_INVALID;
    u=&st.unit[a];
    if(u->x>=MAP_W||u->z>=MAP_H||UNIT_CELL(u)>=MAP_N||u->dc==DC_GONE)return TARGETS_INVALID;
    r=cj_range(common,ab);if(!r)return TARGETS_INVALID;
    if(follow==MATH_BIND){
#if !MOD_CORE && (MOD_PART(MOD_AREA_AI) || MOD_PART(MOD_AI_PREP))
        return ref<16&&st.unit[a].set==21&&math_spell(ab)?W->area_ai.math_mask[ref]:TARGETS_INVALID;
#else
        return ref<16&&st.unit[a].set==21&&math_spell(ab)?math_targets(a,ref):TARGETS_INVALID;
#endif
    }
    // Unsupported shape/family never silently becomes a radial spell.
    if((!magic_formula(batch_formula(r[8]))&&r[8]!=0x31)||(r[3]&32)||(r[4]&10)||((r[4]&4)&&r[8]!=0x31&&r[8]!=0x20&&r[8]!=0x2D&&!(r[6]&32)))return TARGETS_UNSUPPORTED;
    centre=area_resolution_cell(follow,ref);
    if(centre>=MAP_N)return TARGETS_INVALID;
    cx=TILE_X[centre];cz=TILE_Z[centre];flags=r[4];
    col=W->surface+((u16)(W->grid_row[cz]+cx)<<3)+4*node_layer((u8)centre);
    if(col[3]&128)return TARGETS_INVALID;
    if(r[6]&32){
        // Source direct targeting: the weapon resolver with a direct trajectory. One panel
        // (a line or a radius in the record is not used): the aimed body, or the first
        // body on the straight line; terrain in the way leaves nobody.
        u8 t=NUNIT;
        for(i=0;i<NUNIT;i++)if(i!=a&&magic_target(i)&&UNIT_CELL(&st.unit[i])==centre){t=i;break;}
        if(t==NUNIT||(t=cast_direct(a,t))==255)return 0;
        cell=UNIT_CELL(&st.unit[t]);
        for(i=0,bit=1;i<NUNIT;i++,bit<<=1)if(i!=a&&magic_target(i)&&UNIT_CELL(&st.unit[i])==cell)mask|=bit;
        return (s16)mask;
    }
    for(i=0,bit=1;i<NUNIT;i++,bit<<=1){
        if(!magic_target(i))continue;
        u=&st.unit[i];cell=UNIT_CELL(u);
        if(!area_cell(a,centre,r,cell))continue;
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
