// Terrain graph scratch is idle until the complete graph becomes searchable.
// Cache each side's ceiling once, then reuse it for outgoing and incoming links.
#ifdef FFT_CHAPTER
#define EDGE_CEILING(k,t) ((k)<2?W->edge_ceiling[(k)&1][t]:W->edge_ceiling_other[(k)&1][t])
#endif
static void edges_tables(u8 jump,u16 first,u16 last)
{
#ifdef FFT_CHAPTER
    static const u8 shifts[4]={0,2,6,4};u16 t;u8 k,layers=(W->surface_flags&SURFACE_MOVE_UPPER)?2:1;
    if(W->surface&&!W->move_kind)jump=step_jump_cap(jump);
    if(W->surface)for(t=first;t<last;t++){
        const u8 *column=W->surface+((u16)(W->grid_row[TILE_Z[t]]+TILE_X[t])<<3);
        const u8 *floor=column+4*node_layer((u8)t);
        W->blk[t]=(u8)(step_steep(floor,jump)|((floor[1]>>5)<<1));
        for(k=0;k<4;k++){
            u8 edge=surface_edge(floor,shifts[k]),ceiling=127;
            W->nav.queue[k][t]=edge;
            if(W->surface_flags&SURFACE_ROOM)ceiling=surface_ceiling_layers(column,shifts[k],edge,layers);
            if(k<2)W->edge_ceiling[k][t]=ceiling;else W->edge_ceiling_other[k-2][t]=ceiling;
        }
    }
#else
    (void)jump;(void)first;(void)last;
#endif
}
// Cold shared geometry; legacy policy0 preserves prior ordinary graphs.
static void edges_rows(u8 jump,u16 first,u16 last)
{
    u16 t;u8 k,j,layers=1;
#ifdef FFT_CHAPTER
    static const u8 shift[4]={0,2,6,4};
    u8 surface_layers=(W->surface_flags&SURFACE_MOVE_UPPER)?2:1;
    u8 body=W->move_kind?move_config.body:6;
    if(W->surface&&!W->move_kind)jump=step_jump_cap(jump);
    if(W->surface_flags&SURFACE_GRID_UPPER)layers=2;
#endif
    for(t=first;t<last;t++) {
        u8 x=TILE_X[t],z=TILE_Z[t],h=W->hl[t],n=0,*e=EDGE_ROW(t);
        for(k=0;k<4;k++) {
            u8 room=k==0?MAP_W-1-x:k==1?x:k==2?MAP_H-1-z:z;
#ifdef FFT_CHAPTER
            u16 intermediate=jump>=2&&room>=2?node_at(x+dir_x[k],z+dir_z[k],node_layer((u8)t)):MAP_N;
            W->via[t][k]=intermediate<MAP_N?(u8)intermediate:0;
#endif
#ifdef FFT_CHAPTER
            u8 exit=0,ceiling=127;
            if(W->surface) {
                exit=W->nav.queue[k][t];
                if((W->blk[t]&1)&&exit>=W->nav.queue[k^1][t])continue;
                if(W->surface_flags&SURFACE_ROOM) {
                    ceiling=EDGE_CEILING(k,t);
                }
            }
#endif
            for(j=0;j<=(jump>=2)&&j<room;j++) {
                s16 nx=x+dir_x[k]*(j+1),nz=z+dir_z[k]*(j+1);u8 layer;
                for(layer=0;layer<layers;layer++) {
                    u16 next=node_at(nx,nz,layer),via=t;s16 dh;u8 kind;
                    if(next>=MAP_N||!(W->hw[next]&1))continue;
#ifdef FFT_CHAPTER
                    if(W->surface) {
                        u8 entry=W->nav.queue[k^1][next];
                        if((W->blk[next]&1)&&W->nav.queue[k][next]>entry)continue;
                        dh=(s16)(u8)(entry+(W->blk[next]&14))-exit;
                        if((j&&dh>0)||dh>2*jump||dh<-2*jump)continue;
                        if(W->surface_flags&SURFACE_ROOM) {
                            u8 above=EDGE_CEILING(k^1,next);
                            if(above<exit+body||ceiling<entry+body||above<entry+body)continue;
                        }
                    } else
#endif
                    {
                        dh=(s16)W->hs[next]-h;
                        if(dh>2*jump||dh<-2*jump||(j&&(dh<0||dh>jump)))continue;
                    }
                    kind=(u8)(j|(W->hw[next]&2));
                    if(j) {
                        #ifdef FFT_CHAPTER
                        via=intermediate;
#else
                        via=node_at(x+dir_x[k],z+dir_z[k],node_layer((u8)t));
#endif
#ifdef FFT_CHAPTER
                        if(W->surface) {
                            const u8 *middle=W->surface+((u16)(W->grid_row[z+dir_z[k]]+x+dir_x[k])<<3);
                            if(!surface_jump_layers(middle,shift[k],exit,body,surface_layers))continue;
                        } else
#endif
                        if(via<MAP_N&&W->hl[via]>W->hs[next])continue;
                        if(via>=MAP_N)kind|=4; // no intermediate node on this floor; presence is explicit
                    }
                    #ifdef FFT_CHAPTER
                    if(j)kind|=(u8)(k<<6);
#else
                    W->evia[t][n>>1]=via<MAP_N?(u8)via:0;
#endif
                    e[n++]=(u8)next;e[n++]=kind;
                }
            }
        }
        W->en[t]=n;
    }
}


static void edges(u8 jump)
{
    edges_tables(jump,0,MAP_N);edges_rows(jump,0,MAP_N);
#ifdef FFT_CHAPTER
    W->ej=W->move_kind?st.unit[W->move_who-1].jump:W->surface?step_jump_cap(jump):jump;
    W->edge_phase=0;
#else
    W->ej=jump;
#endif
}
#ifdef FFT_CHAPTER
#undef EDGE_CEILING
#endif
