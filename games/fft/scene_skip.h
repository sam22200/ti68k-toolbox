#ifndef FFT_SCENE_SKIP_H
#define FFT_SCENE_SKIP_H
#include "choice_menu.h"
// Shared scene controls: 0 closed, 1 No (default), 2 Yes. Return 0 for normal
// play, 1 for a consumed modal frame, 2 only for explicit confirmed skipping.
static u8 scene_skip_input(u8 *prompt)
{
    if(!*prompt){if(input_pressed(K_ESC)){*prompt=1;return 1;}return 0;}
    if(input_pressed(K_UP|K_DOWN))*prompt=(u8)(3-*prompt);
    if(input_pressed(K_ESC|K_B))*prompt=0;
    else if(input_pressed(K_A|K_ENTER)){
        u8 result=*prompt==2?2:1;*prompt=0;return result;
    }
    return 1;
}
static void choice_frame(const ChoiceLayout *p,const char *title);
static void choice_row(const ChoiceLayout *p,u8 row,const char *label,u8 color);
static void choice_cursor_animated(const ChoiceLayout *p,u8 row,u16 frame);
static void scene_skip_draw(u8 prompt,u16 frame)
{
    static const ChoiceLayout menu={36,30,88,42,10,CHOICE_TAB,3,1,0,28,22,11,122,4,16};
    choice_frame(&menu,"Skip scene?");
    choice_row(&menu,0,"No",C_BLACK);choice_row(&menu,1,"Yes",C_BLACK);
    choice_cursor_animated(&menu,(u8)(prompt-1),frame);
}
#endif
