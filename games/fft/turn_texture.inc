// Native orthographic turn mapper. Affine 8.8 UVs, bounded byte depth, no floats.
static const u8 *tt_uv,*tt_atlas[16];
#ifdef FFT_TURN_TEXTURE_SURFACES
static const u8 *tt_surface;
#endif
static u8 tt_ready;
static s16 tt_cx,tt_cz,tt_px,tt_py;
static u16 tt_faces,tt_pixels,tt_fallbacks;
#define TT_POINTS ((PScreen *)W->bg)
#define TT_DEPTH ((u8 *)W->status)
#ifdef FFT_TURN_TEXTURE_QUARTER_X
#ifdef FFT_TURN_TEXTURE_HALF_Y
#error The 40x100 experiment cannot also use half-height scenery
#endif
#define TT_WIDTH 40
#define TT_HEIGHT 100
#define TT_X_SHIFT 2
#define TT_Y_SHIFT 0
#define TT_COVER_Y_SHIFT 0
// The 500 projected vertices fit before byte 3000 in the 4000-byte background.
#define TT_LEFT ((s16 *)((u8 *)W->bg+3000))
#define TT_RIGHT ((s16 *)((u8 *)W->bg+3200))
#else
#define TT_WIDTH 80
#define TT_X_SHIFT 1
#define TT_COVER_Y_SHIFT 1
#define TT_LEFT span_l
#define TT_RIGHT span_r
#ifdef FFT_TURN_TEXTURE_HALF_Y
#define TT_HEIGHT 25
#define TT_Y_SHIFT 2
#else
#define TT_HEIGHT 50
#define TT_Y_SHIFT 1
#endif
#endif
#define TT_ROW_BYTES (TT_WIDTH/8)
#ifndef __m68k__
#include <stdio.h>
static u8 *tt_files[5];
static const u8 *tt_load(const char *name,u8 slot,u16 expected)
{
    FILE *f; unsigned size;
    if(tt_files[slot]) {free(tt_files[slot]); tt_files[slot]=RT_NULL;}
    {char path[32]; snprintf(path,sizeof path,"%s.bin",name); f=fopen(path,"rb");}
    if(!f) return RT_NULL;
    fseek(f,0,SEEK_END); size=(unsigned)ftell(f); rewind(f);
    if(size==expected) {
        tt_files[slot]=malloc(size);
        if(tt_files[slot] && fread(tt_files[slot],1,size,f)!=size) {free(tt_files[slot]); tt_files[slot]=RT_NULL;}
    }
    fclose(f); return tt_files[slot];
}
#else
static const u8 *tt_load(const char *name,u8 slot,u16 expected)
{
    u16 size; const u8 *p=rt_file(name,&size); (void)slot;
    return p && size>=expected ? p:RT_NULL; // AMS size also includes the OTH/dat trailer.
}
#endif

static void tt_init(void)
{
    const u8 *pages[3]; u8 i;
    tt_ready=0; tt_uv=tt_load("fftuv",0,6936);
    pages[0]=tt_load("fftat0",1,57344); pages[1]=tt_load("fftat1",2,57344); pages[2]=tt_load("fftat2",3,16384);
    if(!pbank || !tt_uv || !pages[0] || !pages[1] || !pages[2]
       || RD16(tt_uv,0)!=0x5454 || RD16(tt_uv,1)!=1 || RD16(tt_uv,2)!=RD16(pbank,1) || RD16(tt_uv,3)!=16) return;
    for(i=0;i<16;i++) tt_atlas[i]=pages[i/7]+(u16)(i%7)*8192U;
#ifdef FFT_TURN_TEXTURE_SURFACES
#ifdef FFT_TURN_TEXTURE_QUARTER_X
    tt_surface=tt_load("fftsurfx",4,21306);
#else
    tt_surface=tt_load("fftsurf",4,31270);
#endif
    if(!tt_surface || RD16(tt_surface,0)!=0x5453 || RD16(tt_surface,1)!=1
       || RD16(tt_surface,2)!=RD16(pbank,0) || RD16(tt_surface,3)!=RD16(pbank,1)
       || RD16(tt_surface,4)!=509 || RD16(tt_surface,5)!=MAP_W
       || RD16(tt_surface,6)!=MAP_H || RD16(tt_surface,7)!=22) return;
#endif
    tt_ready=1;
}

static PScreen tt_project(s16 x,s16 y,s16 z)
{
    PScreen p; s16 dx=x-tt_cx,dz=z-tt_cz;
    s16 across=(s16)((muls16(dx,pc)+muls16(dz,ps))>>7);
    s16 forward=(s16)((muls16(dx,ps)-muls16(dz,pc))>>7);
    p.x=tt_px+(s16)(muls16(across,155)>>8);
    p.y=tt_py-(y>>1)-(s16)(muls16(forward,77)>>8);
    p.q=128-(forward>>2)+(y>>2);
    if(p.q<1) p.q=1; if(p.q>254) p.q=254;
    return p;
}

