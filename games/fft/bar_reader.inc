#ifndef FFT_BAR_READER_INC
#define FFT_BAR_READER_INC
// Accepted reader A. Caller restores the room and supplies the shared town font/wrapper.
// Compose once per page; draw dlg_continue(151,91,clock,1) after the cached page blit.
static void town_reader(const u8 *face,const char *title,const char *article,u8 first,
                        const char *tab,const char *footer)
{
    static const ChoiceLayout box = { 5,33,150,61,7,CHOICE_TAB,3,1,0,6,12,9,154,0,0 };
    char line[64]; u8 row;
    dlg_face(face,1,1,0);
    dlg_name(24,1,(u8)(map_menu_width("Bar owner")+8),"Bar owner",0);
    dlg_name(24,13,(u8)(map_menu_width(title)+8),title,0);
    choice_frame(&box,tab);
    for(row=0;row<7;row++) {
        if(!item_detail_line(article,142,(u8)(first+row),line))break;
        text(9,(s16)(46+row*7),line,C_BLACK,153);
    }
    draw_rect(4,94,152,6,C_BLACK);
    text(7,95,footer,C_WHITE,155);
}
static void bar_reader(const u8 *face,const char *title,const char *article,u8 first)
{town_reader(face,title,article,first,"Rumor","L/R page     SHIFT back");}
#endif
