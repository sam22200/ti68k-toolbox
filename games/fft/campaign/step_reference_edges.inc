// Frozen accepted endpoint graph for cache equivalence/cost checks only.
static void step_reference_edges(u8 jump)
{
    u16 t;u8 k,j,layers=1;
#ifdef FFT_CHAPTER
    u8 surface_layers=(W->surface_flags&SURFACE_MOVE_UPPER)?2:1;
    if(W->surface_flags&SURFACE_GRID_UPPER)layers=2;
#endif
    W->ej=jump;
    for(t=0;t<MAP_N;t++) {
        u8 x=TILE_X[t],z=TILE_Z[t],h=W->hl[t],n=0,*e=EDGE_ROW(t);
        for(k=0;k<4;k++) {
            u8 room=k==0?MAP_W-1-x:k==1?x:k==2?MAP_H-1-z:z;
            for(j=0;j<=(jump>=2)&&j<room;j++) {
                s16 nx=x+dir_x[k]*(j+1),nz=z+dir_z[k]*(j+1);u8 layer;
                for(layer=0;layer<layers;layer++) {
                    u16 next=node_at(nx,nz,layer),via=t;s16 dh;u8 kind;
                    if(next>=MAP_N||!(W->hw[next]&1))continue;
#ifdef FFT_CHAPTER
                    if(W->surface) {
                        static const u8 shift[4]={0,2,6,4};
                        const u8 *src=W->surface+((u16)(W->grid_row[z]+x)<<3);
                        const u8 *dst=W->surface+((u16)(W->grid_row[nz]+nx)<<3);
                        // Current units use ordinary terrain movement. Water/
                        // Float abilities require explicit movement policy data.
                        if(!step_clear(src,dst,node_layer((u8)t),layer,shift[k],jump,0,6,
                                       surface_layers,W->surface_flags&SURFACE_ROOM,j))continue;
                    } else
#endif
                    {
                        dh=(s16)W->hs[next]-h;
                        if(dh>2*jump||dh<-2*jump||(j&&(dh<0||dh>jump)))continue;
                    }
                    kind=(u8)(j|(W->hw[next]&2));
                    if(j) {
                        via=node_at(x+dir_x[k],z+dir_z[k],node_layer((u8)t));
#ifdef FFT_CHAPTER
                        if(W->surface) {
                            static const u8 shift[4]={0,2,6,4};
                            const u8 *floor=W->surface+((u16)(W->grid_row[z]+x)<<3)+4*node_layer((u8)t);
                            const u8 *middle=W->surface+((u16)(W->grid_row[z+dir_z[k]]+x+dir_x[k])<<3);
                            u8 exit=surface_edge(floor,shift[k]);
                            if(!surface_jump_layers(middle,shift[k],exit,6,surface_layers))continue;
                        } else
#endif
                        if(via<MAP_N&&W->hl[via]>W->hs[next])continue;
                        if(via>=MAP_N)kind|=4; // no intermediate node on this floor; presence is explicit
                    }
                    reference_via[t][n>>1]=via<MAP_N?(u8)via:0;
                    e[n++]=(u8)next;e[n++]=kind;
                }
            }
        }
        W->en[t]=n;
    }
}

