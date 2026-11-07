// Character bitmaps are made offline. Only8-bit sprite rows cross host/TI byte order.
#include "bitmap_font.h"

static u16 bf_word(const u8 *p){return (u16)((u16)p[0]<<8|p[1]);}

u8 bf_open(BfFont *f,const u8 *bank,u16 bytes,u8 face)
{
    u16 at,dir,kern,n;
    f->bank=f->glyphs=f->kern=RT_NULL;f->size=f->kerns=0;
    if(!bank||bytes<198||bank[0]!='B'||bank[1]!='M'||bank[2]!='F'||bank[3]!='1'||face>=bank[4])return 0;
    if(bf_word(bank+6)>bytes||bf_word(bank+6)<198)return 0;
    bytes=bf_word(bank+6);
    at=8+(u16)face*8;if(at>bytes-8)return 0;
    dir=bf_word(bank+at);kern=bf_word(bank+at+2);n=bf_word(bank+at+4);
    if(dir>bytes-190U||kern>bytes||((u16)(bytes-kern)>>2)<n)return 0;
    f->bank=bank;f->size=bytes;f->glyphs=bank+dir;f->kern=bank+kern;f->kerns=n;
    return 1;
}

static const u8 *bf_glyph(const BfFont *f,u8 c)
{
    const u8 *g;u16 at,bytes;
    if(c<32||c>126)c='?';
    at=bf_word(f->glyphs+2*(c-32));
    if(!at)at=bf_word(f->glyphs+2*('?' -32));
    if(!at||at>f->size-8U)return RT_NULL;
    g=f->bank+at;
    if(g[4]>32||g[5]>24||bf_word(g)>2048)return RT_NULL;
    bytes=(u16)((g[4]+7)>>3)*g[5]*3;
    return bytes<=f->size-at-8 ? g : RT_NULL;
}

static s16 bf_pair(const BfFont *f,u8 a,u8 b)
{
    u16 lo=0,hi=f->kerns,pair_key=(u16)((u16)a<<8|b);
    while(lo<hi) {
        u16 mid=(lo+hi)>>1,k=bf_word(f->kern+(mid<<2));
        if(k<pair_key)lo=mid+1;else hi=mid;
    }
    if(lo<f->kerns&&bf_word(f->kern+(lo<<2))==pair_key) {
        s16 delta=(s16)bf_word(f->kern+(lo<<2)+2);
        return delta>=-256&&delta<=256?delta:0;
    }
    return 0;
}

u8 bf_measure(const BfFont *f,const char *text,u8 cell,BfMetrics *m)
{
    s16 pen=0;u8 previous=0,ink=0,n=0;
    m->left=m->top=32767;m->right=m->bottom=-32767;m->advance=0;
    if(!f->bank||cell>32)return 0;
    while(*text) {
        u8 c=(u8)*text++;const u8 *g=bf_glyph(f,c);s16 x,y;
        if(!g||n++==128)return 0;
        if(!cell&&previous)pen+=bf_pair(f,previous,c);
        x=(pen>>6)+(s8)g[2];y=(s8)g[3];
        if(g[4]&&g[5]) {
            if(x<m->left)m->left=x;
            if(y<m->top)m->top=y;
            if(x+g[4]>m->right)m->right=x+g[4];
            if(y+g[5]>m->bottom)m->bottom=y+g[5];
            ink=1;
        }
        pen+=cell ? (s16)cell<<6 : (s16)bf_word(g);previous=c;
        if(pen<0||pen>16384)return 0;
    }
    m->advance=(pen+32)>>6;
    if(!ink)m->left=m->top=m->right=m->bottom=0;
    return 1;
}

static void bf_column(const u8 *p,u8 h,s16 x,s16 y,s16 age)
{
    if(age<0)return;
    if(age>=20) {
        RtSprite sp={8,h,p,p+h,p+2*h};draw_sprite(x,y,&sp);
    }else {
        u8 l[24],d[24],m[24],solid=0,mid=0,dim=0,col,row;
        for(col=0;col<8;col++) {
            s16 a=age-col;u8 bit=(u8)(0x80>>col);
            if(a>=12)solid|=bit;else if(a>=6)mid|=bit;else if(a>=0)dim|=bit;
        }
        for(row=0;row<h;row++) {
            u8 ink=(u8)~p[2*h+row];m[row]=(u8)~(ink&(solid|mid|dim));
            l[row]=(p[row]&solid)|(ink&mid);d[row]=(p[h+row]&solid)|(ink&dim);
        }
        {RtSprite sp={8,h,l,d,m};draw_sprite(x,y,&sp);}
    }
}

void bf_draw(const BfFont *f,const char *text,const BfLayout *layout)
{
    BfMetrics m;s16 pen=0,x,y,width;u8 previous=0;
    if(!bf_measure(f,text,layout->cell,&m))return;
    width=layout->cell?m.advance:m.right-m.left;
    x=layout->left+(((s16)layout->width-width)>>1)-(layout->cell?0:m.left);
    y=layout->top+1-m.top;
    while(*text) {
        u8 c=(u8)*text++,col;const u8 *g=bf_glyph(f,c),*p;
        s16 glyph_x,glyph_y;if(!g)return;
        if(!layout->cell&&previous)pen+=bf_pair(f,previous,c);
        glyph_x=x+(pen>>6)+(s8)g[2];glyph_y=y+(s8)g[3];p=g+8;
        for(col=0;col<g[4];col+=8,p+=(u16)g[5]*3)
            bf_column(p,g[5],glyph_x+col,glyph_y,(s16)layout->sweep-(glyph_x+col-layout->left));
        pen+=layout->cell?(s16)layout->cell<<6:(s16)bf_word(g);previous=c;
    }
}
