// Existing choice frame/font, with an idle AI buffer as the five-slot draft.
static const char equipment_slots[6][10]={"Head","Body","Accessory","R.Hand","L.Hand","Done"};
static u8 equipment_stock(u8 id)
{
    s16 n=battle_save.inv[id];u8 k;
    for(k=0;k<5;k++){n+=st.unit[st.act].eq[k]==id;n-=W->ai_tt[k]==id;}
    return n<0?0:n>99?99:(u8)n;
}
static void equipment_choices(void)
{
    const Unit *u=&st.unit[st.act];u16 id;W->ai_no=1;W->ai_grid[0]=255;
    for(id=1;id<255;id++)if(equipment_stock((u8)id)&&
        core_equipment_allowed(common,u->job,u->female,st.support[st.act],W->ai_tt,st.status_row,(u8)id))
        W->ai_grid[W->ai_no++]=(u8)id;
}
static void equipment_input(void)
{
    u32 pressed=rt_keys&~rt_prev;
    if(!equipment_ready(st.act)){st.mode=M_MENU;return;}
    if(pressed&(K_B|K_ESC)){
        if(st.status_col){st.status_col=0;st.sub=st.status_row;}else st.mode=M_ACTS;
        return;
    }
    st.sub=choice_move(st.sub,st.status_col?(u8)W->ai_no:6,pressed);
    if(!(pressed&(K_ENTER|K_A)))return;
    if(!st.status_col){
        if(st.sub==5){if(!equipment_commit(st.act,W->ai_tt))st.deny=12;return;}
        st.status_row=st.sub;equipment_choices();st.status_col=1;st.sub=0;
    }else {W->ai_tt[st.status_row]=W->ai_grid[st.sub];W->status_i=0;st.status_col=0;st.sub=st.status_row;}
}
static void equipment_row(const ChoiceLayout *menu,u8 first,u8 row)
{
    char s[38];u8 id,n=0,k=row+first;
    if(st.status_col){
        id=W->ai_grid[k];n=put_s(s,0,id==255?"Remove":items[id].name);
        if(id!=255){s[n++]=' ';n=put_n(s,n,equipment_stock(id));}
    }else{
        n=put_s(s,0,equipment_slots[k]);
        if(k<5){id=W->ai_tt[k];n=put_s(s,n,": ");n=put_s(s,n,id==255?"-":items[id].name);}
    }
    s[n]=0;choice_row(menu,row,s,C_BLACK);
}
static void equipment_draw(void)
{
    static const ChoiceLayout menu={20,12,136,77,3,CHOICE_TAB,3,1,0,6,10,6,156,-16,7};
    void *sl=rt_light,*sd=rt_dark;
    u8 first=st.status_col?(st.sub>>3)<<3:0,count=st.status_col?(u8)W->ai_no:6,i;
    if(count-first>8)count=first+8;
    if(W->status_i!=st.act+1||W->status_page!=st.status_col||
       W->status_row!=st.status_row||W->status_col!=first){
        W->status_i=st.act+1;W->status_page=st.status_col;W->status_row=st.status_row;
        W->status_col=first;W->status_n=0;W->status_info=255;
    }
    // The tactical portrait cache is idle in this full-screen mode, as in Status.
    rt_light=W->status[0];rt_dark=W->status[1];
    if(!W->status_n){
        draw_rect(0,0,160,100,C_WHITE);ui_text(5,2,unit_name(st.act),C_BLACK,156);
        choice_frame(&menu,st.status_col?equipment_slots[st.status_row]:"Equip Change");
        ui_text(5,94,"ENTER: select  SHIFT: back",C_BLACK,158);
        equipment_row(&menu,first,st.sub-first);W->status_n=1;W->status_info=st.sub;
    }else{
        if(W->status_info!=st.sub){equipment_row(&menu,first,st.sub-first);W->status_info=st.sub;}
        for(i=0;i<2&&W->status_n<=count-first;i++)equipment_row(&menu,first,W->status_n++-1);
    }
    rt_light=sl;rt_dark=sd;copy_box(W->status[0],W->status[1],0,0,10,100);
    draw_hand(menu.x+menu.hand_x,menu.y+menu.hand_y+6*(st.sub-first));
}
