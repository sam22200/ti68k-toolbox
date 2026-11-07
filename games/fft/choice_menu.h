#ifndef FFT_CHOICE_MENU_H
#define FFT_CHOICE_MENU_H
// Screen-local geometry; owners retain their choices, validation and cached planes.
typedef struct {
    s16 x,y;
    u8 width,height,body_y,style;
    u8 title_x,title_y,title_width;
    u8 text_x,row_y,pitch,limit;
    s8 hand_x,hand_y;
} ChoiceLayout;
enum { CHOICE_TAB,CHOICE_PANEL,CHOICE_TAB_PLAIN,CHOICE_BARE };
static u8 choice_move(u8 row,u8 count,u32 pressed)
{
    if(!count)return 0;
    if(row>=count)row=0;
    if(pressed&K_UP)row=row?row-1:count-1;
    if(pressed&K_DOWN)row=row+1<count?row+1:0;
    return row;
}
#endif
