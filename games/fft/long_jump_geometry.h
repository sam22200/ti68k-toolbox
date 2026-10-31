#ifndef FFT_LONG_JUMP_GEOMETRY_H
#define FFT_LONG_JUMP_GEOMETRY_H
#include "step_geometry.h"
// Retail clamps ordinary Jump to7 before deriving the horizontal gap budget.
static u8 long_jump_limit(u8 jump){return step_jump_cap(jump)>>1;}
// Geometry prototype, deliberately not wired into the current16-link graph.
// Each result byte is a lower/upper landing mask for1,2,3 intervening cells.
// A blocked body span aborts the remaining direction, not just one landing.
static void long_jump_masks(const u8 *surface,u8 width,u8 height,u8 sx,u8 sz,u8 source_layer,
                            u8 direction,u8 jump,u8 flags,u8 *masks)
{
    static const s8 dxs[4]={1,-1,0,0},dzs[4]={0,0,1,-1};
    static const u8 sides[4]={0,2,6,4};
    s8 dx,dz;u8 side;
    masks[0]=masks[1]=masks[2]=0;if(direction>=4)return;
    dx=dxs[direction];dz=dzs[direction];side=sides[direction];
    u8 gap,limit=long_jump_limit(jump),layers=flags&SURFACE_MOVE_UPPER?2:1;
    s16 x=(s16)sx+dx,z=(s16)sz+dz,stride=dx?dx:(s16)dz*width;
    s16 index=(s16)((u16)sz*width+sx);const u8 *src=surface+((u16)index<<3);
    u8 exit=surface_edge(src+4*source_layer,side);
    index+=stride;jump=step_jump_cap(jump);
    for(gap=1;gap<=limit;gap++) {
        const u8 *destination;u8 layer;
        if((u16)x>=width||(u16)z>=height)break;
        if(!surface_jump_layers(surface+((u16)index<<3),side,exit,6,layers))break;
        x+=dx;z+=dz;index+=stride;
        if((u16)x>=width||(u16)z>=height)break;
        destination=surface+((u16)index<<3);
        for(layer=0;layer<layers;layer++)
            if(step_clear(src,destination,source_layer,layer,side,jump,0,6,layers,flags&SURFACE_ROOM,gap))
                masks[gap-1]|=(u8)(1U<<layer);
    }
}
#endif
