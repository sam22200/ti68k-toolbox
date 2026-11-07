#ifndef FFT_STEP_GEOMETRY_H
#define FFT_STEP_GEOMETRY_H
#include "surface_geometry.h"
static u8 step_jump_cap(u8 jump){return jump<8?jump:7;}
static u8 step_steep(const u8 *floor,u8 jump)
{
    u8 type=floor[2]&5,half=floor[1]&31;
    return type==1||type==4?half>=3:half>jump;
}
// Ordinary terrain endpoint policy. Unit-top paths, Fly/Float and terrain costs
// are separate. source_water is the original Move-in-Water bit, not tile depth.
static u8 step_clear(const u8 *src,const u8 *dst,u8 source_layer,u8 target_layer,
                     u8 side,u8 jump,u8 source_water,u8 body,u8 layers,u8 room,u8 extra)
{
    jump=step_jump_cap(jump);
    const u8 *a=src+4*source_layer,*b=dst+4*target_layer;
    u8 exit=surface_edge(a,side),entry=surface_edge(b,side^2),ceiling=127,above=127;
    s16 delta;
    if(b[3]&128)return 0;
    if(step_steep(a,jump)&&exit>=surface_edge(a,side^2))return 0;
    if(step_steep(b,jump)&&surface_edge(b,side)>entry)return 0;
    if(room){ceiling=surface_ceiling_layers(src,side,exit,layers);above=surface_ceiling_layers(dst,side^2,entry,layers);}
    if(source_water)exit=(u8)(exit+((a[1]>>5)<<1));
    if(above<exit+body||ceiling<entry+body||above<entry+body)return 0;
    entry=(u8)(entry+((b[1]>>5)<<1));
    if(extra&&entry>exit)return 0;
    delta=(s16)entry-exit;if(delta<0)delta=-delta;
    return delta<=((u16)jump<<1);
}
#endif
