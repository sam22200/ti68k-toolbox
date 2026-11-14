// Reviewed compact combat-style menu. The dialogue/portrait remain its owner's.
static void scene_choice_draw(const u8 *record,u8 selected,u8 error,u16 frame)
{
    const char *labels[2];u8 width=64,i,recruit=0;
    ChoiceLayout p={0,4,64,27,3,CHOICE_TAB,3,1,0,6,10,6,152,-16,7};
    for(i=0;i<2;i++){
        u8 needed;labels[i]=(const char *)record+sc_word(sc_option(record,i)+2);
        needed=(u8)(scene_text_width(labels[i])+10);if(needed>width)width=needed;
        {const u8 *o=sc_option(record,i);u8 j;
         for(j=0;j<o[6];j++)if(record[sc_word(o+4)+4*j]==SC_RECRUIT_GUEST)recruit=1;}
    }
    p.width=width;p.x=156-width;
    choice_frame(&p,recruit?"Recruit?":"Choice");choice_list(&p,labels,2,selected,C_BLACK,C_BLACK,frame);
    if(error){
        s16 y=p.y+p.height+3;
        const char *text=error==SC_PARTY_FULL?"Party is full.":error==SC_IO?"Save failed.":
                         error==SC_MISSING_GUEST?"Guest absent":"Choice error";
        draw_rect(p.x,y,p.width,11,C_BLACK);draw_rect(p.x+1,y+1,p.width-2,9,C_WHITE);
        ui_text(p.x+4,y+2,text,C_BLACK,p.limit);
    }
}
