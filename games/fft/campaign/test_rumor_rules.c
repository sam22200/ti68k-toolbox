// Chapter I source event bindings exercised through the actual town service.
#include "worldmap.c"
#include "../../../runtime/platform-sw/rt_sw.h"
#include <assert.h>
#include <stdio.h>
static const char *output;
static void tap(u32 key){assert(sw_step(0));assert(sw_step(key));}
static void shot(const char *name)
{char path[512];snprintf(path,sizeof path,"%s/%s.png",output,name);assert(!sw_write_png(path,1));}
static void checkpoint(u8 bit,const u8 *expected,u8 n)
{
    Save kept;u8 i,at;
    save.flags[bit>>3]|=(u8)(1U<<(bit&7));
    kept=save;
    // These are the fixture chapter's authored completion flags, not reader side effects.
    for(at=0;at<LOC_N;at++) {
        wm.at=at;assert(rumor_count()==(at==2||at==6||at==9?n:0));
        if(at==2||at==6||at==9)for(i=0;i<n;i++)assert(rumor_record(rumor_nth(i))[0]==expected[i]);
    }
    wm.at=6;wm.screen=SC_BAR;wm.trow=wm.ttop=0;p_screen=255;wm.changed=0;
    tap(K_ENTER);assert(wm.screen==SC_RUMORS);assert(rumor_count()==n);
    assert(!memcmp(&kept,&save,sizeof save)&&!wm.changed);
}
int main(int argc,char **argv)
{
    static const u8 early[]={0,1,2},middle[]={0,2,3},late[]={2,3,4,5};
    Save kept;u8 i;char line[64],actual[64];u16 consumed;
    assert(argc==2);output=argv[1];sw_init(0);
    assert(bar_art&&bar_text&&bar_text[6]==6&&bank);memset(save.flags,0,sizeof save.flags);
    checkpoint(1,early,3);rt_frame=0;game_render();shot("early-list");
    // Event02D removes the King's old story and opens the condition report.
    checkpoint(32,middle,3);shot("middle-list");
    tap(K_DOWN);tap(K_DOWN);tap(K_ENTER);assert(wm.rumor==3);shot("condition");
    // Event03A removes the Death Corps' introductory story and adds two reports.
    checkpoint(33,late,4);shot("late-top");
    assert(rumor_box.y+rumor_box.height<80); // common War Funds plate starts at80
    tap(K_DOWN);tap(K_DOWN);tap(K_DOWN);
    assert(wm.trow==3&&wm.ttop==1);shot("late-bottom");
    assert(rumor_box.x+rumor_box.width<=160);assert(rumor_box.width==136);
    kept=save;tap(K_ENTER);assert(wm.rumor==5&&strlen(rumor_string(5,6))==33);shot("dycedarg");
    // All six source articles are wrapped at the same boundaries as the C renderer.
    for(i=0;i<6;i++) {
        const u8 *r=rumor_record(i);const char *text=rumor_string(i,8);u8 page,row,col;
        consumed=0;
        for(page=0;page<r[3];page++) {
            u16 offset=b16(bar_text+b16(r+12)+2*(u16)page);
            for(row=0;row<7;row++) {
                if(!item_detail_line(text,142,(u8)(page*7+row),line))break;
                assert(item_detail_line(text+offset,142,row,actual));assert(!strcmp(line,actual));
                for(col=0;line[col];col++)if(line[col]!=' ') {
                    while(text[consumed]==' ')consumed++;
                    assert(line[col]==text[consumed++]);
                }
            }
        }
        while(text[consumed]==' ')consumed++;assert(!text[consumed]);
    }
    tap(K_ESC);assert(wm.trow==3&&wm.ttop==1&&wm.screen==SC_RUMORS);
    tap(K_DOWN);assert(!wm.trow&&!wm.ttop);tap(K_UP);assert(wm.trow==3&&wm.ttop==1);
    assert(!memcmp(&save,&kept,sizeof save)&&!wm.changed);
    // Persist a late event independently of reading, then reenter the actual Bar.
    wm.changed=1;assert(leave(0)==0);sw_init(0);assert(!memcmp(&save,&kept,sizeof save));
    wm.screen=SC_BAR;wm.trow=0;p_screen=255;tap(K_ENTER);assert(rumor_count()==4);
    // The Chapter II boundary retires all six; no later text is added to this chapter bank.
    checkpoint(34,RT_NULL,0);tap(K_ENTER);assert(wm.screen==SC_RUMORS);shot("boundary-empty");
    release_map();puts("Rumor rules:4 source events,6 articles,43 towns,source replacements,3-row scroll,reentry;no reader progression.");
    return 0;
}
