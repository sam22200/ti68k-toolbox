// Shared portrait/stat renderer. The owner provides its font/outlined-text adapter;
// layout selects placement, formatting and meter style. No allocation or full redraw policy.
#include "unit_card.h"
#include "portrait32.inc"
static u8 card_uint(char *s,u8 n,u16 v)
{
    char b[5];u8 k=0;
    do{b[k++]=(char)('0'+v%10);v/=10;}while(v);
    while(k)s[n++]=b[--k];
    return n;
}
#ifndef CARD_MANAGEMENT_LAYOUTS
static u8 card_string(char *s,u8 n,const char *p){while(*p)s[n++]=*p++;return n;}
#endif
#ifdef CARD_MANAGEMENT_LAYOUTS
static void card_number(s16 x,s16 y,u16 v,u8 right,u8 context)
{
    char s[6];s[card_uint(s,0,v)]=0;
    CARD_TEXT(right?x-CARD_WIDTH(s):x,y,s,C_BLACK,159,context);
}
static void card_pair(s16 x,s16 value_x,s16 y,const char *label,u16 v,u8 right,u8 context)
{
    CARD_TEXT(x,y,label,C_BLACK,159,context);card_number(value_x,y,v,right,context);
}
#endif
static void unit_card_bar(s16 x,s16 y,u16 v,u16 max,u8 width,u8 color,u8 style,u8 *opacity)
{
    u16 n;
    if(style==UC_BAR_FULL){draw_rect(x,y,width,2,color);return;}
    // HP/MP are capped at999; width<=60 keeps this arithmetic unsigned16-bit.
    n=max?(u16)((width*v+max-1)/max):0;if(n>width)n=width;
    if(style==UC_BAR_ROUND) {
        void *sl=rt_light,*sd=rt_dark;
        draw_rect(x+1,y-1,width,4,C_WHITE);draw_rect(x,y,width+2,2,C_WHITE);
        draw_rect(x+1,y,width,2,C_LGRAY);if(n)draw_rect(x+1,y,n,2,color);
        if(opacity) {
            rt_light=rt_dark=opacity;
            draw_rect(x+1,y-1,width,4,C_BLACK);draw_rect(x,y,width+2,2,C_BLACK);
            rt_light=sl;rt_dark=sd;
        }
    } else {
        draw_rect(x,y,width,2,C_WHITE);if(n)draw_rect(x,y,n,2,color);
    }
}
static void unit_card_draw(const UnitCard *d,u8 layout,s16 y,u8 part,u8 context,u8 *opacity)
{
    u8 mp=part==UC_MP;
#ifndef CARD_MANAGEMENT_LAYOUTS
    char s[32];u8 n=0;
#endif
    if(part==UC_PORTRAIT) {
        static const u8 px[]={2,5,0,61,2},py[]={1,15,0,0,1};
        if(d->portrait)portrait32(d->portrait,px[layout],y+py[layout]);return;
    }
#ifdef CARD_MANAGEMENT_LAYOUTS
    if(layout==UC_FORMATION) {
        if(part==UC_NAME)CARD_TEXT(37,y,d->name,C_BLACK,159,context);
        else if(part==UC_JOB)CARD_TEXT(37,y+7,d->job,C_BLACK,159,context);
        else if(part==UC_LEVEL)card_pair(95,108,y,"Lv",d->level,0,context);
        else if(part==UC_EXP)card_pair(124,141,y,"EXP",d->exp,0,context);
        else if(part==UC_BRAVE)card_pair(37,61,y+23,"Brave",d->brave,0,context);
        else if(part==UC_FAITH)card_pair(83,107,y+23,"Faith",d->faith,0,context);
        else if(part==UC_HP||mp) {
            s16 x=mp?96:37;
            card_pair(x,x+14,y+13,mp?"MP":"HP",mp?d->mp:d->hp,0,context);
            unit_card_bar(x+14,y+19,0,0,35,mp?C_DGRAY:C_BLACK,UC_BAR_FULL,0);
        }
    } else if(layout==UC_MANAGEMENT) {
        if(part==UC_NAME)CARD_TEXT(40,y+16,d->name,C_BLACK,159,context);
        else if(part==UC_JOB)CARD_TEXT(40,y+25,d->job,C_BLACK,159,context);
        else if(part==UC_LEVEL)card_pair(96,118,y+16,"Lv",d->level,1,context);
        else if(part==UC_HP||mp) {
            s16 x=mp?86:40;
            card_pair(x,x+40,y+34,mp?"MP":"HP",mp?d->mp:d->hp,1,context);
            unit_card_bar(x,y+41,0,0,40,mp?C_DGRAY:C_BLACK,UC_BAR_FULL,0);
        }
    }
#else
    if(layout==UC_STATUS) {
        if(part==UC_NAME)CARD_TEXT(24,y+2,d->name,C_BLACK,92,context);
        else if(part==UC_JOB)CARD_TEXT(24,y+9,d->job,C_BLACK,92,context);
        else if(part==UC_LEVEL) {
            n=card_string(s,n,"Lv ");n=card_uint(s,n,d->level);
            n=card_string(s,n," Exp ");n=card_uint(s,n,d->exp);s[n]=0;
            CARD_TEXT(24,y+17,s,C_BLACK,92,context);
        } else if(part==UC_HP||mp) {
            u16 v=mp?d->mp:d->hp,m=mp?d->max_mp:d->max_hp;
            n=card_string(s,n,mp?"MP ":"HP ");n=card_uint(s,n,v);s[n++]='/';
            n=card_uint(s,n,m);s[n]=0;CARD_TEXT(96,y+(mp?17:3),s,C_BLACK,159,context);
            unit_card_bar(96,y+(mp?24:10),v,m,60,mp?C_DGRAY:C_BLACK,UC_BAR_FLAT,0);
        }
    } else {
        u8 side=layout==UC_BATTLE_RIGHT; s16 x=side?19:22,limit=side?60:74;
        if(part==UC_NAME)CARD_TEXT(side?limit-CARD_WIDTH(d->name):x,y+1,d->name,C_BLACK,limit,context);
        else if(part==UC_LEVEL) {
            n=card_string(s,n,"Lv");n=card_uint(s,n,d->level);s[n]=0;
            CARD_TEXT(side?62:1,y+23,s,C_BLACK,side?80:20,context);
        } else if(part==UC_HP||mp) {
            u16 v=mp?d->mp:d->hp,m=mp?d->max_mp:d->max_hp;
            n=card_uint(s,n,v);s[n++]='/';n=card_uint(s,n,m);s[n]=0;
            CARD_TEXT(x,y+(mp?19:8),mp?"MP":"HP",C_BLACK,limit,context);
            CARD_TEXT(x+41-CARD_WIDTH(s),y+(mp?19:8),s,C_BLACK,x+41,context);
            unit_card_bar(x,y+(mp?26:15),v,m,40,mp?C_DGRAY:C_BLACK,UC_BAR_ROUND,opacity);
        }
    }
#endif
}
#undef CARD_TEXT
#undef CARD_WIDTH
#undef CARD_MANAGEMENT_LAYOUTS
