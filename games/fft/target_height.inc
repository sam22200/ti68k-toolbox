// Source targeting-height producer; shared donors are resolved before Dead.
#ifndef FFT_TARGET_HEIGHT_INC
#define FFT_TARGET_HEIGHT_INC
#include "melee_rules.h"
static MeleeSpan melee_span(u8 who)
{
    MeleeSpan span;const Unit *u;u8 j,cell,h,depth,body=6,shared=0,f[3];const u8 *p;
    span.id=255;span.feet=span.top=span.pad=0;
    if(who>=NUNIT)return span;u=&st.unit[who];
    if(!unit_on_field(u)||unit_airborne(u))return span;
    cell=UNIT_CELL(u);
    for(j=0;j<NUNIT;j++)if(j!=who&&st.unit[j].dc!=DC_GONE&&st.unit[j].x<MAP_W&&st.unit[j].z<MAP_H&&UNIT_CELL(&st.unit[j])==cell){
        shared=2;if((u8)(u->job-94)>2)u=&st.unit[j];break;
    }
    // Retail checks Dead on the borrowed height donor, after the shared-cell
    // lookup. HP also covers synthetic KO fixtures without a status bitmap.
    if((!u->hp&&!DC_OBJ(u->dc))||(core_status_current(&u->statuses,0)&32))return span;
    if(core_status_current(&u->statuses,2)&6)body=4;
    if(W->surface){
        p=W->surface+((u16)(W->grid_row[u->z]+u->x)<<3)+4*node_layer(cell);
        h=(u8)((p[0]<<1)+(p[1]&31));depth=p[1]>>5;
        cj_movement(common,u->job,st.movement[u==&st.unit[who]?who:j],f);
        if(core_status_current(&u->statuses,2)&64)h+=(depth<<1)+2;
        else if(depth){if(f[2]&128)h+=depth<<1;else if(f[2]&64)h+=(depth<<1)-2;}
    }else h=map_tiles[cell].stand;
    span.feet=h;span.top=h+body+shared;span.id=who;return span;
}
#endif
