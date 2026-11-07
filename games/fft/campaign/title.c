// PS1-inspired front end; three independent saves, existing font/glove/neutral Ramza.
#include "../../../runtime/core/rt.h"
#include "../art.h"
#include "save.h"
#include <stdlib.h>
#include "../base.inc"
#include "flow.h"
#ifndef FFT_TITLE_VARIANT
#define FFT_TITLE_VARIANT 0             // review1=A faded map;2=B animated serif/black
#endif
#if FFT_TITLE_VARIANT==2
#ifdef FFT_SHARED_SERIF
#include "flow_draw.inc"
static const u8 *title_flow;
#else
#include "../serif_reveal.inc"
#endif
#endif

enum { TITLE_MAIN,TITLE_FILES,TITLE_OVERWRITE };
enum { SLOT_EMPTY,SLOT_VALID,SLOT_DAMAGED };
typedef struct {u8 kind,legacy,level,place;} TitleSlot;
typedef struct {
    u8 screen,row,action,answer,notice,result,slot,wrote;
    u8 selected[SAVE_SELECT_BYTES];
#if FFT_TITLE_VARIANT==2
    u8 reveal;
#endif
} TitleState;
static TitleState title;
static TitleSlot title_slots[3];
static const u8 *title_bank,*title_mg;
static u8 *title_back;
static Save *title_draft;
static u8 title_background;
static u8 title_save_ready;
static u16 tb16(const u8 *p){return (u16)((u16)p[0]<<8|p[1]);}
static const u8 *menu_font(void){return title_mg+tb16(title_mg+18);}
#include "mgtext.inc"
static u8 title_width(const char *s)
{
    const u8 *a=menu_font()+103;u8 w=0;while(*s)w+=a[(u8)*s++-32];return w;
}
#define CHOICE_TEXT(x,y,s,c,l) text(x,y,s,c,l)
#define CHOICE_WIDTH(s) title_width(s)
#include "../choice_menu.inc"

static void title_scan(void)
{
    u8 i;
    for(i=0;i<3;i++) {
        TitleSlot *s=&title_slots[i];u8 j;
        memset(s,0,sizeof *s);
        if(!save_load_from(save_slot_name(i+1),title_draft)) {
            if(rt_load(save_slot_name(i+1),title_draft,sizeof *title_draft)||
               rt_load(save_slot_name(i+1),title_draft,SAVE_V2_BYTES)){s->kind=SLOT_DAMAGED;continue;}
            if(i||!save_load_from(SAVE_LEGACY_NAME,title_draft))continue;
            s->legacy=1;
        }
        s->kind=SLOT_VALID;s->level=1;
        for(j=0;j<title_draft->n;j++)if(title_draft->unit[j].type==2){s->level=title_draft->unit[j].level;break;}
        s->place=title_draft->flags[0]&2?(title_draft->loc==25?3:2):title_draft->flags[0]&1?1:0;
    }
}

