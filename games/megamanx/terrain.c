#include "terrain.h"
#include <string.h>
static const u8 *cells;
static const u8 *images;
static u8 ready;
u8 terrain_init(void)
{
    u16 size;
    const u8 *p;
    ready=0;cells=images=RT_NULL;
    p=rt_file("mmxmap",&size);
    if(!p || size<1040 || (memcmp(p,"MXM1",4) && memcmp(p,"MXM2",4))) return 0;
    if(p[4]!=4 || p[5] || p[6]!=1 || p[7] || p[8]!=1 || p[9] || p[10] || p[11]!=64 || p[12] || p[13]!=16)return 0;
    if(p[3]=='2') {
        if(size<17424 || p[14]!=0x44 || p[15]!=0x10)return 0;
        images=p+1040;
    }
    cells=p+16;ready=1;return 1;
}
u8 terrain_ready(void) { return ready; }
u8 terrain_class(s16 x,s16 y)
{
    if(!ready || (u16)x>=MX_WIDTH || (u16)(y-MX_TOP)>=MX_HEIGHT) return 0;
    return cells[(((u16)(y-MX_TOP)>>4)<<6)+((u16)x>>4)]&63;
}
s16 terrain_surface(s16 x,s16 y)
{
    u16 k=terrain_class(x,y),u=((u16)x&15)+1;
    s16 h=17;
    if(k==1)h=16-(u>>1);
    else if(k==2)h=8-(u>>1);
    else if(k==3)h=8+(u>>1);
    else if(k==4)h=u>>1;
    else if(k>=5 && k<=8)h=16-((k-5)<<2)-(u>>2);
    else if(k>=9 && k<=12)h=12-((k-9)<<2)+(u>>2);
    else if(k==0x13 || k>=0x33)h=(k==0x33 || k>=0x3e)?2:0;
    return h==17?-1:(y&~15)+h;
}
u8 terrain_solid(s16 x,s16 y)
{
    s16 surface=terrain_surface(x,y);
    return surface>=0 && y>=surface;
}
#define W10(op) op op op op op op op op op op
#ifdef __m68k__
#define ROWS(op) do { for(r=0;r<RT_H;++r) { W10(op) s+=22;d+=5; } } while(0)
static void image_plane(u8 *dst,const u8 *src,u16 x,u16 y)
{
    const u16 *s=(const u16 *)(src+(y<<6)+((x>>4)<<1));
    u16 *d=(u16 *)dst,sh=x&15,r;
    if(!sh)ROWS(*d++=*s++;);
    else if(sh==8) {
        const u8 *b=(const u8 *)s+1;u8 *a=dst;
        for(r=0;r<RT_H;++r) { W10(*a++=*b++;*a++=*b++;) b+=44;a+=10; }
    } else if(sh<7)ROWS(*d++=(u16)(((*(const u32 *)s)<<sh)>>16);++s;);
    else { u16 rs=16-sh;ROWS(*d++=(u16)(*(const u32 *)s>>rs);++s;); }
}
#undef ROWS
#else
static void image_plane(u8 *dst,const u8 *src,u16 x,u16 y)
{
    u16 r,c;u8 sh=x&7;
    for(r=0;r<RT_H;++r) {
        const u8 *s=src+((y+r)<<6)+(x>>3);
        for(c=0;c<20;++c)dst[c]=sh?(u8)((s[c]<<sh)|(s[c+1]>>(8-sh))):s[c];
        dst+=RT_PBYTES;
    }
}
#endif
#undef W10
void terrain_render(u16 camx,u16 camy)
{
    u16 x,y,x0=camx>>3,y0=camy>>3;
    if(images) { image_plane(rt_light,images,camx,camy);image_plane(rt_dark,images+8192,camx,camy);return; }
    draw_rect(0,0,RT_W,RT_H,C_LGRAY);
    for(y=y0;y<=y0+13 && y<16;++y)
        for(x=x0;x<=x0+20 && x<64;++x)
            if(cells[(y<<6)+x])draw_rect((x<<3)-camx,(y<<3)-camy,8,8,C_DGRAY);
}
