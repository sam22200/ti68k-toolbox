// TARGETS4: fill a private manual forecast and its radial display panels.
static u8 area_preview(void)
{
    AreaUi *q=&W->area_ui;const u8 *r,*p;u16 c;u8 i,x,z,h;
    if(st.mode!=M_AREA||!batch_forecast(&q->forecast))return 0;
    r=cj_range(common,q->forecast.ability);x=TILE_X[q->centre];z=TILE_Z[q->centre];
    p=W->surface+((u16)(W->grid_row[z]+x)<<3)+4*node_layer(q->centre);
    h=(u8)core_area_height(p);memset(q->panels,0,sizeof q->panels);
    for(c=0;c<MAP_N;c++){
        s16 dx=(s16)TILE_X[c]-x,dz=(s16)TILE_Z[c]-z;
        if((r[4]&1)&&c==UNIT_CELL(&st.unit[st.act]))continue;
        if(r[1]!=255){
            if((dx<0?-dx:dx)+(dz<0?-dz:dz)>r[1])continue;
            p=W->surface+((u16)(W->grid_row[TILE_Z[c]]+TILE_X[c])<<3);
            if(core_area_layer(p,h,r[2],r[4]&32,W->surface_flags&SURFACE_MOVE_UPPER)!=node_layer((u8)c)+1)continue;
        }
        if(r[4]&192){
            u8 occupied=0,marked=0;
            for(i=0;i<NUNIT;i++)if(magic_target(i)&&UNIT_CELL(&st.unit[i])==c){
                occupied=1;if(q->forecast.mask&((u16)1<<i))marked=1;
            }
            if(occupied&&!marked)continue;
        }
        q->panels[c>>3]|=(u8)(1U<<(c&7));
    }
    memset(st.reach,255,sizeof st.reach);
    for(c=0;c<MAP_N;c++)if(q->panels[c>>3]&(1U<<(c&7)))st.reach[c]=1;
    hl_seq++;q->count=q->row=q->top=0;
    for(i=0;i<NUNIT;i++)if(q->forecast.mask&((u16)1<<i))q->ids[q->count++]=i;
    i=unit_at_cell(q->centre);q->follow_id=i&&!(r[4]&16)?i-1:255;
    q->ready=1;q->dirty=0;q->forecast_dirty=1;q->weather=st.weather;
    return 1;
}
