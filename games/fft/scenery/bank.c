#include "bank.h"
#include <string.h>

#ifdef __m68k__
#include "../../../lib/unpack68k.h"
#define U16(p) (*(const u16 *)(p))
#define U32(p) (*(const u32 *)(p))
#define sb_unpack zx0_asm
#else
#define U16(p) ((u16)((u16)(p)[0]<<8|(p)[1]))
#define U32(p) ((u32)U16(p)<<16|U16((p)+2))
// Same portable debug decoder used by fft.c. Native always calls the existing
// verified 68000 implementation; this is not a new native decompressor.
static int sb_bit(const u8 **s, u16 *bb)
{
    u16 t;
    if (*bb == 0x8000) *bb = (u16)(*(*s)++ << 8 | 0x80);
    t = *bb; *bb = (u16)(*bb << 1);
    return (t & 0x8000) != 0;
}
static u16 sb_gamma(const u8 **s, u16 *bb)
{
    u16 x = 1;
    while (!sb_bit(s, bb)) { x += x; if (sb_bit(s, bb)) x++; }
    return x;
}
static u8 *sb_unpack(const u8 *s, u8 *d)
{
    u16 dist = 1, n, bb = 0x8000;
    for (;;) {
        for (n = sb_gamma(&s, &bb); n; n--) *d++ = *s++;     // literals
        if (!sb_bit(&s, &bb)) {                                // repeat the last offset
            for (n = sb_gamma(&s, &bb); n; n--, d++) *d = d[-dist];
            if (!sb_bit(&s, &bb)) continue;
        }
        for (;;) {                                              // new offset
            u16 lo;
            n = 1;
            while (!sb_bit(&s, &bb)) { n += n; if (!sb_bit(&s, &bb)) n++; }
            if (n == 256) return d;
            lo = *s++;
            dist = (u16)((n << 7) - (lo >> 1));
            n = 1;
            if (!(lo & 1)) do { n += n; if (sb_bit(&s, &bb)) n++; } while (!sb_bit(&s, &bb));
            for (n++; n; n--, d++) *d = d[-dist];
            if (!sb_bit(&s, &bb)) break;
        }
    }
}
#endif

u8 sb_bind(SbScene *s,const u8 *bank,u16 bs,const u8 *map,u16 ms)
{
    u16 count,total;
    memset(s,0,sizeof *s);
    if(!bank||!map||bs<20||ms<16||memcmp(bank,"SBN1",4)||memcmp(map,"SCM1",4))return 0;
    s->bank=bank;s->map=map;s->sprite_count=U16(bank+4);
    s->materials=bank[6];s->shapes=bank[7];s->shapes_off=U16(bank+8);
    s->tops_off=U16(bank+10);s->units_off=U16(bank+12);s->caps_off=U16(bank+14);
    s->directory=U16(bank+16);s->width=map[4];s->height=map[5];s->rows=U16(map+6);
    count=(u16)(s->width*s->height);s->plane=(u16)(s->rows*40);
    total=U16(bank+18);
    if(!count||count>255||!s->rows||s->rows>218||!s->materials||!s->shapes||
       U16(map+12)!=count||ms<16+count*8||bs<total||
       (u32)s->shapes_off+256>total||
       (u32)s->tops_off+(u32)s->materials*s->shapes*8>total||
       (u32)s->units_off+8>total||(u32)s->caps_off+56>total||
       (u32)s->directory+(u32)s->sprite_count*8>total)return 0;
    return 1;
}

static void sb_sprite(SbScene *s,u16 id,s16 x,s16 y,u8 depth,u8 *col,u8 *cov,u8 *seen)
{
    const u8 *r,*p;u8 rows; s16 bx,yy;u8 shift;
    if(id==0xFFFF)return;
    r=s->bank+s->directory+(u16)(id<<3);
    x+=(s8)r[2];y+=(s8)r[3];rows=r[1];p=s->bank+U16(r+4);
    bx=x>>3;shift=(u8)x&7;s->pieces++;s->sprite_rows+=rows;
    for(yy=y;rows;rows--,yy++,p+=12){
        u32 l,d,m;u16 o;u8 k;
        if((u16)yy>=s->rows)continue;
        l=U32(p)>>shift;d=U32(p+4)>>shift;m=U32(p+8)>>shift;
        o=(u16)(yy*40);
        for(k=0;k<4;k++,l<<=8,d<<=8,m<<=8){
            s16 b=bx+k;u16 at;u8 visible;
            if((u16)b>=40)continue;
            at=(u16)(o+b);visible=(u8)(m>>24)&(u8)~seen[at];
            if(!visible)continue;
            seen[at]|=visible;s->touched_bytes++;
            if(col){col[at]|=(u8)(l>>24)&visible;col[s->plane+at]|=(u8)(d>>24)&visible;}
            // Front-to-back composition: complete seen mask also remembers
            // pixels whose third/lower diagonal is intentionally not stored.
            if(!cov[s->plane+at]||cov[at]==depth){
                cov[at]=depth;cov[s->plane+at]|=visible;
            }else if(!cov[3*s->plane+at]||cov[2*s->plane+at]==depth){
                cov[2*s->plane+at]=depth;cov[3*s->plane+at]|=visible;
            }
        }
    }
}

