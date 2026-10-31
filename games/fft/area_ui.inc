// Approved manual area UI. Never load another image from this image.
#if !MOD_CORE
#define CHOICE_TEXT(x,y,s,c,l) ui_text(x,y,s,c,l)
#define CHOICE_WIDTH(s) ui_width(s)
#if defined(__m68k__) && defined(FFT_REACH_CONTOUR)
#define hand_gfx CONTOUR_REF_hand_gfx
#endif
#include "choice_menu.inc"
#endif
#include "forecast_box.inc"
static void area_page(void)
{
    AreaUi *q=&W->area_ui;
    if(q->row<q->top)q->top=q->row;
    if(q->row>=q->top+AREA_UI_ROWS)q->top=q->row-AREA_UI_ROWS+1;
}
// Same scalar presentation, using the already computed batch, without queries.
static void area_line(u8 t,char *s)
{
    const CastVictim *v=&W->area_ui.forecast.victim[t];
    u8 n=0,kind=v->effect&EFFECT_KIND_MASK,harm=effect_damage(kind);u16 amount=v->amount;
    if(!v->hit||kind==AB_NULL)n=put_s(s,n,"Miss");
    else if(kind==AB_QUICK||kind==AB_GOLEM)n=put_s(s,n,kind==AB_GOLEM?"Golem":amount>>8?"CT 0":"Quick");
    else if(kind==AB_BRAVE||kind==AB_FAITH){s8 v=(s8)amount;n=put_s(s,n,kind==AB_BRAVE?"Br":"Fa");s[n++]=v<0?'+':'-';n=put_n(s,n,(u16)(v<0?-v:v));}
    else if(kind==AB_STATUS||kind==AB_CLEANSE)n=put_s(s,n,"Status");
    else if(kind==AB_MP_DAMAGE||kind==AB_MP_HEAL){s[n++]=kind==AB_MP_DAMAGE?'-':'+';n=put_n(s,n,amount);n=put_s(s,n,"MP");}
    else{
        if((s16)amount<0){harm=!harm;amount=(u16)(0U-amount);}
        if(harm&&st.unit[t].hp&&amount>=st.unit[t].hp)n=put_s(s,n,"KO");
        else{
            if(!harm)amount=action_hp(AB_HEAL,st.unit[t].hp,st.unit[t].max_hp,amount)-st.unit[t].hp;
            s[n++]=harm?'-':'+';n=put_n(s,n,amount);n=put_s(s,n,"HP");
        }
    }
    s[n++]=' ';n=put_n(s,n,v->hit);s[n++]='%';s[n]=0;
}
static u8 area_input(u8 begin)
{
    AreaUi *q=&W->area_ui;u32 pressed=rt_keys&~rt_prev;
    if(begin){
        if(!cast_can_order(st.act,(u8)CURSOR_CELL,st.abil,0)){st.deny=16;return AREA_NONE;}
        memset(q,0,sizeof *q);q->centre=(u8)CURSOR_CELL;
        q->forecast.actor=st.act;q->forecast.ability=st.abil;q->forecast.ref=q->centre;
        st.mode=M_AREA;return AREA_QUERY;
    }
    if(pressed&(K_ESC|K_B)){
        if(q->confirm)q->confirm=0;else return AREA_BACK;
    }else if(q->dirty||q->weather!=st.weather){
        q->ready=0;q->forecast.phase=CAST_BATCH_EMPTY;return AREA_QUERY;
    }else if(!q->confirm){
        u8 old=q->row;q->row=choice_move(q->row,q->count,pressed);area_page();
        if(old!=q->row)q->forecast_dirty=1;
        if(pressed&(K_A|K_ENTER))q->confirm=1,q->answer=0;
    }else{
        u8 count=q->follow_id==255?2:3;
        q->answer=choice_move(q->answer,count,pressed);
        if(pressed&(K_A|K_ENTER)){
            if(q->answer==count-1)q->confirm=0;else return AREA_SEND;
        }
    }
    return AREA_NONE;
}
static void area_draw(u8 header)
{
    AreaUi *q=&W->area_ui;char s[48];u8 k,n,allies=0;
    if(!header&&q->ready){
        if(q->forecast_dirty){
            for(k=0;k<AREA_UI_ROWS&&q->top+k<q->count;k++)area_line(q->ids[q->top+k],q->line[k]);
            if(q->count)forecast_panel_text(q->line[q->row-q->top]);
            q->forecast_dirty=0;
        }
        area_paint(q->count?q->ids[q->row]+1:0);
    }
    draw_rect(0,0,160,8,C_BLACK);n=put_s(s,0,ABIL_NAME(st.abil));
    if(header)n=put_s(s,n," / Select panel");
    else{
        for(k=0;k<q->count;k++)if(st.unit[q->ids[k]].team!=TEAM_ENEMY)allies++;
        n=put_s(s,n," / ");n=put_n(s,n,q->count);n=put_s(s,n," targets");
        if(allies){n=put_s(s,n," / ");n=put_n(s,n,allies);n=put_s(s,n," ally");}
    }
    s[n]=0;ui_text(2,1,s,C_WHITE,158);if(header||!q->ready)return;
    if(!q->confirm){
        static const ChoiceLayout layouts[4]={
            {61,11,98,22,3,CHOICE_TAB,3,1,0,7,10,8,157,-16,7},
            {61,11,98,30,3,CHOICE_TAB,3,1,0,7,10,8,157,-16,7},
            {61,11,98,38,3,CHOICE_TAB,3,1,0,7,10,8,157,-16,7},
            {61,11,98,46,3,CHOICE_TAB,3,1,0,7,10,8,157,-16,7}};
        u8 shown=q->count>AREA_UI_ROWS?AREA_UI_ROWS:q->count;
        const ChoiceLayout *p=&layouts[(shown?shown:1)-1];choice_frame(p,"Targets");
        if(!q->count)choice_row(p,0,"No targets",C_BLACK);
        for(k=0;k<shown;k++){
            u8 t=q->ids[q->top+k];s16 y=p->y+p->row_y+k*p->pitch;
            ui_text(p->x+p->text_x,y,unit_name(t),C_BLACK,104);ui_text(106,y,q->line[k],C_BLACK,157);
        }
        if(q->count)choice_cursor_animated(p,q->row-q->top,st.tick);
        for(k=0;k<2;k++)if(k?q->top+shown<q->count:q->top){
            u8 y=k?p->y+p->height-4:p->y+4,r;
            for(r=0;r<3;r++)draw_rect(153-(k?2-r:r),y+r,k?5-2*r:1+2*r,1,C_BLACK);
        }
    }else{
        static const char labels[3][7]={"Unit","Panel","Cancel"};
        static const ChoiceLayout layouts[2]={
            {95,17,64,30,3,CHOICE_TAB,3,1,0,6,10,8,157,-16,7},
            {95,17,64,38,3,CHOICE_TAB,3,1,0,6,10,8,157,-16,7}};
        u8 fixed=q->follow_id==255,count=fixed?2:3;
        const ChoiceLayout *p=&layouts[!fixed];choice_frame(p,"Target");
        for(k=0;k<count;k++)choice_row(p,k,labels[k+fixed],C_BLACK);
        choice_cursor_animated(p,q->answer,st.tick);
    }
}
#if !MOD_CORE
u16 mod_entry(u8 op,u8 a)
{
    (void)a;if(op==AREA_DRAW||op==AREA_HEADER){area_draw(op==AREA_HEADER);return 0;}
    return area_input(op==AREA_BEGIN);
}
#endif
