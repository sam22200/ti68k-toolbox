// PC-only design alternatives. Use the real management snapshot/portrait/plate and menus.
#include "manage.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <assert.h>
#include <stdio.h>
#include "../base.inc"
#define DLG_TEXT(x,y,s,c,l) text(x,y,s,c,l)
#define DLG_WIDTH(s) card_width(s)
#include "../dialogue_box.inc"
#define WF_TEXT(x,y,s,c,l) text(x,y,s,c,l)
#define WF_WIDTH(s) card_width(s)
#include "../war_funds.inc"
#define map_menu_width(s) card_width(s)
#include "../bar_reader.inc"

static const char *output;
static const u8 *room;
static u8 source[1024];
static const char *title,*description;
static const u8 candidates[]={1,2,3,4},chosen[]={1,3};
static u16 fee;static u8 min_days,max_days;

static void shot(const char *name)
{char path[512];snprintf(path,sizeof path,"%s/%s.png",output,name);assert(!sw_write_png(path,1));}
static void background(void)
{draw_clear();memcpy(rt_light,room+360,3000);memcpy(rt_dark,room+3360,3000);}
static ChoiceLayout box(s16 x,s16 y,u8 width,u8 rows)
{
    ChoiceLayout p={0,0,0,0,7,CHOICE_TAB,3,1,0,6,12,9,158,0,0};
    p.x=x;p.y=y;p.width=width;p.height=(u8)(13+9*rows);
    p.hand_x=CHOICE_HAND_X(p.text_x);p.hand_y=CHOICE_HAND_Y(p.row_y);
    assert(x+p.width<=160);return p;
}
static u8 is_chosen(u8 slot){return slot==chosen[0]||slot==chosen[1];}
static u32 funds(void)
{return (u32)save.money[0]<<24|(u32)save.money[1]<<16|(u16)save.money[2]<<8|save.money[3];}
static void help(void)
{war_funds(funds());plate_pair(105,"ENTER mark","F5 next  SHIFT back");}
static void team(u8 count,s16 x,s16 y)
{
    const char *names[3];u8 i,w=0;ChoiceLayout p;
    for(i=0;i<count;i++){names[i]=i<2?unit_name(unit(chosen[i])):"-";if(card_width(names[i])>w)w=card_width(names[i]);}
    p=box(x,y,(u8)(w+12),count);choice_frame(&p,count==3?"Team 2/3":"Team");
    for(i=0;i<count;i++)choice_row(&p,i,names[i],C_BLACK);
}
static void workers(u8 variant)
{
    ChoiceLayout p;u8 i,w=0;
    background();if(variant)draw_rect(0,32,160,68,C_WHITE);
    // This is exactly management's real unit sheet plate, not a second stat/portrait renderer.
    plate_unit(unit(candidates[1]),RT_NULL,0);
    if(!variant) {
        for(i=0;i<4;i++)if(card_width(unit_name(unit(candidates[i])))>w)w=card_width(unit_name(unit(candidates[i])));
        p=box(24,36,(u8)(w+12),3);choice_frame(&p,"Units");
        for(i=0;i<3;i++)choice_row(&p,i,unit_name(unit(candidates[i])),is_chosen(candidates[i])?C_DGRAY:C_BLACK);
        // Same shared scroll marks as every common list; only four fixture units qualify.
        choice_scroll_marks(&p,0,1);
        team(3,96,36);
    } else {
        u8 names=0,jobs=0;char label[32];
        for(i=0;i<4;i++) {
            u8 a=card_width(unit_name(unit(candidates[i]))),b=card_width(job_name(unit(candidates[i]),career(unit(candidates[i]))));
            if(a>names)names=a;if(b>jobs)jobs=b;
        }
        p=box(24,36,(u8)(names+jobs+39),3);choice_frame(&p,"Units 2/3");
        for(i=0;i<3;i++) {
            const SaveUnit *u=unit(candidates[i]);s16 y=(s16)(p.y+p.row_y+i*p.pitch);
            snprintf(label,sizeof label,"%c %s",is_chosen(candidates[i])?'*':' ',unit_name(u));
            choice_row(&p,i,label,C_BLACK);
            text((s16)(p.x+names+20),y,job_name(u,career(u)),C_BLACK,156);
            number((s16)(p.x+p.width-13),y,u->level);
        }
        choice_scroll_marks(&p,0,1);
    }
    assert(p.y+p.height<80);choice_cursor_animated(&p,1,rt_frame);help();
}
static void detail(void)
{
    char footer[64];background();
    snprintf(footer,sizeof footer,"%ug  %u-%ud   ENTER take",fee,min_days,max_days);
    assert(card_width(footer)<=148);
    town_reader(room+12,title,description,0,"Proposition",footer);
    dlg_continue(151,91,rt_frame,1);
}
static void confirm(void)
{
    const char *const rows[]={"Yes","No"};char question[48];ChoiceLayout p;
    background();snprintf(question,sizeof question,"Send 2 units for %u days?",min_days);
    dlg_town_says(room+12,"Bar owner",title,question);
    p=box(24,36,(u8)(card_width("Yes")+12),2);
    choice_frame(&p,"Send?");choice_row(&p,0,rows[0],C_BLACK);choice_row(&p,1,rows[1],C_BLACK);
    choice_cursor_animated(&p,1,rt_frame);team(2,92,36);
    {s16 left=war_funds(funds());plate_pair((s16)(left-3),"Fee",wf_digits(question,fee));}
}
int main(int argc,char **argv)
{
    FILE *f;char path[512],name[32];size_t size;u16 bytes;u8 i,phase;Save before;
    assert(argc==2);output=argv[1];sw_init(0);assert(bank&&common&&identity&&save.n==5);
    room=rt_file("ffbr",&bytes);assert(room&&bytes>=6360&&!memcmp(room,"FBR1",4));
    snprintf(path,sizeof path,"%s/sample.bin",output);f=fopen(path,"rb");assert(f);
    size=fread(source,1,sizeof source,f);assert(feof(f)&&size>6);fclose(f);
    min_days=source[0];max_days=source[1];fee=b16(source+2);title=(const char *)source+4;
    assert(memchr(title,0,size-4));description=title+strlen(title)+1;
    assert(description<(const char *)source+size&&memchr(description,0,size-(description-(const char *)source)));
    assert(fee==600&&min_days==8&&max_days==11);
    for(i=0;i<4;i++)assert(unit(candidates[i])->type<2&&unit(candidates[i])->job>=0x4a&&unit(candidates[i])->job<=0x5d);
    before=save;rt_frame=0;detail();shot("detail");confirm();shot("confirm");
    for(i=0;i<2;i++) {
        workers(i);shot(i?"b-workers":"a-workers");
        for(phase=0;phase<=8;phase++) {
            rt_frame=(u16)(phase*4);workers(i);snprintf(name,sizeof name,"%c-hand-%u",i?'b':'a',phase);shot(name);
        }
    }
    assert(!memcmp(&save,&before,sizeof save)&&!mg.changed);leave();
    puts("Propositions review: shared management plate, town dialogue/reader/menus; Save unchanged; shipping disabled.");
    return 0;
}
