// Existing mask storage; sparse cardinal shapes avoid a full board scan.
#ifndef FFT_WEAPON_RANGE_MASK_INC
#define FFT_WEAPON_RANGE_MASK_INC
static void weapon_range_mask(u8 who)
{
    static const s8 dx[4]={1,-1,0,0},dz[4]={0,0,1,-1};
    const Unit *u=&st.unit[who];u8 o=UNIT_CELL(u),weapon=hand_weapon(u,0),flags;
    u8 d,k,layer,layers=W->grid?2:1,steps;
    if(weapon>=128)weapon=0;flags=cj_equipment_item(common,weapon)[2];
    if(!(flags&192)){ranged_reach(255,who,o);return;}
    steps=flags&128?1:2;
    for(d=0;d<4;d++){
        s16 x=u->x,z=u->z;
        for(k=0;k<steps;k++){
            x+=dx[d];z+=dz[d];
            for(layer=0;layer<layers;layer++){
                u16 cell=node_at(x,z,layer);
                if(cell<MAP_N&&ranged_reach(who,o,(u8)cell))st.reach[cell]=1;
            }
        }
    }
}
#endif
