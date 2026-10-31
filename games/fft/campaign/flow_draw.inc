// Shared glyph review accepted2026-10-24; legacy strips are explicit controls.
#ifdef FFT_SHARED_SERIF
#include "../../../lib/bitmap_font.c"
static void qfont_text(const u8 *bank,u8 face,const char *text,s16 y,u16 sweep,u8 cell)
{
    BfFont font;const u8 *p=qpart(bank,Q_FONT);
    BfLayout layout={0,y,160,sweep,cell};
    if(bf_open(&font,p,qword(p+6),face))bf_draw(&font,text,&layout);
}
static void qtitle_reveal(const u8 *bank,u8 title,s16 y,u16 sweep)
{
    const u8 *p=qpart(bank,Q_TITLES);
    if(title>=p[0])return;
    p+=1+(u16)title*25;
    qfont_text(bank,p[0],(const char *)p+1,y,sweep,0);
}
#else
#include "../serif_reveal.inc"
static void qtitle_reveal(const u8 *bank,u8 title,s16 y,u16 sweep)
{
    serif_reveal(qpart(bank,Q_TITLES)+1+(u16)title*960,20,16,0,y,sweep);
}
#endif
static void qtitle(const u8 *bank,u8 title,s16 y)
{
    qtitle_reveal(bank,title,y,255);
}
static void qshade(void)
{
    u8 row,k;
    for(row=0;row<100;row++) {
        u8 *l=(u8 *)rt_light+(u16)row*RT_PBYTES,*d=(u8 *)rt_dark+(u16)row*RT_PBYTES;
        for(k=0;k<20;k++){u8 a=l[k],b=d[k];l[k]=(u8)(b|~a);d[k]=(u8)(b|a);}
    }
}
static void qshade_reveal(u8 phase)
{
    static const u8 masks[4][4]={{0,0,0,0},{0x88,0,0x22,0},
                               {0xAA,0x55,0xAA,0x55},{0xEE,0x55,0xBB,0x55}};
    u8 row,k;
    if(phase>=4){qshade();return;}
    for(row=0;row<100;row++) {
        u8 mask=masks[phase][row&3],*l=(u8 *)rt_light+(u16)row*RT_PBYTES;
        u8 *d=(u8 *)rt_dark+(u16)row*RT_PBYTES;
        for(k=0;k<20;k++) {
            u8 a=l[k],b=d[k];l[k]=(u8)((a&~mask)|((b|~a)&mask));
            d[k]=(u8)((b&~mask)|((b|a)&mask));
        }
    }
}
