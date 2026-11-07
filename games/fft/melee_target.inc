// Source-height adapter. Geometry is pure; the preparing owner may redirect its packet.
#ifndef FFT_MELEE_TARGET_INC
#define FFT_MELEE_TARGET_INC
#include "melee_rules.h"
#include "target_height.inc"
static u8 melee_target(u8 a,u8 t)
{
    MeleeSpan actor=melee_span(a),target=melee_span(t),middle[2];u8 floors[2],weapon,flags,d,layer;
    s16 dx,dz,x,z;u16 cell;const Unit *u,*v;const u8 *p;
    floors[0]=floors[1]=255;
    if(a>=NUNIT||t>=NUNIT||actor.id==255)return 255;u=&st.unit[a];v=&st.unit[t];
    weapon=hand_weapon(u,0);if(weapon>=128)weapon=0;flags=cj_weapon(common,weapon)[1];
    dx=(s16)v->x-u->x;dz=(s16)v->z-u->z;d=(dx<0?-dx:dx)+(dz<0?-dz:dz);
    if(flags&128)return core_melee_overlap(&actor,&target,0)?target.id:255;
    if(!(flags&64)||!d||d>2||(dx&&dz))return 255;
    if(d==1)return core_melee_lunge(&actor,&target,1,floors,middle);
    x=(u->x+v->x)>>1;z=(u->z+v->z)>>1;
    for(layer=0;layer<2;layer++){
        middle[layer].id=255;
        cell=node_at(x,z,layer);
        if(W->surface){p=W->surface+((u16)(W->grid_row[z]+x)<<3)+(layer<<2);if(!(p[3]&128))floors[layer]=(u8)((p[0]<<1)+(p[1]&31));}
        else if(cell<MAP_N)floors[layer]=map_tiles[cell].stand;
        if(cell<MAP_N){u8 found=unit_at_cell((u8)cell);if(found){MeleeSpan v=melee_span(found-1);middle[layer].feet=v.feet;middle[layer].top=v.top;middle[layer].id=v.id;}}
    }
    return core_melee_lunge(&actor,&target,2,floors,middle);
}
static u8 melee_cache(void)
{
    u8 a=st.act,t,home=UNIT_CELL(&st.unit[a]),work=4,scans=64;u16 c;
    {u8 w=hand_weapon(&st.unit[a],0);if(w>=128)w=0;
     if(cj_weapon(common,w)[1]&48)return projectile_cache(cj_weapon(common,w)[1]);
     if(!(cj_weapon(common,w)[1]&192)){memset(W->ai_melee_mask,255,sizeof W->ai_melee_mask);W->ai_weapon_left&=(u16)~512;return W->ai_weapon_left==0;}}
    while(W->ai_melee_at<MAP_N&&work&&scans--){
        c=W->ai_melee_at++;W->ai_melee_mask[c]=0;memset(W->ai_reflect[c],0,7);
        if(!W->ai_m[c]&&c!=home)continue;
        unit_place(&st.unit[a],(u8)c);
        for(t=0;t<NUNIT;t++)if(t!=a&&st.unit[t].hp&&unit_on_field(&st.unit[t])&&
           ranged_reach(a,(u8)c,UNIT_CELL(&st.unit[t]))){
            weapon_reflect_pair(a,t,(u8)c);
            if(melee_target(a,t)==t)W->ai_melee_mask[c]|=(u16)1<<t;
        }
        work--;
    }
    unit_place(&st.unit[a],home);
    if(W->ai_melee_at==MAP_N)W->ai_weapon_left&=(u16)~512;
    return W->ai_weapon_left==0;
}
#endif
