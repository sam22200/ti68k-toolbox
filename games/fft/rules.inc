// The chapters' rules beyond Gariland's first battle (campaign/ENGINE.md M3), each in effect
// through the chapter's data: a weapon's reach and its projectile (WEAPON_REACH), Charge
// waiting on the CT clock (AF_CHARGE, charge_ticks), weapon skills, drain and a cross splash
// (AF_WEAPON, AF_DRAIN, AF_AREA), the units' fixed records (UNIT_RAW).

// A due deferred actor (255: none). Origin/weapon changes cancel only Charge.
#ifdef FFT_MODULES
static u8 pending_next(void){return (u8)mod_call(MOD_ORDER,18,0);}
#else
#include "pending_next.inc"
#endif

// Return one while an action owns presentation; zero resumes the same clock
// phase. Each rejected due slot is consumed once, with no recursive dispatch.
static u8 charge_resolve(void)
{
    u8 a,result;
    if(st.action.phase!=CTX_IDLE)return 1;
    a=pending_next();
    if(a>=NUNIT)return 0;
#ifdef FFT_MODULES
    result=(u8)mod_call(st.pending[a].state==DEFER_PERFORMING?MOD_PERFORMANCE:MOD_COMMAND,4,a);
#else
    result=st.pending[a].state==DEFER_PERFORMING?performance_pending(a):command_pending(a);
#endif
    return cast_dispatch(result);
}

// the units' records as the chapter gives them (stats, signs, equipment: no draw)


#include "weapon_range.h"
// Origin may be a hypothetical AI tile. Source Float/water traits affect only
// the origin; destination panel heights include the water surface.
static u8 weapon_origin_height(u8 who,u8 cell)
{
    const Unit *u=&st.unit[who];const u8 *p;u8 h,depth,f[3];
    if(!W->surface)return (u8)(map_tiles[cell].stand+((core_status_current(&u->statuses,2)&64)?2:0));
    p=W->surface+((u16)(W->grid_row[TILE_Z[cell]]+TILE_X[cell])<<3)+4*node_layer(cell);
    h=(u8)((p[0]<<1)+(p[1]&31));depth=p[1]>>5;
    cj_movement(common,u->job,st.movement[who],f);
    if((f[2]&8)||(core_status_current(&u->statuses,2)&64))h+=(depth<<1)+2;
    else if(depth){if(f[2]&128)h+=depth<<1;else if(f[2]&64)h+=(depth<<1)-2;}
    return h;
}
static u16 weapon_range_spec(u8 who,u8 origin)
{
    u8 weapon=hand_weapon(&st.unit[who],0),flags;
    if(weapon>=128)weapon=0;
    flags=cj_equipment_item(common,weapon)[2];
    return (u16)((flags&240)|items[weapon].reach)<<8|weapon_origin_height(who,origin);
}
static u8 __attribute__((noinline)) weapon_range_cell(u8 o,u8 c,u16 spec)
{
    u8 flags=(u8)(spec>>8),d;u16 target;const u8 *p;
    s16 dx=(s16)TILE_X[c]-TILE_X[o],dz=(s16)TILE_Z[c]-TILE_Z[o];
    if(!map_tiles[c].walk)return 0;
    d=(dx<0?-dx:dx)+(dz<0?-dz:dz);
    if(flags&WEAPON_STRIKE){if(d!=1)return 0;}
    else if(flags&WEAPON_LUNGE){if(!d||d>2||(dx&&dz))return 0;}
    else if(flags&WEAPON_DIRECT)return d>=3&&d<=(flags&15);
    else if(d<3)return 0;
    target=map_tiles[c].stand;
    if(W->surface){
        p=W->surface+((u16)(W->grid_row[TILE_Z[c]]+TILE_X[c])<<3)+4*node_layer(c);
        target=(u16)((p[0]<<1)+(p[1]&31)+((p[1]>>5)<<1));
    }
    return core_weapon_reaches(flags,flags&15,dx,dz,(u8)spec,target);
}
static u8 ranged_reach(u8 a,u8 o,u8 c)
{
    u16 spec,i;
    // Reserved owner255 fills the existing manual mask, preparing the origin
    // once. Ordinary actor IDs remain pure point queries for AI and reactions.
    if(a==255){
        if(o>=NUNIT||c>=MAP_N)return 0;
        spec=weapon_range_spec(o,c);
        for(i=0;i<MAP_N;i++)if(weapon_range_cell(c,(u8)i,spec))st.reach[i]=1;
        return 1;
    }
    if(a>=NUNIT||o>=MAP_N||c>=MAP_N)return 0;
    spec=weapon_range_spec(a,o);return weapon_range_cell(o,c,spec);
}

// a projectile in flight, a weapon skill's strike
static void weapon_effect(void)
{
    u8 t=st.act_t;
    if(st.mode!=M_ACT)return;
    if(st.abil==ABIL_DEFEND)return;
    if((st.abil==NO_ABIL||(abilities[st.abil].flags&AF_CHARGE))&&WEAPON_REACH(&st.unit[st.atk])>1){
        if(t>=ACT_HIT-8&&t<ACT_HIT){
            Feet a=unit_feet(st.rot,st.atk),b=unit_feet(st.rot,st.tgt);u8 k=t-(ACT_HIT-8);
            s16 x=a.x+((b.x-a.x)*k>>3)-st.camx,y=a.y+((b.y-a.y)*k>>3)-st.camy-12;
            static const u8 arc[8]={0,3,5,6,6,5,3,0};y-=arc[k];
            draw_rect(x-2,y-1,6,3,C_WHITE);draw_rect(x-1,y,4,1,C_BLACK);
        }
    }else if(st.abil!=NO_ABIL&&st.abil!=ABIL_DEFEND&&(abilities[st.abil].flags&AF_WEAPON)&&t>=ACT_HIT-5&&t<ACT_HIT+6){
        Feet b=unit_feet(st.rot,st.tgt);s16 x=b.x-st.camx,y=b.y-st.camy;
        u8 h=(u8)(6+(t%5)*4);draw_rect(x-3,y-h,6,h,C_WHITE);draw_rect(x-1,y-h,2,h,C_DGRAY);
    }
}
