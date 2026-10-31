#include "proj.h"
#include <string.h>
#ifdef SP_PROFILE
#include "../../../tools/m68kbench/bench.h"
#define ZB(n) BENCH_BEGIN(n)
#define ZE(n) BENCH_END(n)
#else
#define ZB(n)
#define ZE(n)
#endif

#ifdef __m68k__
#define U16(p) (*(const u16 *)(p))
#else
#define U16(p) ((u16)((u16)(p)[0]<<8|(p)[1]))
#endif

u8 sp_bind(SpScene *s,const u8 *bank,u16 bs,const u8 *map,u16 ms)
{
    u16 count,total,i;const u8 *p,*end;
    memset(s,0,sizeof *s);
    if(!bank||!map||bs<24||ms<16||memcmp(bank,"SPG1",4)||memcmp(map,"SPM1",4))return 0;
    s->bank=bank;s->map=map;
    if(U16(bank+4)>255)return 0;
    s->piece_count=(u8)U16(bank+4);s->patterns=bank[7];
    s->shapes_off=U16(bank+8);s->tops_off=U16(bank+10);s->units_off=U16(bank+12);
    s->caps_off=U16(bank+14);s->directory=U16(bank+16);total=U16(bank+20);
    s->width=map[4];s->height=map[5];s->rows=U16(map+6);
    count=(u16)(s->width*s->height);s->plane=(u16)(s->rows*40);
    if(!count||count>255||!s->rows||s->rows>218||U16(map+8)!=count||ms<16+count*16||
       bs<total||(u32)U16(bank+18)+(u32)s->patterns*64>total||
       (u32)s->directory+(u32)s->piece_count*4>total)return 0;
    s->textures=bank+U16(bank+18);
    p=bank+s->directory+4*s->piece_count;end=s->textures;
    for(i=0;i<s->piece_count;i++){
        u8 rows=bank[s->directory+4*i];
        s->piece[i]=p;
        while(rows--){if(p+2>end)return 0;p+=2+p[1];}
        if(p>end)return 0;
    }
    for(i=0;i<count;i++){            // pattern indices must stay inside the bank
        const u8 *t=map+16+(i<<4);u8 k;
        for(k=4;k<13;k++)if(t[k]>=s->patterns)return 0;
    }
    return 1;
}

// ones[k]: the low 32-k bits set; a row mask is ones[s]^ones[s+n], no variable shift.
static const u32 ones[33]={
    0xFFFFFFFFUL,0x7FFFFFFFUL,0x3FFFFFFFUL,0x1FFFFFFFUL,0x0FFFFFFFUL,0x07FFFFFFUL,0x03FFFFFFUL,0x01FFFFFFUL,
    0x00FFFFFFUL,0x007FFFFFUL,0x003FFFFFUL,0x001FFFFFUL,0x000FFFFFUL,0x0007FFFFUL,0x0003FFFFUL,0x0001FFFFUL,
    0x0000FFFFUL,0x00007FFFUL,0x00003FFFUL,0x00001FFFUL,0x00000FFFUL,0x000007FFUL,0x000003FFUL,0x000001FFUL,
    0x000000FFUL,0x0000007FUL,0x0000003FUL,0x0000001FUL,0x0000000FUL,0x00000007UL,0x00000003UL,0x00000001UL,0};

typedef union { u32 l; u8 b[4]; } SpRow;

// Few live values, so GCC keeps the texel pointer and texture in registers.
static __attribute__((noinline)) u32 sp_texels(const u8 *p,const u8 *tex,u8 n,u32 *dark,u8 align)
{
    u32 lv=0,dv=0;
    do{u8 t=tex[*p++];lv+=lv;dv+=dv;if(t&0x80)lv++;if(t&0x40)dv++;}while(--n);
    *dark=dv<<align;return lv<<align;
}
#ifdef __m68k__
#define BYTE(r,k) ((r).b[k])
#else
#define BYTE(r,k) ((u8)((r).l>>(24-8*(k))))   // host is little-endian
#endif

