#ifndef FFT_KNOCKBACK_GEOMETRY_H
#define FFT_KNOCKBACK_GEOMETRY_H
#include "surface_geometry.h"
// Recoil uses unwrapped edge heights, unlike the byte-valued ceiling query.
static u16 knock_edge(const u8 *floor,u8 shift)
{
    u16 half=floor[1]&31,side=(floor[2]>>shift)&3;
    return (u16)((floor[0]<<1)+(side&1?half:0)+(side&2?half<<1:0));
}
// Geometry selection only:1 lower,2 upper,0 rejected. Choose the highest
// eligible landing first; the caller rejects missing/untargetable/occupied
// destinations afterwards, without falling back to another floor.
static u8 knock_layer(const u8 *source,const u8 *destination,u8 side,u16 *fall)
{
    u16 height=knock_edge(source,side),lower=255,upper=255;u8 layer;
    if(!(destination[3]&128))lower=knock_edge(destination,side^2);
    if(!(destination[7]&128))upper=knock_edge(destination+4,side^2);
    if(height<lower&&height<upper)return 0;
    if(height<upper) {
        // Retail uses ceiling depth in half-height units here (not doubled).
        if((s16)upper-(s16)(destination[7]&31)<(s16)(height+4))return 0;
        layer=0;
    } else if(height<lower) {
        if((s16)lower-(s16)(destination[3]&31)<(s16)(height+4))return 0;
        layer=1;
    } else layer=lower>=upper?0:1;
    if(destination[4*layer+3]&128)return 0;
    *fall=(u16)(height-(layer?upper:lower));return layer+1;
}
// Valid under the cached lower-only/body6<=127 proof. Even a blocked upper's
// sentinel underside is then above the four-half-unit recoil headroom.
static u8 knock_lower(const u8 *source,const u8 *destination,u8 side,u16 *fall)
{
    u16 height,landing;if(destination[3]&128)return 0;
    height=knock_edge(source,side);landing=knock_edge(destination,side^2);
    if(landing>height)return 0;
    *fall=(u16)(height-landing);return 1;
}
#endif
