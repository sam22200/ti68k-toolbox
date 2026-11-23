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
// Where the glove goes, so that every menu places it alike (CHOICE_MENU.md rule 4).
// The sprite is 32 wide and 14 tall; its pointing finger ends at +16 on rows 4..6, and the
// sway adds up to 3. At maximum sway the tip is text_x-2: exactly one empty
// pixel (text_x-1) separates it from the text. It enters the box, and the finger --
// not the fist -- sits on the label's own middle row.
#define CHOICE_HAND_X(text_x) ((s8)((text_x)-21))
#define CHOICE_HAND_Y(row_y)  ((s8)((row_y)-2))
static u8 choice_move(u8 row,u8 count,u32 pressed)
{
    if(!count)return 0;
    if(row>=count)row=0;
    if(pressed&K_UP)row=row?row-1:count-1;
    if(pressed&K_DOWN)row=row+1<count?row+1:0;
    return row;
}
#endif
