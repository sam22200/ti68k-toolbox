// Shared compact stat labels. Forecasts put the label first; impact numbers last.
#ifndef FFT_STAT_TEXT_H
#define FFT_STAT_TEXT_H
static u8 stat_text(char *s,u8 kind,AbId ab,u16 amount,u8 prefix)
{
    s8 value=(s8)amount;u8 n=0;
    const char *label=kind==AB_BRAVE?"Br":kind==AB_FAITH?"Fa":kind==AB_BREAK?(ab==89||ab==96?"PA":"MA"):"Sp";
    if(prefix)n=put_s(s,n,label);
    s[n++]=value<0?'+':'-';n=put_n(s,n,(u16)(value<0?-value:value));
    if(!prefix)n=put_s(s,n,label);
    return n;
}
#endif
