// Compact geometry/search oracle, no commercial assets or hardware accesses.
#ifdef __m68k__
#define USE_TI89
#define MIN_AMS 200
#include <tigcclib.h>
#include "../../../tools/m68kbench/bench.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif
#include "../tactical_grid.h"
#include "../tactical_search.inc"
typedef struct {
    CoreGrid grid;CoreNavGraph graph;CoreNavSearch search;
    CoreTilePosition pos[256];
    u8 row[18],lookup[512],present[64],edge[256][16],via[256][8];
    u8 en[256],terrain[256],blocked[256],dist[256],from[256],path[256],need[256];
} Fixture;
static Fixture *f;
static u32 digest=17;
static u16 checks,failures;
static void verify(u16 value,u16 expected,u16 line)
{
    checks++;digest=(digest<<1|digest>>31)^value;
    if(value!=expected){failures++;
#ifndef __m68k__
        printf("line%u: %u expected%u\n",line,value,expected);
#else
        BENCH_VALUE(line);
#endif
    }
}
#define CHECK(v,e) verify((u16)(v),(u16)(e),__LINE__)
static void setup(u8 width,u8 height,u16 count)
{
    u16 i;
    memset(f,0,sizeof(*f));f->grid.count=count;f->grid.width=width;f->grid.height=height;
    f->grid.position=f->pos;f->grid.lookup=f->lookup;f->grid.present=f->present;f->grid.row=f->row;
    for(i=0;i<height;i++)f->row[i]=(u8)(i*width);
    f->graph.count=count;f->graph.stride=16;f->graph.edge=&f->edge[0][0];
    f->graph.edge_bytes=f->en;f->graph.via=&f->via[0][0];f->graph.terrain=f->terrain;
}
static void position(u8 id,u8 x,u8 z,u8 layer)
{
    u16 at=f->row[z]+x+((u16)layer<<8);
    f->pos[id].x=x;f->pos[id].z_layer=z|(layer<<5);
    f->lookup[at]=id;f->present[at>>3]|=(u8)(1U<<(at&7));f->terrain[id]=1;
}
static void link(u8 from,u8 to,u8 kind,u8 via)
{
    u8 n=f->en[from];
    if(n>=16){CHECK(n<16,1);return;}
    f->edge[from][n]=to;f->edge[from][n+1]=kind;f->via[from][n>>1]=via;f->en[from]=n+2;
}
static void search(u8 origin,u8 limit,u8 out,u16 budget,u8 needs,u16 remaining)
{
    u16 steps=0;u8 started;
    CHECK(core_nav_graph_mode_valid(&f->graph,(out&CORE_NAV_WIDE)!=0),1);
#ifdef __m68k__
    BENCH_NAME(4,"Compact node search");BENCH_BEGIN(4);
#endif
    started=core_nav_start(&f->search,f->graph.count,origin,limit,f->dist,f->from,f->blocked,
                         out,needs?f->need:0,remaining);
    while(!core_nav_step(&f->search,&f->graph,budget)&&steps<1024)steps++;
#ifdef __m68k__
    BENCH_END(4);
#endif
    CHECK(started,1);
    CHECK(steps<1024,1);CHECK(f->search.fault,0);
}
// Independent O(V^2) minimum-distance reference. Only fixtures pay this cost.
static void oracle(u8 origin,u8 limit,u8 out,u8 *result)
{
    u8 visited[256];u16 i,iteration;
    memset(result,255,256);memset(visited,0,sizeof visited);result[origin]=0;
    for(iteration=0;iteration<f->graph.count;iteration++) {
        u16 best=256,minimum=255;
        for(i=0;i<f->graph.count;i++)if(!visited[i]&&result[i]<minimum){best=i;minimum=result[i];}
        if(best==256||minimum>=limit)break;
        visited[best]=1;
        for(i=0;i<f->en[best];i+=2) {
            u8 to=f->edge[best][i],kind=f->edge[best][i+1],wet=f->terrain[best]&2;
            u16 distance=minimum+((kind&32)?3+((kind&1)?1+((kind>>3)&3):0):1+(kind&1)+(wet&&(out||(kind&3)))+((kind>>3)&3));
            if(f->blocked[to]||((kind&1)&&!(kind&4)&&f->blocked[f->via[best][i>>1]])||
               distance>limit||((kind&2)&&!wet&&distance==limit))continue;
            if(distance<result[to])result[to]=(u8)distance;
        }
    }
}
static void full_grid(void)
{
    u16 i;u8 id;CoreTilePosition p;
    setup(16,16,256);
    // Deliberately reverse all compact IDs; coordinate arithmetic is no oracle.
    for(i=0;i<256;i++)position((u8)(i^255),(u8)(i&15),(u8)(i>>4),0);
    CHECK(core_grid_valid(&f->grid),1);
    for(i=0;i<256;i++) {
        CHECK(core_grid_find(&f->grid,(s16)(i&15),(s16)(i>>4),0,&id),1);CHECK(id,i^255);
        CHECK(core_grid_position(&f->grid,id,&p),1);CHECK(p.x,i&15);CHECK(p.z_layer,i>>4);
        if((i&15)<15)link(id,(u8)((i+1)^255),0,0);
        if(i&15)link(id,(u8)((i-1)^255),0,0);
        if(i<240)link(id,(u8)((i+16)^255),0,0);
        if(i>=16)link(id,(u8)((i-16)^255),0,0);
        f->need[i]=1;
    }
    search(255,31,0,1,1,256);CHECK(f->search.remaining,0);
    for(i=0;i<256;i++)CHECK(f->dist[i^255],(i&15)+(i>>4));
    CHECK(core_nav_path(256,255,0,f->dist,f->from,f->path,256),31);
    CHECK(f->path[0],255);CHECK(f->path[30],0);
    search(255,31,CORE_NAV_WIDE,1,1,256);CHECK(f->search.remaining,0);
    for(i=0;i<256;i++)CHECK(f->dist[i^255],(i&15)+(i>>4));
    f->path[0]=99;CHECK(core_nav_path(256,255,0,f->dist,f->from,f->path,30),0);CHECK(f->path[0],99);
    id=37;CHECK(core_grid_find(&f->grid,0,0,1,&id),0);CHECK(id,37);
    CHECK(core_grid_find(&f->grid,-1,0,0,&id),0);CHECK(core_grid_find(&f->grid,0,16,0,&id),0);
    f->from[0]=0;CHECK(core_nav_path(256,255,0,f->dist,f->from,f->path,256),0);
    f->grid.count=255;CHECK(core_grid_valid(&f->grid),0);f->grid.count=256;
    f->grid.lookup=0;f->grid.present=0;f->grid.position=0;
    CHECK(core_grid_valid(&f->grid),1);CHECK(core_grid_find(&f->grid,15,15,0,&id),1);CHECK(id,255);
    CHECK(core_grid_position(&f->grid,255,&p),1);CHECK(p.x,15);CHECK(p.z_layer,15);
}
static void layers(void)
{
    u16 i;u8 lower,upper,rot;CoreTilePosition p;
    setup(4,18,74);
    for(i=0;i<72;i++)position((u8)i,(u8)(i&3),(u8)(i>>2),0);
    position(72,0,0,1);position(73,3,17,1);
    CHECK(core_grid_valid(&f->grid),1);
    CHECK(core_grid_find(&f->grid,0,0,0,&lower),1);CHECK(lower,0);
    CHECK(core_grid_find(&f->grid,0,0,1,&upper),1);CHECK(upper,72);
    CHECK(core_grid_position(&f->grid,72,&p),1);CHECK(p.x,0);CHECK(p.z_layer,32);
    CHECK(core_grid_find(&f->grid,1,0,1,&upper),0);CHECK(upper,72);
    link(0,1,0,0);link(1,72,0,0);link(72,73,1,2);
    search(0,10,0,1,0,0);CHECK(f->dist[72],2);CHECK(f->dist[73],4);
    f->blocked[2]=1;search(0,10,0,7,0,0);CHECK(f->dist[73],255);CHECK(f->dist[72],2);
    f->blocked[2]=0;f->blocked[72]=1;search(0,10,0,65535,0,0);CHECK(f->dist[1],1);CHECK(f->dist[72],255);
    for(rot=0;rot<4;rot++)for(i=0;i<74;i++) {
        s16 u,v,x,z;
        core_grid_view(4,18,rot,f->pos[i].x,f->pos[i].z_layer&31,&u,&v);
        core_grid_world(4,18,rot,u,v,&x,&z);
        CHECK(x,f->pos[i].x);CHECK(z,f->pos[i].z_layer&31);
    }
    {s16 u,v;core_grid_view(4,18,1,3,2,&u,&v);CHECK(u,15);CHECK(v,3);}
    f->en[0]=3;CHECK(core_nav_graph_valid(&f->graph),0);f->en[0]=2;
    f->edge[0][0]=255;CHECK(core_nav_graph_valid(&f->graph),0);f->edge[0][0]=1;
    f->edge[0][1]=4;CHECK(core_nav_graph_valid(&f->graph),0);f->edge[0][1]=0;
    f->grid.count=257;CHECK(core_grid_valid(&f->grid),0);
}
static void explicit_via(void)
{
    setup(16,16,256);link(0,255,1,72);
    f->blocked[127]=1;search(0,5,0,1,0,0);CHECK(f->dist[255],2);
    f->blocked[72]=1;search(0,5,0,1,0,0);CHECK(f->dist[255],255);
    CHECK(core_nav_start(&f->search,f->graph.count,0,255,f->dist,f->from,f->blocked,0,0,0),0);
    f->graph.count=257;CHECK(core_nav_graph_valid(&f->graph),0);
}
static void full_layers(void)
{
    u16 i;u8 id;
    setup(16,8,256);
    for(i=0;i<256;i++)position((u8)i,(u8)(i&15),(u8)((i&127)>>4),(u8)(i>>7));
    CHECK(core_grid_valid(&f->grid),1);
    CHECK(core_grid_find(&f->grid,15,7,1,&id),1);CHECK(id,255);
    for(i=0;i<256;i++) {
        if((i&15)<15)link((u8)i,(u8)(i+1),0,0);
        if(i&15)link((u8)i,(u8)(i-1),0,0);
        if((i&127)<112)link((u8)i,(u8)(i+16),0,0);
        if((i&127)>=16)link((u8)i,(u8)(i-16),0,0);
        f->need[i]=1;
    }
    link(0,128,0,0);link(128,0,0,0);
    search(0,25,0,7,1,256);CHECK(f->search.remaining,0);
    for(i=0;i<256;i++)CHECK(f->dist[i],(i&15)+((i&127)>>4)+(i>>7));
    CHECK(core_nav_path(256,0,255,f->dist,f->from,f->path,256),24);CHECK(f->path[23],255);
    f->en[0]-=2;search(0,25,0,1,0,0);CHECK(f->dist[127],22);CHECK(f->dist[255],255);
}
static void weighted(void)
{
    u16 seed,i;u8 reference[256],dist[256],from[256];
    for(seed=0;seed<16;seed++) {
        setup(16,16,256);
        for(i=0;i<256;i++){f->terrain[i]=(i*17+seed*3)%7?1:3;f->blocked[i]=(i*7+seed*11)%17==0;}
        f->blocked[255]=0;
        for(i=0;i<256;i++) {
            u8 destinations[4],n=0,k;
            if((i&15)<15)destinations[n++]=(u8)(i+1);
            if(i&15)destinations[n++]=(u8)(i-1);
            if(i<240)destinations[n++]=(u8)(i+16);
            if(i>=16)destinations[n++]=(u8)(i-16);
            if(seed>=12){n=4;for(k=0;k<n;k++)destinations[k]=(u8)(i*73+k*61+seed*17);}
            for(k=0;k<n;k++)link((u8)i,destinations[k],(u8)((f->terrain[destinations[k]]&2)|((i+k+seed)%11==0)),(u8)(i^16));
        }
        oracle(255,40,(u8)(seed&1),reference);
        search(255,40,(u8)(seed&1),65535,0,0);
        memcpy(dist,f->dist,256);memcpy(from,f->from,256);
        for(i=0;i<256;i++)CHECK(dist[i],reference[i]);
        search(255,40,(u8)(seed&1),seed&1?1:7,0,0);
        for(i=0;i<256;i++){CHECK(f->dist[i],dist[i]);if(dist[i]!=255)CHECK(f->from[i],from[i]);}
        search(255,40,(u8)((seed&1)|CORE_NAV_WIDE),seed&1?1:7,0,0);
        for(i=0;i<256;i++){CHECK(f->dist[i],dist[i]);if(dist[i]!=255)CHECK(f->from[i],from[i]);}
    }
}
static void wide_costs(void)
{
    u16 i,seed;u8 reference[256],dist[256],from[256];
    // Three cost5 records in one bucket: improve its middle, tail and head,
    // then settle their new equal-cost records in insertion order.
    setup(16,16,256);f->terrain[0]=3;
    link(0,1,17,99);link(0,3,17,99);link(0,4,17,99);link(0,2,0,0);
    link(2,3,0,0);link(2,4,0,0);link(2,1,0,0);
    link(3,255,0,0);link(4,255,0,0);link(1,255,0,0);
    search(0,9,CORE_NAV_WIDE,1,0,0);
    CHECK(f->dist[1],2);CHECK(f->dist[3],2);CHECK(f->dist[4],2);
    CHECK(f->dist[255],3);CHECK(f->from[255],3);CHECK(f->search.wide.total,0);
    // Cost-class validation belongs at graph binding, outside the hot edge loop.
    CHECK(core_nav_graph_valid(&f->graph),0);
    CHECK(core_nav_graph_mode_valid(&f->graph,1),1);
    // A zero-budget yield must retain the requested origin and its frontier.
    f->need[0]=f->need[255]=1;
    CHECK(core_nav_start(&f->search,256,0,9,f->dist,f->from,f->blocked,CORE_NAV_WIDE,f->need,2),1);
    CHECK(core_nav_step(&f->search,&f->graph,0),CORE_NAV_MORE);
    CHECK(f->search.remaining,2);CHECK(f->search.wide.total,1);
    while(!core_nav_step(&f->search,&f->graph,1)){}
    CHECK(f->search.remaining,0);CHECK(f->from[255],3);
    // Restart after completion AND after an abandoned partially expanded search.
    CHECK(core_nav_start(&f->search,256,0,9,f->dist,f->from,f->blocked,CORE_NAV_WIDE,0,0),1);
    CHECK(core_nav_step(&f->search,&f->graph,1),CORE_NAV_MORE);
    search(255,9,CORE_NAV_WIDE,1,0,0);CHECK(f->dist[255],0);CHECK(f->dist[0],255);
    CHECK(core_nav_start(&f->search,256,0,9,f->dist,f->from,f->blocked,4,0,0),0);
    f->edge[0][1]=24;CHECK(core_nav_graph_mode_valid(&f->graph,1),0);
    f->edge[0][1]=8;CHECK(core_nav_graph_mode_valid(&f->graph,1),0);
    f->edge[0][1]=25;CHECK(core_nav_graph_mode_valid(&f->graph,1),0);
    f->edge[0][1]=64;CHECK(core_nav_graph_mode_valid(&f->graph,1),0);
    f->edge[0][1]=17;CHECK(core_nav_graph_mode_valid(&f->graph,1),1);
    // Independent distance reference, cycles, all spans/water modes, node255;
    // yielded and whole searches must also have identical predecessor ties.
    for(seed=0;seed<8;seed++) {
        setup(16,16,256);
        for(i=0;i<256;i++) {
            u8 k;f->terrain[i]=(i+seed)%3?1:3;f->blocked[i]=(i*7+seed)%29==0;
            for(k=0;k<4;k++) {
                u8 to=(u8)(i*73+k*61+seed*17),gap=(i+k+seed)%4;
                u8 kind=(u8)((gap?1+((gap-1)<<3):0)|((to+seed)%3?0:2));
                if((i*3+k+seed)%7==0)kind|=32;
                link((u8)i,to,kind,(u8)(i^16));
            }
        }
        f->blocked[255]=0;oracle(255,40,seed&1,reference);
        search(255,40,(seed&1)|CORE_NAV_WIDE,65535,0,0);
        memcpy(dist,f->dist,256);memcpy(from,f->from,256);
        for(i=0;i<256;i++)CHECK(dist[i],reference[i]);
        search(255,40,(seed&1)|CORE_NAV_WIDE,1,0,0);
        for(i=0;i<256;i++){CHECK(f->dist[i],dist[i]);if(dist[i]!=255)CHECK(f->from[i],from[i]);}
    }
    // Explicit terrain3 makes a three-gap leap cost6 on dry OR wet source.
    for(seed=0;seed<2;seed++) {
        setup(16,16,256);f->terrain[0]=seed?3:1;link(0,255,49,0);
        search(0,6,CORE_NAV_WIDE,1,0,0);CHECK(f->dist[255],6);
        search(0,5,CORE_NAV_WIDE,65535,0,0);CHECK(f->dist[255],255);
        f->edge[0][1]=32;search(0,3,CORE_NAV_WIDE,1,0,0);CHECK(f->dist[255],3);
        CHECK(core_nav_graph_valid(&f->graph),0);
    }
    setup(16,16,256);
    for(i=0;i<255;i++)link((u8)i,(u8)(i+1),0,0);
    search(255,254,CORE_NAV_WIDE,1,0,0);CHECK(f->dist[255],0);
    search(1,254,CORE_NAV_WIDE,1,0,0);CHECK(f->dist[255],254);
    search(0,254,CORE_NAV_WIDE,65535,0,0);CHECK(f->dist[254],254);CHECK(f->dist[255],255);
}
static void trials(void){full_grid();layers();explicit_via();full_layers();weighted();wide_costs();}
#ifndef __m68k__
int main(void)
{
    f=malloc(sizeof(*f));if(!f)return 2;trials();free(f);
    printf("grid %08lX failures %u checks %u\n",(unsigned long)digest,failures,checks);
    return failures!=0;
}
#else
void _main(void)
{
    f=malloc(sizeof(*f));if(!f){BENCH_VALUE(0xBAD);return;}
    trials();free(f);BENCH_VALUE(sizeof(CoreNavSearch));BENCH_VALUE(sizeof(CoreGrid));
    BENCH_VALUE(digest);BENCH_VALUE(failures);BENCH_VALUE(checks);
}
#endif