void title_cleanup(void)
{
    if(title_back)free(title_back);
    title_back=RT_NULL;
    if(title_draft)free(title_draft);
    title_draft=RT_NULL;
}
void game_init(void)
{
    u16 size,mg_size;
    title_cleanup();memset(&title,0,sizeof title);memset(title_slots,0,sizeof title_slots);title_save_ready=0;
    title_bank=rt_file("fftitle",&size);title_mg=rt_file("ffmg",&mg_size);
#if FFT_TITLE_VARIANT==2 && defined(FFT_SHARED_SERIF)
    title_flow=rt_file("ffflow",RT_NULL);
    if(!qvalid(title_flow))title_bank=RT_NULL;
#endif
    if(title_bank&&(size<10||memcmp(title_bank,
#if FFT_TITLE_VARIANT==2
#ifdef FFT_SHARED_SERIF
       "FTB2",
#else
       "FTB1",
#endif
#else
       "FTL1",
#endif
       4)||tb16(title_bank+4)<8||
       tb16(title_bank+6)<=tb16(title_bank+4)||tb16(title_bank+6)>=size))title_bank=RT_NULL;
    if(title_mg&&(mg_size<6416||memcmp(title_mg,"FMG3",4)||
       tb16(title_mg+18)>mg_size-2668U||tb16(title_mg+14)>mg_size-6396U))title_mg=RT_NULL;
    title_back=malloc(4000);title_draft=malloc(sizeof *title_draft);title_background=255;
    if(title_draft)title_scan();
    rt_state=&title;rt_state_size=sizeof title;
}
void game_scenario(u16 n)
{
    title.screen=n==1||n==2||n==3?TITLE_FILES:TITLE_MAIN;title.action=n==3?2:n==2;
    title.row=title.answer=title.notice=title.result=title.slot=title.wrote=0;
    if(n==3) {
        u8 selected[SAVE_SELECT_BYTES];
        title_save_ready=title_draft&&save_load(title_draft);
        if(rt_load(SAVE_SELECT_NAME,selected,sizeof selected)&&!memcmp(selected,SAVE_SELECT_MAGIC,4)&&
           selected[4]>=1&&selected[4]<=3&&!selected[5])title.row=selected[4]-1;
        if(!title_save_ready)title.notice=2;
    }
#if FFT_TITLE_VARIANT==2
    title.reveal=n?180:0;
#endif
}
static u8 title_accept(void)
{
    TitleSlot *s=&title_slots[title.row];title.slot=title.row+1;
    if(title.action==2) {
        if(!title_save_ready){title.notice=2;return 1;}
        title.wrote=1;
    }else if(title.action) {
        if(s->kind!=SLOT_VALID){title.notice=s->kind==SLOT_DAMAGED?2:1;return 1;}
        if(!save_load_from(s->legacy?SAVE_LEGACY_NAME:save_slot_name(title.slot),title_draft)){
            title.notice=2;return 1;
        }
        title.wrote=s->legacy;
    } else {
        memset(title_draft,0,sizeof *title_draft);memcpy(title_draft->magic,SAVE_MAGIC,4);
        title_draft->money[2]=7;title_draft->money[3]=208;title.wrote=1;
    }
    if(title.wrote&&!rt_save(save_slot_name(title.slot),title_draft,sizeof *title_draft)){
        title.notice=3;title.wrote=0;return 1;
    }
    title.result=title.action==2?3:title.action?2:1;return 0;
}
u8 game_update(void)
{
    u32 pressed=rt_keys&~rt_prev;
    if(!title_bank||!title_mg||!title_back||!title_draft)return !(pressed&K_ESC);
    if(title.notice){if(pressed&(K_ENTER|K_A|K_B|K_ESC))title.notice=0;return 1;}
#if FFT_TITLE_VARIANT==2
    if(title.screen==TITLE_MAIN&&title.reveal<180) {
        if(pressed)title.reveal=180;else title.reveal++;
        return 1; // the skip key never also chooses or exits
    }
#endif
    if(title.screen==TITLE_MAIN) {
        if(pressed&K_ESC)return 0;
        title.row=choice_move(title.row,2,pressed);
        if(pressed&(K_ENTER|K_A)){title.action=title.row;title.row=0;title.screen=TITLE_FILES;}
    } else if(title.screen==TITLE_FILES) {
        if(pressed&(K_B|K_ESC)){if(title.action==2)return 0;title.screen=TITLE_MAIN;title.row=title.action;return 1;}
        title.row=choice_move(title.row,3,pressed);
        if(pressed&(K_ENTER|K_A)) {
            if(title.action!=1&&title_slots[title.row].kind!=SLOT_EMPTY){title.screen=TITLE_OVERWRITE;title.answer=1;}
            else return title_accept();
        }
    } else {
        if(pressed&(K_B|K_ESC)){title.screen=TITLE_FILES;return 1;}
        title.answer=choice_move(title.answer,2,pressed);
        if(pressed&(K_ENTER|K_A)) {
            if(!title.answer)return title_accept();
            title.screen=TITLE_FILES;
        }
    }
    return 1;
}

