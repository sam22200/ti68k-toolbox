// Transition-only proxy: convex faces, painter order and word spans, no pixel depth.
static PVertex cfeet(u8 who);
static const u8 *ultra_bank;
#ifndef __m68k__
#include <stdio.h>
static u8 ultra_pc[8192];
#endif

static void ultra_assets(void)
{
    ultra_bank = RT_NULL;
#ifdef __m68k__
    ultra_bank = rt_file("fftlod",RT_NULL);
#else
    { FILE *f = fopen("fftlod.bin","rb");
      if (f) { if (fread(ultra_pc,1,sizeof ultra_pc,f)>8) ultra_bank=ultra_pc; fclose(f); } }
#endif
    if (ultra_bank && (RD16(ultra_bank,0)>500 || RD16(ultra_bank,1)>128 || RD16(ultra_bank,3)!=1)) ultra_bank=RT_NULL;
}

static u8 ultra_motion(void)
{
    return ultra_bank && (st.ptrans || st.cphase == 1 || st.cphase == 3);
}

static u8 ultra_clip(PVertex *out,const PVertex *in,u8 n,u8 plane)
{
    u8 i,count=0;
    for (i=0;i<n;i++) {
        PVertex a=in[i],b=in[i+1<n ? i+1:0];
        s16 da=pdistance(a,plane),db=pdistance(b,plane);
        if (da>=0) out[count++]=a;
        if ((da<0)!=(db<0)) {
            s16 q=divs32_16((s32)da*256,da-db); PVertex v;
            v.x=a.x+(s16)(muls16(b.x-a.x,q)>>8);
            v.y=a.y+(s16)(muls16(b.y-a.y,q)>>8);
            v.z=a.z+(s16)(muls16(b.z-a.z,q)>>8); out[count++]=v;
        }
    }
    return count;
}

static void ultra_expand(void)
{
    static const u8 twice[16]={0,3,12,15,48,51,60,63,192,195,204,207,240,243,252,255};
    u16 y,x; u8 plane;
    for (plane=0;plane<2;plane++) {
        u8 *dst=plane ? rt_dark:rt_light;
        const u8 *src=PTHALF[plane];
        for (y=0;y<50;y++) for (x=0;x<10;x++) {
            u8 b=*src++; u16 off=y*60+x*2;
            dst[off]=dst[off+30]=twice[b>>4]; dst[off+1]=dst[off+31]=twice[b&15];
        }
    }
}

static void ultra_render(void)
{
    u16 nv=RD16(ultra_bank,0),nf=RD16(ultra_bank,1),i,nvisible=0;
    const u8 *faces=ultra_bank+8+nv*6,*corners=faces+nf*6,*src=ultra_bank+8;
    u16 *sorted=(u16 *)(vbuf+6600);
    s16 *depth=(s16 *)(vbuf+6900);
    static const u16 patterns[2][8]={
        {0,0,0,0,0,0,0,0}, {0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff,0xffff}};
    pcamera(); draw_clear(); memset(PTHALF,0,1000);
    D.l=PTHALF[0]; D.d=PTHALF[1]; D.stride=10; D.ox=D.oy=0;
    D.x0=0; D.y0=4; D.x1=80; D.y1=46;
    for (i=0;i<nv;i++,src+=6) {
        u8 plane,code=0;
        PVERT[i]=ptransform((s16)RD16(src,0),(s16)RD16(src,1),(s16)RD16(src,2));
        for (plane=0;plane<5;plane++) if (pdistance(PVERT[i],plane)<0) code|=1<<plane;
        PCODES[i]=code; if (!code) PPROJECT[i]=pproject(PVERT[i]);
    }
    for (i=0;i<nf;i++) {
        const u8 *f=faces+i*6; u16 first=RD16(f,0),j; s16 sum=0,q,k; u8 both=31;
        for (j=0;j<f[2];j++) { u16 v=RD16(corners,first+j); sum+=PVERT[v].z; both&=PCODES[v]; }
        if (both) continue;
        q=(s16)(muls16(sum,f[5])>>8);
        for (k=nvisible;k && depth[k-1]<q;k--) { depth[k]=depth[k-1]; sorted[k]=sorted[k-1]; }
        sorted[k]=i; depth[k]=q; nvisible++;
    }
    for (i=0;i<nvisible;i++) {
        const u8 *f=faces+sorted[i]*6; u16 first=RD16(f,0);
        PVertex a[32],b[32],*in=a,*out=b; s16 x[32],y[32]; u8 n=f[2],j,plane,either=0;
        s32 area=0;
        for (j=0;j<n;j++) { u16 v=RD16(corners,first+j); a[j]=PVERT[v]; either|=PCODES[v]; }
        for (plane=0;plane<5 && n>=3;plane++) if (either&(1<<plane)) {
            PVertex *swap; n=ultra_clip(out,in,n,plane); swap=in; in=out; out=swap; }
        if (n<3) continue;
        for (j=0;j<n;j++) {
            PScreen p=either ? pproject(in[j]):PPROJECT[RD16(corners,first+j)]; x[j]=p.x; y[j]=p.y; }
        for (j=0;j<n;j++) { u8 next=j+1<n ? j+1:0; area+=muls16(x[j],y[next])-muls16(y[j],x[next]); }
        if (area>=0) continue;
        fill_poly(x,y,n,patterns[f[3]&1],patterns[f[3]&2 ? 1:0]);
    }
    // Cheap actor silhouettes retain projected positions; no pose/mask cache is rebuilt.
    for (i=0;i<NUNIT;i++) {
        PVertex feet=cfeet((u8)i),v=ptransform(feet.x,feet.y,feet.z),head;
        PScreen p,h; s16 height,x[4],y[4],w;
        if (v.z<P_NEAR) continue;
        head=ptransform(feet.x,feet.y+28,feet.z); if (head.z<P_NEAR) continue;
        p=pproject(v); h=pproject(head); height=p.y-h.y;
        if (height<2) continue; if (height>22) height=22; w=(height>>2)+1;
        x[0]=x[3]=p.x-w; x[1]=x[2]=p.x+w;
        y[0]=y[1]=p.y-height; y[2]=y[3]=p.y;
        { s16 bx[4],by[4]; u8 j;
          for (j=0;j<4;j++) { bx[j]=x[j]+(j==0 || j==3 ? -1:1); by[j]=y[j]+(j<2 ? -1:1); }
          fill_poly(bx,by,4,patterns[0],patterns[0]); }
        fill_poly(x,y,4,patterns[1],patterns[1]);
    }
    ultra_expand();
    draw_text(2,0,"ULTRA LOW POLY",F_SMALL,C_BLACK);
    draw_text(2,93,"Transition proxy",F_SMALL,C_BLACK);
    vbuf_rot=0xff;
}
