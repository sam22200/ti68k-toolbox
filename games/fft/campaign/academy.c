// Compact Chapter 1 bridge. Original map/text/actors, one fixed adapted staging.
#include "../../../runtime/core/rt.h"
#include "data/academy.h"
#include <stdlib.h>
#include <string.h>
typedef struct {u8 message,offset,reveal,done,white;u16 tick;} Academy;
Academy academy;
static const u8 *bank;
static u8 *cache,dirty;
static char lines[6][48];
static u8 rows,width,height,next,total,drawn;
static u16 be16(const u8 *p){return (u16)((u16)p[0]<<8|p[1]);}
static const char *speaker(void){return (const char*)bank+be16(bank+AC_MESSAGES+2*academy.message);}
static const char *body(void){const char*s=speaker();while(*s++);return s+academy.offset;}
u8 chapter_completed(void){return academy.done;}

static void text(s16 x,s16 y,const char *s,u8 color)
{
    static const u8 zero[5]={0};
    for(;*s;s++){
        u8 c=(u8)*s-32;const u8*g=bank+96+(u16)c*5;u8 mask[5],r;
        for(r=0;r<5;r++)mask[r]=(u8)~g[r];
        {RtSprite sp={8,5,color&1?g:zero,color&2?g:zero,mask};draw_sprite(x,y,&sp);}
        x+=bank[c];
    }
}
static void layout(void)
{
    const char*s=body();u8 at=0,r;
    rows=width=total=0;memset(lines,0,sizeof lines);
    for(r=0;r<6&&s[at];r++){
        u8 start=at,last=at,n=0,w=0,lastn=0,lastw=0;
        while(s[at]&&n<47){u8 a=bank[(u8)s[at]-32];if(w+a>146)break;
            if(s[at]==' '){last=at;lastn=n;lastw=w;}w+=a;at++;n++;}
        if(s[at]&&last>start){at=last;n=lastn;w=lastw;}
        memcpy(lines[r],s+start,n);total+=n;if(w>width)width=w;
        while(s[at]==' ')at++;
        rows++;
    }
    next=at;width+=8;height=7+rows*7;academy.reveal=drawn=0;dirty=1;
}
void game_init(void)
{
    bank=rt_file("ffac",RT_NULL);if(cache)free(cache);cache=malloc(2*RT_PSIZE);academy.done=0;
    rt_state=&academy;rt_state_size=sizeof academy;
}
void game_scenario(u16 n)
{
    memset(&academy,0,sizeof academy);academy.message=n<AC_NMESSAGES?(u8)n:0;
    if(bank)layout();
}
u8 game_update(void)
{
    if(input_pressed(K_ESC))return 0;
    if(!bank||!cache)return 1;
    academy.tick++;
    if(input_pressed(K_CATALOG)){academy.white^=1;dirty=1;drawn=0;}
    if(academy.reveal<total){
        academy.reveal+=3;if(academy.reveal>total||input_pressed(K_ENTER|K_A))academy.reveal=total;
    }else if(input_pressed(K_ENTER|K_A)){
        if(body()[next])academy.offset+=next;
        else {academy.offset=0;if(++academy.message==AC_NMESSAGES){academy.done=1;return 0;}}
        layout();
    }
    return 1;
}
void game_render(void)
{
    void*l=rt_light,*d=rt_dark;u8 r,col;u16 n=0;
    if(!bank||!cache){draw_clear();draw_text(4,40,"Missing Academy bank",F_SMALL,C_BLACK);return;}
    rt_light=cache;rt_dark=cache+RT_PSIZE;
    if(dirty){
        s16 x=(160-width)>>1,y=100-height,nw=8;const char*s=speaker();
        memset(cache,0,2*RT_PSIZE);
        for(r=0;r<100;r++){
            memcpy(cache+(u16)r*RT_PBYTES,bank+AC_BG+(u16)r*20,20);
            memcpy(cache+RT_PSIZE+(u16)r*RT_PBYTES,bank+AC_BG+2000+(u16)r*20,20);
        }
        for(r=0;r<AC_NACTORS;r++){
            const u8 *p=bank+AC_ACTORS+(u16)r*160;u16 bits[3][26];u8 a,b;
            for(a=0;a<3;a++)for(b=0;b<26;b++)bits[a][b]=be16(p+4+(u16)a*52+2*b);
            {RtSprite sp={16,26,bits[0],bits[1],bits[2]};draw_sprite((s16)be16(p),(s16)be16(p+2),&sp);}
        }
        draw_rect(x,y,width,height,academy.white?C_BLACK:C_WHITE);
        draw_rect(x+1,y+1,width-2,height-2,academy.white?C_WHITE:C_BLACK);
        draw_rect(x+2,y+2,width-4,2,academy.white?C_LGRAY:C_DGRAY);
        while(*s)nw+=bank[(u8)*s++-32];
        if(*speaker()){
            draw_rect(x,y-11,nw,11,academy.white?C_BLACK:C_WHITE);
            draw_rect(x+1,y-10,nw-2,10,academy.white?C_WHITE:C_BLACK);
            text(x+4,y-8,speaker(),academy.white?C_BLACK:C_WHITE);
        }
        dirty=0;
    }
    for(r=0;r<rows;r++){
        s16 x=((160-width)>>1)+4,y=104-height+r*7;
        for(col=0;lines[r][col];col++,n++){
            char s[2];s[0]=lines[r][col];s[1]=0;
            if(n>=drawn&&n<academy.reveal)text(x,y,s,academy.white?C_BLACK:C_WHITE);
            x+=bank[(u8)s[0]-32];
        }
    }
    drawn=academy.reveal;rt_light=l;rt_dark=d;
    for(r=0;r<100;r++){
        memcpy((u8*)l+(u16)r*RT_PBYTES,cache+(u16)r*RT_PBYTES,20);
        memcpy((u8*)d+(u16)r*RT_PBYTES,cache+RT_PSIZE+(u16)r*RT_PBYTES,20);
    }
}
