// Emit every current-map graph difference for independent original-MIPS checks.
#include <stdio.h>
#include "../../../runtime/platform-sw/rt_sw.h"
#include "battle.c"
static u8 previous[256][32],counts[256];
static u8 contains(const u8 *row,u8 n,u8 target,u8 kind)
{
    u8 i;for(i=0;i<n;i+=2)if(row[i]==target&&row[i+1]==kind)return 1;return 0;
}
static void emit(u16 origin,u8 target,u8 kind,u8 old,u8 now,u8 jump)
{
    s16 x=TILE_X[origin],z=TILE_Z[origin],dx=((s16)TILE_X[target]-x)/2,dz=((s16)TILE_Z[target]-z)/2;
    static const u8 shift[4]={0,2,6,4};u8 k=dx>0?0:dx<0?1:dz>0?2:3,i;
    const u8 *floor=W->surface+((u16)(W->grid_row[z]+x)<<3)+4*node_layer((u8)origin);
    const u8 *middle=W->surface+((u16)(W->grid_row[z+dz]+x+dx)<<3);
    u8 height=(u8)(surface_edge(floor,shift[k])+((floor[1]>>5)<<1));
    if(!(kind&1)){printf("walk %u %u %u %u %u\n",jump,origin,target,old,now);return;}   // surface walk/ceiling edges: core-surface
    // Landing minus exit height (the source's battle_move_propagate_destination test).
    {s16 dh=(s16)(u8)(W->nav.queue[k^1][target]+(W->blk[target]&14))-(s16)W->nav.queue[k][origin];
     // Steep panels: battle_move_init_source_panel / battle_move_propagate_destination (return 3).
     u8 steep=((W->blk[origin]&1)&&W->nav.queue[k][origin]>=W->nav.queue[k^1][origin])||
              ((W->blk[target]&1)&&W->nav.queue[k][target]>W->nav.queue[k^1][target]);
     printf("%u %u %u %u %u %u %u %d %u ",jump,origin,target,old,now,shift[k],height,dh,steep);}
    for(i=0;i<8;i++)printf("%02x",middle[i]);putchar('\n');
}
int main(int argc,char **argv)
{
    const u8 *surface;u8 needed,jump;u16 t;sw_init(argc>1?(u16)atoi(argv[1]):201);
    if(!W||!W->surface)return 3;surface=W->surface;needed=W->surface_flags;
    for(jump=1;jump<=4;jump++) {
        W->surface=0;W->surface_flags=0;edges(jump);
        for(t=0;t<MAP_N;t++)memcpy(previous[t],EDGE_ROW(t),32);memcpy(counts,W->en,MAP_N);
        W->surface=surface;W->surface_flags=needed;edges(jump);
        for(t=0;t<MAP_N;t++) {
            u8 i;
            for(i=0;i<counts[t];i+=2)if(!contains(EDGE_ROW(t),W->en[t],previous[t][i],previous[t][i+1]))
                emit(t,previous[t][i],previous[t][i+1],1,0,jump);
            for(i=0;i<W->en[t];i+=2)if(!contains(previous[t],counts[t],EDGE_ROW(t)[i],EDGE_ROW(t)[i+1]))
                emit(t,EDGE_ROW(t)[i],EDGE_ROW(t)[i+1],0,1,jump);
        }
    }
    return 0;
}
