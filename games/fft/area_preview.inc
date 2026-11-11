// TARGETS4: fill a private manual forecast and its radial display panels.
static u8 area_preview(void)
{
    AreaUi *q=&W->area_ui;const u8 *r;u16 c;u8 i;
    if(st.mode!=M_AREA||!batch_forecast(&q->forecast))return 0;
    r=cj_range(common,q->forecast.ability);memset(q->panels,0,sizeof q->panels);
    if(q->forecast.follow==MATH_BIND||(r[6]&32)){   // Math; direct targeting: the recipients' panels
        for(i=0;i<NUNIT;i++)if(q->forecast.mask&((u16)1<<i)){
            c=UNIT_CELL(&st.unit[i]);q->panels[c>>3]|=(u8)(1U<<(c&7));
        }
    }else for(c=0;c<MAP_N;c++){
        if(!area_cell(st.act,q->centre,r,(u8)c))continue;
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
    i=unit_at_cell(q->centre);q->follow_id=q->forecast.follow!=MATH_BIND&&i&&!(r[4]&16)?i-1:255;
    q->ready=1;q->dirty=0;q->forecast_dirty=1;q->weather=st.weather;
    return 1;
}
