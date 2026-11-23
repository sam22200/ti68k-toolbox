#ifndef FFT_BAR_BANK_H
#define FFT_BAR_BANK_H
// Cold archive binding, byte reads only; accepts the native DAT trailer after total.
static u8 bar_art_valid(const u8 *p,u16 size)
{
    return (u8)(p && size>=6360 && !memcmp(p,"FBR1",4) && b16(p+4)==6360 &&
                b16(p+6)==12 && b16(p+8)==360 && !b16(p+10));
}
static u8 bar_text_valid(const u8 *p,u16 size)
{
    u16 total,first,at,i,j;u8 ids[8]={0};
    if(!p||size<8||memcmp(p,"FRU1",4))return 0;
    total=b16(p+4);first=(u16)(8+14*(u16)p[6]);
    if(!p[6]||p[6]>64||p[7]||total>size||first>=total)return 0;
    for(at=8;at<first;at+=14) {
        u8 id=p[at];
        if(id>=64||ids[id>>3]&(1U<<(id&7))||!p[at+3]||p[at+3]>37||!b16(p+at+4)||p[at+4]&128)return 0;
        ids[id>>3]|=(u8)(1U<<(id&7));
        for(j=6;j<=8;j+=2) {
            u16 begin=b16(p+at+j);
            i=begin;if(i<first||i>=total)return 0;
            for(;i<total&&p[i];i++)if(p[i]<32||p[i]>126)return 0;
            if(i==total||i-begin>(j==6?40:4096))return 0;
        }
        i=b16(p+at+10);
        if(i) {
            u16 end;
            if(i<first||i>=total)return 0;
            end=(u16)(i+1+2*(u16)p[i]);if(end<i||end>total)return 0;
            for(i++;i<end;i+=2)if(p[i]>=64||p[i+1]>15)return 0;
        }
        {
            u16 index=b16(p+at+12),length=0,last=0;
            const u8 *s=p+b16(p+at+8);
            while(s[length])length++;
            if(index<first||index>=total||2*(u16)p[at+3]>total-index||b16(p+index))return 0;
            for(j=0;j<p[at+3];j++) {
                u16 next=b16(p+index+2*j);
                if(next>=length||(j&&next<=last))return 0;
                last=next;
            }
        }
    }
    return 1;
}
#endif
