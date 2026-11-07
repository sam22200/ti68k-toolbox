// Dial search, costs1..3, stable FIFO order. A node's outstanding tentative
// costs differ by at most two, so
// they occupy DIFFERENT buckets; strict improvements never add it twice to the
// same bucket before that bucket is reset. Positive costs never requeue the
// origin: at most255 other IDs occupy a bucket, so its count fits a byte even
// with256 valid nodes. Graph count and requested-node count remain16-bit.
// Keep the new span-cost path opt-in for bound battle builds until graph/body
// integration AND the resident packed-code limit are addressed. Pure graph
// fixtures exercise both paths without calculator packaging constraints.
#if !defined(CORE_NAV_MAP) || defined(FFT_NAV_SPANS)
#define CORE_NAV_HAS_SPANS
#endif
static u8 core_nav_graph_mode_valid(const CoreNavGraph *g,u8 wide)
{
    u16 i;
    if(!g->count||g->count>FFT_CORE_TILE_MAX||!g->stride||g->stride>254||(g->stride&1))return 0;
    for(i=0;i<g->count;i++) {
        const u8 *e=g->edge+i*g->stride,*via=g->via+i*(g->stride>>1);
        u16 k,n=g->edge_bytes[i];
        if(n>g->stride||(n&1))return 0;
        for(k=0;k<n;k+=2)
            if((u16)e[k]>=g->count||(e[k+1]&~(wide?63U:7U))||(e[k+1]&24)==24||
               ((e[k+1]&28)&&!(e[k+1]&1))||
               ((e[k+1]&1)&&!(e[k+1]&4)&&(u16)via[k>>1]>=g->count))return 0;
    }
    return 1;
}
static u8 core_nav_graph_valid(const CoreNavGraph *g){return core_nav_graph_mode_valid(g,0);}
// Costs use0xFF for unreachable; limit0..254 is independent of tile identity.
// Validate graph buffers once when loading/building, before starting a search.
static void core_nav_begin(CoreNavSearch *s,CoreTileCount count,u8 origin,u8 limit,
                           u8 *dist,u8 *from,const u8 *blocked,u8 out,
                           const u8 *need,u16 remaining)
{
    memset(s->size,0,sizeof s->size);memset(dist,255,count);
    s->distance=0;s->index=0;s->limit=limit;s->out=out;s->dist=dist;s->from=from;
    s->blocked=blocked;s->need=need;s->remaining=remaining;s->done=0;s->fault=0;
    dist[origin]=0;if(from)from[origin]=origin;
#ifdef CORE_NAV_HAS_SPANS
    if(out&CORE_NAV_WIDE) {
        memset(s->wide.count,0,sizeof s->wide.count);
        s->wide.total=1;s->wide.count[0]=1;s->wide.head[0]=s->wide.tail[0]=origin;
    } else
#endif
    {s->queue[0][0]=origin;s->size[0]=1;}
    if(need&&!remaining)s->done=1;
}
static u8 core_nav_start(CoreNavSearch *s,CoreTileCount count,u8 origin,u8 limit,
                           u8 *dist,u8 *from,const u8 *blocked,u8 out,
                           const u8 *need,u16 remaining)
{
    s->done=1;s->fault=1;
#ifndef CORE_NAV_HAS_SPANS
    if(out&CORE_NAV_WIDE)return 0;
#endif
    if(!count||count>FFT_CORE_TILE_MAX||(u16)origin>=count||limit==255||
       (out&~15U)||(need&&remaining>count))return 0;
    core_nav_begin(s,count,origin,limit,dist,from,blocked,out,need,remaining);return 1;
}
#ifdef CORE_NAV_MAP
#define NAV_EDGE(id) EDGE_ROW(id)
#ifdef CORE_NAV_COMPACT
#define NAV_VIA(id) (CORE_NAV_MAP->via[id])
#define NAV_VIA_BLOCKED(id,kind,via_id) NAV_BLOCKED(via[(u16)(kind)>>6])
#else
#define NAV_VIA(id) (CORE_NAV_MAP->evia[id])
#define NAV_VIA_BLOCKED(id,kind,via_id) NAV_BLOCKED(via_id)
#endif
#define NAV_COUNT(id) (CORE_NAV_MAP->en[id])
#define NAV_TERRAIN(id) (CORE_NAV_MAP->hw[id])
#define NAV_BLOCKED(id) (CORE_NAV_MAP->blk[id])
#else
#define NAV_EDGE(id) (g->edge+(u16)(id)*g->stride)
#define NAV_VIA(id) (g->via+(u16)(id)*(g->stride>>1))
#define NAV_VIA_BLOCKED(id,kind,via_id) NAV_BLOCKED(via_id)
#define NAV_COUNT(id) (g->edge_bytes[id])
#define NAV_TERRAIN(id) (g->terrain[id])
#define NAV_BLOCKED(id) (s->blocked[id])
#endif
// Longer costs must never enter the four-bucket append-only queue. Eight
// linked buckets support costs1..6 without another allocation or tile sentinel.
// Strict improvement removes a node's old record before appending its new one;
// equal-cost order stays FIFO. A bucket count can represent all256 IDs.
#ifdef CORE_NAV_HAS_SPANS
static u8 core_nav_wide_step(CoreNavSearch *s,const CoreNavGraph *g,u16 budget)
{
    static const u8 costs[3][4]={{1,2,1,2},{1,3,2,3},{2,3,2,3}};
    u8 *dist=s->dist,*from=s->from;const u8 *need=s->need;
    u16 left=s->remaining;s16 d=s->distance,limit=s->limit;
    u8 terrain_mask=s->out&CORE_NAV_FLOAT?221:255; // Float: no water surcharge or explicit cost3
    for(;d<limit&&s->wide.total;d++) {
        u8 bucket=d&7;
        while(s->wide.count[bucket]) {
            u8 id=s->wide.head[bucket],wet=NAV_TERRAIN(id)&terrain_mask&2;
            const u8 *edge,*end,*via,*costs_here;
            if(need&&(need[id]&1)&&!--left) {
                s->remaining=0;s->done=1;return CORE_NAV_DONE;
            }
            if(!budget) {
                s->distance=(u8)d;s->remaining=(u16)(left+(need&&(need[id]&1)));
                return CORE_NAV_MORE;
            }
            budget--;s->wide.total--;
            if(--s->wide.count[bucket])s->wide.head[bucket]=s->wide.next[id];
            edge=NAV_EDGE(id);end=edge+NAV_COUNT(id);via=NAV_VIA(id);
            costs_here=costs[wet?((s->out&CORE_NAV_OUT)?2:1):0];
            while(edge<end) {
                u8 next=*edge++,kind=*edge++&terrain_mask,via_id=0,b;
#ifndef CORE_NAV_COMPACT
                via_id=*via++;
#endif
                s16 cost;
                if(NAV_BLOCKED(next)||((kind&1)&&!(kind&4)&&NAV_VIA_BLOCKED(id,kind,via_id)))continue;
                cost=d+((s->out&CORE_NAV_CUSTOM)?1+((NAV_TERRAIN(next)>>4)&3)+(kind&1):((kind&32)?3+((kind&1)?1+((kind>>3)&3):0):costs_here[kind&3]+((kind>>3)&3)));
                if(cost>limit||cost>=dist[next]||(!(s->out&CORE_NAV_CUSTOM)&&(kind&2)&&!wet&&cost==limit))continue;
                if(dist[next]!=255) {
                    b=dist[next]&7;
                    if(--s->wide.count[b]) {
                        if(s->wide.head[b]==next)s->wide.head[b]=s->wide.next[next];
                        else if(s->wide.tail[b]==next)s->wide.tail[b]=s->wide.previous[next];
                        else {
                            s->wide.next[s->wide.previous[next]]=s->wide.next[next];
                            s->wide.previous[s->wide.next[next]]=s->wide.previous[next];
                        }
                    }
                    s->wide.total--;
                }
                b=cost&7;dist[next]=(u8)cost;if(from)from[next]=id;
                if(s->wide.count[b]) {
                    s->wide.next[s->wide.tail[b]]=next;s->wide.previous[next]=s->wide.tail[b];
                } else s->wide.head[b]=next;
                s->wide.tail[b]=next;s->wide.count[b]++;s->wide.total++;
            }
        }
    }
    s->remaining=left;s->distance=(u8)d;s->done=1;return CORE_NAV_DONE;
}
#endif
static u8 core_nav_step(CoreNavSearch *s,const CoreNavGraph *g,u16 budget)
{
    u8 *dist=s->dist,*from=s->from;
    const u8 *need=s->need;
    if(s->done)return CORE_NAV_DONE;
#ifdef CORE_NAV_HAS_SPANS
    if(s->out&CORE_NAV_WIDE)return core_nav_wide_step(s,g,budget);
#endif
    static const u8 costs[3][4]={{1,2,1,2},{1,3,2,3},{2,3,2,3}};
    u8 *sizes=s->size,*queues[4];
    u16 left=s->remaining;u8 index=s->index,bucket_index;
    s16 d=s->distance,limit=s->limit,out=s->out;
    u8 terrain_mask=out&CORE_NAV_FLOAT?221:255;
    for(bucket_index=0;bucket_index<4;bucket_index++)queues[bucket_index]=s->queue[bucket_index];
    for(;d<limit;d++,index=0) {
        s16 bucket=d&3;
        const u8 *q=queues[bucket];
        if(!(sizes[0]|sizes[1]|sizes[2]|sizes[3]))break;
        for(;index<sizes[bucket];index++) {
            s16 id=q[index],wet;
            const u8 *edge,*end,*via,*costs_here;
            if(dist[id]!=d)continue;
            if(need&&(need[id]&1)&&!--left) {
                s->remaining=0;s->done=1;return CORE_NAV_DONE;
            }
            if(!budget) {
                s->distance=d;s->index=index;
                s->remaining=(u16)(left+(need&&(need[id]&1)));
                return CORE_NAV_MORE;
            }
            budget--;
            wet=NAV_TERRAIN(id)&terrain_mask&2;edge=NAV_EDGE(id);
            via=NAV_VIA(id);end=edge+NAV_COUNT(id);
            costs_here=costs[wet?(out?2:1):0];
            while(edge<end) {
                s16 next=*edge++,kind=*edge++&terrain_mask,via_id=0,cost,b;
#ifndef CORE_NAV_COMPACT
                via_id=*via++;
#endif
                if(NAV_BLOCKED(next)||((kind&1)&&!(kind&4)&&NAV_VIA_BLOCKED(id,kind,via_id)))continue;
                cost=d+((s->out&CORE_NAV_CUSTOM)?1+((NAV_TERRAIN(next)>>4)&3)+(kind&1):costs_here[kind&3]);
                if(cost>limit||cost>=dist[next]||(!(s->out&CORE_NAV_CUSTOM)&&(kind&2)&&!wet&&cost==limit))continue;
                b=cost&3;
                dist[next]=(u8)cost;if(from)from[next]=id;
                queues[b][sizes[b]++]=next;
            }
        }
        sizes[bucket]=0;
    }
    s->remaining=left;s->distance=d;s->index=index;s->done=1;return CORE_NAV_DONE;
}
#undef NAV_EDGE
#undef NAV_VIA
#undef NAV_VIA_BLOCKED
#undef NAV_COUNT
#undef NAV_TERRAIN
#undef NAV_BLOCKED
#undef CORE_NAV_HAS_SPANS
// Includes origin and target; count256 and a predecessor ID255 are legal.
// Reject cycles/bad references and short output buffers before writing anything.
#include "tactical_path.h"
