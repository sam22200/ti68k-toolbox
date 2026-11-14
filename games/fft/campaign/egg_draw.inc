// Approved formation egg/card and compact menu coordinates. Archive sprites
// are pooled, read in place and keep the existing16x26 three-plane convention.
static u8 egg_art_valid(const u8 *p,u16 size)
{
    u8 i;
    if(p&&size>=6&&!memcmp(p+size-6,"\0dat\0\xf8",6))size-=6;
    if(!p||size<16||memcmp(p,"FEG2",4)||!p[4]||p[4]>8||p[5]!=16||p[6]!=26||p[7]||
       size!=16U+(u16)p[4]*156U)return 0;
    for(i=0;i<8;i++)if(p[8+i]>=p[4])return 0;
    return 1;
}
static void egg_actor(s16 x,s16 y,u8 slot)
{
    u8 logical=(u8)(world_egg_color(&save.world,slot)*2+((save.world.egg[slot]&15)<2));
    const u8 *p=egg_bank+16+(u16)egg_bank[8+logical]*156;
#ifdef __m68k__
    RtSprite sprite={16,26,p,p+52,p+104};
#else
    u16 rows[3][26];u8 i,j;RtSprite sprite={16,26,rows[0],rows[1],rows[2]};
    for(i=0;i<3;i++)for(j=0;j<26;j++)rows[i][j]=b16(p+52*i+2*j);
#endif
    draw_sprite(x,y,&sprite);
}
static void formation_actor(s16 x,s16 y,u8 slot)
{
    if(save_egg(&save,slot))egg_actor(x,y,slot);
    else if(unit(slot)->type==3)monster_actor(x,y,unit(slot));
    else actor(x,y,picture(unit(slot),career(unit(slot))));
}
static void egg_card(u8 slot)
{
    u8 left=save.world.egg[slot]&15,total=save.world.egg[slot]>>4;
    draw_rect(0,63,160,37,C_WHITE);egg_actor(7,64,slot);
    black(30,66,"Monster Egg");number(30,76,left);black(37,76,left==1?"day to hatch":"days to hatch");
    draw_rect(30,85,122,5,C_BLACK);draw_rect(31,86,120,3,C_WHITE);
    draw_rect(31,86,(s16)((u16)(total-left)*120/total),3,C_DGRAY);
    black(37,95,"ALPHA:menu SHIFT:back");
}
static void egg_menu(u8 confirmation)
{
    static const ChoiceLayout menu={100,39,56,21,3,CHOICE_TAB,3,1,0,6,10,6,152,-16,7};
    static const ChoiceLayout confirm={92,33,64,27,3,CHOICE_TAB,3,1,0,6,10,6,152,-16,7};
    if(confirmation){
        static const char *const labels[]={"No","Yes"};
        choice_frame(&confirm,"Dismiss?");choice_list(&confirm,labels,2,confirmation-1,C_BLACK,C_BLACK,0);
    }else{
        choice_frame(&menu,"Egg");choice_row(&menu,0,"Dismiss",C_BLACK);choice_cursor_animated(&menu,0,0);
    }
}
