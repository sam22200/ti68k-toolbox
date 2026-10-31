#ifndef FFT_TACTICAL_GRID_H
#define FFT_TACTICAL_GRID_H
#include "core_limits.h"

// Compact identity is independent of rectangle coordinates and source layer
// addresses. Byte255 is an ordinary ID; presence belongs to a separate bitmap.
typedef struct {
    CoreTileCount count;
    u8 width, height;
    const CoreTilePosition *position;
    const u8 *row;                      // lower source row offsets0..255
    const u8 *lookup, *present;          // source addresses0..511, bitmap64 bytes
} CoreGrid;

// Movement edges are pairs (destination,kind), in stable encounter order.
// kind bit0: leap; bit1: destination water; bit2: no intermediate node exists.
// bits3..4: extra gap cells beyond the first (0..2), only with bit0 set.
// bit5: explicit destination terrain cost3 (gap cost added, source water ignored).
// Wide-cost mode is required for nonzero extra gaps or explicit terrain3. The graph must separately
// validate every intermediate actor span before it is used for longer jumps.
// A via ID is meaningful only for a leap with bit2 clear. No ID is a sentinel.
// Counts are bytes per row, stride is bytes. Geometry/Jump filters build the
// graph; this common search never derives a midpoint by averaging compact IDs.
typedef struct {
    CoreTileCount count;
    u16 stride;
    const u8 *edge, *edge_bytes, *via, *terrain;
} CoreNavGraph;
typedef struct {
    union {
        u8 queue[4][FFT_CORE_TILE_MAX];
        // Eight FIFO buckets with one intrusive record per pending node.
        // Fits inside the existing four queues, also borrowed by edge caching.
        struct {
            u8 next[FFT_CORE_TILE_MAX],previous[FFT_CORE_TILE_MAX];
            u8 head[8],tail[8];
            u16 count[8],total;
        } wide;
    };
    u8 size[4], index;                  // at most255 non-origin IDs per bucket; positive costs
    u16 remaining;                      // all256 requested nodes may still be pending
    u8 distance, limit, out, done, fault;
    u8 *dist, *from;
    const u8 *blocked, *need;
} CoreNavSearch;
enum { CORE_NAV_MORE, CORE_NAV_DONE };
enum { CORE_NAV_OUT=1, CORE_NAV_WIDE=2 };

static u8 core_grid_rect_id(u8 width,u8 height,const u8 *row,s16 x,s16 z,CoreTileId *id)
{
    if((u16)x>=width||(u16)z>=height)return 0;
    *id=(u8)(row[z]+x);return 1;
}
static u8 core_grid_find(const CoreGrid *g,s16 x,s16 z,u8 layer,CoreTileId *id)
{
    u16 at;
    if(!g->lookup)return !layer&&core_grid_rect_id(g->width,g->height,g->row,x,z,id);
    if((u16)x>=g->width||(u16)z>=g->height||layer>=FFT_CORE_LAYER_MAX)return 0;
    at=(u16)(g->row[z]+x+((u16)layer<<8));
    if(!(g->present[at>>3]&(1U<<(at&7))))return 0;
    *id=g->lookup[at];return 1;
}
static u8 core_grid_position(const CoreGrid *g,CoreTileId id,CoreTilePosition *p)
{
    if((u16)id>=g->count)return 0;
    if(g->position) { p->x=g->position[id].x;p->z_layer=g->position[id].z_layer; }
    else {
        u8 z=0;
        while(z+1<g->height&&id>=g->row[z+1])z++;
        p->x=(u8)(id-g->row[z]);p->z_layer=z;
    }
    return 1;
}
static u8 core_grid_valid(const CoreGrid *g)
{
    u16 i,area,present=0;
    if(!g->width||!g->height||g->width>FFT_CORE_AXIS_MAX||g->height>FFT_CORE_AXIS_MAX||
       !g->count||g->count>FFT_CORE_TILE_MAX)return 0;
    area=(u16)g->width*g->height;if(area>256)return 0;
    for(i=0;i<g->height;i++)if(g->row[i]!=i*g->width)return 0;
    if(!g->lookup)return g->count==area&&!g->present&&!g->position;
    if(!g->present||!g->position)return 0;
    for(i=0;i<64;i++) {
        u8 byte=g->present[i];
        while(byte){present+=(byte&1);byte>>=1;}
    }
    if(present!=g->count)return 0;
    for(i=0;i<g->count;i++) {
        const CoreTilePosition *p=&g->position[i];u8 id;
        if((p->z_layer&0xC0)||!core_grid_find(g,p->x,p->z_layer&31,p->z_layer>>5,&id)||id!=i)return 0;
    }
    return 1;
}
static void core_grid_view(u8 width,u8 height,u8 rot,s16 x,s16 z,s16 *u,s16 *v)
{
    switch(rot) {
    case 0:*u=x;*v=z;break;
    case 1:*u=height-1-z;*v=x;break;
    case 2:*u=width-1-x;*v=height-1-z;break;
    default:*u=z;*v=width-1-x;break;
    }
}
static void core_grid_world(u8 width,u8 height,u8 rot,s16 u,s16 v,s16 *x,s16 *z)
{
    switch(rot) {
    case 0:*x=u;*z=v;break;
    case 1:*x=v;*z=height-1-u;break;
    case 2:*x=width-1-u;*z=height-1-v;break;
    default:*x=width-1-v;*z=u;break;
    }
}
#endif
