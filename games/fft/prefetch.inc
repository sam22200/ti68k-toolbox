// Cooperative fixed-camera geometry preparation. Only the selected front preset is kept.
// No tactical state, ISO scratch, displayed plane or actor pose is stored in this cache.
#define PIDLE_READY 7
#define PIDLE_BUDGET 96
#define PIDLE_BYTES (13600UL+1700+3400+sizeof(PIdle))

static void pidle_free(void)
{
    if(!pidle) return;
    if(pidle->depth) free(pidle->depth);
    if(pidle->planes) free(pidle->planes);
    if(pidle->water) free(pidle->water);
    free(pidle); pidle=RT_NULL;
}

static void pidle_init(void)
{
    if(!pidle) {
        pidle=malloc(sizeof *pidle);
        if(!pidle) return;
        memset(pidle,0,sizeof *pidle);
        // Cache only the85 unobscured rows. Status supplies temporary vertices.
        pidle->depth=malloc(13600); pidle->planes=malloc(3400);
        pidle->water=malloc(1700);
        if(!pidle->depth || !pidle->planes || !pidle->water) {
            pidle_free(); return;
        }
    }
    pidle->stage=pidle->delay=0;
}

static void pidle_key(u8 *descriptor)
{
    descriptor[0]=st.unit[U_RAMZA].x; descriptor[1]=st.unit[U_RAMZA].z;
    descriptor[2]=st.unit[U_RAMZA].face; descriptor[3]=st.pzoom; descriptor[4]=st.pwire;
}

static u8 pidle_matches(void)
{
    u8 descriptor[5]; pidle_key(descriptor);
    return pidle && !memcmp(pidle->descriptor,descriptor,sizeof descriptor);
}

static void pidle_work(u16 budget);

static u8 pidle_complete(void)
{
    static const u8 yaw[4]={16,48,32,0};
    if(!pidle || !pidle->stage || !pidle_matches() || st.psolid!=1
        || st.pyaw!=yaw[st.unit[U_RAMZA].face] || st.pside!=6
        || st.pdist!=(st.pzoom ? 80 : 40) || st.ppitch!=(st.pzoom ? 6 : 1)
        || st.ptrans || st.cphase) {
        if(pidle && pidle->stage!=PIDLE_READY) pidle->stage=0;
        return 0;
    }
    while(pidle->stage!=PIDLE_READY) pidle_work(512);
    return 1;
}

static u8 pidle_take(void)
{
    u16 y,x;
    const PlaneWord *l,*d; PlaneWord *dl,*dd;
    if(!pidle_complete()) return 0;
    pcamera();
    memcpy(PDEPTH+8*160,pidle->depth,13600); memcpy(PCACHE+8*20,pidle->water,1700);
    l=(const PlaneWord *)pidle->planes[0]; d=(const PlaneWord *)pidle->planes[1];
    dl=(PlaneWord *)(rt_light+8*30); dd=(PlaneWord *)(rt_dark+8*30);
    for(y=0;y<85;y++) {
        for(x=0;x<5;x++) { dl[x]=*l++; dd[x]=*d++; }
        dl=(PlaneWord *)((u8 *)dl+30); dd=(PlaneWord *)((u8 *)dd+30);
    }
    pfaces=pidle->faces;
    return 1; // keep immutable geometry across ISO return; actors/masks remain live
}