// Run after the runtime has torn down: TI rt_save holds one pending write only.
// The slot is flushed first by rt_ti; selection metadata is a second small write.
u8 title_finish(void)
{
    Save *verified;u8 valid;
    if(!title.result||!title_draft)return 0;
    verified=malloc(sizeof *verified);if(!verified)return 0;
    valid=save_load_from(save_slot_name(title.slot),verified);
    if(title.wrote)valid&=!memcmp(verified,title_draft,sizeof *verified);
    free(verified);if(!valid)return 0;
    memcpy(title.selected,SAVE_SELECT_MAGIC,4);title.selected[4]=title.slot;title.selected[5]=0;
    return rt_save(SAVE_SELECT_NAME,title.selected,sizeof title.selected)?title.result:0;
}
u8 title_verified(void)
{
    u8 selected[SAVE_SELECT_BYTES];
    return title.result&&rt_load(SAVE_SELECT_NAME,selected,sizeof selected)&&
           !memcmp(selected,title.selected,sizeof selected)?title.result:0;
}
static void title_background_draw(u8 which)
{
    u16 y;
#if FFT_TITLE_VARIANT==2
    if(title_background!=which){zx0_unpack(title_bank+tb16(title_bank+4),title_back);title_background=which;}
#else
    if(title_background!=which){zx0_unpack(title_bank+tb16(title_bank+4+2*which),title_back);title_background=which;}
#endif
    for(y=0;y<100;y++) {
        const PlaneWord *s=(const PlaneWord *)(title_back+y*20),*t=(const PlaneWord *)(title_back+2000+y*20);
        PlaneWord *l=(PlaneWord *)((u8 *)rt_light+y*RT_PBYTES),*d=(PlaneWord *)((u8 *)rt_dark+y*RT_PBYTES);
        l[0]=s[0];l[1]=s[1];l[2]=s[2];l[3]=s[3];l[4]=s[4];
        d[0]=t[0];d[1]=t[1];d[2]=t[2];d[3]=t[3];d[4]=t[4];
    }
}
static void title_ramza(s16 x,s16 y)
{
    const u8 *p=title_mg+tb16(title_mg+14)+40U*156U;
#ifdef __m68k__
    RtSprite sp={16,26,p,p+52,p+104};
#else
    u16 rows[3][26];u8 i,j;RtSprite sp={16,26,rows[0],rows[1],rows[2]};
    for(i=0;i<3;i++)for(j=0;j<26;j++)rows[i][j]=tb16(p+i*52+2*j);
#endif
    draw_sprite(x,y,&sp);
}
void game_render(void)
{
    u8 i;
    if(!title_bank||!title_mg||!title_back||!title_draft){draw_clear();draw_text(3,35,"Missing title data / RAM",0,C_BLACK);return;}
#if FFT_TITLE_VARIANT==2
    if(title.screen==TITLE_MAIN)draw_rect(0,0,160,100,C_BLACK);
    else title_background_draw(1);
#else
    title_background_draw(title.screen!=TITLE_MAIN);
#endif
    if(title.screen==TITLE_MAIN) {
        static const char *const labels[]={"New Game","Continue"};
        static const ChoiceLayout menu={0,0,160,100,0,CHOICE_BARE,0,0,0,65,78,10,155,44,75};
#if FFT_TITLE_VARIANT==2
        u8 t=title.reveal;
#ifdef FFT_SHARED_SERIF
        qtitle_reveal(title_flow,10,24,(u16)(t<86?t:86)*2);
#else
        if(title_background!=2){zx0_unpack(title_bank+tb16(title_bank+6),title_back);title_background=2;}
        serif_reveal(title_back,20,16,0,24,(u16)(t<86?t:86)*2);
#endif
        if(t>86) {
            u8 w=t<114?(u8)((t-86)*4):112;
            draw_rect(24,47,w,1,C_WHITE);
        }
        if(t>114) {
#ifdef FFT_SHARED_SERIF
            qtitle_reveal(title_flow,11,55,(u16)(t-114)*3);
#else
            serif_reveal(title_back+960,20,16,0,55,(u16)(t-114)*3);
#endif
        }
        if(t<180)return;
#else
        draw_rect(64,67,32,8,C_BLACK);text(69,69,"- 68k -",C_WHITE,96);
        for(i=0;i<2;i++)draw_rect(63,76+10*i,45,9,C_BLACK);
#endif
        choice_frame(&menu,""); // CHOICE_BARE deliberately draws no title or frame
        choice_list(&menu,labels,2,title.row,C_WHITE,C_LGRAY,rt_frame);
    } else {
        static const ChoiceLayout cursor={0,0,160,100,0,CHOICE_BARE,0,0,0,47,12,28,159,3,18};
        static const char *const places[]={"Orbonne Monastery","Military Academy","Gariland","Mandalia Plains"};
        draw_rect(0,0,160,7,C_BLACK);text(5,1,title.action==2?"Save - Select data":title.action?"Continue - Select data":"New Game - Select data",C_WHITE,157);
        for(i=0;i<3;i++) {
            TitleSlot *s=&title_slots[i];s16 y=9+28*i;
            char label[]="Data 1";label[5]+=(char)i;
            draw_rect(22,y,136,28,C_BLACK);draw_rect(23,y+1,134,26,i==title.row?C_WHITE:C_LGRAY);
            choice_row(&cursor,i,label,C_BLACK);
            if(s->kind==SLOT_VALID) {
                title_ramza(26,y+1);black(47,y+11,"Ramza");black(83,y+11,"Lv");number(94,y+11,s->level);
                black(47,y+19,places[s->place]);
            } else black(65,y+13,s->kind==SLOT_DAMAGED?"Damaged data":"No data");
        }
        if(title.screen==TITLE_FILES)choice_cursor_animated(&cursor,title.row,rt_frame);
        draw_rect(0,94,160,6,C_BLACK);text(3,95,"ENTER:select  SHIFT:back",C_WHITE,159);
        if(title.screen==TITLE_OVERWRITE) {
            static const ChoiceLayout menu={61,39,98,27,3,CHOICE_TAB,3,1,0,6,10,8,157,-15,7};
            choice_frame(&menu,"Overwrite data?");choice_row(&menu,0,"Overwrite",C_BLACK);choice_row(&menu,1,"Return",C_BLACK);
            choice_cursor_animated(&menu,title.answer,rt_frame);
        }
    }
    if(title.notice) {
        static const char *const notes[]={"","No saved data.","Cannot read this data.","Could not save data."};
        panel(20,39,120,23);black(27,43,notes[title.notice]);black(27,52,"ENTER or SHIFT");
    }
}