static void sp_piece(SpScene *s,u8 id,const u8 *tex,s16 x,s16 y,u8 depth,u8 *col,u8 *cov,u8 *seen)
{
    const u8 *r,*p;u8 rows,shift;s16 bx,yy;u16 plane=s->plane;
    u8 *cov1=cov+plane,*cov2=cov1+plane,*cov3=cov2+plane,*dark=col?col+plane:0;
    if(id==0xFF)return;
    r=s->bank+s->directory+(id<<2);
    x+=(s8)r[1];y+=(s8)r[2];rows=r[0];p=s->piece[id];
    bx=x>>3;shift=(u8)x&7;s->pieces++;
#ifdef __m68k__
    if(col&&bx>=0&&bx<=36){             // no horizontal clipping: proj68k.s
        SpRows a;s16 base;
        for(;rows&&y<0;rows--,y++)p+=2+p[1];
        if(y>=(s16)s->rows)return;
        if(y+rows>(s16)s->rows)rows=(u8)(s->rows-y);
        if(!rows)return;
        base=(s16)(y*40)+bx;
        a.p=p;a.tex=tex;a.seen=seen+base;a.light=col+base;a.dark=dark+base;
        a.cov0=cov+base;a.cov1=cov1+base;a.twoplane=(u16)(2*plane);
        a.depth=depth;a.shift=shift;a.rows=rows;
        ZB(23);sp_rows_asm(&a);ZE(23);
        return;
    }
#endif
    for(yy=y;rows;rows--,yy++){
        SpRow l,d,m;u8 n,st,k,any,vis[4];s16 first,last,base;
        st=(u8)(*p++ +shift);n=*p++;
        if(!n)continue;
        if((u16)yy>=s->rows){p+=n;continue;}
        m.l=ones[st]^ones[st+n];first=st>>3;last=(st+n-1)>>3;
        if(bx+first<0)first=-bx;
        if(bx+last>39)last=39-bx;
        base=(s16)(yy*40)+bx;any=0;
        // Visible bits first: a fully hidden row never reads its texels.
        for(k=(u8)first;(s16)k<=last;k++)any|=vis[k]=BYTE(m,k)&(u8)~seen[base+k];
        if(!any){p+=n;continue;}
        ZB(20);
        l.l=sp_texels(p,tex,n,&d.l,(u8)(32-st-n));p+=n;
        ZE(20);ZB(21);
        for(k=(u8)first;(s16)k<=last;k++){
            u8 visible=vis[k];u16 at;
            if(!visible)continue;
            at=(u16)(base+k);seen[at]|=visible;
            if(col){col[at]|=BYTE(l,k)&visible;dark[at]|=BYTE(d,k)&visible;}
            // Same two-layer coverage packing as sb_sprite.
            if(!cov1[at]||cov[at]==depth){cov[at]=depth;cov1[at]|=visible;}
            else if(!cov3[at]||cov2[at]==depth){cov2[at]=depth;cov3[at]|=visible;}
        }
        ZE(21);
    }
}

static const u8 *sp_tile(const SpScene *s,u8 rot,s16 u,s16 v)
{
    s16 x,z;
    switch(rot){
    case 0:x=u;z=v;break;
    case 1:x=v;z=s->height-1-u;break;
    case 2:x=s->width-1-u;z=s->height-1-v;break;
    default:x=s->width-1-v;z=u;break;
    }
    if((u16)x>=s->width||(u16)z>=s->height)return 0;
    return s->map+16+(u16)((z*s->width+x)<<4);
}

void sp_compose(SpScene *s,u8 rot,u8 *color,u8 *coverage,u8 *seen)
{
    s16 width,height,diag,u;
    rot&=3;width=rot&1?s->height:s->width;height=rot&1?s->width:s->height;
    s->pieces=0;
    if(color)memset(color,0,(u16)(2*s->plane));
    memset(coverage,0,(u16)(4*s->plane));memset(seen,0,s->plane);
    for(diag=width+height-2;diag>=0;diag--){
        s16 first=diag-height+1,last=diag;
        if(first<0)first=0;if(last>=width)last=width-1;
        for(u=first;u<=last;u++){
            s16 v=diag-u,cx,cy;u8 h[4],lo,key=0,c,side;const u8 *t=sp_tile(s,rot,u,v),*n;
            for(c=0;c<4;c++)h[c]=t[(8-c-rot)&3];
            lo=h[0];for(c=1;c<4;c++)if(h[c]<lo)lo=h[c];
            for(c=0;c<4;c++)key|=(u8)((h[c]-lo)<<(c<<1));
            cx=320-(u-v+height)*12;cy=64+diag*6;
            sp_piece(s,s->bank[s->tops_off+(s->bank[s->shapes_off+key]<<2)+rot],
                     s->textures+(t[4]<<6),cx,cy-lo*6,(u8)diag,color,coverage,seen);
            for(side=0;side<2;side++){
                u8 a=side?1:2,b=side?2:3,k=h[a]<h[b]?h[a]:h[b],bottom=0,hh,w0;
                s16 delta=(s16)h[a]-h[b];const u8 *upper,*lower;
                n=sp_tile(s,rot,u+(side?0:1),v+(side?1:0));
                if(n){
                    u8 n0=n[(8-rot)&3],n1=n[(8-(side?3:1)-rot)&3];
                    bottom=n0<n1?n0:n1;
                }
                w0=(u8)((side?7-rot:6-rot)&3);
                upper=s->textures+(t[5+2*w0]<<6);lower=s->textures+(t[6+2*w0]<<6);
                if(delta)sp_piece(s,s->bank[s->caps_off+side*7+delta+3],upper,cx,cy-k*6,(u8)diag,color,coverage,seen);
                for(hh=k;hh>bottom;hh--)
                    sp_piece(s,s->bank[s->units_off+side],hh==k?upper:lower,cx,cy-(hh-1)*6,(u8)diag,color,coverage,seen);
            }
        }
    }
}
