#ifndef FFT_JUMP_ACTOR_GEOMETRY_H
#define FFT_JUMP_ACTOR_GEOMETRY_H
#include "surface_geometry.h"
// Source-query prototype, not wired to the battle graph. actors holds two
// four-byte movement-record tails: unit/team flags, body, standing, top.
// Frontier flags are source movement flags, independent of compact node IDs.
// Use both layers when actors may be on either one, even if terrain is classified
// lower-only. Runtime record generation/abilities are a separate contract.
static u8 jump_actor_column_clear(const u8 *column,const u8 *actors,const u8 *frontier,
                                 u8 side,u8 height,u8 body)
{
    u8 layer,lowest=127;u16 head=(u16)height+body;
    for(layer=0;layer<2;layer++,column+=4,actors+=4) {
        u8 low,high,a,b;
        if(frontier[layer]&128) {
            if(frontier[layer]&4)return 0;
        } else {
            a=surface_edge(column,side);b=surface_edge(column,side^2);
            low=(u8)((a<b?a:b)-((column[3]&31)<<1));
            high=(u8)((a>b?a:b)+((column[1]>>5)<<1));
            if(low<lowest)lowest=low;
            if((height<low&&low<head)||(height<high&&high<head)||(low<=height&&head<=high))return 0;
        }
        if((frontier[layer]&12)!=4||!(actors[0]&64))continue;
        low=actors[2];high=(u8)(low+3);
        if(height<high&&high<head)return 0;
        high=actors[3];if(low<lowest)lowest=low;
        if((height<low&&low<head)||(height<high&&high<head)||(low<=height&&head<=high))return 0;
    }
    return lowest==127||height>=lowest;
}
#endif
