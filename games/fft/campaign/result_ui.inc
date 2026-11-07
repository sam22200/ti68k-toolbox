#include "flow_draw.inc"
static void result_money(u32 value,s16 y)
{
    static const u32 powers[]={10000000UL,1000000UL,100000UL,10000UL,1000UL,100UL,10UL,1UL};
    u8 chars[13],i,n=0,on=0;
#ifndef FFT_SHARED_SERIF
    const u8 *p=qpart(flow_bank,Q_TITLES),*digits=p+1+(u16)p[0]*960;
#endif
    for(i=0;i<8;i++) {
        u8 v=0;
        while(value>=powers[i]){value-=powers[i];v++;}
        if(v||on||i==7){chars[n++]=v;on=1;}
    }
#ifdef FFT_SHARED_SERIF
    for(i=0;i<n;i++)chars[i]+='0';
    chars[n++]=' ';chars[n++]='G';chars[n++]='I';chars[n++]='L';chars[n]=0;
    qfont_text(flow_bank,2,(const char *)chars,y,255,8);
#else
    chars[n++]=10;chars[n++]=11;chars[n++]=12;chars[n++]=13;
    for(i=0;i<n;i++) {
        const u8 *g=digits+(u16)chars[i]*48;RtSprite sp={8,16,g,g+16,g+32};
        draw_sprite((160-n*8)/2+i*8,y,&sp);
    }
#endif
}
static void result_icon(s16 x,s16 y,u8 item)
{
    const u8 *it=flow_ui+qword(flow_ui+6)+(u16)item*14;
    const u8 *p=flow_ui+qword(flow_ui+12)+(u16)it[12]*24;
    u16 l[10],d[10],m[10],ink[12];u8 r;
    memset(ink,0,sizeof ink);
    for(r=0;r<8;r++)ink[r+2]=(u16)((u16)(u8)~p[16+r]<<7);
    for(r=0;r<10;r++) {
        u16 a=ink[r+1],ring=(u16)(a|(a<<1)|(a>>1)|ink[r]|ink[r+2]);
        l[r]=r&&r<9?(u16)((u16)p[r-1]<<7):0;
        d[r]=r&&r<9?(u16)((u16)p[8+r-1]<<7):0;m[r]=(u16)~ring;
    }
    {RtSprite sp={16,10,l,d,m};draw_sprite(x-1,y-1,&sp);}
}
static void result_draw(void)
{
    if(!st.reward_page)qshade_reveal(st.open_clock>>1);else qshade();
    if(!st.reward_page){
        qtitle_reveal(flow_bank,0,23,(u16)st.open_clock*3);
        qtitle_reveal(flow_bank,1,52,st.open_clock>48?((u16)st.open_clock-48)*3:0);
        qtitle_reveal(flow_bank,2,67,st.open_clock>72?((u16)st.open_clock-72)*3:0);
    }
    else if(st.reward_page==1){qtitle_reveal(flow_bank,3,20,(u16)st.open_clock*3);result_money(st.reward_base,52);}
    else {
        u8 i;
        qtitle_reveal(flow_bank,4,12,(u16)st.open_clock*3);result_money(st.reward_bonus,30);
        for(i=0;i<4&&st.reward_scroll+i<st.reward_n;i++) {
            u8 id=st.reward_items[st.reward_scroll+i];
            const char *name=items[id].name,*p=name;u8 width=0;
            s16 left;
            while(*p)width+=UI_SHARED(ADV)[(u8)*p++-32];
            left=(160-width-(flow_ui?13:0))/2;
            if(flow_ui)result_icon(left,48+11*i,id);
            left+=flow_ui?13:0;
            ui_text(left+1,50+11*i,name,C_BLACK,155);
            ui_text(left,49+11*i,name,C_WHITE,154);
        }
        if(st.reward_scroll)ui_text(152,47,"^",C_WHITE,160);
        if(st.reward_scroll+4<st.reward_n)ui_text(152,81,"v",C_WHITE,160);
    }
}