// Solve one attribute plane per triangle. No division/multiplication in the pixel loop.
static u8 tt_gradient(s16 a,s16 b,s16 c,s16 x1,s16 y1,s16 x2,s16 y2,s16 det,s16 *dx,s16 *dy)
{
    s32 nx=muls16(b-a,y2)-muls16(c-a,y1),ny=muls16(x1,c-a)-muls16(x2,b-a);
    s32 limit=muls16(-det,127);
    if(nx>limit || nx< -limit || ny>limit || ny< -limit) {*dx=*dy=0; return 0;}
    *dx=divs32_16(nx*256,det); *dy=divs32_16(ny*256,det);
    return 1;
}

// Refer to projected vertices in place. Struct copies called AMS memcpy per corner.
static void tt_triangle(const PScreen *const *p,const u8 *uv,u8 shade,u8 count)
{
    s16 x1=p[1]->x-p[0]->x,y1=p[1]->y-p[0]->y,x2=p[2]->x-p[0]->x,y2=p[2]->y-p[0]->y;
    s32 determinant=muls16(x1,y2)-muls16(x2,y1);
    s16 lo=TT_HEIGHT,hi=0,i,y,det,udx=0,udy=0,vdx=0,vdy=0,qdx,qdy;
    s32 ur=0,vr=0,qr;
    const u8 *texture=RT_NULL;
    if(determinant>=0 || determinant< -32767) return;
    for(i=0;i<count;i++) {if(p[i]->y<lo) lo=p[i]->y; if(p[i]->y>hi) hi=p[i]->y;}
    if(lo<0) lo=0; if(hi>TT_HEIGHT) hi=TT_HEIGHT; if(lo>=hi) return;
    {s16 left=p[0]->x,right=left;
     for(i=1;i<count;i++) {if(p[i]->x<left) left=p[i]->x; if(p[i]->x>right) right=p[i]->x;}
     if(right<=0 || left>=TT_WIDTH) return;}
    det=(s16)determinant; tt_faces++;
#ifndef FFT_TURN_TEXTURE_SPANS
    for(y=lo;y<hi;y++) {TT_LEFT[y-lo]=32767; TT_RIGHT[y-lo]= -32767;}
#endif
    for(i=0;i<count;i++) {
        s16 next=i+1; const PScreen *a,*b; s16 ya,yb,dx; s32 x;
#ifdef FFT_TURN_TEXTURE_SPANS
        s16 *edge; u8 left;
#endif
        if(next==count) next=0;
#ifdef FFT_TURN_TEXTURE_MERGE
        // Quad storage retains the first triangle basis; boundary is 0,1,3,2.
        a=p[count==4 && i>=2 ? 5-i:i]; b=p[count==4 && next>=2 ? 5-next:next];
#else
        a=p[i]; b=p[next];
#endif
        if(a->y==b->y) continue;
#ifdef FFT_TURN_TEXTURE_SPANS
        left=a->y<b->y;
#endif
        if(a->y>b->y) {const PScreen *swap=a;a=b;b=swap;}
        ya=a->y<lo ? lo:a->y; yb=b->y>hi ? hi:b->y; if(ya>=yb) continue;
        dx=divs32_16((s32)(b->x-a->x)*256,b->y-a->y);
        x=(s32)a->x*256+muls16(dx,ya-a->y)+(dx>>1)+128;
#ifdef FFT_TURN_TEXTURE_SPANS
        // A clockwise convex contour has exactly one directed edge per side/row.
        edge=(left ? TT_LEFT:TT_RIGHT)+(ya-lo);
        for(y=ya;y<yb;y++,x+=dx) *edge++=(s16)(x>>8);
#else
        for(y=ya;y<yb;y++,x+=dx) {
            s16 xx=(s16)(x>>8),row=y-lo;
            if(xx<TT_LEFT[row]) TT_LEFT[row]=xx; if(xx>TT_RIGHT[row]) TT_RIGHT[row]=xx;
        }
#endif
    }
    if(!tt_gradient(p[0]->q,p[1]->q,p[2]->q,x1,y1,x2,y2,det,&qdx,&qdy)) tt_fallbacks++;
#ifndef FFT_TURN_TEXTURE_FLAT
    if(uv[6]!=255) {
        if(tt_gradient(uv[0],uv[2],uv[4],x1,y1,x2,y2,det,&udx,&udy)
           && tt_gradient(uv[1],uv[3],uv[5],x1,y1,x2,y2,det,&vdx,&vdy)) texture=tt_atlas[uv[6]];
        else tt_fallbacks++;
    }
#endif
    qr=(s32)p[0]->q*256+muls16(qdy,lo-p[0]->y)+(((s32)qdx+qdy)>>1);
    if(texture) {
        ur=(s32)uv[0]*256+muls16(udy,lo-p[0]->y)+(((s32)udx+udy)>>1);
        vr=(s32)uv[1]*256+muls16(vdy,lo-p[0]->y)+(((s32)vdx+vdy)>>1);
    }
    for(y=lo;y<hi;y++,qr+=qdy,ur+=udy,vr+=vdy) {
        s16 x=TT_LEFT[y-lo],end=TT_RIGHT[y-lo]; u16 u,v,off; s32 q; u8 *depth;
        if(x<0) x=0; if(end>TT_WIDTH) end=TT_WIDTH; if(x>=end) continue;
        q=qr+muls16(qdx,x-p[0]->x);
        u=(u16)(ur+muls16(udx,x-p[0]->x)); v=(u16)(vr+muls16(vdx,x-p[0]->x));
        depth=TT_DEPTH+y*TT_WIDTH+x; off=y*TT_ROW_BYTES+((u16)x>>3);
        while(x<end) {
            u8 bit=0x80>>(x&7),mask=0,l=0,d=0;
            do {
                s16 z=(s16)(q>>8);
                if(z>*depth && (u16)z<256) {
                    u8 colour=shade;
                    if(texture) {
                        u16 index=((v>>8)&127)*64+((u>>9)&63);
                        u8 packed=texture[index]; colour=u&256 ? packed&15:packed>>4;
                    }
                    if(colour<4) {
                        *depth=(u8)z; mask|=bit;
                        if(colour&1) l|=bit; if(colour&2) d|=bit;
                        tt_pixels++;
                    }
                }
                q+=qdx; u+=udx; v+=vdx; depth++; x++; bit>>=1;
            } while(bit && x<end);
            if(mask) {half[0][off]=(half[0][off]&~mask)|l; half[1][off]=(half[1][off]&~mask)|d;}
            off++;
        }
    }
}

