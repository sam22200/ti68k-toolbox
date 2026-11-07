// Requires CHOICE_TEXT(x,y,label,color,limit), CHOICE_WIDTH(label), art.h and rt.h.
#include "choice_menu.h"
static void choice_hand_at(s16 x,s16 y)
{
    RtSprite sp={32,HAND_H,hand_gfx[0],hand_gfx[1],hand_gfx[2]};
    draw_sprite(x,y,&sp);
}

static void choice_frame(const ChoiceLayout *p,const char *title)
{
    s16 y=p->y+p->body_y,h=p->height-p->body_y;
    if(p->style==CHOICE_BARE)return;
    draw_rect(p->x,y,p->width,h,C_BLACK);
    draw_rect(p->x+1,y+1,p->width-2,h-2,p->style!=CHOICE_PANEL?C_LGRAY:C_WHITE);
    if(p->style!=CHOICE_PANEL) {
        if(p->style==CHOICE_TAB)draw_rect(p->x+1,y+1,1,h-2,C_WHITE);
        draw_rect(p->x,p->y,p->title_width?p->title_width:CHOICE_WIDTH(title)+6,8,C_BLACK);
    } else draw_rect(p->x+2,y+2,p->width-4,1,C_LGRAY);
    CHOICE_TEXT(p->x+p->title_x,p->y+p->title_y,title,
                p->style!=CHOICE_PANEL?C_WHITE:C_BLACK,p->limit);
}

static void choice_row(const ChoiceLayout *p,u8 row,const char *label,u8 color)
{
    CHOICE_TEXT(p->x+p->text_x,p->y+p->row_y+(u16)row*p->pitch,label,color,p->limit);
}

static void choice_cursor(const ChoiceLayout *p,u8 row)
{
    choice_hand_at(p->x+p->hand_x,p->y+p->hand_y+(u16)row*p->pitch);
}

static void choice_cursor_animated(const ChoiceLayout *p,u8 row,u16 frame)
{
    // Same bounded three-pixel sway as battle; no saved state or extra graphics.
    static const u8 sway[8]={0,1,2,3,3,2,1,0};
    choice_hand_at(p->x+p->hand_x+sway[(frame>>2)&7],p->y+p->hand_y+(u16)row*p->pitch);
}

static void choice_list(const ChoiceLayout *p,const char *const *labels,u8 count,
                        u8 selected,u8 color,u8 other,u16 frame)
{
    u8 row;
    for(row=0;row<count;row++)choice_row(p,row,labels[row],row==selected?color:other);
    choice_cursor_animated(p,selected,frame);
}

#undef CHOICE_TEXT
#undef CHOICE_WIDTH
