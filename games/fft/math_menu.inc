// Math uses the existing paged skill menu and shared area confirmation.
static u8 math_menu(void)
{
    u32 pressed=rt_keys&~rt_prev;u8 n;AbId ab;
    if(st.mode==M_ACTS){
        if(known(st.act,0)==NO_ABIL)return 0;
        st.mode=M_SKILLS;st.sub=128;st.skill=0;
    }else if(pressed&(K_ESC|K_B)){
        if((st.sub&96)==0){st.mode=M_ACTS;st.sub=1;}
        else st.sub=(st.sub&15)|((st.sub&96)==64?160:128);
        st.skill=0;
    }else{
        n=(u8)known(st.act,255);st.skill=choice_move(st.skill,n,pressed);
        if(pressed&(K_A|K_ENTER)){
            ab=known(st.act,st.skill);if(ab==NO_ABIL)return 0;
            if((st.sub&96)==0)st.sub=160|((ab-414)<<2);
            else if((st.sub&96)==32)st.sub=192|(st.sub&12)|(ab-418);
            else if(can_use(st.act,ab)){st.abil=ab;return 2;}
            st.skill=0;
        }
    }
    if(pressed&(K_A|K_ENTER|K_B|K_ESC))W->win_k[0]=0;
    return 0;
}