#ifdef FFT_TURN_TEXTURE_SURFACES
static void tt_original(u16 face)
{
    const u8 *src=pbank+2120+RD16(pbank,0)*6U+((u32)face<<3);
    const PScreen *p[3]; u8 k;
    for(k=0;k<3;k++) p[k]=&TT_POINTS[RD16(src,k)];
    tt_triangle(p,tt_uv+8+((u32)face<<3),src[6],3);
}

static void tt_surfaces(u8 yaw)
{
    const u8 *record=tt_surface+144,*list;
    u16 count,slot=yaw>>2;
#ifdef FFT_TURN_TEXTURE_HALF_Y
    slot+=16;
#endif
#ifdef FFT_TURN_TEXTURE_ALL_FACES
    count=RD16(tt_surface,4); list=RT_NULL;
#else
    list=tt_surface+RD16(tt_surface+16,slot*2);
    count=RD16(tt_surface+16,slot*2+1);
#endif
    while(count--) {
        const u8 *r;
#ifdef FFT_TURN_TEXTURE_ALL_FACES
        r=record; record+=22;
#else
        r=record+RD16(list,0)*22U; list+=2;
#endif
#ifdef FFT_TURN_TEXTURE_MERGE
        if(r[17]==4) {
            const PScreen *p[4]; s16 k,x1,y1,x2,y2; s32 a,b;
            for(k=0;k<4;k++) p[k]=&TT_POINTS[RD16(r,k)];
            x1=p[1]->x-p[0]->x; y1=p[1]->y-p[0]->y;
            x2=p[2]->x-p[0]->x; y2=p[2]->y-p[0]->y;
            a=muls16(x1,y2)-muls16(x2,y1);
            x1=p[3]->x-p[1]->x; y1=p[3]->y-p[1]->y;
            x2=p[2]->x-p[1]->x; y2=p[2]->y-p[1]->y;
            b=muls16(x1,y2)-muls16(x2,y1);
            if(a<0 && b<0 && a>=-32767 && b>=-32767
#ifdef FFT_TURN_TEXTURE_SPANS
               // Rounding can collapse a corner: directed spans require strict convexity.
               && muls16(p[1]->x-p[0]->x,p[3]->y-p[0]->y)-muls16(p[3]->x-p[0]->x,p[1]->y-p[0]->y)<0
               && muls16(p[3]->x-p[0]->x,p[2]->y-p[0]->y)-muls16(p[2]->x-p[0]->x,p[3]->y-p[0]->y)<0
#endif
              ) {
                tt_triangle(p,r+8,r[16],4); continue;
            }
        }
#endif
        tt_original(RD16(r,9));
        if(r[17]==4) tt_original(RD16(r,10));
    }
}
#endif