static void pidle_work(u16 budget)
{
    static const u8 yaw[4]={16,48,32,0};
    u8 camera[5],*old_l,*old_d;
    u16 old_faces=pfaces,nv,nf;
    // Save only the camera fields pcamera reads. Battle state and clocks never change.
    camera[0]=st.pyaw; camera[1]=st.ppitch; camera[2]=st.pside;
    camera[3]=st.pdist; camera[4]=plow;
    st.pyaw=yaw[pidle->descriptor[2]]; st.ppitch=st.pzoom ? 6 : 1;
    st.pside=6; st.pdist=st.pzoom ? 80 : 40;
    old_l=rt_light; old_d=rt_dark;
    rt_light=pidle->planes[0]; rt_dark=pidle->planes[1];
    pidle_active=1; pcamera(); pfaces=pidle->faces;
    nv=RD16(pbank,0); nf=RD16(pbank,1);
    ZB(18);
    while(budget>=24 && pidle->stage!=PIDLE_READY) {
        u16 i=pidle->index;
        if(!pidle->stage) {
            pidle->faces=pfaces=0; pidle->row=pidle->end=0;
            pidle->index=0; pidle->stage=1;
            memset(pidle->water,0,1700); memset(pidle->planes,0,3400);
            W->status_i=0; budget=0;
#ifdef FFT_TEXTURE_REVIEW
            memset(review_faces,0,sizeof review_faces); memset(review_edges,0,sizeof review_edges);
#endif
        } else if(pidle->stage==1) {
            u16 n=13600-i;
            if(n>1024) n=1024;
            memset(pidle->depth+i,0,n); pidle->index+=n; budget-=24;
            if(n<1024) { pidle->index=0; pidle->stage=2; }
        } else if(pidle->stage==2) {
            const u8 *p=pbank+2120+nv*6+nf*8+i*8;
            if(i==RD16(pbank,3)) { pidle->index=0; pidle->stage=3; continue; }
            PVISIBLE[i]=(s16)RD16(p,0)*pcamx+(s16)RD16(p,1)*pcamy
                +(s16)RD16(p,2)*pcamz>(s16)RD16(p,3);
            pidle->index++; budget-=8;
        } else if(pidle->stage==3) {
            if(i==nv) { pidle->index=0; pidle->stage=4; continue; }
            pvertex(i); pidle->index++; budget-=8;
        } else if(pidle->stage==4) {
            if(pidle->row<pidle->end) {
                s16 y=pidle->row,a=pidle->spans[0][y],b=pidle->spans[1][y];
                if(y>=8 && y<93 && a<b && b>0 && a<160) {
                    s16 x=a<0 ? 0 : a,end=b>160 ? 160 : b;
                    s16 cost=4+((end-x)>>3),step;
                    if(budget<(u16)cost) break;
                    step=divs32_16((s32)pidle->spans[3][y]-pidle->spans[2][y],b-a);
                    cspan(y,x,end,pidle->spans[2][y]+muls16(step,x-a),step,pidle->shade);
                    budget-=cost;
                }
                pidle->row++;
            } else {
                if(i==nf) { pidle->index=0; pidle->stage=5; continue; }
                pface(i); pidle->index++;
                budget-=pidle->row<pidle->end ? 24 : 4;
            }
        } else if(pidle->stage==5) {
            const u8 *p=pbank+2120+nv*6+nf*8+RD16(pbank,3)*8;
#ifdef FFT_TEXTURE_REVIEW
            review_face=0;
#endif
            if(i==RD16(p,0)) { pidle->stage=6; continue; }
            { u16 cost=fixed_edge_range(i,i+1);
              budget=budget>cost ? budget-cost : 0; }
            pidle->index++;
        } else {
#ifdef FFT_TEXTURE_PREVIEW
            if(!st.pwire) preview_texture(0); // PC reference only; never in the native budget
#endif
            pidle->stage=PIDLE_READY;
        }
    }
    pidle->faces=pfaces;
    ZE(18);
    pidle_active=0; rt_light=old_l; rt_dark=old_d;
    st.pyaw=camera[0]; st.ppitch=camera[1]; st.pside=camera[2];
    st.pdist=camera[3]; plow=camera[4]; pfaces=old_faces;
}

static void pidle_interrupt(void)
{
    // These renderers overwrite Status, which holds a partial job's vertices.
    if(pidle && pidle->stage<PIDLE_READY && (st.turn || st.mode==M_STATUS || st.mode==M_ORDER))
        pidle->stage=pidle->delay=0;
}

static void pidle_tick(u16 budget)
{
    if(!pidle || !pbank || !W) return;
    if(st.perspective || st.turn || st.ptrans || st.cphase || st.ai || st.banner || st.morph_mask
        || st.want!=0xFF || st.psolid!=1 || st.mode!=M_MENU || rt_keys
        || st.unit[U_RAMZA].x>=MAP_W || st.unit[U_RAMZA].z>=MAP_H) {
        pidle->delay=0; return;
    }
    if(!pidle_matches()) { pidle_key(pidle->descriptor); pidle->stage=0; pidle->delay=0; }
    if(pidle->stage==PIDLE_READY || pidle->delay++<16) return;
    pidle->delay=16;
    pidle_work(budget);
}
