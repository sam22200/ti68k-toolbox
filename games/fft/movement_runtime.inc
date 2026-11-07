#include "movement_rules.h"
#include "movement_access.inc"
#if !defined(FFT_MODULE) || MOD_PART(MOD_MOVEMENT)
static void edges(u8 jump);
static void edges_tables(u8 jump,u16 first,u16 last);
static void edges_rows(u8 jump,u16 first,u16 last);
#if !defined(FFT_MODULES)
#define move_config (W->move_config)
static void movement_begin(u8 who)
{
    const Unit *u=&st.unit[who];u8 f[3],status=core_status_current(&u->statuses,2),weather=st.weather;
    u16 i,area=(u16)MAP_W*MAP_H;const u8 *costs;
    cj_movement(common,u->job,st.movement[who],f);
    core_movement_config(&move_config,f,u->jump,status);
    W->move_kind=((f[0]|f[1]|f[2])||(W->surface_flags&SURFACE_COSTS)||weather>=3)?(move_config.kind<2?(u8)(3-move_config.kind):1):0;
    W->move_who=who+1;W->move_status=status;W->move_weather=weather;W->move_code=st.movement[who];W->move_job=u->job;
    weather=weather==3||weather==6?2:weather==4||weather==7?3:1;
    costs=cj_section(common,CJ_BATTLE)+CJ_MOVEMENT+((u16)move_config.row<<6);
    for(i=0;i<MAP_N;i++){
        const Tile *t=&map_tiles[i];u8 terrain=t->water?17:0,cost=1,walk=t->walk!=0,landing=0;
        if(W->surface&&(W->surface_flags&SURFACE_TERRAIN))terrain=W->surface[(area<<3)+((u16)(W->grid_row[TILE_Z[i]]+TILE_X[i])<<1)+node_layer((u8)i)];
        if(W->move_kind){
            cost=core_movement_cost(costs[terrain],f[1],weather);if(cost==255)walk=0;
            if(t->water>=4&&!move_config.water)walk=0;
            if((t->water==3&&!(move_config.stored&208))||(t->water>=4&&!move_config.water)||(terrain==18&&!move_config.lava))landing=64;
        }
        W->hw[i]=(u8)(walk|(t->water<<1)|((cost==255?3:cost-1)<<4)|landing);
    }
}
static void movement_prepare(u8 who)
{
    movement_begin(who);
    if(W->move_kind<2)edges(W->move_kind?move_config.jump:st.unit[who].jump);
    else W->ej=st.unit[who].jump;
    W->edge_phase=0;
}
#endif
#endif
#if MOD_CORE || MOD_PART(MOD_MOVEMENT)
// Once per immutable map binding. Retain the established ordinary-water
// adapter; dry slow surfaces and severe weather use the source cost table.
static void movement_profile(void)
{
#ifdef FFT_MODULES
    mod_call(MOD_MOVEMENT,9,0);
#else
    u16 i,area=(u16)MAP_W*MAP_H;const u8 *costs=cj_section(common,CJ_BATTLE)+CJ_MOVEMENT;
    W->surface_flags&=(u8)~SURFACE_COSTS;
    if(!W->surface||!(W->surface_flags&SURFACE_TERRAIN))return;
    for(i=0;i<MAP_N;i++)if(map_tiles[i].walk&&!map_tiles[i].water){
        u8 surface=W->surface[(area<<3)+((u16)(W->grid_row[TILE_Z[i]]+TILE_X[i])<<1)+node_layer((u8)i)];
        if(costs[surface]>1&&costs[surface]!=255){W->surface_flags|=SURFACE_COSTS;break;}
    }
#endif
}
#ifndef FFT_MODULES
static u8 movement_plain(u8 who)
{
    const Unit *u=&st.unit[who];const u8 *f;
    if(st.movement[who]||W->move_kind||st.weather>=3||(W->surface_flags&SURFACE_COSTS))return 0;
    f=cj_section(common,CJ_BATTLE)+CJ_MOVEMENT+384+(u16)u->job*3;
    return !(f[0]|f[1]|f[2]);
}
static u8 movement_key(u8 who)
{
    const Unit *u=&st.unit[who];
    return W->move_who==who+1&&W->move_status==core_status_current(&u->statuses,2)&&
        W->move_weather==st.weather&&W->move_code==st.movement[who]&&W->move_job==u->job;
}
static u8 movement_cached(u8 who)
{
    return !W->edge_phase&&W->ej==step_jump_cap(st.unit[who].jump)&&(movement_plain(who)||movement_key(who));
}
#endif
static void movement_ready(u8 who)
{
#ifdef FFT_MODULES
    mod_call(MOD_MOVEMENT,12,who);
#else
    if(movement_cached(who))return;
    if(movement_plain(who))edges(st.unit[who].jump);else movement_prepare(who);
#endif
}
static u8 movement_poll(u8 who)
{
#ifdef FFT_MODULES
    return (u8)mod_call(MOD_MOVEMENT,13,who);
#else
    const Unit *u=&st.unit[who];u16 end;
    if(W->edge_phase&&!movement_key(who))W->edge_phase=0;
    if(!W->edge_phase){
        if(movement_cached(who))return 1;
        if(!movement_plain(who))movement_begin(who);
        else{
            W->move_who=who+1;W->move_status=core_status_current(&u->statuses,2);
            W->move_weather=st.weather;W->move_code=st.movement[who];W->move_job=u->job;
        }
        if(W->move_kind>=2){W->ej=u->jump;return 1;}
        W->edge_jump=W->move_kind?move_config.jump:u->jump;W->edge_at=0;W->edge_phase=1;
    }
    end=W->edge_at+4;if(end>MAP_N)end=MAP_N;
    if(W->edge_phase==1)edges_tables(W->edge_jump,W->edge_at,end);
    else edges_rows(W->edge_jump,W->edge_at,end);
    W->edge_at=end;
    if(end==MAP_N){
        W->edge_at=0;
        if(W->edge_phase==1)W->edge_phase=2;
        else{W->edge_phase=0;W->ej=W->move_kind?u->jump:step_jump_cap(u->jump);return 0;}
    }
    return 0;
#endif
}
#endif
#if MOD_CORE
// A direct destination map reuses existing dist/from storage and the search
// budget API. Native Fly/Teleport traverse columns without intermediate actors;
// ordinary landing occupancy/tree rules are still filtered by their callers.
static void movement_direct(u8 who,u8 origin,u8 limit,u8 *dist,u8 *from,u8 out)
{
    u16 i;u8 fly=W->move_kind==3;u8 layer=node_layer(origin);(void)who;(void)out;
    for(i=0;i<MAP_N;i++){
        s16 dx=(s16)TILE_X[i]-TILE_X[origin],dz=(s16)TILE_Z[i]-TILE_Z[origin];u8 d=(u8)((dx<0?-dx:dx)+(dz<0?-dz:dz));
        dist[i]=(node_layer((u8)i)==layer&&(W->hw[i]&1)&&(!fly||d<=limit))?d:255;
        if(from)from[i]=origin;
    }
    dist[origin]=0;if(from)from[origin]=origin;
    W->nav.done=1;W->nav.fault=0;
}
#endif
