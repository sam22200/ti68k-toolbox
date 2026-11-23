// Real management mode: shared unit sheet, accepted Bar A and common panels.
#include "../base.inc"
#include "breeding.h"
#include "services.h"
#include "proposition_transactions.h"
#include "bar_bank.h"
#define DLG_TEXT(x,y,s,c,l) text(x,y,s,c,l)
#define DLG_WIDTH(s) card_width(s)
#include "../dialogue_box.inc"
#define WF_TEXT(x,y,s,c,l) text(x,y,s,c,l)
#define WF_WIDTH(s) card_width(s)
#include "../war_funds.inc"
#define map_menu_width(s) card_width(s)
#include "../bar_reader.inc"
#undef map_menu_width
enum { PS_HOME,PS_LIST,PS_TEXT,PS_WORKERS,PS_DAYS,PS_CONFIRM,PS_ACTIVE,PS_JOB,PS_RECALL,PS_REPORTS,PS_REPORT };
static struct {
    const u8 *rules,*rewards,*prose,*room,*gates,*clock,*stock;
    u8 valid,screen,row,top,id,page,n,team[3],days,replace,note;
    u8 candidates[16],count;
    ChoiceLayout cursor;
} ps;
static u32 ps_money(void){return prop_long(save.money);}
static void ps_label(char *out,const char *s,u8 width)
{
    u8 n=0,w=0;const u8 *advance=menu_font()+103;
    while(*s&&n<36){u8 c=(u8)*s;if(w+advance[c-32]>width)break;out[n++]=*s++;w+=advance[c-32];}
    if(*s&&n>=3){n-=3;out[n++]='.';out[n++]='.';out[n++]='.';}out[n]=0;
}
static void ps_join(char *out,const char *a,const char *b,const char *c)
{while(*a)*out++=*a++;while(*b)*out++=*b++;while(*c)*out++=*c++;*out=0;}
static ChoiceLayout ps_box(s16 x,s16 y,u8 width,u8 rows)
{
    ChoiceLayout p={0,0,0,0,7,CHOICE_TAB,3,1,0,6,12,9,158,0,0};
    p.x=x;p.y=y;p.width=width;p.height=(u8)(13+9*rows);
    p.hand_x=CHOICE_HAND_X(p.text_x);p.hand_y=CHOICE_HAND_Y(p.row_y);return p;
}
static PropositionContext ps_context(void)
{
    PropositionContext c;u8 i;
    c.unlocked=service_open(&save,ps.gates,0);c.location=save.loc?(u8)(save.loc-1):255;c.tier=0;
    c.zodiac=(u8)(wb_zodiac(ps.clock,save.world.month,save.world.day)+1);
    if(ps.stock) {
        const u8 *f=ps.stock+6+512+256+256+6*((u16)ps.stock[5]+1)+43;
        for(i=1;i<16;i++)if(f[i]!=255&&(!f[i]||(save.flags[(f[i]-1)>>3]&(1U<<((f[i]-1)&7)))))c.tier=i;
    }
    return c;
}
static u8 ps_count(u8 screen)
{
    u8 i,n=0;PropositionContext c=ps_context();
    if(screen==PS_LIST){for(i=0;i<96;i++)if(prop_available(ps.rules,&save.missions.dispatches,i,&c))n++;}
    else for(i=0;i<save.missions.dispatches.n;i++)if(screen==PS_ACTIVE||prop_report_ready(&save.missions.dispatches,i,c.location))n++;
    return n;
}
static u8 ps_nth(u8 screen,u8 row)
{
    u8 i;PropositionContext c=ps_context();
    if(screen==PS_LIST){for(i=0;i<96;i++)if(prop_available(ps.rules,&save.missions.dispatches,i,&c)&&!row--)return i;}
    else for(i=0;i<save.missions.dispatches.n;i++)if((screen==PS_ACTIVE||prop_report_ready(&save.missions.dispatches,i,c.location))&&!row--)return save.missions.dispatches.job[i].id;
    return 255;
}
static void ps_screen(u8 screen){ps.screen=screen;ps.row=ps.top=0;dirty=1;}
static void ps_candidates(void)
{
    u8 i;u16 mask=service_workers(&save);ps.count=0;
    for(i=0;i<save.n;i++)if(mask&(1U<<i))ps.candidates[ps.count++]=i;
}
static u8 ps_selected(u8 slot)
{u8 i;for(i=0;i<ps.n;i++)if(ps.team[i]==slot)return 1;return 0;}
static u8 ps_end(u16 result)
{props_result=result;mg.changed=0;if(cache)free(cache);cache=RT_NULL;return 0;}
#ifndef __m68k__
static u8 ps_store(const Save *s,const PropositionState *state,void *ctx)
{(void)state;(void)ctx;return story_choice_store_file(s);}
#endif
static u8 ps_dispatch(void)
{
    PropositionContext c=ps_context();u32 money=ps_money();u8 result,old_status=prop_status(&save.missions.dispatches,ps.id);
    result=prop_dispatch(ps.rules,&save.missions.dispatches,ps.id,&c,ps.team,ps.n,ps.days,save.n,service_workers(&save),&money);
    if(result!=PROP_OK){ps.note=result;return 1;}
    prop_put_long(save.money,money);
    // A queued native save is durable only after runtime teardown. The
    // supervisor reenters this mode and reloads before showing its result.
#ifdef __m68k__
    rt_save(SAVE_NAME,&save,sizeof save);return ps_end(2);
#else
    if(!story_choice_store_file(&save)) {
        u8 j=--save.missions.dispatches.n;memset(&save.missions.dispatches.job[j],0,sizeof(PropositionJob));
        prop_set_status(&save.missions.dispatches,ps.id,old_status);prop_put_long(save.money,money+prop_fee(ps.rules,ps.id));
        ps.note=PROP_STORAGE;return 1;
    }
    return ps_end(2);
#endif
}
static u8 ps_prepare(u8 id)
{
    u8 result;PropositionContext c=ps_context();
#ifdef __m68k__
    result=prop_prepare(ps.rules,ps.rewards,&save.missions,&save,id,c.location,service_world_draw,&save);
    if(result==PROP_OK){rt_save(SAVE_NAME,&save,sizeof save);return ps_end(2);}
#else
    result=prop_prepare_saved(ps.rules,ps.rewards,&save.missions,&save,id,c.location,service_world_draw,&save,ps_store,RT_NULL);
    if(result==PROP_OK){ps.id=id;ps_screen(PS_REPORT);return 1;}
#endif
    ps.note=result;return 1;
}
static void props_start(u16 n)
{
    u16 size;memset(&ps,0,sizeof ps);props_on=1;props_result=1;
    ps.rules=rt_file("ffpr",&size);if(!prop_bank_valid(ps.rules,size))return;
    ps.rewards=rt_file("ffpe",&size);if(!prop_rewards_valid(ps.rewards,size)||!prop_reward_rows_valid(ps.rules,ps.rewards))return;
    ps.prose=rt_file("ffpt",&size);if(!service_text_valid(ps.prose,size))return;
    ps.room=rt_file("ffbr",&size);if(!bar_art_valid(ps.room,size))return;
    ps.gates=rt_file("ffsv",&size);if(!service_bank_valid(ps.gates,size))return;
    ps.clock=rt_file("ffwb",&size);if(!world_rules_valid(ps.clock,size))return;
    ps.stock=rt_file("ffsh",&size);if(!ps.stock||size<1468||memcmp(ps.stock,"FSH2",4))return;
    if(!prop_state_valid(&save.missions,ps.rules,save.n))return;
    ps.valid=1;ps.note=n==101?PROP_STORAGE:0;
    if(!service_open(&save,ps.gates,0)){ps.note=PROP_LOCKED;return;}
    if(save.missions.report.ready&&save.loc==save.missions.report.location+1){ps.id=save.missions.report.id;ps.screen=PS_REPORT;}
    dirty=1;
}
static u8 props_update(u32 key)
{
    u8 count;if(!key)return 1;dirty=1;
    if(!ps.valid)return key&(K_ESC|K_B)?ps_end(1):1;
    if(ps.note){if(key&(K_ENTER|K_A|K_B|K_ESC))ps.note=0;return 1;}
    if(!service_open(&save,ps.gates,0))return key&(K_ESC|K_B)?ps_end(1):1;
    if(ps.screen==PS_REPORT) {
        if(key&(K_ENTER|K_A)) {
            u8 result;PropositionContext c=ps_context();
#ifdef __m68k__
            result=prop_collect(&save.missions,ps.rules,&save,ps.id,c.location);
            if(result==PROP_OK){rt_save(SAVE_NAME,&save,sizeof save);return ps_end(2);}
#else
            result=prop_collect_saved(&save.missions,ps.rules,&save,ps.id,c.location,ps_store,RT_NULL);
            if(result==PROP_OK)return ps_end(2);
#endif
            ps.note=result;
        }
        // The report cannot be edited/replaced by another mission or roster
        // operation before its single settlement; ESC leaves it durably pending.
        if(key&(K_ESC|K_B))return ps_end(1);
        return 1;
    }
    if(key&(K_ESC|K_B)) {
        if(ps.replace){ps.replace=0;ps.row=ps.top;ps.top=ps.row>=3?ps.row-2:0;return 1;}
        if(ps.screen==PS_HOME)return ps_end(1);
        if(ps.screen==PS_CONFIRM||ps.screen==PS_DAYS){ps_screen(PS_WORKERS);return 1;}
        if(ps.screen==PS_TEXT){ps_screen(PS_LIST);return 1;}
        if(ps.screen==PS_WORKERS){ps_screen(PS_TEXT);return 1;}
        if(ps.screen==PS_RECALL){ps_screen(PS_JOB);return 1;}
        if(ps.screen==PS_JOB){ps_screen(PS_ACTIVE);return 1;}
        ps_screen(PS_HOME);return 1;
    }
    if(ps.screen==PS_HOME) {
        ps.row=choice_move(ps.row,4,key);
        if(key&(K_ENTER|K_A)){if(ps.row==3)return ps_end(1);ps_screen(ps.row==0?PS_LIST:ps.row==1?PS_ACTIVE:PS_REPORTS);}
    } else if(ps.screen==PS_LIST||ps.screen==PS_ACTIVE||ps.screen==PS_REPORTS) {
        count=ps_count(ps.screen);ps.row=choice_move(ps.row,count?count:1,key);
        if(ps.row<ps.top)ps.top=ps.row;if(ps.row>=ps.top+3)ps.top=(u8)(ps.row-2);
        if(count&&(key&(K_ENTER|K_A))) {
            ps.id=ps_nth(ps.screen,ps.row);ps.page=0;
            if(ps.screen==PS_REPORTS)return ps_prepare(ps.id);
            ps_screen(ps.screen==PS_LIST?PS_TEXT:PS_JOB);
        }
    } else if(ps.screen==PS_TEXT) {
        u8 pages=ps.prose[12+6*(u16)ps.id];
        if(key&K_LEFT){if(ps.page)ps.page--;}
        if(key&K_RIGHT){if(ps.page+1<pages)ps.page++;}
        if(key&(K_ENTER|K_A)) {
            ps_candidates();if(!ps.count){ps.note=PROP_TEAM;return 1;}
            if(save.missions.dispatches.n==8){ps.note=PROP_FULL;return 1;}
            if(ps_money()<prop_fee(ps.rules,ps.id)){ps.note=PROP_GIL;return 1;}
            ps.n=0;memset(ps.team,255,3);ps.days=prop_row(ps.rules,ps.id)[6];ps_screen(PS_WORKERS);
        }
    } else if(ps.screen==PS_WORKERS) {
        u8 slot,i;
        if(ps.replace) {
            ps.row=choice_move(ps.row,3,key);
            if(key&(K_ENTER|K_A)){ps.team[ps.row]=ps.candidates[ps.top];ps.replace=0;ps.row=ps.top;ps.top=ps.row>=3?ps.row-2:0;}
            return 1;
        }
        ps.row=choice_move(ps.row,ps.count,key);
        if(ps.row<ps.top)ps.top=ps.row;if(ps.row>=ps.top+3)ps.top=(u8)(ps.row-2);
        if(key&(K_ENTER|K_A)) {
            slot=ps.candidates[ps.row];
            for(i=0;i<ps.n&&ps.team[i]!=slot;i++);
            if(i<ps.n){for(;i+1<ps.n;i++)ps.team[i]=ps.team[i+1];ps.team[--ps.n]=255;}
            else if(ps.n<3)ps.team[ps.n++]=slot;
            else {ps.replace=1;ps.top=ps.row;ps.row=0;}
        }
        if(key&K_F5){if(!ps.n)ps.note=PROP_TEAM;else ps_screen(PS_DAYS);}
    } else if(ps.screen==PS_DAYS) {
        const u8 *r=prop_row(ps.rules,ps.id);
        if((key&(K_LEFT|K_DOWN))&&ps.days>r[6])ps.days--;
        if((key&(K_RIGHT|K_UP))&&ps.days<r[7])ps.days++;
        if(key&(K_ENTER|K_A)){ps_screen(PS_CONFIRM);ps.row=1;}
    } else if(ps.screen==PS_CONFIRM) {
        ps.row=choice_move(ps.row,2,key);
        if(key&(K_ENTER|K_A)){if(!ps.row)return ps_dispatch();ps_screen(PS_WORKERS);}
    } else if(ps.screen==PS_JOB) {
        ps.row=choice_move(ps.row,2,key);
        if(key&(K_ENTER|K_A)){if(!ps.row){ps_screen(PS_RECALL);ps.row=1;}else ps_screen(PS_ACTIVE);}
    } else if(ps.screen==PS_RECALL) {
        ps.row=choice_move(ps.row,2,key);
        if(key&(K_ENTER|K_A)) {
            if(!ps.row) {
                u8 slot=prop_job_index(&save.missions.dispatches,ps.id);
                if(prop_recall(&save.missions.dispatches,slot)) {
#ifdef __m68k__
                    rt_save(SAVE_NAME,&save,sizeof save);return ps_end(2);
#else
                    if(!story_choice_store_file(&save)){save.missions.dispatches.job[slot].flags&=(u8)~PROP_RECALLED;ps.note=PROP_STORAGE;return 1;}
                    return ps_end(2);
#endif
                }
            }
            ps_screen(PS_ACTIVE);
        }
    }
    return 1;
}
static void ps_team(u8 rows,s16 x,s16 y)
{
    u8 i,w=0;char tab[16],digit[9];ChoiceLayout p;
    for(i=0;i<rows;i++){u8 n=card_width(i<ps.n?unit_name(unit(ps.team[i])):"-");if(n>w)w=n;}
    ps_join(tab,"Team ",wf_digits(digit,ps.n),"/3");p=ps_box(x,y,(u8)(w+12),rows);
    choice_frame(&p,rows==3?tab:"Team");
    for(i=0;i<rows;i++)choice_row(&p,i,i<ps.n?unit_name(unit(ps.team[i])):"-",C_BLACK);
    if(ps.replace)ps.cursor=p;
}
static void props_page(void)
{
    char title[40],label[48],digits[9];u8 i;
    draw_clear();if(!ps.valid){draw_text(3,40,"Missing service data",0,C_BLACK);return;}
    memcpy(rt_light,ps.room+360,3000);memcpy(rt_dark,ps.room+3360,3000);
    if(ps.note||!service_open(&save,ps.gates,0)) {
        static const char *const notes[]={"","Available in Chapter II.","Not available here.","Eight missions already active.","Choose free human units.","Invalid duration.","Not enough gil.","Invalid mission state.","Save failed. Try again."};
        dlg_town_says(ps.room+12,"Bar owner",notes[ps.note?ps.note:PROP_LOCKED],"SHIFT back");war_funds(ps_money());return;
    }
    if(ps.screen==PS_WORKERS) {
        u8 w=0,shown=ps.count>3?3:ps.count,at=ps.replace?ps.top:ps.row;
        plate_unit(unit(ps.candidates[at]),RT_NULL,0);
        for(i=0;i<ps.count;i++){u8 n=card_width(unit_name(unit(ps.candidates[i])));if(n>w)w=n;}
        ps.cursor=ps_box(24,36,(u8)(w+12),shown);choice_frame(&ps.cursor,"Units");
        for(i=0;i<shown;i++) {
            u8 index=(u8)((ps.replace?(at>=3?at-2:0):ps.top)+i);
            if(index<ps.count)choice_row(&ps.cursor,i,unit_name(unit(ps.candidates[index])),ps_selected(ps.candidates[index])?C_DGRAY:C_BLACK);
        }
        choice_scroll_marks(&ps.cursor,!ps.replace&&ps.top,!ps.replace&&ps.top+shown<ps.count);
        ps_team(3,96,36);war_funds(ps_money());plate_pair(105,"ENTER mark","F5 next  SHIFT back");return;
    }
    if(ps.screen==PS_TEXT) {
        const u8 *r=ps.prose+8+6*(u16)ps.id;const char *article=(const char *)ps.prose+prop_word(r);
        u16 off=prop_word(ps.prose+prop_word(r+2)+2*(u16)ps.page);
        ps_label(title,prop_title(ps.rules,ps.id),124);
        ps_join(label,wf_digits(digits,prop_fee(ps.rules,ps.id)),"g  ENTER take","");
        town_reader(ps.room+12,title,article+off,0,"Proposition",label);return;
    }
    if(ps.screen==PS_REPORT) {
        const PropositionReport *r=&save.missions.report;ChoiceLayout p=ps_box(5,33,150,5);
        plate_unit(unit(r->speaker),RT_NULL,0);p.height=61;choice_frame(&p,"Report");
        text(9,46,r->result==0?"Success":r->result==1?"Failed":"Unsuccessful",C_BLACK,153);
        ps_join(label,"Gil +",wf_digits(digits,prop_long(r->gil)),"");text(9,53,label,C_BLACK,153);
        for(i=0;i<r->n;i++) {
            char name[32];ps_label(name,unit_name(unit(r->unit[i])),76);
            ps_join(label,name," JP +",wf_digits(digits,prop_word(r->jp[i])));text(9,(s16)(60+7*i),label,C_BLACK,153);
        }
        if(r->kind) {
            u8 index=(u8)(r->kind==PROP_TREASURE?46+r->index:r->kind==PROP_LAND?27+r->index:r->index);
            const char *name=(const char *)ps.prose+prop_word(ps.prose+584+2*(u16)index);
            if(item_detail_line(name,142,0,label))text(9,81,label,C_BLACK,153);
            if(item_detail_line(name,142,1,label))text(9,88,label,C_BLACK,153);
        }
        draw_rect(4,94,152,6,C_BLACK);text(7,95,"ENTER collect   SHIFT later",C_WHITE,155);return;
    }
    ps_label(title,prop_title(ps.rules,ps.id),124);
    if(ps.screen==PS_CONFIRM||ps.screen==PS_DAYS) {
        ps_join(label,"Send ",wf_digits(digits,ps.n)," units?");
        dlg_town_says(ps.room+12,"Bar owner",title,ps.screen==PS_DAYS?"How many days?":label);
        if(ps.screen==PS_DAYS) {
            const char *day=wf_digits(digits,ps.days);
            ps.cursor=ps_box(24,36,(u8)(card_width(day)+12),1);choice_frame(&ps.cursor,"Days");choice_row(&ps.cursor,0,day,C_BLACK);
            plate_pair(105,"L/R days","ENTER next SHIFT back");
        } else {
            ps.cursor=ps_box(24,36,(u8)(card_width("Yes")+12),2);choice_frame(&ps.cursor,"Send?");
            choice_row(&ps.cursor,0,"Yes",C_BLACK);choice_row(&ps.cursor,1,"No",C_BLACK);ps_team(ps.n,92,36);
            {s16 left=war_funds(ps_money());plate_pair((s16)(left-3),"Fee",wf_digits(digits,prop_fee(ps.rules,ps.id)));}return;
        }
        war_funds(ps_money());return;
    }
    if(ps.screen==PS_JOB||ps.screen==PS_RECALL) {
        u8 slot=prop_job_index(&save.missions.dispatches,ps.id);const PropositionJob *j=&save.missions.dispatches.job[slot];
        dlg_town_says(ps.room+12,"Bar owner",title,(j->flags&PROP_RETURNED)?"Returned to the origin Bar.":"Your units are away.");
        ps.cursor=ps_box(24,36,(u8)(card_width(ps.screen==PS_RECALL?"Yes":"Recall")+12),2);choice_frame(&ps.cursor,ps.screen==PS_RECALL?"Recall?":"Mission");
        choice_row(&ps.cursor,0,ps.screen==PS_RECALL?"Yes":"Recall",C_BLACK);
        choice_row(&ps.cursor,1,ps.screen==PS_RECALL?"No":"Back",C_BLACK);
        ps_join(label,wf_digits(digits,j->elapsed)," days elapsed","");text(24,75,label,C_WHITE,156);war_funds(ps_money());return;
    }
    dlg_town_says(ps.room+12,"Bar owner","Welcome. Come in","and relax.");
    if(ps.screen==PS_HOME) {
        static const char *const rows[]={"Available","Active","Reports","Leave"};
        ps.cursor=ps_box(24,36,(u8)(card_width("Available")+12),4);choice_frame(&ps.cursor,"Propositions");
        for(i=0;i<4;i++)choice_row(&ps.cursor,i,rows[i],C_BLACK);
    } else {
        u8 n=ps_count(ps.screen),shown=n>3?3:n,w=0;if(!shown)shown=1;
        for(i=0;i<n;i++){ps_label(label,prop_title(ps.rules,ps_nth(ps.screen,i)),116);if(card_width(label)>w)w=card_width(label);}
        if(!n)w=card_width("Nothing here");ps.cursor=ps_box(24,36,(u8)(w+12),shown);
        choice_frame(&ps.cursor,ps.screen==PS_LIST?"Available":ps.screen==PS_ACTIVE?"Active":"Reports");
        if(!n)choice_row(&ps.cursor,0,"Nothing here",C_DGRAY);
        for(i=0;i<shown&&ps.top+i<n;i++){ps_label(label,prop_title(ps.rules,ps_nth(ps.screen,ps.top+i)),116);choice_row(&ps.cursor,i,label,C_BLACK);}
        choice_scroll_marks(&ps.cursor,ps.top,ps.top+shown<n);
    }
    war_funds(ps_money());
}
static void props_live(void)
{
    if(!ps.valid||ps.note||!service_open(&save,ps.gates,0))return;
    if(ps.screen==PS_TEXT){dlg_continue(151,91,rt_frame,1);return;}
    if(ps.screen==PS_REPORT)return;
    if(ps.screen==PS_WORKERS)choice_cursor_animated(&ps.cursor,ps.replace?ps.row:(u8)(ps.row-ps.top),rt_frame);
    else choice_cursor_animated(&ps.cursor,(u8)(ps.row-ps.top),rt_frame);
}
