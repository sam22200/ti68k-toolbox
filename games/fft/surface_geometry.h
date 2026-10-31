#ifndef FFT_SURFACE_GEOMETRY_H
#define FFT_SURFACE_GEOMETRY_H
#include "core_limits.h"
// FGS1: magic,width,height,u16 area, then8 bytes per SOURCE column.
// Each layer: base height, depth/half height, slope, ceiling depth | blocked<<7.
// Collision geometry is independent of compact walkable nodes (blocked upper
// records can still collide with a projectile in the retail routine).
#define SURFACE_HEADER 8U
#define SURFACE_ROOM 1U
#define SURFACE_MOVE_UPPER 2U
#define SURFACE_HIT_UPPER 4U
#define SURFACE_GRID_UPPER 8U
static u8 surface_valid(const u8 *b,u16 bytes,u8 width,u8 height)
{
    u16 area,n,i;
    if(!b||bytes<8||!width||!height||width>FFT_CORE_AXIS_MAX||height>FFT_CORE_AXIS_MAX)return 0;
    area=(u16)width*height;n=(u16)(8+(area<<3));
    if(area>256||b[0]!='F'||b[1]!='G'||b[2]!='S'||b[3]!='1'||b[4]!=width||b[5]!=height||
       ((u16)b[6]<<8|b[7])!=area||(bytes!=n&&bytes!=n+6))return 0;
    for(i=11;i<n;i+=4)if(b[i]&0x60)return 0;
    return 1;
}
static u8 surface_edge(const u8 *layer,u8 shift)
{
    u8 half=layer[1]&31,side=(layer[2]>>shift)&3;
    return (u8)((layer[0]<<1)+(side&1?half:0)+(side&2?half<<1:0));
}
// Match the original byte result,127 cap and strict-above comparison.
static u8 surface_ceiling_layers(const u8 *column,u8 shift,u8 exit_height,u8 layers)
{
    u8 layer,ceiling=127;
    for(layer=0;layer<layers;layer++,column+=4)if(!(column[3]&128)) {
        u8 candidate=(u8)(surface_edge(column,shift)-((column[3]&31)<<1));
        if(candidate>exit_height&&candidate<ceiling)ceiling=candidate;
    }
    return ceiling;
}
static u8 surface_ceiling(const u8 *column,u8 shift,u8 exit_height)
{return surface_ceiling_layers(column,shift,exit_height,2);}
// Returns0 clear,1 lower,2 upper. World height points upward here; the
// original vy is negative. The top is inclusive, the underside exclusive.
static u8 surface_hit_layers(const u8 *column,s16 height,u8 layers)
{
    u8 layer;
    for(layer=0;layer<layers;layer++,column+=4) {
        u16 h=column[0],depth=column[3]&31;
        s16 top=(s16)((h<<3)+(h<<2)),bottom;
        if(!depth)depth=layer?1:h;
        bottom=(s16)(top-(s16)((depth<<3)+(depth<<2)));
        if(height<=top&&height>bottom)return layer+1;
    }
    return 0;
}
static u8 surface_hit(const u8 *column,s16 height)
{return surface_hit_layers(column,height,2);}
// Geometry-only prefix of the retail ordinary candidate check. Jump, costs,
// occupancy and intermediate spans are separate policies, not hidden here.
static u8 surface_room(u8 source_ceiling,u8 destination_ceiling,u8 source_exit,u8 entry,u8 body)
{
    return source_ceiling>=entry+body&&destination_ceiling>=source_exit+body&&
           source_ceiling>=source_exit+body;
}
// Terrain-only horizontal-jump clearance at the SOURCE exit height. Unlike
// projectile solids, movement skips blocked records. Both facing slope edges,
// water depth and the whole body matter; touching a span endpoint is allowed.
// Enemy body spans are a separate occupancy policy, not encoded in this bank.
static u8 __attribute__((noinline)) surface_jump_layers(const u8 *column,u8 side,u8 height,u8 body,u8 layers)
{
    u8 layer,lowest=127;u16 head=(u16)height+body;
    for(layer=0;layer<layers;layer++,column+=4)if(!(column[3]&128)) {
        u8 a=surface_edge(column,side),b=surface_edge(column,side^2),low,high;
        u8 water=(column[1]>>5)<<1,depth=(column[3]&31)<<1;
        low=(u8)((a<b?a:b)-depth);high=(u8)((a>b?a:b)+water);
        if(low<lowest)lowest=low;
        if((height<low&&low<head)||(height<high&&high<head)||(low<=height&&head<=high))return 0;
    }
    return lowest==127||height>=lowest;
}
static u8 surface_jump_clear(const u8 *column,u8 side,u8 height,u8 body)
{return surface_jump_layers(column,side,height,body,2);}
// A dense LOWER-only rectangle has no constraining ceiling when every upper
// record is blocked and every possible lower exit plus water/body fits below
// the retail127 sentinel. This is a proof-based map property, not a map ID rule.
static u8 surface_simple_lower(const u8 *column,u16 area)
{
    u16 i;
    for(i=0;i<area;i++,column+=8) {
        u16 half=column[1]&31;
        if(!(column[7]&128)||(column[0]<<1)+half+(half<<1)+((column[1]>>5)<<1)+6>127)return 0;
    }
    return 1;
}
// Once per binding, independently classify movement and positive-height rays.
// An untargetable upper record may still affect movement. A blocked upper
// record may still stop a projectile. No runtime map-ID exceptions.
static u8 surface_profile(const u8 *column,u16 area)
{
    u16 i;u8 flags=0;
    for(i=0;i<area;i++,column+=8) {
        u16 half=column[1]&31;
        if(!(column[7]&128))flags|=SURFACE_MOVE_UPPER|SURFACE_ROOM;
        if(column[4])flags|=SURFACE_HIT_UPPER;
        if((column[0]<<1)+half+(half<<1)+((column[1]>>5)<<1)+6>127)flags|=SURFACE_ROOM;
    }
    return flags;
}
// Descriptor footer: SGE1,u16 section offset,u16 section bytes. Accept raw
// descriptor bytes or the six-byte AMS OTH trailer, validating every span.
static const u8 *surface_descriptor(const u8 *desc,u16 bytes,u8 width,u8 height,u16 minimum)
{
    u8 attempt;
    for(attempt=0;attempt<2;attempt++) {
        u16 n,start,size;const u8 *footer;
        if(bytes<8+6*attempt)continue;n=bytes-6*attempt;footer=desc+n-8;
        if(footer[0]!='S'||footer[1]!='G'||footer[2]!='E'||footer[3]!='1')continue;
        start=(u16)((u16)footer[4]<<8|footer[5]);size=(u16)((u16)footer[6]<<8|footer[7]);
        if((start&1)||start<minimum||start>n-8||size!=n-8-start)continue;
        if(surface_valid(desc+start,size,width,height))return desc+start;
    }
    return 0;
}
#endif