static u8 sb_heights(const SbScene *s,u8 rot,s16 u,s16 v,u8 *h)
{
    s16 x,z;u8 c;const u8 *t;
    switch(rot){
    case 0:x=u;z=v;break;
    case 1:x=v;z=s->height-1-u;break;
    case 2:x=s->width-1-u;z=s->height-1-v;break;
    default:x=s->width-1-v;z=u;break;
    }
    if((u16)x>=s->width||(u16)z>=s->height)return 0;
    t=s->map+16+(u16)((z*s->width+x)<<3);
    for(c=0;c<4;c++)h[c]=t[(8-c-rot)&3];
    return 1;
}

void sb_compose(SbScene *s,u8 rot,u8 *color,u8 *coverage,u8 *seen)
{
    s16 width=rot&1?s->height:s->width,height=rot&1?s->width:s->height;
    s16 diag,u;
    rot&=3;s->pieces=s->sprite_rows=s->touched_bytes=0;
    if(color)memset(color,0,(u16)(2*s->plane));
    memset(coverage,0,(u16)(4*s->plane));memset(seen,0,s->plane);
    for(diag=width+height-2;diag>=0;diag--){
        s16 first=diag-height+1,last=diag;
        if(first<0)first=0;if(last>=width)last=width-1;
        for(u=first;u<=last;u++){
            s16 v=diag-u,x,z,cx,cy;u8 h[4],nh[4],lo,key=0,c,shape,mat,wall,side;
            const u8 *t;u16 id;
            sb_heights(s,rot,u,v,h);lo=h[0];
            for(c=1;c<4;c++)if(h[c]<lo)lo=h[c];
            for(c=0;c<4;c++)key|=(u8)((h[c]-lo)<<(c<<1));
            shape=s->bank[s->shapes_off+key];
            switch(rot){
            case 0:x=u;z=v;break;
            case 1:x=v;z=s->height-1-u;break;
            case 2:x=s->width-1-u;z=s->height-1-v;break;
            default:x=s->width-1-v;z=u;break;
            }
            t=s->map+16+(u16)((z*s->width+x)<<3);mat=t[4];wall=t[5];
            cx=320-(u-v+height)*12;cy=64+diag*6;
            id=U16(s->bank+s->tops_off+2*(u16)(((u16)mat*s->shapes+shape)*4+rot));
            sb_sprite(s,id,cx,cy-lo*6,(u8)diag,color,coverage,seen);
            for(side=0;side<2;side++){
                u8 a=side?1:2,b=side?2:3,k=h[a]<h[b]?h[a]:h[b],bottom=0,hh;
                s16 delta=(s16)h[a]-h[b];
                if(sb_heights(s,rot,u+(side?0:1),v+(side?1:0),nh))
                    bottom=nh[0]<nh[side?3:1]?nh[0]:nh[side?3:1];
                if(delta){
                    id=U16(s->bank+s->caps_off+2*(u16)((wall*2+side)*7+delta+3));
                    sb_sprite(s,id,cx,cy-k*6,(u8)diag,color,coverage,seen);
                }
                id=U16(s->bank+s->units_off+2*(u16)(wall*2+side));
                for(hh=k;hh>bottom;hh--)sb_sprite(s,id,cx,cy-(hh-1)*6,(u8)diag,color,coverage,seen);
            }
        }
    }
}

u8 sb_patch(const u8 *p,u8 rot,u8 *color,u8 *coverage,u16 plane,u8 *scratch)
{
    u16 n;const u8 *r;
    if(!p||memcmp(p,"SBP1",4))return 0;
    r=p+U16(p+4+(rot&3)*2);n=U16(r);r+=2;
    while(n--){
        u16 at=U16(r),off=U16(r+2),length=256,i;u8 *dst;
        r+=4;if((u32)at+length>6UL*plane)length=(u16)(6UL*plane-at);
        if(sb_unpack(p+off,scratch)!=scratch+length)return 0;
        // A 256-byte patch may straddle the color/coverage allocation boundary.
        for(i=0;i<length;i++){
            u16 pos=(u16)(at+i);
            if(pos<2*plane){if(!color)continue;dst=color+pos;}
            else {if(!coverage)continue;dst=coverage+pos-2*plane;}
            *dst^=scratch[i];
        }
    }
    return 1;
}

u16 sb_checksum(const u8 *p,u16 n)
{
    u16 a=0,b=0;while(n--){a+=*p++;if(a>=255)a-=255;b+=a;if(b>=255)b-=255;}
    return (u16)(b<<8|a);
}