#ifdef FFT_TURN_TEXTURE_QUARTER_X
static void __attribute__((noinline)) tt_expand_horizontal(void)
{
    const u8 *hl=half[0],*hd=half[1]; u8 *dl=rt_light,*dd=rt_dark; u16 y,j;
    for(y=0;y<100;y++,dl+=RT_PBYTES,dd+=RT_PBYTES)
        for(j=0;j<5;j++) {
            u16 l=dbl[*hl++],d=dbl[*hd++];
            // Two applications of the existing bit-doubling LUT expand 8 to 32 bits.
            WR16(dl,j*2,dbl[l>>8]); WR16(dl,j*2+1,dbl[l&255]);
            WR16(dd,j*2,dbl[d>>8]); WR16(dd,j*2+1,dbl[d&255]);
        }
}
#endif

static void tt_turn(void)
{
    u16 i,nv=RD16(pbank,0); const u8 *src=pbank+2120;
#ifndef FFT_TURN_TEXTURE_SURFACES
    u16 nf=RD16(pbank,1); const u8 *uv=tt_uv+8;
#endif
    s16 cu,cv; u8 yaw=(56-16*st.rot-st.turn*(st.turn_t+1)*4)&63,order_u[NUNIT],j;
    PScreen feet[NUNIT];
    tt_faces=tt_pixels=tt_fallbacks=0;
    ps=psine(yaw); pc=psine(yaw+16); tt_cx=st.cx*28+14; tt_cz=st.cz*28+14;
    view_of(st.rot,st.cx,st.cz,&cu,&cv);
    tt_px=scene_x(st.rot,cu,cv)-st.camx; tt_py=scene_y(cu,cv)+HH-st.camy;
    ZB(18); memset(half,0,sizeof half); memset(TT_DEPTH,0,4000);
    for(i=0;i<nv;i++,src+=6) {
        PScreen p=tt_project((s16)RD16(src,0),(s16)RD16(src,1),(s16)RD16(src,2));
        p.x>>=TT_X_SHIFT; p.y>>=TT_Y_SHIFT; TT_POINTS[i]=p;
    }
    ZE(18); ZB(19);
#ifdef FFT_TURN_TEXTURE_SURFACES
    tt_surfaces(yaw);
#else
    for(i=0;i<nf;i++,src+=8,uv+=8) {
        const PScreen *p[3]; u8 k;
        for(k=0;k<3;k++) p[k]=&TT_POINTS[RD16(src,k)];
        tt_triangle(p,uv,src[6],3);
    }
#endif
    ZE(19); ZB(20);
#ifdef FFT_TURN_TEXTURE_HALF_Y
    // Expand backwards so the source rows survive; actor depth stays at 80x50.
    for(i=49;i;i--) {
        memcpy(half[0]+i*10,half[0]+(i>>1)*10,10);
        memcpy(half[1]+i*10,half[1]+(i>>1)*10,10);
        memcpy(TT_DEPTH+i*80,TT_DEPTH+(i>>1)*80,80);
    }
#endif
#ifdef FFT_TURN_TEXTURE_QUARTER_X
    tt_expand_horizontal();
#else
    turn_expand();
#endif
    ZE(20); ZB(21);
    for(i=0;i<NUNIT;i++) {
        const Unit *u=&st.unit[i]; PScreen p;
        if(u->x<MAP_W && u->z<MAP_H) p=tt_project(u->x*28+14,map_tiles[u->z*MAP_W+u->x].stand*6,u->z*28+14);
        else {p.x=p.y= -1000;p.q=0;}
        feet[i]=p;
        for(j=(u8)i;j && feet[order_u[j-1]].q>p.q;j--) order_u[j]=order_u[j-1];
        order_u[j]=(u8)i;
    }
    for(j=0;j<NUNIT;j++) {
        u8 who=order_u[j],r; PScreen p=feet[who]; Cover c;
        if(p.x< -8 || p.x>=168 || p.y<0 || p.y>=130) continue;
        memset(&c,0,sizeof c); c.r0=0;c.r1=UNIT_SH-1;c.wet=unit_in_water(who);
        for(r=0;r<UNIT_SH;r++) {
            s16 y=p.y-UNIT_FOOT+r,x=p.x-8; u16 mask=0,bit=0x8000;
            if((u16)y<100) {
                const u8 *row=TT_DEPTH+(y>>TT_COVER_Y_SHIFT)*TT_WIDTH; u8 k;
                for(k=0;k<16;k++,x++,bit>>=1)
                    if((u16)x<160 && p.q+1<row[x>>TT_X_SHIFT]) mask|=bit;
            }
            c.cm[r]=mask;
        }
        draw_unit_covered(p.x,p.y,(u8)(4-((yaw+8)>>4))&3,who,&c);
    }
    ZE(21); W->bg_ok=0; pvalid=0;
}
